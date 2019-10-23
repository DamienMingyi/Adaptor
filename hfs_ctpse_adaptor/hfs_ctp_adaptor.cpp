#include "hfs_ctp_adaptor.hpp"
#include "hfs_log.hpp"
#include <thread>

hfs_ctp_adaptor::hfs_ctp_adaptor(const a2ftool::XmlNode &_cfg)
  :hfs_base_adaptor(_cfg) {
    iRequestID = 0;
    m_state = CTP_INIT;
}

hfs_ctp_adaptor::~hfs_ctp_adaptor() {

}

bool hfs_ctp_adaptor::start(){
    //初始化，发起连接
    maxRef = current() * 100;
    readConfig();
    std::thread userAPIThread(std::bind(&hfs_ctp_adaptor::startUserAPI, this));
    userAPIThread.detach();

    finished = success = false;
    int ret = Login();
    sleep(15);
    if (m_state != CTP_LOGGEDON) 
        return -1;
    return 0;
}

bool hfs_ctp_adaptor::stop() {
    if(pUserApi) {
        pUserApi->Release();
    }
    return true;
}

bool hfs_ctp_adaptor::isLoggedOn() {
    return loggedon;
}

bool hfs_ctp_adaptor::onRequest(int teid, hfs_order_t &order) {
    switch(order.type) {
    case 'O':
        process_new_entrust(teid, order);
        break;
    case 'X':
        process_cancel_entrust(teid, order);
        break;
    case 'H':
        //return adap.resendTradeHistory(teid);
        break;
    default:
        LOG_ERROR("unexpected order type: {}", order.type);
        return false;
    };

    // fprintf(stderr, "try saveOrder\n");
    // try {
        //if(!adap.saveOrder(teid, &order)) throw "saveOrder error!";
    // }
    // catch(const char *estr) {
        // LOG_ERROR("onRequest: {}", estr);
    // }

    return true;
}

bool hfs_ctp_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
    if (m_state != CTP_LOGGEDON) {
		LOG_INFO("adaptor not login, entrust failed");
		hfs_order_t od;
		memcpy(&od, &order, sizeof(hfs_order_t));
		// od.qty = order.qty;
		od.type = 'J';
		od.state = ORD_REJECTED;
		strncpy(od.algo, "adaptor not login", sizeof(od.algo)-1);
		if (!onResponse(teid, m_id, od)) {
			LOG_ERROR("onResponse failed, teid:{}, acct:{}, pid:{}, osep:{}", teid, order.exch, order.pid, order.oseq);
		}
		return false;
    }
  
    assert(order.type == 'O');  
    dump(&order);
    
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, params.BrokerID.c_str());
    strcpy(req.InvestorID, params.UserName.c_str());
    strcpy(req.InstrumentID, order.symbol);
    sprintf(req.OrderRef, "%d", getNextRef());
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向: 
    TThostFtdcDirectionType dir = order.side == 'B' ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    req.Direction = dir;
    ///组合开平标志: 开仓
    if(order.ocflag == 'O') {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    }
    else if(order.ocflag == 'D') {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
    }
    else if(order.ocflag == 'C') {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
    }
    else if(order.ocflag == 'Y') {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
    }
    else { 
        LOG_ERROR("unkown order.ocflag:{}", order.ocflag);
        return false;
    }

    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation; //默认都是投机
    ///价格
    double pricetick = 0.1;
    if (priceTickMap.find(order.symbol) != priceTickMap.end()) {
        pricetick = priceTickMap[order.symbol];
    }
    req.LimitPrice = round(order.prc / pricetick) * pricetick;
    ///数量
    req.VolumeTotalOriginal = order.qty;
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///GTD日期
    //TThostFtdcDateTypeGTDDate;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///止损价
    //TThostFtdcPriceTypeStopPrice;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///业务单元
    //TThostFtdcBusinessUnitTypeBusinessUnit;
    ///请求编号
    //TThostFtdcRequestIDTypeRequestID;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    printInputOrder(&req);

    string sid = req.OrderRef;

    if(pUserApi->ReqOrderInsert(&req, ++iRequestID) != 0) {
        LOG_ERROR("发送委托失败！");
        return false;
    }
    if(!registerTrader(teid, order, sid)) {
        LOG_ERROR("注册trader失败！{} {} {} {}", teid, order.pid, order.oseq, sid);
    }
    
    return true;
}

bool hfs_ctp_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
    std::lock_guard<std::mutex> slk(mutexSync); //目前同一时间，只能一个撤单

    assert(order.type == 'X');

    dump(&order);

    canceled = false;

    EntrustNode *enode = getENode(teid, order.pid, order.oseq);
    if(!enode) {
        LOG_ERROR("试图cancel不存在的order！ teid= {} oseq={}", teid, order.oseq);
        return true;
    }
    string ref = enode->sid;

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, params.BrokerID.c_str());
    ///投资者代码
    strcpy(req.InvestorID, params.UserName.c_str());
    ///报单操作引用
    //TThostFtdcOrderActionRefTypeOrderActionRef;
    ///报单引用
    sprintf(req.OrderRef, "%s", ref.c_str());
    ///请求编号
    //TThostFtdcRequestIDTypeRequestID;
    ///前置编号
    req.FrontID = FRONT_ID;
    ///会话编号
    req.SessionID = SESSION_ID;
    ///交易所代码
    //TThostFtdcExchangeIDTypeExchangeID;
    ///报单编号
    //TThostFtdcOrderSysIDTypeOrderSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
    //TThostFtdcPriceTypeLimitPrice;
    ///数量变化
    //TThostFtdcVolumeTypeVolumeChange;
    ///用户代码
    //TThostFtdcUserIDTypeUserID;
    ///合约代码
    strcpy(req.InstrumentID, order.symbol);

    int iResult = pUserApi->ReqOrderAction(&req, ++iRequestID);
    if(iResult != 0) {
        LOG_ERROR("[ctp adaptor] 报单操作请求: 失败");
        return false;
    }

    return true;
}

bool hfs_ctp_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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
        std::lock_guard<std::mutex> lk(muEvec);
        this->evec.push_back(node);
        this->smap.insert(make_pair(sid, node));
        return true;
    }
    catch(std::exception &ex) {
        LOG_ERROR("注册trader失败！原因：{}", ex.what());
        return false;
    }
}

EntrustNode *hfs_ctp_adaptor::getENode(int teid, int pid, int oseq) {
    std::lock_guard<std::mutex> lk(muEvec);
    EntrustNode *rv = nullptr;
    // EntrustInfoVec::iterator itr = evec.begin();
    for(auto node : evec) {
        if(node->nid.teid == teid && node->nid.pid == pid && node->nid.oseq == oseq) {
        rv = node;
        break;
        }
    }
    return rv;
}

EntrustNode *hfs_ctp_adaptor::getENode(string sid) {
    std::lock_guard<std::mutex> lk(muEvec);
    EntrustNode *enode = nullptr;
    // for(int i=evec.size()-1;i>=0;i--) {
    //   if(sid == string(evec[i].sid)) {
    //     enode = &evec[i];
    //     break;
    //   }
    // }
    auto iter = smap.find(sid);
    if (iter != smap.end()) {
        enode = smap[sid];
    }
    return enode;
}

bool hfs_ctp_adaptor::readConfig() {
    a2ftool::XmlNode ctpcfg = cfg;
    params.QuoteAddress = ctpcfg.getAttrDefault("QuoteAddress", "");
    params.TradeAddress = ctpcfg.getAttrDefault("TradeAddress", "");
    params.BrokerID     = ctpcfg.getAttrDefault("BrokerID", "");
    params.UserName     = ctpcfg.getAttrDefault("UserName", "");
    params.Password     = ctpcfg.getAttrDefault("Password", "");
    params.AuthKey      = ctpcfg.getAttrDefault("AuthKey", "");
    m_id                = ctpcfg.getAttrDefault("id", "");
    m_position_file     = ctpcfg.getAttrDefault("position_path", "./position.txt");
    m_app_id            = ctpcfg.getAttrDefault("APPID", "Aft_bsim_1.0");
    // m_prefix            = ctpcfg.getAttrDefault("prefix", 100000);
    return true;
}


/////////////////////////////////////////////////////////////////
void hfs_ctp_adaptor::OnFrontConnected()
{
    LOG_INFO("[ctp adaptor] OnFrontConnected");

    m_state = CTP_CONNECTED;

    finished = success = true;
}

void hfs_ctp_adaptor::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    LOG_INFO("[ctp adaptor] OnRspAuthenticate UserProductInfo:{}, AppID:{}, AppType:{}",
                pRspAuthenticateField->UserProductInfo, pRspAuthenticateField->AppID, pRspAuthenticateField->AppType);
    IsErrorRspInfo(pRspInfo);
    //注册用户终端信息
    /*CThostFtdcUserSystemInfoField info;
    memset(&info, 0, sizeof(info));
    strcpy(info.BrokerID, params.BrokerID.c_str());
    strcpy(info.UserID, params.UserName.c_str());
    cout << CTP_GetSystemInfo(info.ClientSystemInfo, info.ClientSystemInfoLen) << endl;
    LOG_INFO("systeminfo:{}", info.ClientSystemInfo);
    
    strcpy(info.ClientAppID, "Aft_bsim_1.0");
    pUserApi->RegisterUserSystemInfo(&info);
    sleep(1);
    finished = success = true;
    */
    //认证成功，发起登录请求
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, params.BrokerID.c_str());
    strcpy(req.UserID, params.UserName.c_str());
    strcpy(req.Password, params.Password.c_str());
    finished = success = false;
    int ret = pUserApi->ReqUserLogin(&req, ++iRequestID);
    if(ret != 0) {
        LOG_ERROR("send login error!");
    }
}
void hfs_ctp_adaptor::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG_INFO("OnRspUserLogin");

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) //登录成功
    {
        // 保存会话参数
        FRONT_ID = pRspUserLogin->FrontID;
        SESSION_ID = pRspUserLogin->SessionID;
        int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
        iNextOrderRef++;
        sprintf(ORDER_REF, "%d", iNextOrderRef);
        // maxRef = iNextOrderRef;

        ///获取当前交易日
        LOG_INFO("tradingday:{}, FrontID:{}, SessionID:{}, MaxOrderRef:{}", pUserApi->GetTradingDay(),
                    pRspUserLogin->FrontID, pRspUserLogin->SessionID, pRspUserLogin->MaxOrderRef);

        //请求结算
        CThostFtdcSettlementInfoConfirmField nreq;
        memset(&nreq, 0, sizeof(nreq));
        strcpy(nreq.BrokerID, params.BrokerID.c_str());
        strcpy(nreq.InvestorID, params.UserName.c_str());
        // finished = success = false;
        int ret = pUserApi->ReqSettlementInfoConfirm(&nreq, ++iRequestID);
        if(ret != 0) {
            LOG_ERROR("请求结算错误");
        }

        m_state = CTP_LOGGEDON;
        
    }
    else {
        LOG_ERROR("login failed");
        // success = false;
    }

    // finished = true;
}

void hfs_ctp_adaptor::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG_INFO("[ctp adaptor] OnRspSettlementInfoConfirm");

    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        //打印结算信息
        //TODO
    }

    if(bIsLast) {
        finished = success = true;
    }
    qryPosition();
}

void hfs_ctp_adaptor::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

    if (IsErrorRspInfo(pRspInfo)) {
        LOG_ERROR("[ctp adaptor] qry position failed");
        return;
    }
    
    if (pInvestorPositionDetail != nullptr) {
        LOG_INFO("OnRspQryInvestorPositionDetail:instrumentID:{},exch:{},OpenDate:{},TradingDay:{},Volume:{},TradeType:{}",
                pInvestorPositionDetail->InstrumentID, pInvestorPositionDetail->ExchangeID,
                pInvestorPositionDetail->OpenDate, pInvestorPositionDetail->TradingDay,
                pInvestorPositionDetail->Volume, pInvestorPositionDetail->TradeType);
        
        std::string instrumentID = pInvestorPositionDetail->InstrumentID;
        auto iter = holdings.find(instrumentID);
        if (iter == holdings.end()) {
            hfs_holdings_t *holding = new hfs_holdings_t();
            memset(holding, 0, sizeof(hfs_holdings_t));
            holdings.insert(make_pair(instrumentID, holding));
        } 
        hfs_holdings_t *holding = holdings[instrumentID];

        strcpy(holding->symbol, instrumentID.c_str());
        // holding->cidx = idnum;
        strcpy(holding->exch, pInvestorPositionDetail->ExchangeID);
        if (pInvestorPositionDetail->Volume != 0) {
        if (pInvestorPositionDetail->Direction == THOST_FTDC_D_Buy) { // long
            holding->avgprc_long = (holding->qty_long * holding->avgprc_long +
                                    pInvestorPositionDetail->Volume * pInvestorPositionDetail->OpenPrice) /
                                    (holding->qty_long + pInvestorPositionDetail->Volume);
            holding->qty_long += pInvestorPositionDetail->Volume;
            if (string(pInvestorPositionDetail->OpenDate) < string(pInvestorPositionDetail->TradingDay)) { // yestoday
                holding->qty_long_yd += pInvestorPositionDetail->Volume;
            }
        }
        else if (pInvestorPositionDetail->Direction == THOST_FTDC_D_Sell) { // long
            holding->avgprc_short = (holding->qty_short * holding->avgprc_short +
                                        pInvestorPositionDetail->Volume * pInvestorPositionDetail->OpenPrice) /
                                    (holding->qty_short + pInvestorPositionDetail->Volume);
            holding->qty_short += pInvestorPositionDetail->Volume;
            if (string(pInvestorPositionDetail->OpenDate) < string(pInvestorPositionDetail->TradingDay)) { // yestoday
                holding->qty_short_yd += pInvestorPositionDetail->Volume;
            }
        }
        }
        dump(holding);
  }
  
    if (bIsLast){
        std::ofstream ofs;
        ofs.open(m_position_file, std::ios::out);
        for (auto iter = holdings.begin(); iter != holdings.end(); ++iter) {
        ofs << iter->second->symbol << "," << (int)iter->second->qty_long_yd << "," << (int)iter->second->qty_short_yd << "," 
            << (int)iter->second->qty_long << "," << (int)iter->second->qty_short << endl;
        }
        ofs.close();
        success = true;
        finished = true;
        LOG_INFO("CTP Qry Position done");
        qryInstrument();        
    }
}

void hfs_ctp_adaptor::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if(IsErrorRspInfo(pRspInfo)) {
      return;
  }
  // LOG_INFO("OnRspQryInstrument:instrumentID:{},exchangeID:{},VolumeMultiple:{},PriceTick:{},LongMarginRatio:{},ShortMarginRatio:{}",
  //          pInstrument->InstrumentID, pInstrument->ExchangeID, pInstrument->VolumeMultiple,
  //          pInstrument->PriceTick, pInstrument->LongMarginRatio, pInstrument->ShortMarginRatio);

  string instrumentID = pInstrument->InstrumentID;
  auto iter = insInfoMap.find(instrumentID);
  if (iter == insInfoMap.end()) {
      hfs_instrument *ins = new hfs_instrument();
      ins->tkr = instrumentID;
      ins->exch = pInstrument->ExchangeID;
      ins->valid = true;
      ins->multiplier = (float)pInstrument->VolumeMultiple;
      ins->marginrate = (float)pInstrument->LongMarginRatio;
      insInfoMap.insert(make_pair(instrumentID, ins));
      priceTickMap[instrumentID] = pInstrument->PriceTick;
  }
  if (bIsLast) {
    success = true;
    finished = true;
  }
}

///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
void hfs_ctp_adaptor::OnFrontDisconnected(int nReason)
{
    LOG_INFO("[ctp adaptor] OnFrontDisconnected Reason = {}", nReason);
    m_state = CTP_DISCONNECTED;

    //重新登录CTP， 此时可能需要比较久的等待
    // LOG_INFO("[ctp adaptor] Restart...");
    // if(0 == Login()) {
    //     LOG_INFO("[ctp adaptor] Restart successfully!");
    // }
    // else {
    //     LOG_INFO("[ctp adaptor] Restart failed!");
    // }
}

void hfs_ctp_adaptor::OnHeartBeatWarning(int nTimeLapse)
{
    LOG_INFO("[ctp adaptor] OnHeartBeatWarning nTimerLapse = {}", nTimeLapse);
}

void hfs_ctp_adaptor::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG_INFO("[ctp adaptor] OnRspError");
    IsErrorRspInfo(pRspInfo);
}

bool hfs_ctp_adaptor::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        LOG_INFO("[ctp adaptor] ErrorID={},ErrorMsg={}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    return bResult;
}

/*******************************************************************
 * 以上完成登录过程：连接->登录->查询持仓
 * 之后开始允许委托、撤单、处理成交反馈
 * 如果CTP断掉连接，CTP自身会自动启用重连函数
 * 如果因为物理原因导致CTP断线，需要重启程序，恢复之前的状态
*******************************************************************/

void hfs_ctp_adaptor::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    char buf[1024];
    sprintf(buf, "[ctp adaptor] OnRspOrderInsert %d %d ref=%s ID=%d msg=%s\n", nRequestID, bIsLast, pInputOrder->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    LOG_INFO(buf);

    string ref = pInputOrder->OrderRef;
    EntrustNode *enode = getENode(ref);
    if(enode == nullptr) {
        LOG_ERROR("找不到对应的EntrustNode！ ref = {}", ref);
        return;
    }

    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    order.qty   = enode->qty;
    order.type = 'J';

    int teid = enode->nid.teid;
    if(!onResponse(teid, m_id, order)) {
        LOG_ERROR("转发消息失败！");
        return;
    }
}

void hfs_ctp_adaptor::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
    char buf[1024];
    sprintf(buf, "[ctp adaptor] OnErrRtnOrderInsert ref=%s ID=%d msg=%s\n", pInputOrder->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    LOG_INFO(buf);

    string ref = pInputOrder->OrderRef;
    EntrustNode *enode = getENode(ref);
    if(enode == NULL) {
        LOG_ERROR("找不到对应的EntrustNode！ ref = {}", ref);
        return;
    }

    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    // order.teid = enode->nid.teid;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    order.qty = enode->qty;
    order.type = 'J';

    int teid = enode->nid.teid;
    if(!onResponse(teid, m_id, order)) {
        LOG_ERROR("转发消息失败！");
        return;
    }
}

void hfs_ctp_adaptor::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char buf[1024];
    sprintf(buf, "[ctp adaptor] OnRspOrderAction ref=%s ID=%d msg=%s", pInputOrderAction->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    LOG_INFO(buf);
}

void hfs_ctp_adaptor::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {
    char buf[1024];
    sprintf(buf, "[ctp adaptor] OnErrRtnOrderAction ref=%s ID=%d msg=%s", pOrderAction->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    LOG_INFO(buf);

}

///报单通知
void hfs_ctp_adaptor::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    if(!IsMyOrder(pOrder)) return;

    // LOG_INFO("报单响应：{} {} {} {}", pOrder->OrderRef, pOrder->OrderSubmitStatus, pOrder->OrderStatus, pOrder->StatusMsg);

    printFtdcOrder(pOrder);

    string ref = pOrder->OrderRef;
    EntrustNode *enode = getENode(ref);
    if(enode == nullptr) {
        LOG_ERROR("找不到对应的EntrustNode！ ref = {}", ref);
        return;
    }
    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted) { 
        order.qty = enode->qty;
        order.type = 'A';
        order.state = ORD_NEW;  
        if(pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
        order.type = 'C';
        order.state = ORD_CANCELED;
        order.qty = pOrder->VolumeTotal;
        }  
    }
    else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
        order.qty = enode->qty;
        order.type = 'J';
        order.state = ORD_REJECTED;
    }
    else if(pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
        order.type = 'C';
        order.state = ORD_CANCELED;
        order.qty = pOrder->VolumeTotal;
    }
    else if(pOrder->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected) {
        //sendFeedback(pOrder->OrderRef, 'J');
        return;
    }
    else {
        return; //do nothing
    }
    int teid = enode->nid.teid;
    if(!onResponse(teid, m_id, order)) {
        LOG_ERROR("转发消息失败！");
        return;
    } 
}

///成交通知
void hfs_ctp_adaptor::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    string ref = pTrade->OrderRef;
    EntrustNode *enode = getENode(ref);
    if(enode == NULL) {
        LOG_ERROR("找不到对应的EntrustNode！ ref = {}", ref);
        return;
    }

    float prc = (float)pTrade->Price;
    int   qty = pTrade->Volume;

    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    order.type = 'E';
    order.qty  = qty;
    order.prc  = prc;

    int teid = enode->nid.teid;

    enode->eqty+= qty;

    // 更新持仓
    // float profit = 0.0; // 平仓盈亏
    // float releaseMargin = 0.0; // 释放保证金
    if (holdings.find(order.symbol) == holdings.end()) {
        hfs_holdings_t *holding = new hfs_holdings_t();
        memset(holding, 0, sizeof(hfs_holdings_t));
        strcpy(holding->symbol, pTrade->InstrumentID);
        strcpy(holding->exch, pTrade->ExchangeID);
        holding->cidx = order.idnum;
        holdings.insert(make_pair(order.symbol, holding));
    }
    hfs_holdings_t *holding = holdings[order.symbol];

    if (pTrade->Direction == THOST_FTDC_D_Buy) {
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) { // buy open
            holding->avgprc_long = (holding->qty_long * holding->avgprc_long +
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_long + pTrade->Volume);
            holding->qty_long += pTrade->Volume;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_Close) { // buy close 
            holding->avgprc_short = (holding->qty_short * holding->avgprc_short -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_short - pTrade->Volume);
            holding->qty_short -= pTrade->Volume;
            if (holding->qty_short == 0) {
                holding->avgprc_short = 0;
            }
            if (holding->qty_short_yd > 0)
            holding->qty_short_yd -= pTrade->Volume;
            // profit = (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseYesterday) { // close yd
            holding->avgprc_short = (holding->qty_short * holding->avgprc_short -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_short - pTrade->Volume);

            holding->qty_short_yd -= pTrade->Volume;
            holding->qty_short -= pTrade->Volume;
            if (holding->qty_short == 0) {
                holding->avgprc_short = 0;
            }
            // profit = (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday) { // close td
            holding->avgprc_short = (holding->qty_short * holding->avgprc_short -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_short - pTrade->Volume);

            holding->qty_short -= pTrade->Volume;
            if (holding->qty_short == 0) {
                holding->avgprc_short = 0;
            }
            // profit = (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
    }
    else if (pTrade->Direction == THOST_FTDC_D_Sell) {
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) { // sell open
            holding->avgprc_short = (holding->qty_short * holding->avgprc_short +
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_short + pTrade->Volume);
            holding->qty_short += pTrade->Volume;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_Close) { // sell close
            holding->avgprc_long = (holding->qty_long * holding->avgprc_long -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_long - pTrade->Volume);
            holding->qty_long -= pTrade->Volume;
            if (holding->qty_long_yd > 0)
            holding->qty_long_yd -= pTrade->Volume;
            if (holding->qty_long == 0) {
                holding->avgprc_long = 0;
            }
            // profit = -1 * (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseYesterday) { // close yd
            holding->avgprc_long = (holding->qty_long * holding->avgprc_long -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_long - pTrade->Volume);
            holding->qty_long_yd -= pTrade->Volume;
            holding->qty_long -= pTrade->Volume;
            if (holding->qty_long == 0) {
                holding->avgprc_long = 0;
            }
            // profit = -1 * (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
        else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday) { // close td
            holding->avgprc_long = (holding->qty_long * holding->avgprc_long -
                                    pTrade->Volume * pTrade->Price) /
                                    (holding->qty_long - pTrade->Volume);
            holding->qty_long -= pTrade->Volume;
            if (holding->qty_long == 0) {
                holding->avgprc_long = 0;
            }
            // profit = -1 * (od->prc - pTrade->Price) * pTrade->Volume * insInfo->multiplier;
            // releaseMargin = pTrade->Price * pTrade->Volume * insInfo->multiplier * insInfo->marginrate;
        }
    }
    dump(holding);

    if(!onResponse(teid, m_id, order)) {
        LOG_ERROR("转发消息失败！");
        return;
    }
}

bool hfs_ctp_adaptor::IsMyOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->FrontID == FRONT_ID) &&
	     (pOrder->SessionID == SESSION_ID));
}

bool hfs_ctp_adaptor::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
  return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
	  (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
	  (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

int hfs_ctp_adaptor::startUserAPI() {
    pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
    pUserApi->RegisterSpi((CThostFtdcTraderSpi*)this);
    pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);                    // 注册公有流
    pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);                // 注册私有流
    pUserApi->RegisterFront((char*)params.TradeAddress.c_str());      // connect
    pUserApi->Init();
    pUserApi->Join();

    return 0;
}

int hfs_ctp_adaptor::Login() {
    // std::lock_guard<std::mutex> lk(mtxCommon);

    for(;m_state<=CTP_DISCONNECTED;) { //等待到连接成功
        sleep(10); 
    } 
    // 认证
    if (params.AuthKey != "") {
      CThostFtdcReqAuthenticateField field;
      memset(&field, 0, sizeof(field));
      strcpy(field.BrokerID, params.BrokerID.c_str());
      strcpy(field.UserID, params.UserName.c_str());
      strcpy(field.UserProductInfo, "Bsim");
      strcpy(field.AuthCode, params.AuthKey.c_str());
      strcpy(field.AppID, m_app_id.c_str());
      finished = success = false;
      int r = pUserApi->ReqAuthenticate(&field, ++iRequestID);
      if (r != 0) {
          cout << "ReqAuthenticate failed " << r << endl; 
      }
      //等待登陆成功
      //for(;!finished;) { sleep(10); }
    } else {
        //成功，发起登录请求
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, params.BrokerID.c_str());
        strcpy(req.UserID, params.UserName.c_str());
        strcpy(req.Password, params.Password.c_str());
        finished = success = false;
        int ret = pUserApi->ReqUserLogin(&req, ++iRequestID);
        if(ret != 0) {
            LOG_ERROR("send login error!");
            return -1; //发送登录请求失败
        }
    }    

    return 0;
}
void hfs_ctp_adaptor::qryPosition() {
    // 查询持仓
    sleep(1000);
    cout <<"qryPosition" << endl;
    CThostFtdcQryInvestorPositionDetailField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, params.BrokerID.c_str());
    strcpy(req.InvestorID, params.UserName.c_str());
    finished = success = false;
    int ret = pUserApi->ReqQryInvestorPositionDetail(&req, ++iRequestID);
    if (ret != 0) {
        cout << "req position error, errorid:" << ret << endl;
    }

}
void hfs_ctp_adaptor::qryInstrument() {
    // 查询合约信息  priceTick  
    sleep(1000);
    cout << "qry instrument" << endl;
    CThostFtdcQryInstrumentField field;
    memset(&field, 0, sizeof(field));
    finished = success = false;
    int ret = pUserApi->ReqQryInstrument(&field, ++iRequestID);
    if (ret != 0) {
        cout << "qry instrument error : " << ret << endl;
    }
}
double hfs_ctp_adaptor::myfloor(double price, double tick) {
    return int(price / tick + 0.5) * tick;
}

void hfs_ctp_adaptor::sleep(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

void hfs_ctp_adaptor::printFtdcOrder(CThostFtdcOrderField *pOrder) {
    LOG_INFO("[tradingtool] OnRtnOrder:instrumentID:{}, OrderRef:{}, VolumeTotalOriginal:{}, VolumeTrade:{}, StatusMsg:{}, OrderStatus:{}, OrderSubmitStatus:{}",
             pOrder->InstrumentID, pOrder->OrderRef, pOrder->VolumeTotalOriginal, pOrder->VolumeTraded,
             pOrder->StatusMsg, (char)pOrder->OrderStatus, (char)pOrder->OrderSubmitStatus);
}


void hfs_ctp_adaptor::printTrade(CThostFtdcTradeField *pTrade) {
        LOG_INFO("[tradingtool] OnRtnTrade:order_ref:{}, instrumentID:{}, side:{},price:{},volume:{}",
             pTrade->OrderRef, pTrade->InstrumentID, (pTrade->Direction == THOST_FTDC_D_Buy ? 'B' : 'S'),
             pTrade->Price, pTrade->Volume);
}

void hfs_ctp_adaptor::printInputOrder(CThostFtdcInputOrderField *pOrder) {
    LOG_INFO("[tradingtool] newEntrust:InstrumentID:{}, OrderRef:{}, Direction:{}, CombOffsetFlag:{}, LimitPrice:{}, VolumeTotalOriginal:{}",
	  pOrder->InstrumentID, pOrder->OrderRef, pOrder->Direction, pOrder->CombOffsetFlag, pOrder->LimitPrice, pOrder->VolumeTotalOriginal);
}

void hfs_ctp_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_ctp_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}
