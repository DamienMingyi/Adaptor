#include "hfs_fix_adaptor.hpp"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"
#include <iostream>
#ifndef WIN32
#include <sys/time.h>
#endif

const int ClientMsgId = 10029;
const int ProductType = 17612;

hfs_fix_adaptor::hfs_fix_adaptor(const a2ftool::XmlNode &_cfg) : hfs_base_adaptor(_cfg), loggedon(false) {
    try {
        a2ftool::XmlNode tradeInfo = cfg; //.getChild("tradeInfo");
        m_id = tradeInfo.getAttrDefault("id", "uattest003");
        m_user = tradeInfo.getAttrDefault("user", "uattest003");
        m_account_id = tradeInfo.getAttrDefault("account", "uat_test_003");
        m_passwd = tradeInfo.getAttrDefault("passwd", "v637jFHE");
        m_server = tradeInfo.getAttrDefault("server", "1.202.164.24");
        m_sender_compid = tradeInfo.getAttrDefault("senderCompID", "FIXUAT003");
        m_target_compid = tradeInfo.getAttrDefault("targetCompID", "CICCIMSUAT");
        m_sender_subid = tradeInfo.getAttrDefault("senderSubID", "matic903211");
        m_bHK = tradeInfo.getAttrDefault("bHK", true);
        m_fix_cfg = tradeInfo.getAttrDefault("fixCfg", "./fix.cfg");      
        m_export_path = tradeInfo.getAttrDefault("export_path", "./export");
        m_sysinfo = cfg.getAttrDefault("sysinfo", "");
        m_rc = new RiskControl(cfg);  
        int key[] = { 1, 2, 3, 4, 5 };//加密字符
        char s[64] = {0};
        strncpy(s, m_passwd.c_str(), 63);
        decode(s, key);
        m_passwd = s;

        int timex = current();
	    m_orderRef = (timex/10000*3600)+((timex%10000)/100*60)+(timex%100);
	    m_orderRef = (today()%10)*100 + m_orderRef;
    }
    catch(std::exception &ex) {
        throw ex.what();
    }
}

hfs_fix_adaptor::~hfs_fix_adaptor() {
    stop();
}

bool hfs_fix_adaptor::start() {
    init_file();
    try {
        string fix_cfg = m_fix_cfg;
        FIX::SessionSettings settings(fix_cfg);

        FIX::FileStoreFactory storeFactory(settings);			//store msgseqnum
        //FIX::ScreenLogFactory logFactory(settings);			// log in screen
        FIX::FileLogFactory logFactory(settings);				//log in files,path configure in config file
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
        // auto qry_thread = std::thread(std::bind(&hfs_fix_adaptor::qry_thread, this));
        // qry_thread.detach();
    }
    catch(std::exception &ex) {
        LOG_ERROR("启动Adaptor失败，原因:{}", ex.what());
        return false;
    }

    return true;
}
void hfs_fix_adaptor::init_file()
{
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
        return;
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
        return;
    }
}
void hfs_fix_adaptor::login() {
	// FIX42::UserLogonRequest message;

	// message.set(FIX::RequestID(newSeq()));//tag 8088
	// message.set(FIX::EncryptMethod(0));//tag 98 不加密
	// message.set(FIX::ClientID(m_user));//tag 109 fix username
    // message.set(FIX::LogonPasswd(m_passwd));//tag 8001 fix passwd
    
    FIX42::Logon message;

	message.set(FIX::EncryptMethod(0));//tag 98 不加密
	message.set(FIX::HeartBtInt(2));//tag 109 fix username
	message.set(FIX::ResetSeqNumFlag("Y"));//tag 8001 fix passwd
    message.setField(FIX::Username(m_user));
	message.setField(FIX::Password(m_passwd));
    message.getHeader().set(FIX::SenderSubID(m_sender_subid));
    
	try {
		FIX::Session::sendToTarget(message, m_sender_compid, m_target_compid);//tag49 & tag56
        std::cout << "send logon with passwd: " + message.toString() << std::endl;
        LOG_INFO("[fix adaptor {}] logon : {}", m_id, message.toString());
	}
	catch (std::exception & e) {
		std::cout << "UserLogonRequest Not Sent: " << e.what();
	}
}
void hfs_fix_adaptor::logout()
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
bool hfs_fix_adaptor::stop() {
    try {
        m_initiator->stop();
        loggedon = false;
    }
    catch(std::exception &ex) {
        LOG_ERROR("关闭adaptor失败！");
    }

    return true;
}
void hfs_fix_adaptor::qry_thread() {
    while(true) {
        char fname[64] = {0};
        sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
        if (m_position_file.is_open())
        {
            m_position_file.close();
        }
        m_position_file.open(fname, std::ios::out);
        m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;

        sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
        if (m_entrust_file.is_open())
        {
            m_entrust_file.close();
        }
        m_entrust_file.open(fname, std::ios::out);
        m_entrust_file << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;

        sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
        if (m_detail_file.is_open())
        {
            m_detail_file.close();
        }
        m_detail_file.open(fname, std::ios::out);
        m_detail_file << "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID" << endl;

        qry_order();
        qry_position();
        qry_detail();
        sleep(60);
    }
}
void hfs_fix_adaptor::qry_order() {
    try {
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

void hfs_fix_adaptor::qry_position() {
    try{
        FIX::Message message;
        message.getHeader().setField(FIX::BeginString("FIX.4.2"));
        message.getHeader().setField(FIX::MsgType("AN"));
        message.getHeader().setField(FIX::SenderSubID(m_sender_subid));
        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.setField(ClientMsgId, m_sysinfo);
        message.setField(1, m_account_id);
        message.setField(710, newSeq()); //PosReqID
        message.setField(724, "0"); //PosReqType
        message.setField(FIX::TransactTime());
        message.setField(715, to_string(today()));
        FIX::Session::sendToTarget(message);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("[fix adaptor {}] qry_position failed:{}", m_id, e.what());
    }
}

void hfs_fix_adaptor::qry_detail() {
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

bool hfs_fix_adaptor::isLoggedOn() {
    return loggedon;
}

bool hfs_fix_adaptor::onRequest(int teid, hfs_order_t &order) {
    dump(&order);
    switch(order.type) {
    case 'O':
        process_new_entrust(teid, order);
        break;
    case 'X':
        process_cancel_entrust(teid, order);
        break;
    case 'H':
    default:
        LOG_INFO ("unexpected order type: {}", order.type);
        return false;
    };

    return true;
}

bool hfs_fix_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
    if (!loggedon) {
        LOG_INFO("[fix adaptor {}] adaptor not login, entrust failed", m_id);
		hfs_order_t od;
		memcpy(&od, &order, sizeof(hfs_order_t));
		od.qty = order.qty;
		od.type = 'J';
		od.state = ORD_REJECTED;
		strncpy(od.algo, "adaptor not login", sizeof(od.algo)-1);
		if (!onResponse(teid, m_id, od)) {
			LOG_ERROR("[fix adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
        }
        login();
        for(;!loggedon;) { //wait until loggedon
            sleep(5);
            if(!loggedon) {
                LOG_INFO("继续尝试连接！");
            }
        }
		return false;
    }
    try {
        m_rc->OnInsertOrder(&order);
        char client_id[32] = {0};
        sprintf(client_id, "%d%03d_%d", teid, order.oseq, ++m_orderRef);
        string symbol(order.symbol);
        FIX42::NewOrderSingle message(
        FIX::ClOrdID(client_id), 
        FIX::HandlInst('1'),
        FIX::Symbol(symbol.substr(0,6)),
        order.side=='B' ? FIX::Side_BUY : FIX::Side_SELL,
        FIX::TransactTime(),
        FIX::OrdType(FIX::OrdType_LIMIT) );

        message.getHeader().setField(FIX::SenderCompID(m_sender_compid)); 
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.getHeader().set(FIX::SenderSubID(m_sender_subid));   // tag 50
        // message.set(FIX::SecondaryClOrdID(order.orderid));

        // char prcBuf[64];
        // sprintf(prcBuf, "%.3f", order.prc);
        message.set(FIX::OrderQty(order.qty));   //量
        message.set(FIX::Price(round(order.prc/0.01)*0.01));   //价
        message.set(FIX::Currency("CNY"));	
        string exch = symbol.substr(7, 2);
        if (exch == "SH") {
            // XSSC 沪股通
            if (m_bHK) message.set(FIX::SecurityExchange("SS"));				//tag207
            else message.set(FIX::SecurityExchange("SS"));				//tag207
        }
        else if (exch == "SZ") {
            // XSEC 深股通
            if (m_bHK) message.set(FIX::SecurityExchange("SZ"));
            else message.set(FIX::SecurityExchange("SZ"));
        }
        message.set(FIX::Account(m_account_id));        

        message.set(FIX::SecurityID(symbol.substr(0, 6)));
        message.set(FIX::IDSource("8"));  // ExchangeSymbol
        //newOrderSingle.set(FIX::TimeInForce('0'));
        message.setField(ClientMsgId,m_sysinfo.c_str());
        message.setField(ProductType,"119001");
    
        FIX::Session::sendToTarget( message);
        // struct timeval begin;
        // gettimeofday(&begin, NULL);
        // printf("time is %ld.%ld, ClOrdID is :%s\n",begin.tv_sec,begin.tv_usec,sid.c_str());

        if(!registerTrader(teid, order, client_id)) {
            LOG_INFO("[fix adaptor {}] registerTrader failed！teid:{}, pid:{}, oseq:{}, client_id:{}",m_id, teid ,order.pid, order.oseq, client_id);
        }
    }
    catch ( std::exception & e ) {
        LOG_ERROR("[fix adaptor {}] process entrust failed:{}",m_id, e.what());
        return false;
    }

    return true;
}

bool hfs_fix_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
    try {
        //第一步，构造message
        EntrustNode *enode = getENode(teid, order.pid, order.oseq);
        if(!enode) {
            LOG_ERROR("[fix adaptor {}] process cancel failed:teid:{}, oseq:{}, pid:{}, order not exists ",
                         teid, order.oseq, order.pid);
            return true;
        }
        LOG_INFO("[fix adaptor {}] cancel order : {}", m_id, enode->sid);
        string sid = newSeq();
        FIX42::OrderCancelRequest message( 
            FIX::OrigClOrdID(enode->sid),
            FIX::ClOrdID(sid.c_str()),
            FIX::Symbol(enode->tkr),
            order.side=='B' ? FIX::Side_BUY : FIX::Side_SELL, 
            FIX::TransactTime() );
        message.set(FIX::Symbol(enode->tkr));
        message.set(FIX::OrderID(enode->origOrder.orderid));
        message.set(FIX::Account(m_account_id));

        message.getHeader().setField(FIX::SenderCompID(m_sender_compid));
        message.getHeader().setField(FIX::TargetCompID(m_target_compid));
        message.getHeader().set(FIX::SenderSubID(m_sender_subid));   // tag 50
        message.setField(ClientMsgId, m_sysinfo.c_str());
        //第二步，发送message
        FIX::Session::sendToTarget( message);

        // struct timeval begin;
        // gettimeofday(&begin, NULL);
        // LOG_INFO("time is %ld.%ld, Origin ClOrdID is :%s" << begin.tv_sec << ' ' << begin.tv_usec << ' ' << enode->sid);
    }
    catch ( std::exception & e ) {
        LOG_INFO("[fix adaptor {}] process entrust failed:{}",m_id, e.what());
        return false;
    }

    return true;
}


bool hfs_fix_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
    try {
        EntrustNode *node = new EntrustNode();
        memset(node, 0, sizeof(EntrustNode));
        node->nid.teid = teid;
        node->nid.pid  = order.pid;
        node->nid.oseq = order.oseq;
        strcpy(node->sid, sid.c_str());
        strcpy(node->tkr, order.symbol);
        node->qty = order.qty;
        node->eqty = 0;
        node->canceled = 0;
        memcpy(&node->origOrder, &order, sizeof(hfs_order_t));
        {
            std::lock_guard<std::mutex> lk(m_nid_mtx);
            // this->evec.push_back(node);
            this->nmap.insert(make_pair(node->nid, node));
        }
        {
            std::lock_guard<std::mutex> lk(m_sid_mtx);
            this->smap.insert(make_pair(sid, node));
        }
        //Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
        // m_entrust_file << m_account_id << "," << order.symbol << ",," << order.tm << ",," << sid << "," << order.side << ",,," << node->origOrder.prc << "," << order.qty << "," << order.prc << "," << 0 << ",," << ",,," << endl;
        // m_entrust_file.flush();

        LOG_INFO("[fix adaptor {}] register trader : {}",m_id, sid);
        return true;
    }
    catch(std::exception &ex) {
        LOG_ERROR("[fix adaptor {}]注册trader失败！原因：{}",m_id, ex.what());
        return false;
    }
}

EntrustNode *hfs_fix_adaptor::getENode(int teid, int pid, int oseq) {
    std::lock_guard<std::mutex> lk(m_nid_mtx);
    EntrustNode *rv = nullptr;
    OrderID oid(teid, pid, oseq);
	if (nmap.find(oid) != nmap.end()) {
		return nmap[oid];
	}
	return rv;
}

EntrustNode *hfs_fix_adaptor::getENode(string& sid) {
    std::lock_guard<std::mutex> lk(m_sid_mtx);
    EntrustNode *enode = nullptr;
    auto iter = smap.find(sid);
    if (iter != smap.end()) {
        enode = smap[sid];
    }
    return enode;
}

string hfs_fix_adaptor::newSeq() {
	try
	{
		std::string ntime("");
		time_t now_time;
		char tmp[20];
		time(&now_time);
		strftime(tmp, sizeof(tmp), "%m%d%H%M%S", localtime(&now_time));
		ntime = tmp;

		m_orderRef++;
		std::string rand = std::to_string(m_orderRef);
		std::string clordId = ntime + rand;
		return m_sender_compid+clordId;
	}
	catch (std::exception & e)
	{
		std::cout << "getNextFixID: " << e.what();
		return "";
	}
}

void hfs_fix_adaptor::onLogon( const FIX::SessionID& sessionID )
{
    loggedon = true;
    LOG_INFO("[fix adaptor {}] Logon - {}",m_id, sessionID.toString());
}

void hfs_fix_adaptor::onLogout( const FIX::SessionID& sessionID )
{
    loggedon = false;
    LOG_INFO("[fix adaptor {}] Logout - {}",m_id, sessionID.toString());
}

void hfs_fix_adaptor::fromApp( const FIX::Message& message, const FIX::SessionID& sessionID )
    throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType )
{
    crack(message, sessionID);
    LOG_INFO("[fix adaptor {}] IN: {}",m_id, message.toString());
}
void hfs_fix_adaptor::onRspQryOrder(const FIX::Message & message) {
    LOG_INFO("[fix adaptor {}] onRspQryOrder {} ", m_id, message.toString());
    //Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
    // FIX::Group group;
    // for (int i = 0; i < message.getFiled(73); ++i) {
    //     message.getGroup(i, group);
    //     m_entrust_file << m_account_id << "," << group.getField(55) << ",,," << group.getField(37) << "," << group.getField(11) << ","
    //                    << group.getField(11) << "," group.getField(11) << "," group.getField(11) << "," group.getField(11) << ","
    // }
        
}

void hfs_fix_adaptor::onRspQryDetail(const FIX::Message &) {
    LOG_INFO("[fix adaptor {}] onRspQryOrder", m_id);
    // Account, StockCode, TradeData, TradeTime, OrderID, EntrustSide, ExecType, LastPx, LastQty, PositionStr, OrigOrderID
    // FIX::Group group;

}

void hfs_fix_adaptor::onRspQryPosition(const FIX::Message &) {

}

void hfs_fix_adaptor::toApp( FIX::Message& message, const FIX::SessionID& sessionID )
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

void hfs_fix_adaptor::toAdmin(FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::DoNotSend)
{
    // LOG_INFO("[fix adaptor {}] OUT Admin: {}", m_id, message.toString());
}
void hfs_fix_adaptor::fromAdmin(const FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon)
{
    // LOG_INFO("[fix adaptor {}] IN Admin: {}", m_id, message.toString());
}
void hfs_fix_adaptor::onMessage( const FIX42::NewOrderSingle& message, const FIX::SessionID& )
{
    FIX::ClOrdID clOrdID;
    message.get(clOrdID);

    FIX::ClearingAccount clearingAccount;
    message.get(clearingAccount);
    LOG_INFO("[fix adaptor {}] onNewOrder:{}--{}",m_id, clOrdID.getValue(), clearingAccount.getValue());
}

void hfs_fix_adaptor::onMessage( const FIX42::OrderCancelRequest& message, const FIX::SessionID& )
{
    FIX::ClOrdID clOrdID;
    message.get(clOrdID);

    LOG_INFO("[fix adaptor {}] onCancelRequestOrder:{}, {}",m_id,
             clOrdID.getValue(), message.toString());
}

//处理来自交易所的反馈
void hfs_fix_adaptor::onMessage( const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) {
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

    LOG_INFO("[fix adaptor {}]onExecutionReportMessage type:{}, status:{}", m_id,
                type.getValue(), status.getValue());

    //根据ExecType确认返回消息的类型
    string client_id = clOrdID.getValue();
    // if (type == '4') {  // 撤单取原始委托号
    //     FIX::OrigClOrdID orgClOrdID;
    //     message.get(orgClOrdID);
    //     client_id = orgClOrdID.getValue();
    // }
    EntrustNode *node = getENode(client_id);
    if(!node) {
        LOG_INFO("[fix adaptor {}] cannot find order:{}",m_id, client_id);
        return;
    }
    hfs_order_t od = {};
    memset(&od, 0, sizeof(hfs_order_t));
    strncpy(node->origOrder.orderid, orderID.getValue().c_str(), sizeof(od.orderid));
    strncpy(od.symbol, node->origOrder.symbol, sizeof(od.symbol)-1);
    strncpy(od.exch, node->origOrder.exch, sizeof(od.exch)-1);
    od.idnum = node->origOrder.idnum;
    od.side = node->origOrder.side;
    od.ocflag = node->origOrder.ocflag;
    od.pid = node->nid.pid;
    od.oseq = node->nid.oseq;
    // strncpy(order.orderid, orderID.c_str(), sizeof(od.orderid));
    // strncpy(node->origOrder.orderid, orderID.c_str(), sizeof(od.orderid));
    if (!IsTradingOrder(node)) {
        LOG_INFO("[fix adaptor {}]finished order , get ret :{}",m_id, client_id);
    }

    switch(type) {
    case '0': //NEW
    case 'A':
        {
            od.type = 'A';
            od.state = ORD_NEW;
            od.qty = node->qty;
            node->origOrder.state = ORD_NEW;
        }
        break;
    case '1': //PARTIAL_FILL
    case '2': //FILL
        {
            od.state = ORD_NEW;
            node->origOrder.state = ORD_NEW;
            od.type = 'E';
            int filled_qty = qty.getValue();
            od.qty = filled_qty - node->eqty;
            od.prc = prc.getValue();
            node->eqty = filled_qty;  // 增量成交            
        }
        break;
    case '8': //REJECTED
        {
            message.get(text);
            LOG_INFO("[fix adaptor {}] order rejected:{}",m_id, text.getValue());
            od.state = ORD_REJECTED;
            od.type = 'J';
            od.qty = node->qty;
            od.prc = node->origOrder.prc;
            node->origOrder.state = ORD_REJECTED;
            strncpy(od.algo, text.getValue().c_str(), sizeof(od.algo)-1);
        }
        break;
    case '4': //CANCELED
        {
            int filled_qty = qty.getValue();
            FIX::LeavesQty lev_qty;
			message.get(lev_qty);
            int cxl_qty = lev_qty.getValue();
            if (filled_qty > node->eqty) {    
                od.state = ORD_NEW;
                node->origOrder.state = ORD_NEW;
                od.type = 'E';
                od.qty = filled_qty - node->eqty;
                od.prc = prc.getValue();
                node->eqty = filled_qty;  // 增量成交
                onResponse(node->nid.teid, m_id, od);
            }
            if (node->eqty + cxl_qty < node->origOrder.qty) {
                od.state = ORD_NEW;
                node->origOrder.state = ORD_NEW;
                od.type = 'E';
                od.qty = node->origOrder.qty - node->eqty - cxl_qty;
                od.prc = node->origOrder.prc;
                node->eqty = node->origOrder.qty - cxl_qty;
                onResponse(node->nid.teid, m_id, od);
            }
            node->origOrder.state = ORD_CANCELED;
            od.state = ORD_CANCELED;
            od.type = 'C';
            od.prc = node->origOrder.prc;
            od.qty = cxl_qty;
        }
        break;
    default:
        LOG_INFO("[fix adaptor {}]  unknown ExecType:{}",m_id, type.getValue());
        return;
    }
    if (!onResponse(node->nid.teid, m_id, od)) {
        LOG_ERROR("[fix adaptor {}]onResponse failed, teid:{}, pid:{}, osep:{}",m_id, node->nid.teid, node->nid.pid, node->nid.oseq);
    }
    m_rc->OnRtnOrder(node->nid.teid, node->origOrder);

    //Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
    m_entrust_file << m_account_id << "," << od.symbol << ",," << node->origOrder.tm << "," << node->origOrder.orderid << "," 
    << client_id  << "," << node->origOrder.side << ",," << type << "," << node->origOrder.prc << "," << node->qty << "," 
    << prc.getValue() << "," << node->eqty << ",," << ",,," << endl;
    m_entrust_file.flush();

    FIX::Side side;
    message.get(side);
    FIX::LastPx lpx(0);
    if (message.isSetField(31)) {
        message.get(lpx);
    }
    int lqty(0);
    if (message.isSetField(32))
        lqty = stoi(message.getField(32));
    FIX::TransactTime trans_time;
    // Account, StockCode, TradeDate, TradeTime, OrderID,ClientID, EntrustSide, ExecType, LastPx, LastQty, OrigOrderID, Msg
    m_detail_file << m_account_id << "," << symbol.getValue() << ",," << trans_time << ","
                  << orderID.getValue() << "," << clOrdID.getValue() << "," << side << ","
                  << type << "," << lpx << "," << lqty << ",," << text << endl;
    m_detail_file.flush();
    
}

void hfs_fix_adaptor::onMessage( const FIX42::OrderCancelReject& message, const FIX::SessionID& sessionID) {
    LOG_INFO("[fix adaptor {}] onCancelRejectMessage:{}",m_id, message.toString());
}

void hfs_fix_adaptor::onMessage( const FIX42::Reject& message, const FIX::SessionID& sessionID) {
    LOG_INFO("[fix adaptor {}] Reject:{}",m_id, sessionID.toString());

}


void hfs_fix_adaptor::onCreate( const FIX::SessionID& ) {
    cout << "[fix adaptor] OnCreate!" << endl;
}

void hfs_fix_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("[fix adaptor {}] holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             m_id, holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_fix_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("[fix adaptor {}] order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
            m_id, order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

bool hfs_fix_adaptor::IsTradingOrder(EntrustNode* node) {
    return !(node->origOrder.state != ORD_NEW && node->origOrder.state != ORD_NONE && node->origOrder.state != ORD_PENDING_NEW && 
        node->origOrder.state != ORD_PENDING_CANCEL);
}

//加密
void hfs_fix_adaptor::encode(char *pstr, int *pkey){
    int len = strlen(pstr);//获取长度
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_fix_adaptor::decode(char *pstr, int *pkey){
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