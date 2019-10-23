#include "hfs_fix2bsim.hpp"
#include "hfs_log.hpp"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"
#include "quickfix/fix42/ListStatusRequest.h"
#include <iostream>
#include <thread>
#include <math.h>

#ifndef WIN32
#include <sys/time.h>
#endif

const int ClientMsgId = 10029;
const int ProductType = 17612;

hfs_fix2bsim::hfs_fix2bsim(const a2ftool::XmlNode &_cfg) : loggedon(false) {
    try {
        a2ftool::XmlNode tradeInfo;
        if (_cfg.hasChild("Gateway"))
        {
            tradeInfo = _cfg.getChild("Gateway").getChild("accounts").getChildren("account")[0];
        } else {
            tradeInfo = _cfg.getChild("accounts").getChildren("account")[0];
        }
        m_id = tradeInfo.getAttrDefault("id", "");
        m_user = tradeInfo.getAttrDefault("user", "matic903211");
        m_account_id = tradeInfo.getAttrDefault("account", "matic903211");
        m_passwd = tradeInfo.getAttrDefault("passwd", "ct`fD444");
        m_server = tradeInfo.getAttrDefault("server", "1.202.164.24");
        m_sender_compid = tradeInfo.getAttrDefault("senderCompID", "QFMFix");
        m_target_compid = tradeInfo.getAttrDefault("targetCompID", "HTSC");
        m_sender_subid = tradeInfo.getAttrDefault("senderSubID", "matic903211");
        m_bHK = tradeInfo.getAttrDefault("bHK", true);
        m_fix_cfg = tradeInfo.getAttrDefault("fixCfg", "./fixconfig/fix.cfg");
        m_export_path = tradeInfo.getAttrDefault("export_path", "./");
        m_sysinfo = tradeInfo.getAttrDefault("sysinfo", "test");
        m_teid = tradeInfo.getAttrDefault("teid",10);
        m_redis_ip = tradeInfo.getAttrDefault("redis_ip", "192.168.1.16");
        m_redis_port = tradeInfo.getAttrDefault("redis_port", 6379);
        string tmp = tradeInfo.getAttrDefault("order_key","CATS_REDIS_ORD_TEST");
        if (tmp.size() > 0)
            m_order_keys.push_back(tmp);
        string tmpIns = tradeInfo.getAttrDefault("ins_key","CATS_REDIS_INS_TEST");
        if (tmpIns.size() > 0)
            m_ins_keys.push_back(tmpIns);
        
        m_clear_redis = tradeInfo.getAttrDefault("clearRedis", false);
        m_ord_redis = new redisMgr(m_redis_ip.c_str(), m_redis_port);
        // m_rc = new RiskControl(tradeInfo);

        int key[] = { 1, 2, 3, 4, 5 };//加密字符
        char s[64] = {0};
        strncpy(s, m_passwd.c_str(), 63);
        decode(s, key);
        m_passwd = s;
    }
    catch(std::exception &ex) {
        throw ex.what();
    }
}

hfs_fix2bsim::~hfs_fix2bsim() {
    stop();
}

bool hfs_fix2bsim::start() {
    init_file();
    try
    {
        string fix_cfg = m_fix_cfg;
        FIX::SessionSettings settings(fix_cfg);

        FIX::FileStoreFactory storeFactory(settings);            //store msgseqnum
        //FIX::ScreenLogFactory logFactory(settings);            // log in screen
        FIX::FileLogFactory logFactory(settings);                //log in files,path configure in config file
        m_initiator = new FIX::SocketInitiator(*this, storeFactory, settings, logFactory);
        m_initiator->start();
        sleep(1);

        login();

        for(;!loggedon;) { //wait until loggedon
            sleep(5);
            if(!loggedon) {
                LOG_INFO("继续尝试连接！");
            }
        }
       // char fname[256] = {0};
        //sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
       // m_entrust_file.open(fname, std::ios::out | std::ios::app);
       // m_entrust_file << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;
    }
    catch(std::exception &ex) {
        LOG_ERROR("启动Adaptor失败，原因:{}", ex.what());
        return false;
    }

    return true;
}
void hfs_fix2bsim::init_file() {
    string fname = m_export_path + "/STOCK_ENTRUST_" + to_string(today()) + ".csv";
    if (file_exists(fname.c_str()))
    {
        m_entrust_file.open(fname, std::ios::app | std::ios::out);
    }
    else
    {
        m_entrust_file.open(fname, std::ios::app | std::ios::out);
        m_entrust_file << "Account,StockCode,TradeDate,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;
    }
    if (!m_entrust_file.is_open())
    {
        LOG_ERROR("[fix adaptor {}] entrust file open failed", m_id);
        return ;
    }
    fname = m_export_path + "/STOCK_DETAIL_" + to_string(today()) + ".csv";
    if (file_exists(fname.c_str()))
    {
        m_detail_file.open(fname, std::ios::app | std::ios::out);
    }
    else
    {
        m_detail_file.open(fname, std::ios::app | std::ios::out);
        m_detail_file << "Account,StockCode,TradeDate,TradeTime,OrderID,ClientID,EntrustSide,ExecType,LastPx,LastQty,OrigOrderID,Msg" << endl;
    }
    if (!m_detail_file.is_open())
    {
        LOG_ERROR("[fix adaptor {}] detail file open failed", m_id);
        return ;
    }
}
void hfs_fix2bsim::login() {
    FIX42::Logon message;

    message.set(FIX::EncryptMethod(0));//tag 98 不加密
    message.set(FIX::HeartBtInt(2));//tag 109 fix username
    message.set(FIX::ResetSeqNumFlag("Y"));//tag 8001 fix passwd
    message.setField(FIX::Username(m_user));
    message.setField(FIX::Password(m_passwd));
    message.getHeader().set(FIX::SenderSubID(m_sender_subid));
    
    try {
        FIX::Session::sendToTarget(message, m_sender_compid, m_target_compid);//tag49 & tag56
        //LOG_INFO("[fix adaptor {}] logon : {}", m_id, message.toString());
    }
    catch (std::exception & e) {
        cout << "UserLogonRequest Not Sent: " << e.what();
    }
}
void hfs_fix2bsim::logout()
{
    FIX42::Logout message;
    try
    {
        FIX::Session::sendToTarget(message, m_sender_compid, m_target_compid); //tag49 & tag56
        std::cout << "send logout: " + message.toString() << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "logout Not Sent: " << e.what();
    }
}
bool hfs_fix2bsim::stop() {
    try {
        m_initiator->stop();
        loggedon = false;
    }
    catch(std::exception &ex) {
        LOG_ERROR("关闭adaptor失败！");
    }

    return true;
}

bool hfs_fix2bsim::isLoggedOn() {
    return loggedon;
}

int hfs_fix2bsim::run() {
    // 从redis拿指令下单
    m_bRunning = true;
    auto qry_thread = std::thread(std::bind(&hfs_fix2bsim::qry_thread, this));
    qry_thread.detach();

    auto ins_td = std::thread(std::bind(&hfs_fix2bsim::monitor_redis, this));
    ins_td.join();
    return 0;
}

void hfs_fix2bsim::monitor_redis() {
    cout << "start monitor redis " << endl;
    redisMgr* redis = new redisMgr(m_redis_ip.c_str(), m_redis_port);
    if (m_clear_redis) {
        for (auto key : m_ins_keys) {
            redis->delKey(key.c_str());
        }
    }

    while (m_bRunning) {
        while (!loggedon)
        {
            login();
            sleep(5000);
        }

        for (auto key : m_ins_keys) {
            string data = redis->popData(key.c_str());
            if(data.size() > 0)
                cout << "redis get " << data << endl;
            if (data.find("instructs") != std::string::npos) {
                LOG_INFO("redis get:{}", data);

                string client_id = redis->getValue(data, "client_id");                
                string tkr = redis->getValue(data, "symbol");

                double price = atof(redis->getValue(data, "ord_price").c_str());
                int qty = atoi(redis->getValue(data, "ord_qty").c_str());

                string ord_no = redis->getValue(data, "order_no");
                char ins_type = redis->getValue(data, "inst_type").at(0);

                if (ins_type == 'O') {
                    char side = redis->getValue(data, "tradeside").at(0);
                    char ocflag = redis->getValue(data, "ord_type").at(0);
                    process_new_entrust(tkr, "", price, qty, side, ocflag, client_id);
                }
                else if (ins_type == 'C') {
                    if (m_order_map.find(ord_no) != m_order_map.end())
                        this->process_cancel_entrust(m_order_map[ord_no]);
                    else {
                        LOG_ERROR("[fix adaptor] cancel order not found:{}", ord_no);
                    }
                }
            }
        }
    }
    delete redis;
    cout << ("monitor_redis exit") << endl;
}

bool hfs_fix2bsim::process_new_entrust(const std::string &instrument_id, const std::string &exchange_id, double price, int volume, char direction, char offset, std::string &client_id) {
    if (!loggedon) {
        LOG_INFO("[fix adaptor {}] adaptor not login, entrust failed", m_id);
        //hfs_order_t od;
        //memcpy(&od, &order, sizeof(hfs_order_t));
        //od.qty = order.qty;
        //od.type = 'J';
        //od.state = ORD_REJECTED;
        //strncpy(od.algo, "adaptor not login", sizeof(od.algo)-1);
        //if (!onResponse(teid, m_id, od)) {
        //    LOG_ERROR("[fix adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
  //      }
        login();
        for(;!loggedon;) { //wait until loggedon
            sleep(5);
            if(!loggedon) {
                cout << "继续尝试连接！" << endl;
            }
        }
        //return false;
    }
       // m_rc->OnInsertOrder(&order);
    LOG_INFO("[fix adaptor] entrust order, symbol:{}, side:{}, price:{}, qty:{}",
                instrument_id, direction, price, volume);
    FIX42::NewOrderSingle message;
    message.set(FIX::ClOrdID(client_id));
    message.set(FIX::HandlInst('1'));
    message.set(FIX::Symbol(instrument_id.substr(0, 6)));
    message.set(FIX::Side(direction == '1' ? FIX::Side_BUY : FIX::Side_SELL));
    message.set(FIX::TransactTime());
    message.set(FIX::OrdType(FIX::OrdType_LIMIT));

    message.getHeader().setField(FIX::SenderCompID(m_sender_compid)); 
    message.getHeader().setField(FIX::TargetCompID(m_target_compid));
    message.getHeader().set(FIX::SenderSubID(m_sender_subid));   // tag 50

    message.set(FIX::OrderQty(volume));   //量
    message.set(FIX::Price(round(price / 0.01)*0.01));   //价
    message.set(FIX::Currency("CNY"));    

    string exch = instrument_id.substr(7, 2);
    if (exch == "SH") {
        // XSSC 沪股通
        if (m_bHK) message.set(FIX::SecurityExchange("SS"));                //tag207
        else message.set(FIX::SecurityExchange("SS"));                //tag207
    }
    else if (exch == "SZ") {
            // XSEC 深股通
        if (m_bHK) message.set(FIX::SecurityExchange("SZ"));
        else message.set(FIX::SecurityExchange("SZ"));
    }
    message.set(FIX::Account(m_account_id));        

    message.set(FIX::SecurityID(instrument_id.substr(0, 6)));
    message.set(FIX::IDSource("8"));  // ExchangeSymbol
    message.setField(ClientMsgId,client_id);
    message.setField(ProductType,"119001");
    FIX::Session::sendToTarget( message);

       /* if(!registerTrader(teid, order, client_id)) {
            LOG_INFO("[fix adaptor {}] registerTrader failed！teid:{}, pid:{}, oseq:{}, client_id:{}",m_id, teid ,order.pid, order.oseq, client_id);
        }*/
    cats_order_t* od = new cats_order_t();
    memset(od, 0, sizeof(cats_order_t));
    strcpy(od->client_id, client_id.c_str());
    strcpy(od->ord_no, client_id.c_str());
    strcpy(od->symbol, instrument_id.c_str());
    od->ord_status[0] = '?';
    od->tradeside[0] = direction;

    sprintf(od->ord_qty, "%d", volume);
    sprintf(od->ord_px, "%f", price);
    strcpy(od->filled_qty, "0");

    sprintf(od->ord_time, "%d", current());

    m_order_lock.lock();
    m_order_map.insert(make_pair(client_id, od));
    m_order_lock.unlock();
    return true;
}

bool hfs_fix2bsim::process_cancel_entrust(cats_order_t *order) {
    try {
        LOG_INFO("[fix adaptor] cancel {}", order->client_id);
        string sid = newSeq();
        FIX42::OrderCancelRequest message;
        message.set(FIX::OrigClOrdID(order->client_id));
        message.set(FIX::ClOrdID(sid.c_str()));
        message.set(FIX::Symbol(order->symbol));
        message.set(FIX::Side(order->tradeside[0] == '1' ? FIX::Side_BUY : FIX::Side_SELL));
        message.set(FIX::TransactTime());
        message.set(FIX::OrderID(m_client2orderno[order->ord_no]));
        message.set(FIX::Account(m_account_id));
        
        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.getHeader().set(FIX::SenderSubID(m_sender_subid));   // tag 50
        message.setField(ClientMsgId,order->client_id);
        //第二步，发送message
        FIX::Session::sendToTarget( message);
    }
    catch ( std::exception & e ) {
        LOG_INFO("[fix adaptor {}] process entrust failed:{}",m_id, e.what());
        return false;
    }
    return true;
}

cats_order_t *hfs_fix2bsim::get_order(string& sid) {
    std::lock_guard<std::mutex> lk(m_order_lock);
    cats_order_t *ord = nullptr;
    auto iter = m_order_map.find(sid);
    if (iter != m_order_map.end()) {
        ord = m_order_map[sid];
    }
    return ord;
}

string hfs_fix2bsim::newSeq() {
    try
    {
        m_orderRef++;
        std::string rand = std::to_string(m_orderRef);
        std::string clordId = rand;
        return m_sender_compid+clordId;
    }
    catch (std::exception & e)
    {
        std::cout << "getNextFixID: " << e.what();
        return "";
    }
}

void hfs_fix2bsim::qry_thread()
{
    while (true)
    {
        char fname[64] = {0};
        sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
        if (m_position_file.is_open())
        {
            m_position_file.close();
        }
        m_position_file.open(fname, std::ios::out);
        m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;        

        sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
        if (m_detail_file.is_open())
        {
            m_detail_file.close();
        }
        m_detail_file.open(fname, std::ios::out);
        m_detail_file << "Account,StockCode,TradeDate,TradeTime,OrderID,ClientID,EntrustSide,ExecType,LastPx,LastQty,OrigOrderID,Msg" << endl;

        // qry_order();
        qry_position();
        sleep(60);
        qry_detail();
        // sleep(60 * 30);
        return;
    }
}
void hfs_fix2bsim::qry_order()
{
    try
    {
        FIX42::ListStatusRequest message;
        message.getHeader().setField(FIX::SenderSubID(m_sender_subid));
        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.setField(ClientMsgId, m_sysinfo);
        message.setField(1, m_account_id);
        message.set(FIX::ListID(newSeq().c_str()));
        FIX::Session::sendToTarget(message);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("[fix adaptor {}] qry_order failed:{}", m_id, e.what());
    }
}

void hfs_fix2bsim::qry_position()
{
    try
    {
        FIX::Message message;
        message.getHeader().setField(FIX::BeginString("FIX.4.2"));
        message.getHeader().setField(FIX::MsgType("AN"));
        message.getHeader().setField(FIX::SenderSubID(m_sender_subid));
        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.setField(ClientMsgId, m_sysinfo);
        message.setField(1, m_account_id);
        message.setField(710, newSeq()); //PosReqID
        message.setField(724, "0");      //PosReqType
        message.setField(FIX::TransactTime());
        message.setField(715, to_string(today()));
        FIX::Session::sendToTarget(message);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("[fix adaptor {}] qry_position failed:{}", m_id, e.what());
    }
}

void hfs_fix2bsim::qry_detail()
{
    try
    {
        FIX::Message message;
        message.getHeader().setField(FIX::BeginString("FIX.4.2"));
        message.getHeader().setField(FIX::MsgType("SL"));
        message.getHeader().setField(FIX::SenderSubID(m_sender_subid));
        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.setField(ClientMsgId, m_sysinfo);
        message.setField(1, m_account_id);
        message.setField(1346, newSeq()); //ApplReqID
        FIX::Session::sendToTarget(message);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("[fix adaptor {}] qry_detail failed:{}", m_id, e.what());
    }
}

void hfs_fix2bsim::onLogon( const FIX::SessionID& sessionID )
{
    loggedon = true;
    LOG_INFO("[fix adaptor {}] Logon - {}",m_id, sessionID.toString());
}

void hfs_fix2bsim::onLogout( const FIX::SessionID& sessionID )
{
    loggedon = false;
    LOG_INFO("[fix adaptor {}] Logout - {}",m_id, sessionID.toString());
}

void hfs_fix2bsim::fromApp( const FIX::Message& message, const FIX::SessionID& sessionID )
    throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType )
{
    string msgType = message.getHeader().getField(35);
    if (msgType == "N")
    {
        // 委托状态查询
        onRspQryOrder(message);
    }
    else if (msgType == "SP")
    {
        onRspQryDetail(message);
    }
    else if (msgType == "AP") {
        onRspQryPosition(message);
    }
    else
        crack(message, sessionID);
    LOG_INFO("[fix adaptor {}] IN: {}",m_id, message.toString());
}

void hfs_fix2bsim::onRspQryOrder(const FIX::Message &message)
{
    // LOG_INFO("[fix adaptor {}] onRspQryOrder : {}", m_id, message.toXML());
    //Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
    
    // FIX::FieldMap::g_const_iterator j;
    // for (j = message.g_begin(); j != message.g_end(); ++j)
    // {
    //     std::vector<FIX::FieldMap *>::const_iterator k;
    //     for (k = j->second.begin(); k != j->second.end(); ++k)
    //     {
    //         m_entrust_file << m_account_id << "," << (*k)->getField(55) << ",,," << (*k)->getField(37) << "," << (*k)->getField(11) << ","
    //                        << (*k)->getField(11) << ","(*k)->getField(11) << ","(*k)->getField(11) << ","(*k)->getField(11) << ","
    //                        << endl;
    //     }
    // }
}

void hfs_fix2bsim::onRspQryDetail(const FIX::Message &message)
{
    LOG_INFO("[fix adaptor {}] onRspQryDetail ", m_id);
    // Account, StockCode, TradeDate, TradeTime, OrderID,ClientID, EntrustSide, ExecType, LastPx, LastQty, PositionStr, OrigOrderID
    
    FIX::FieldMap::g_const_iterator j;
    for (j = message.g_begin(); j != message.g_end(); ++j)
    {
        std::vector<FIX::FieldMap *>::const_iterator k;
        for (k = j->second.begin(); k != j->second.end(); ++k) {
            // cout << (*k)->getField(55) << endl;
            if (stoi((*k)->getField(32)) == 0)
                continue;
            m_detail_file << m_account_id << "," << (*k)->getField(55) << ",," << (*k)->getField(443) << "," << (*k)->getField(37) << "," << (*k)->getField(11) << "," << (*k)->getField(54) << ",0," << (*k)->getField(31) << "," << (*k)->getField(32) << ",," << endl;
        }
    }
}

void hfs_fix2bsim::onRspQryPosition(const FIX::Message &message)
{
    LOG_INFO("[fix adaptor {}] onRspQryPosition", m_id);
    // Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr
    FIX::FieldMap::g_const_iterator j;
    for (j = message.g_begin(); j != message.g_end(); ++j)
    {
        std::vector<FIX::FieldMap *>::const_iterator k;
        for (k = j->second.begin(); k != j->second.end(); ++k)
        {
            int total_qty = atoi((*k)->getField(19274).c_str()) - atoi((*k)->getField(19272).c_str()) + atoi((*k)->getField(19278).c_str());
            int tradeable_qty = atoi((*k)->getField(19264).c_str()) - atoi((*k)->getField(19272).c_str()) + atoi((*k)->getField(19278).c_str()) - atoi((*k)->getField(19275).c_str());
            m_position_file << m_account_id << "," << (*k)->getField(55) << "," << total_qty << "," << tradeable_qty << ",,," << endl;
        }
    }
}
void hfs_fix2bsim::toApp( FIX::Message& message, const FIX::SessionID& sessionID )
    throw( FIX::DoNotSend )
{
    try
    {
        FIX::PossDupFlag possDupFlag;
        message.getHeader().getField( possDupFlag );
        if ( possDupFlag ) throw FIX::DoNotSend();
    }
    catch ( FIX::FieldNotFound& ) { }

    LOG_INFO("[fix adaptor {}] OUT: {}",m_id, message.toString());
}

void hfs_fix2bsim::toAdmin(FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::DoNotSend)
{
    // LOG_INFO("[fix adaptor {}] OUT Admin: {}", m_id, message.toString());
}
void hfs_fix2bsim::fromAdmin(const FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon)
{
    // cout << "[fix adaptor {}] IN Admin: {}" <<  message.toString() << endl;
}
void hfs_fix2bsim::onMessage( const FIX42::NewOrderSingle& message, const FIX::SessionID& )
{
    FIX::ClOrdID clOrdID;
    message.get(clOrdID);
    FIX::ClearingAccount clearingAccount;
    message.get(clearingAccount);
    LOG_INFO("[fix adaptor {}] onNewOrder:{}--{}",m_id, clOrdID.getValue(), clearingAccount.getValue());
}

void hfs_fix2bsim::onMessage( const FIX42::OrderCancelRequest& message, const FIX::SessionID& )
{
    FIX::ClOrdID clOrdID;
    message.get(clOrdID);

    LOG_INFO("[fix adaptor {}] onCancelRequestOrder:{}, {}",m_id,
             clOrdID.getValue(), message.toString());
}

//处理来自交易所的反馈
void hfs_fix2bsim::onMessage( const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) {
    LOG_INFO("[fix adaptor {}] onExecutionReportMessage:{}",m_id, message.toString());
    FIX::OrderID orderID;
    message.get(orderID);

    FIX::ClOrdID clOrdID;
    message.get(clOrdID);

    FIX::ExecType type;
    message.get(type);

    FIX::ExecTransType etype;
    message.get(etype);

    FIX::OrdStatus status;
    message.get(status);

    FIX::ExecID execid;
    message.get(execid);

    FIX::Symbol symbol;
    message.get(symbol);

    FIX::CumQty qty;
    message.get(qty);

    FIX::AvgPx prc(0);
    if (qty.getValue() > 0)
        message.get(prc);

    FIX::Text text("");

    FIX::Side side;
    message.get(side);
    FIX::LastPx lpx(0);
    if (message.isSetField(31)) {
        message.get(lpx);
    }
    // int lqty(0);
    // if (message.isSetField(32))
    //     lqty = stoi(message.getField(32));
    FIX::TransactTime trans_time;

    LOG_INFO("[fix adaptor {}]onExecutionReportMessage type:{}, status:{}", m_id,
                type.getValue(), status.getValue());

    //根据ExecType确认返回消息的类型
    string client_id = clOrdID.getValue();
     //if (type == '4') {  // 撤单取原始委托号
     //    FIX::OrigClOrdID orgClOrdID;
     //    message.get(orgClOrdID);
     //    client_id = orgClOrdID.getValue();
     //}
     cats_order_t* od = get_order(client_id);
     if (od == nullptr) {
         LOG_INFO("cannot find order : {}", client_id);
         return;
     }
    
    switch(type) {
    case '0': //NEW
    case 'A':
        od->ord_status[0] = '0';
        break;
    case '1': //PARTIAL_FILL
    case '2': //FILL
        // od->ord_status[0] = type;
        if (atoi(od->filled_qty) > (int)qty.getValue())
        {
            LOG_ERROR("get error rtn, filled_qty:{}, rtn_qty:{}", od->filled_qty, qty);
            return;
        }
        sprintf(od->filled_qty, "%f", qty.getValue());
        sprintf(od->avg_px, "%f", prc.getValue());
        if (qty.getValue() >= atoi(od->ord_qty))
        {
            od->ord_status[0] = '2';
        } else {
            od->ord_status[0] = '1';
        }
        break;
    case '8': //REJECTED
        od->ord_status[0] = '5';
        message.get(text);
        strncpy(od->err_msg, text.getValue().c_str(), sizeof(od->err_msg));
        LOG_INFO("order rejected : {}", text.getValue());
        break;
    case '4': //CANCELED
        od->ord_status[0] = type;
        sprintf(od->filled_qty, "%f", qty.getValue());
        sprintf(od->avg_px, "%f", prc.getValue());
        sprintf(od->cxl_qty, "%d", atoi(od->ord_qty) - atoi(od->filled_qty));
        break;
    default:
        LOG_INFO("[fix adaptor {}]  unknown ExecType:{}",m_id, type.getValue());
        return;
    }
    LOG_INFO("update order:{},status:{},ord_no:{},", od->client_id, od->ord_status[0], orderID.getValue());
    string order_no = orderID.getValue();
    if(order_no != "")
    {
        m_client2orderno.insert(make_pair(client_id,order_no));
    }

    m_ord_redis->pushData(m_order_keys, od);

    // m_entrust_file << "Account,StockCode,TradeDate,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID"
    m_entrust_file << m_account_id << "," << od->symbol << ",," << od->ord_time << "," << order_no << ","
                   << client_id << "," << od->tradeside << ",," << type << "," << od->ord_px << "," << od->ord_qty << ","
                   << od->avg_px << "," << od->filled_qty << "," << od->cxl_qty << ","
                   << ",," << endl;
    m_entrust_file.flush();
}

void hfs_fix2bsim::onMessage( const FIX42::OrderCancelReject& message, const FIX::SessionID& sessionID) {
    LOG_INFO("[fix adaptor {}] onCancelRejectMessage:{}", m_id, message.toString());
}

void hfs_fix2bsim::onMessage( const FIX42::Reject& message, const FIX::SessionID& sessionID) {
    LOG_INFO("[fix adaptor {}] Reject:{}",m_id, sessionID.toString());
}


void hfs_fix2bsim::onCreate( const FIX::SessionID& ) {
    cout << "[fix adaptor] OnCreate!" << endl;
}

//加密
void hfs_fix2bsim::encode(char *pstr, int *pkey){
    int len = strlen(pstr);//获取长度
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_fix2bsim::decode(char *pstr, int *pkey){
    int len = strlen(pstr);
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

bool file_exists(const char *fname)
{
    std::ifstream _file;
    _file.open(fname, std::ios::in);
    if (!_file)
    {
        return false;
    }
    else
    {
        return true;
    }
}