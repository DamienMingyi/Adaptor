#include "hfs_jt_adaptor.hpp"
#include "hfs_log.hpp"
#include <thread>

hfs_jt_adaptor::hfs_jt_adaptor(const a2ftool::XmlNode &_cfg)
  :hfs_base_adaptor(_cfg) {
    iRequestID = 0;
    m_state = INIT;

    int timex = current();
	m_orderRef = (timex/10000*3600)+((timex%10000)/100*60)+(timex%100);
    m_orderRef = (today()%10)*100 + m_orderRef;
    //TODO 委托批号设置， 批量撤单用
    m_order_bsn = _cfg.getAttrDefault("OrderBsn", 100);
    m_rc = new RiskControl(_cfg);
    m_dte = today();
}

hfs_jt_adaptor::~hfs_jt_adaptor() {
    if(pUserApi) {
        pUserApi->Exit();
        delete pUserApi;
    }

}

bool hfs_jt_adaptor::start(){
    //初始化
    readConfig();
    pUserApi = new CCliCosTradeApi();
    pUserApi->RegisterSpi(this);
    pUserApi->RegisterServer(params.Server.c_str(), params.Port);
    // 初始化连接
	int ret = pUserApi->Init();
	if (ret != 0)
	{    
		printf("交易API 初始化失败\n");
		cout << ShowErrorInfo(ret) << endl;	
		cout << pUserApi->GetLastErrorText() << std::endl;		
        return false;
	}
	else
	{
		printf("交易API初始化成功\n");
    }
    pUserApi->RegisterAcct('D', params.SysInfo.c_str(), params.SysInfo.length());
    Login();
    StkLogin();
    subTopic();
    auto qry_thread = std::thread(std::bind(&hfs_jt_adaptor::qryThreadFunc, this));
    qry_thread.detach();
    // qryAccount();
    // m_pos_str = "";
    // qryPosition();
    // while(!finished) sleep(1);
    // while(true) {
    //     if (m_pos_str.empty()) {
    //         cout << "qryPosition done" << endl;
    //         break;
    //     }
    //     qryPosition();
    //     while(!finished) sleep(1);
    // }
    // m_pos_str = "";
    // qryOrder();
    // while(!finished) sleep(1);
    // while(true) {
    //     if (m_pos_str.empty()) {
    //         cout << "qryOrder done" << endl;
    //         break;
    //     }
    //     qryOrder();
    //     while(!finished) sleep(1);
    // }
    // m_pos_str = "";    
    // qryTrade();
    // while(!finished) sleep(1);
    // while(true) {
    //     if (m_pos_str.empty()) {
    //         cout << "qryTrade done" << endl;
    //         break;
    //     }
    //     qryTrade();
    //     while(!finished) sleep(1);
    // }
    ret = cancelOrderByBsn(m_order_bsn);
    return ret == 0;
}
void hfs_jt_adaptor::initFile()
{
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

    sprintf(fname, "%s/STOCK_ACCOUNT_%d.csv", m_export_path.c_str(), today());
    if (m_account_file.is_open())
    {
        m_account_file.close();
    }
    m_account_file.open(fname, std::ios::out);
    m_account_file << "Account,Available,MarketValue,AssetBalance" << endl;
}
bool hfs_jt_adaptor::stop() {
    if(pUserApi) {
        pUserApi->Exit();
    }
    return true;
}

bool hfs_jt_adaptor::isLoggedOn() {
    return loggedon;
}

bool hfs_jt_adaptor::onRequest(int teid, hfs_order_t &order) {
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

// bool hfs_jt_adaptor::onResponse(int teid, std::string execid, hfs_order_t* order) {
  //return adap.onResponse(teid, execid, order);
  // return ordfunc(teid,execid,order);
  // return hfs_trading_tool::getInstance()->push_exch_order(order);
// }

bool hfs_jt_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
    if (m_state != LOGGEDON) {
		LOG_INFO("[jt adaptor]adaptor not login, entrust failed");
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
    if (m_rc->OnInsertOrder(&order) == false || m_rc->IsCanInsert() == false) {
		LOG_INFO("[jt adaptor {}] entrust failed, RiskControl refused", m_id);
		push_reject_order(teid, order, "RickControl refused");
		return false;
	}
  
    assert(order.type == 'O');  
  
    CReqCosOrderField field;
    memset(&field, 0, sizeof(field));
    sprintf(field.szCuacctCode, "%lld", m_acctCode);

    string symbol(order.symbol);
    if (symbol.find("SH") != string::npos) {
        strcpy(field.szStkbd, "10");  // 上海A股
        strncpy(field.szTrdacct, m_trdAcct_SH.c_str(), sizeof(field.szTrdacct)-1);
    } else {
        strcpy(field.szStkbd, "00");
        strncpy(field.szTrdacct, m_trdAcct_SZ.c_str(), sizeof(field.szTrdacct)-1);
    }
    strcpy(field.szTrdCode, symbol.substr(0,6).c_str());
    sprintf(field.szOrderPrice, "%.2f", round(order.prc * 100) / 100);
    field.llOrderQty = order.qty;
    // 证券业务
    if (order.side == 'B')
        field.iStkBiz = 100;
    else field.iStkBiz = 101;

    field.iStkBizAction = 100;  // 限价委托  101 撤单委托
    field.iAttrCode = 0;
    char buf[10] = {0};
    // if (++m_orderRef > 1e5) m_orderRef = 1;
	sprintf(buf, "%d_%d", teid, ++m_orderRef);
    field.iOrderBsn = m_order_bsn;
    strncpy(field.szCliOrderNo, buf, sizeof(field.szCliOrderNo));

    strncpy(field.szCliRemark, params.Remark.c_str(), sizeof(field.szCliRemark));
    if (pUserApi->ReqOrder(&field, ++iRequestID) != 0)
    {
        LOG_ERROR("[sw adpator] entrusts failed. msg:{}", pUserApi->GetLastErrorText());
        push_reject_order(teid, order, "ReqOrder failed");
        return false;
    }
    string sid(buf);
    if(!registerTrader(teid, order, sid)) {
        LOG_ERROR("registert trader failed : {} {} {} {}", teid, order.pid, order.oseq, sid);
    }
    mutexOrder.lock();
    request2clientid.insert(make_pair(iRequestID, sid));
    mutexOrder.unlock();
    return true;
}

bool hfs_jt_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
    assert(order.type == 'X');

    if (m_rc->IsCanCancel() == false) {
		LOG_INFO("[ht adaptor {}] RiskControl refuse cancel request", m_id);
		return false;
	}

    EntrustNode *enode = getENode(teid, order.pid, order.oseq);
    if(!enode) {
        LOG_ERROR("试图cancel不存在的order！ teid= {} oseq={}", teid, order.oseq);
        return true;
    }
    // string ref = enode->sid;

    CReqCosCancelOrderField field;
    memset(&field, 0, sizeof(field));
    sprintf(field.szCuacctCode, "%lld", m_acctCode);
    
    // string symbol(enode->origOrder.symbol);
    // if (symbol.find("SH") != string::npos) {
    //     strcpy(field.szStkbd, "10");  // 上海A股
    // } else {
    //     strcpy(field.szStkbd, "00");
    // }
    field.iOrderDate = m_dte;
    field.iOrderNo = atoi(enode->origOrder.orderid);
    strncpy(field.szCliRemark, params.Remark.c_str(), sizeof(field.szCliRemark));
    int iResult = pUserApi->ReqCancelOrder(&field, ++iRequestID);
    if(iResult != 0) {
        LOG_ERROR("[sw adpator] cancel failed. msg:{}", pUserApi->GetLastErrorText());
        return false;
    }

    return true;
}

void hfs_jt_adaptor::push_reject_order(int teid, hfs_order_t &order, const char *errmsg)
{
    hfs_order_t od;
    memcpy(&od, &order, sizeof(hfs_order_t));
    // od.qty = order.qty;
    od.type = 'J';
    od.state = ORD_REJECTED;
    strncpy(od.algo, errmsg, sizeof(od.algo) - 1);
    if (!onResponse(teid, m_id, od))
    {
        LOG_ERROR("[jt adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}", m_id, teid, order.pid, order.oseq);
    }
}

int hfs_jt_adaptor::cancelOrderByBsn(int bsn) {
    
    CReqCosCancelOrderField field;
    memset(&field, 0, sizeof(field));
    sprintf(field.szCuacctCode, "%lld", m_acctCode);

    field.iOrderBsn = bsn;
    int iResult = pUserApi->ReqCancelOrder(&field, ++iRequestID);
    if(iResult != 0) {
        LOG_ERROR("[sw adpator] cancel failed. msg:{}", pUserApi->GetLastErrorText());
        return -1;
    }

    return 0;
}

bool hfs_jt_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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
        this->nmap[node->nid] = node;
	}
    {
        std::lock_guard<std::mutex> lk(m_sid_mtx);
        this->smap[sid] = node;
    }
    LOG_INFO("[jt adaptor {}]register trader : {}", m_id, sid);
    return true;
  }
  catch(std::exception &ex) {
        LOG_ERROR("注册trader失败！原因：{}", ex.what());
        return false;
  }
}

EntrustNode *hfs_jt_adaptor::getENode(int teid, int pid, int oseq) {
    OrderID oid = OrderID(teid, pid, oseq);
    return getENode(oid);
}
EntrustNode *hfs_jt_adaptor::getENode(OrderID& oid) {
	std::lock_guard<std::mutex> lk(m_nid_mtx);
	EntrustNode *rv = nullptr;
	if (nmap.find(oid) != nmap.end()) {
		return nmap[oid];
	}
	return rv;
}

EntrustNode *hfs_jt_adaptor::getENode(string sid) {
    std::lock_guard<std::mutex> lk(m_sid_mtx);
    EntrustNode *enode = nullptr;

    auto iter = smap.find(sid);
    if (iter != smap.end()) {
        enode = smap[sid];
    }
    return enode;
}

bool hfs_jt_adaptor::readConfig() {
    params.Server       = cfg.getAttrDefault("server", "");
    params.Port         = cfg.getAttrDefault("port", 0);
    params.AcctType     = cfg.getAttrDefault("accttype", "Z");
    params.UserName     = cfg.getAttrDefault("account", "");
    params.Password     = cfg.getAttrDefault("password", "");
    params.AuthData     = cfg.getAttrDefault("authdata", "");
    // params.Channel      = cfg.getAttrDefault("Channel", '0');
    params.SysInfo      = cfg.getAttrDefault("sysInfo", "");
    params.Remark       = cfg.getAttrDefault("Remark", "");
    params.cos_user     = cfg.getAttrDefault("cosuser", "");
    params.cos_password = cfg.getAttrDefault("cospassword", "");
    params.cos_authdata = cfg.getAttrDefault("cosauthdata", "");
    m_id                = cfg.getAttrDefault("id", "");
    m_export_path       = cfg.getAttrDefault("exportpath", "./");
    // m_prefix            = ctpcfg.getAttrDefault("prefix", 100000);
    return true;
}

/////////////////////////////////////////////////////////////////
// 客户端与服务器成功建立通信连接后，该方法被调用
int hfs_jt_adaptor::OnConnected(void)
{
    cout << "[jt adaptor] connected " << endl;
    m_state = CONNECTED;    
    return 0;
}

// 客户端与服务器成功的通信连接断开时，该方法被调用
int hfs_jt_adaptor::OnDisconnected(int p_nReason, const char *p_pszErrInfo)
{
    cout << "[jt adaptor] disconnected" << endl;
    LOG_INFO("[jt adaptor] server disconnect, id:{}, code:{}, msg:{}", m_id, p_nReason, p_pszErrInfo);
    m_state = DISCONNECTED;
    
    return 0;
}

int hfs_jt_adaptor::OnRspHeartBeat(CFirstSetField *p_pFirstSet, CRspHeartBeatField *p_pRspHeartBeatField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSet == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspHeartBeat p_pFirstSet is NULL");
        return -1;
    }
    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspHeartBeat MsgCode:{}, MsgLevel:{}, MsgDebug:{}, MsgText:{}",
                    p_pFirstSet->iMsgCode, p_pFirstSet->chMsgLevel, p_pFirstSet->szMsgDebug, p_pFirstSet->szMsgText);
    }
    if (/*p_iFieldNum > 0 &&*/ NULL != p_pRspHeartBeatField)
    {
        LOG_INFO("[jt adaptor] OnRspHeartBeat ServerName:{}, IP:{}, NodeID:{}, NodeGID:{}, NodePW:{}, NodeType:{}, NodeUse:{}, LocalNodeID:{}, BackupIP:{}",
                    p_pRspHeartBeatField->szServerSiteName, p_pRspHeartBeatField->szServerSiteIP, p_pRspHeartBeatField->szServerNodeId, 
                    p_pRspHeartBeatField->szServerNodeGid, p_pRspHeartBeatField->szServerNodePwd, p_pRspHeartBeatField->szServerNodeType,
                    p_pRspHeartBeatField->szServerLocalNodeId, p_pRspHeartBeatField->szServerBackIp);
    }
    return 0;
}

// 用户登录请求响应
int hfs_jt_adaptor::OnRspStkUserLogin(CFirstSetField *p_pFirstSetField, CRspStkUserLoginField *p_pRspUserLoginField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspUserLogin p_pFirstSet is NULL");
        return -1;
    }

    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspUserLogin code:{}, msg:{}", p_pFirstSetField->iMsgCode, p_pFirstSetField->szMsgText);        
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspUserLoginField)
    {
        
        loggedon = true;
        m_state = LOGGEDON;
        m_custCode=p_pRspUserLoginField->llCustCode;
        m_acctCode=p_pRspUserLoginField->llCuacctCode;
        if (p_pRspUserLoginField->chStkex == '0') {
            m_trdAcct_SZ = p_pRspUserLoginField->szStkTrdacct;
        }
        else m_trdAcct_SH = p_pRspUserLoginField->szStkTrdacct;

        LOG_INFO("[jt adaptor] OnRspUserLogin CustCode:{}. CuacctCode:{}, chStkex:{}, szStkbd:{}, szStkTrdacct:{}, iIntOrg:{}, chTrdacctStatus:{}, szStkpbu:{}, szAcctType:{}, szAcctId:{}, szTrdacctName:{}, szSessionId:{}",
            p_pRspUserLoginField->llCustCode, p_pRspUserLoginField->llCuacctCode, p_pRspUserLoginField->chStkex,
            p_pRspUserLoginField->szStkbd, p_pRspUserLoginField->szStkTrdacct, p_pRspUserLoginField->iIntOrg, 
            p_pRspUserLoginField->chTrdacctStatus, p_pRspUserLoginField->szStkpbu, p_pRspUserLoginField->szAcctType,
            p_pRspUserLoginField->szAcctId, p_pRspUserLoginField->szTrdacctName, p_pRspUserLoginField->szSessionId);
    }
    if (p_iFieldNum == p_iFieldIndex) {
        LOG_INFO("[jt adaptor] login success acctCode:{}, trdAcct_SH:{}, trdAcct_SZ:{}", m_acctCode, m_trdAcct_SH, m_trdAcct_SZ);
        finished = success = true;
    }
    return 0;
}

int hfs_jt_adaptor::OnRspCosLogin(CFirstSetField *p_pFirstSetField, CRspCosLoginField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspCosLogin p_pFirstSet is NULL");
        return -1;
    }

    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspCosLogin code:{}, msg:{}", p_pFirstSetField->iMsgCode, p_pFirstSetField->szMsgText);        
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {        
        loggedon = true;
        m_state = LOGGEDON;
        m_session_id = p_pRspField->szSessionId;

        LOG_INFO("[jt adaptor] OnRspCosLogin szSessionId:{}",
            p_pRspField->szSessionId);
    }
    if (p_iFieldNum == p_iFieldIndex) {
        // LOG_INFO("[jt adaptor] login success acctCode:{}, trdAcct_SH:{}, trdAcct_SZ:{}", m_acctCode, m_trdAcct_SH, m_trdAcct_SZ);
        finished = success = true;
    }
    return 0;
}

int hfs_jt_adaptor::OnRspAutoLogin(CFirstSetField *p_pFirstSet, CRspCosAutoLoginField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    cout << "OnRspAutoLogin" << endl;
    if (NULL==p_pFirstSet) return -1;
    if (NULL==p_pRspField) {
        LOG_ERROR("[jt adaptor {}] OnRspAutoLogin get error, error_code:{}, error_msg:{}", p_pFirstSet->iMsgCode, p_pFirstSet->szMsgText);
    }
    return 0;
}
// 买卖委托响应
int hfs_jt_adaptor::OnRspOrder(CFirstSetField *p_pFirstSetField, CRspCosOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspOrder p_pFirstSet is NULL");
        return -1;
    }  
    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        if (IsErrorRspInfo(p_pFirstSetField, "OnRspOrder")) {
            // reject
            mutexOrder.lock();
            string client_id = request2clientid[p_llRequestId];
            mutexOrder.unlock();
            EntrustNode* enode = getENode(client_id);
            if (enode == nullptr) {
                LOG_INFO("[jt adaptor {}] OnRspOrder cannot find enode : {}",m_id, p_llRequestId);
                return 0;
            }
            hfs_order_t order;
            memset(&order, 0, sizeof(hfs_order_t));
            strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
            order.idnum = enode->origOrder.idnum;
            order.side = enode->origOrder.side;
            order.ocflag = enode->origOrder.ocflag;
            order.pid = enode->nid.pid;
            order.oseq = enode->nid.oseq;
            order.qty = enode->qty;	
            order.type = 'J';
            order.state = ORD_REJECTED;
            enode->origOrder.state = ORD_REJECTED;
            if (!onResponse(enode->nid.teid, m_id, order)) {
                LOG_ERROR("[jt adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
            }
        }
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspOrder FieldNum:{}, FieldIndex:{}, 客户委托号:{}, 委托批号:{}, 合同序号:{}, 资产账户:{}, 委托数量:{}, 执行状态:{}, 执行信息:{}",
                 p_iFieldNum, p_iFieldIndex, p_pRspField->szCliOrderNo, p_pRspField->iOrderBsn, p_pRspField->iOrderNo, p_pRspField->szCuacctCode,
                 p_pRspField->llOrderQty, p_pRspField->chExeStatus, p_pRspField->szExeInfo);

        string client_id = p_pRspField->szCliOrderNo;
        char status = p_pRspField->chExeStatus;
        EntrustNode* enode = getENode(client_id);
        if (enode == nullptr) {
            LOG_INFO("[jt adaptor {}] OnRspOrder cannot find enode : {}",m_id, client_id);
            return 0;
        }
        hfs_order_t order;
        memset(&order, 0, sizeof(hfs_order_t));
        strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
        sprintf(enode->origOrder.orderid, "%d", p_pRspField->iOrderNo);
        order.idnum = enode->origOrder.idnum;
        order.side = enode->origOrder.side;
        order.ocflag = enode->origOrder.ocflag;
        order.pid = enode->nid.pid;
        order.oseq = enode->nid.oseq;
        order.qty = enode->qty;	
        order.prc = enode->origOrder.prc;
        if (status == '2') {
            order.type = 'A';
            order.state = ORD_NEW;
            enode->origOrder.state = ORD_NEW;
        } else if (status == '9') {
            order.type = 'J';
            order.state = ORD_REJECTED;
            enode->origOrder.state = ORD_REJECTED;
            strncpy(order.algo, p_pRspField->szExeInfo, sizeof(order.algo));
        } else if (status == '5' || status == '6') {
            order.type = 'C';
            order.state = ORD_CANCELED;
            enode->origOrder.state = ORD_CANCELED;            
        } else {
            return 0;
        }

        if (!onResponse(enode->nid.teid, m_id, order)) {
            LOG_ERROR("[jt adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
        }
    }
    return 0;
}
// 委托撤单响应
int hfs_jt_adaptor::OnRspCancelOrder(CFirstSetField *p_pFirstSetField, CRspCosCancelOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspCancelOrder p_pFirstSet is NULL");
        return -1;
    }    
    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspCancelOrder 委托撤单响应: code:{}, msg:{}", p_pFirstSetField->iMsgCode ,p_pFirstSetField->szMsgText); 
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspCancelOrder FieldNum:{}. FieldIndex:{}, 委托批号:{}, 合同序号:{}, 资产账户:{}, 执行状态:{}, 执行信息:{}",
               p_iFieldNum, p_iFieldIndex, p_pRspField->iOrderBsn, p_pRspField->iOrderNo, p_pRspField->szCuacctCode, p_pRspField->chExeStatus, p_pRspField->szExeInfo);
    }
    return 0;
}
// 最大可交易数计算响应
int hfs_jt_adaptor::OnRspMaxTradeQty(CFirstSetField *p_pFirstSet, CRspStkMaxTradeQtyField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex)
{
    if (p_pFirstSet == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspMaxTradeQty p_pFirstSet is NULL");
        return -1;
    }    
    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspMaxTradeQty code:{}, msg:{}", p_pFirstSet->iMsgCode, p_pFirstSet->szMsgText);
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspMaxTradeQty 交易市场:{}, 交易板块:{}, 证券代码:{}, 最大委托数量:{}, 证券账户:{}",
                p_pRspField->chStkex, p_pRspField->szStkbd, p_pRspField->szStkCode, p_pRspField->llOrderQty, p_pRspField->szTrdacct);      
    }
    return 0;
}
// 批量委托响应
int hfs_jt_adaptor::OnRspQuantityOrder(CFirstSetField *p_pFirstSetField, CRspStkQuantityOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRspQuantityOrder p_pFirstSet is NULL");
        return -1;
    }

    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {
        LOG_INFO("[jt adaptor] OnRspQuantityOrder code:{}, msg:{}", p_pFirstSetField->iMsgCode, p_pFirstSetField->szMsgText);
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        // g_vc6_vs10_IO<<"返回内容[" << p_iFieldNum <<","<<p_iFieldIndex<< "]:" <<std::endl;
        // g_vc6_vs10_IO<<"  委托批号            :"<<p_pRspField->iOrderBsn;                     
        // g_vc6_vs10_IO<<"  资产账户            :"<<p_pRspField->llCuacctCode;                  
        // g_vc6_vs10_IO<<"  证券账户委托成功累计:"<<p_pRspField->iTrdacctSuccCnt;               
        // g_vc6_vs10_IO<<"  委托数量成功累计    :"<<p_pRspField->llOrderQtySum;                 
        // g_vc6_vs10_IO<<"  交易单元            :"<<p_pRspField->szStkpbu[8 + 1];               
        // g_vc6_vs10_IO<<"  交易板块            :"<<p_pRspField->szStkbd[2 + 1];                
        // g_vc6_vs10_IO<<"  证券代码            :"<<p_pRspField->szStkCode[8 + 1];              
        // g_vc6_vs10_IO<<"  证券名称            :"<<p_pRspField->szStkName[16 + 1];             
        // g_vc6_vs10_IO<<"  证券业务            :"<<p_pRspField->iStkBiz;                       
        // g_vc6_vs10_IO<<"  业务行为            :"<<p_pRspField->iStkBizAction;                 
        // g_vc6_vs10_IO<<"  账户序号  :"<<p_pRspField->iCuacctSn<<std::endl;								 
    }

    return 0;
}
// 批量撤单响应
int hfs_jt_adaptor::OnRspQuantityCancelOrder(CFirstSetField *p_pFirstSetField, CRspStkQuantityCancelOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_pFirstSetField == NULL) 
    {

        return -1;
    }

    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {

    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        // g_vc6_vs10_IO<<"返回内容[" << p_iFieldNum <<","<<p_iFieldIndex<< "]:" <<std::endl;
        // g_vc6_vs10_IO<<"  资产账户:"<<p_pRspField->llCuacctCode;         
        // g_vc6_vs10_IO<<"  交易板块:"<<p_pRspField->szStkbd;              
        // g_vc6_vs10_IO<<"  委托批号:"<<p_pRspField->iOrderBsn;            
        // g_vc6_vs10_IO<<"  合同序号:"<<p_pRspField->szOrderId;            
        // g_vc6_vs10_IO<<"  撤单结果:"<<p_pRspField->iCancelRet;           
        // g_vc6_vs10_IO<<"  返回信息:"<<p_pRspField->szRetInfo<<std::endl;            
    }
    return 0;
}
// 可用资金查询响应
int hfs_jt_adaptor::OnRspQryExpendableFund(CFirstSetField *p_pFirstSetField, CRspStkQryExpendableFundField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (p_iFieldNum == 0 || p_iFieldIndex == 1) 
    {	
        if (IsErrorRspInfo(p_pFirstSetField, "OnRspQryExpendableFund")) {
            finished = true;
            return -1;
        }
    }
    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspQryExpendableFund 客户代码:{}, 资产账户:{}, 货币代码:{}, 内部机构:{}, 资产总值:{}, 资金资产:{}, 市值:{}, 融资总金额:{}, 资金昨日余额:{}, 资金余额:{}, 资金可用金额:{}, 资金冻结金额:{}, 资金解冻金额:{}, 资金交易冻结金额:{}, 资金交易解冻金额:{}, 资金交易在途金额:{}, 资金交易轧差金额:{}, 资金状态:{}, 资产账户属性:{}",
                p_pRspField->llCustCode, p_pRspField->llCuacctCode, p_pRspField->chCurrency, p_pRspField->iIntOrg, 
                p_pRspField->szMarketValue, p_pRspField->szFundValue, p_pRspField->szStkValue, p_pRspField->szFundLoan, p_pRspField->szFundPrebln,
                p_pRspField->szFundBln, p_pRspField->szFundAvl, p_pRspField->szFundFrz, p_pRspField->szFundUfz, p_pRspField->szFundTrdFrz,
                p_pRspField->szFundTrdUfz, p_pRspField->szFundTrdOtd, p_pRspField->szFundTrdBln, p_pRspField->chFundStatus,
                p_pRspField->chCuacctAttr);
        m_account_file << params.UserName << "," << p_pRspField->szFundBln << "," << p_pRspField->szStkValue << "," << p_pRspField->szMarketValue << endl;
    }
    if (p_iFieldNum == p_iFieldIndex) {
        finished = success = true;
    }
    return 0;
}
// 当日委托查询响应
int hfs_jt_adaptor::OnRspQryOrderInfo(CFirstSetField *p_pFirstSetField, CRspCosOrderInfoField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {

    if (IsErrorRspInfo(p_pFirstSetField, "OnRspQryOrderInfo")) {
        success = finished = true;            
        m_pos_str = "";
        return -1;
    }
    if (p_iFieldNum == 0 || p_pFirstSetField->iMsgCode == 100) {  // no data
        LOG_INFO("[jt adaptor] qry detail done");
        success = finished = true;
        m_pos_str = "";
        m_entrust_file.close();
        return 0;
    }  

    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspQryOrderInfo 定位串查询:{}, 委托日期:{}, 委托时间:{}, 委托批号:{}, 合同序号:{}, 客户代码:{}, 资产账户:{}, 证券业务:{}, 业务行为:{}, 证券代码:{}, 委托价格:{}, 委托数量:{}, 已撤单数量:{}, 已成交数量:{}",
                p_pRspField->szQryPos, p_pRspField->iOrderDate, p_pRspField->szOrderTime, p_pRspField->szCliOrderNo,
                p_pRspField->iOrderNo, p_pRspField->chExeStatus, p_pRspField->szCustCode, 
                p_pRspField->szCuacctCode, p_pRspField->iStkBiz, p_pRspField->iStkBizAction, 
                p_pRspField->szTrdCode, p_pRspField->szOrderPrice, p_pRspField->llOrderQty, p_pRspField->llWithdrawnQty,p_pRspField->llMatchedQty);

        //"Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID"
        m_entrust_file << p_pRspField->szCuacctCode << "," << p_pRspField->szTrdCode << "," << p_pRspField->iOrderDate << ","
                        << p_pRspField->szOrderTime << "," << p_pRspField->iOrderNo << "," << p_pRspField->szCliOrderNo << ","
                        << p_pRspField->iStkBiz << "," << "," << p_pRspField->chExeStatus << "," << p_pRspField->szOrderPrice << ","
                        << p_pRspField->llOrderQty << "," << atof(p_pRspField->szMatchedAmt)/p_pRspField->llMatchedQty << ","
                        << p_pRspField->llMatchedQty << "," << p_pRspField->llWithdrawnQty << ",," << p_pRspField->szQryPos << ","
                        << endl;
    }
    if (p_iFieldNum == p_iFieldIndex) {
        success = finished = true;
        m_pos_str = p_pRspField->szQryPos;
    }
    return 0;
}
// 当日成交查询响应
int hfs_jt_adaptor::OnRspQryMatch(CFirstSetField *p_pFirstSetField, CRspCosMatchField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    if (IsErrorRspInfo(p_pFirstSetField, "OnRspQryCurrDayFill")) {
        finished = true;
        m_pos_str = "";
        return -1;
    }
    if (p_iFieldNum == 0 || p_pFirstSetField->iMsgCode == 100) {
        LOG_INFO("[jt adaptor] qry detail done");
        finished = true;
        m_detail_file.close();
        m_pos_str = "";
        return 0;
    }

    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspQryCurrDayFill 定位串查询:{}, 成交时间:{}, 委托日期:{}, 客户委托号:{}, 委托批号:{}, 合同序号:{}, 客户代码:{}, 资产账户:{}, 证券业务:{}, 证券代码:{}, 成交编号:{}, 成交价格:{}, 已成交数量:{}, 成交类型:{}",
                p_pRspField->szQryPos, p_pRspField->szMatchedTime, p_pRspField->iOrderDate, p_pRspField->szClientId, p_pRspField->iOrderBsn,
                p_pRspField->iOrderNo, p_pRspField->szCustCode, p_pRspField->szCuacctCode, p_pRspField->iStkBiz, p_pRspField->szTrdCode, p_pRspField->szMatchedSn,
                p_pRspField->szMatchedPrice, p_pRspField->llMatchedQty, p_pRspField->chMatchedType);

        if (!m_detail_file.is_open()) {
            char fname[1024] = {0};
	        sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(),today());
            m_detail_file.open(fname, std::ios::out);
        }
        // "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID"
        m_detail_file << p_pRspField->szCuacctCode << "," << p_pRspField->szTrdCode << "," << p_pRspField->iOrderDate << ","
                     << p_pRspField->szMatchedTime << "," << p_pRspField->iOrderNo << "," << p_pRspField->iStkBiz << ","
                     << p_pRspField->chMatchedType << "," << p_pRspField->szMatchedPrice << "," << p_pRspField->llMatchedQty << ","
                     << p_pRspField->szQryPos << "," << endl;
    }
    if (p_iFieldNum == p_iFieldIndex) {
        finished = success = true;
        m_pos_str = p_pRspField->szQryPos;
    }
    return 0;
}
// 股票持仓查询
int hfs_jt_adaptor::OnRspQryExpendableShares(CFirstSetField *p_pFirstSet, CRspStkQryExpendableSharesField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex)
{
    if (IsErrorRspInfo(p_pFirstSet, "OnRspQryExpendableShares")) {
        finished = true;
        m_position_file.close();
        m_pos_str = "";
        return -1;
    }
    if (p_iFieldNum == 0 || p_pFirstSet->iMsgCode == 100) {
        LOG_INFO("[jt adaptor] qry position done");
        finished = true;
        m_position_file.close();
        m_pos_str = "";
        return 0;
    }

    if (/*p_iFieldNum > 0 &&*/ NULL!=p_pRspField)
    {
        LOG_INFO("[jt adaptor] OnRspQryExpendableShares 定位串:{}, 客户代码:{}, 资产账户:{}, 交易板块:{}, 证券代码:{}, 证券名称:{}, 证券昨日余额:{}, 证券余额:{}, 证券可用数量:{}, 当前拥股数:{}, ",
                p_pRspField->szQryPos, p_pRspField->llCustCode, p_pRspField->llCuacctCode, p_pRspField->szStkbd, p_pRspField->szStkCode,
                p_pRspField->szStkName, p_pRspField->llStkPrebln, p_pRspField->llStkBln, p_pRspField->llStkAvl, p_pRspField->llStkQty);
        
        string symbol(p_pRspField->szStkCode);
        if (p_pRspField->szStkbd[0] == '0') {
            symbol.append(".SZ");
        } else if (p_pRspField->szStkbd[0] == '1') {
            symbol.append(".SH");
        }
        if (holdings.find(symbol) == holdings.end()) {
            hfs_holdings_t* holding = new hfs_holdings_t();
            memset(holding, 0, sizeof(hfs_holdings_t));
            strcpy(holding->symbol, symbol.c_str());
            holdings.insert(make_pair(symbol, holding));
        }
        hfs_holdings_t* holding = holdings[symbol];
        holding->qty_long = p_pRspField->llStkQty;
        holding->qty_long_yd = p_pRspField->llStkAvl;
        holding->avgprc_long = atof(p_pRspField->szProfitPrice);
        //"Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" 
        if (!m_position_file.is_open()) {
            LOG_ERROR("[jt adaptor] OnRspQryExpendableShares holding file not opened"); 
            char fname[1024] = {0};
	        sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(),today());
            m_position_file.open(fname, std::ios::out);
            m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;
        }
        m_position_file << p_pRspField->llCuacctCode << "," << p_pRspField->szStkCode << "," << p_pRspField->llStkQty << ","
                        << p_pRspField->llStkAvl << "," << p_pRspField->szCostPrice << "," << p_pRspField->szMktVal << ","
                        << p_pRspField->szQryPos << endl;
    }
    if (p_iFieldNum == p_iFieldIndex) {
        finished = success = true;
        m_pos_str = p_pRspField->szQryPos;
    }
    return 0;
}
int hfs_jt_adaptor::OnRtnTsuOrder(CRtnTsuOrderField *p_pOrderField)
{
    if (p_pOrderField == NULL) {
        LOG_ERROR("[jt adaptor {}] OnRtnTsuOrder failed {}", m_id, pUserApi->GetLastErrorText());
        return -1;
    }
    LOG_INFO("[jt adaptor {}] symbol:{}, client_id:{}, status:{}, errormsg:{}", m_id,
             p_pOrderField->szStkCode, p_pOrderField->szCliOrderNo, p_pOrderField->chExeStatus, p_pOrderField->szErrorMsg);

    string client_id(p_pOrderField->szCliOrderNo);
    EntrustNode *enode = getENode(client_id);
    if (enode == nullptr)
    {
        LOG_INFO("[jt adaptor {}] OnRtnTsuOrder cannot find enode : {}", m_id, client_id);
        return 0;
    }
    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    sprintf(enode->origOrder.orderid, "%d", p_pOrderField->iOrderNo);
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    //TODO 回报处理
    char status = p_pOrderField->chExeStatus;
    if (status == '2') {
        order.type = 'A';
        order.state = ORD_NEW;
        enode->origOrder.state = ORD_NEW;
    } else if (status == '9') {
        order.type = 'J';
        order.state = ORD_REJECTED;
        enode->origOrder.state = ORD_REJECTED;
        strncpy(order.algo, p_pOrderField->szErrorMsg, sizeof(order.algo));
    } else if (status == '5' || status == '6') {
        order.type = 'C';
        order.state = ORD_CANCELED;
        enode->origOrder.state = ORD_CANCELED;            
    } else {
        return 0;
    }

    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[jt adaptor {}] OnRtnTsuOrder failed, teid:{}, pid:{}, osep:{}", m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
    }
    return 0;
}
// 成交回报
int hfs_jt_adaptor::OnRtnOrder(CRtnOrderField * p_pRtnOrderFillField) {
    if (p_pRtnOrderFillField == NULL) 
    {
        LOG_INFO("[jt adaptor] OnRtnOrder p_pRtnOrderFillField is NULL");		
        return -1;
    }
    LOG_INFO("[jt adaptor] OnRtnOrder szTrdacct:{}, iCuacctSn:{}, iMatchedDate:{}, szMatchedTime:{}, szStkbd:{}, szMatchedSn:{}, szStkCode:{},  llMatchedQty:{}, szMatchedPrice:{}, szFundAvl:{}, llStkAvl:{}, chIsWithdraw:{}, iOrderBsn:{}, szCliOrderNo:{}, chMatchedType:{}, iStkBiz:{}, iStkBizAction:{}, szOfferRetMsg:{}",
        p_pRtnOrderFillField->szTrdacct, p_pRtnOrderFillField->iCuacctSn, p_pRtnOrderFillField->iMatchedDate, 
        p_pRtnOrderFillField->szMatchedTime, p_pRtnOrderFillField->szStkbd,
        p_pRtnOrderFillField->szMatchedSn, p_pRtnOrderFillField->szStkCode,
        p_pRtnOrderFillField->llMatchedQty, p_pRtnOrderFillField->szMatchedPrice, 
        p_pRtnOrderFillField->szFundAvl, p_pRtnOrderFillField->llStkAvl, p_pRtnOrderFillField->chIsWithdraw,
        p_pRtnOrderFillField->iOrderBsn, p_pRtnOrderFillField->szCliOrderNo, p_pRtnOrderFillField->chMatchedType, p_pRtnOrderFillField->iStkBiz,
        p_pRtnOrderFillField->iStkBizAction, p_pRtnOrderFillField->szErrorMsg);

    string client_id(p_pRtnOrderFillField->szCliOrderNo);
    EntrustNode* enode = getENode(client_id);
    if (enode == nullptr) {
        LOG_INFO("[jt adaptor {}] OnRtnOrder cannot find enode : {}",m_id, client_id);
        return 0;
    }
    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
    // sprintf(enode->origOrder.orderid, "%s", p_pRtnOrderFillField->szOrderId);
    order.idnum = enode->origOrder.idnum;
    order.side = enode->origOrder.side;
    order.ocflag = enode->origOrder.ocflag;
    order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;
    if (p_pRtnOrderFillField->chMatchedType == '2' && p_pRtnOrderFillField->llMatchedQty > 0)
    { // 成交
        order.state = ORD_NEW;		
        enode->origOrder.state = ORD_NEW;
        order.type = 'E';
        order.qty = p_pRtnOrderFillField->llMatchedQty;
        order.prc = atof(p_pRtnOrderFillField->szMatchedPrice);
		enode->eqty += p_pRtnOrderFillField->llMatchedQty;  // 增量成交
		enode->eamt += order.qty * order.prc;
		if (enode->eqty >= enode->qty) {
			enode->origOrder.state = ORD_FILLED;
		}
    }
    else if (p_pRtnOrderFillField->chMatchedType == '1' || p_pRtnOrderFillField->chMatchedType == '0' || p_pRtnOrderFillField->llMatchedQty < 0)
    { // 撤单
        enode->origOrder.state = ORD_CANCELED;
        order.state = ORD_CANCELED;
        order.type = 'C';
        order.prc = enode->origOrder.prc;
        order.qty = -p_pRtnOrderFillField->llWithdrawnQty;
    } else {
        LOG_INFO("[jt adaptor] unknown matchType:{}", p_pRtnOrderFillField->chMatchedType);
        return 0;
    }
    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[jt adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
    }
    // TODO  handle holding
    return 0;
}

//订阅响应
int hfs_jt_adaptor::OnRtnSubTopic(CRspSubTopicField* p_pRspSubTopicField)
{
    if (p_pRspSubTopicField == NULL)
    {
        LOG_INFO("[jt adaptor] sub failed {}", pUserApi->GetLastErrorText());
        return -1;
    }
    LOG_INFO("[jt adaptor] OnRtnSubTopic Topic:{}, Filter:{}, Channel:{}, DataSet:{}, AcceptSn:{},",
            p_pRspSubTopicField->szTopic, p_pRspSubTopicField->szFilter, p_pRspSubTopicField->szChannel, 
            p_pRspSubTopicField->chDataSet, p_pRspSubTopicField->llAcceptSn);
    return 0;
}

//取消订阅响应
int hfs_jt_adaptor::OnRtnUnsubTopic(CRspUnsubTopicField* p_pRspUnsubTopicField)
{
    if (p_pRspUnsubTopicField == NULL)
    {
        LOG_INFO("取消订阅失败");
        return -1;
    }
    LOG_INFO("[jt adaptor] OnRtnUnsubTopic Topic:{}, AcceptSn:{}",
            p_pRspUnsubTopicField->szTopic, p_pRspUnsubTopicField->llAcceptSn);
    return 0;
}

int hfs_jt_adaptor::OnRtnTradeRisk(CRtnTradeRiskInfoField* p_pRspField) {
    return 0;
}
int hfs_jt_adaptor::OnRspQryCanWithdrawOrder(CFirstSetField *p_pFirstSet, CRspCosCanWithdrawOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) {
    return 0;
}
bool hfs_jt_adaptor::IsErrorRspInfo(CFirstSetField *p_pFirstSet, std::string prefix)
{   
    if (p_pFirstSet && p_pFirstSet->iMsgCode != 0) {
        LOG_INFO("[jt adaptor] {} code:{}, msg:{}",prefix, ShowErrorInfo(p_pFirstSet->iMsgCode), p_pFirstSet->szMsgText);
        return true;
    }
    return false;
}

bool hfs_jt_adaptor::IsMyOrder(CRtnStkOrderConfirmField *pOrder)
{
    return true;
}

bool hfs_jt_adaptor::IsTradingOrder(CRtnStkOrderConfirmField *pOrder)
{
    return true;
}

int hfs_jt_adaptor::startUserAPI() {
    return 0;
}

int hfs_jt_adaptor::Login() {
    for(;m_state<=DISCONNECTED;) { //等待到连接成功
        sleep(10); 
    } 
    std::lock_guard<std::mutex> lk(mtxCommon);

    CReqCosLoginField field = {0};

    strncpy(field.szUserCode, params.cos_user.c_str(), sizeof(field.szUserCode)-1);
    strncpy(field.szEncryptKey, params.cos_password.c_str(), sizeof(field.szEncryptKey)-1);
    strncpy(field.szAuthData, params.cos_authdata.c_str(), sizeof(field.szAuthData)-1);
    char szThirdParty[1024+1]={0};
    GetThirdParty(szThirdParty, sizeof(szThirdParty));
    cout << "thirdparty: " << szThirdParty << endl;
    strncpy(field.szThirdParty, szThirdParty, sizeof(field.szThirdParty)-1);
	
	finished = success = false;
	int iRetCode=pUserApi->ReqCosLogin(&field, 1);
	if (iRetCode) {
        cout << "[jt adaptor] login failed : " << pUserApi->GetLastErrorText() << endl;
        return -1;
    }

    //等待登陆成功
    for(;!finished;) { sleep(10); }

    if(m_state != LOGGEDON) return -1;

    LOG_INFO("[jt adaptor] logged in successfully");
    
    return 0;
}
int hfs_jt_adaptor::StkLogin() {
    for(;m_state<=DISCONNECTED;) { //等待到连接成功
        sleep(10); 
    } 
    // std::lock_guard<std::mutex> lk(mtxCommon);

    CReqStkUserLoginField field = {0};

    strncpy(field.szAcctType, params.AcctType.c_str(), sizeof(field.szAcctType)-1);
    strncpy(field.szAcctId, params.UserName.c_str(), sizeof(field.szAcctId)-1);
    strncpy(field.szEncryptKey, params.Password.c_str(), sizeof(field.szEncryptKey)-1);
    strncpy(field.szAuthData, params.AuthData.c_str(), sizeof(field.szAuthData)-1);
	field.chUseScope = '0';            // 使用范围 '0':现货登录交易
	field.chAuthType = '0';            // 认证类型 '0':密码
	finished = success = false;
	int iRetCode=pUserApi->ReqStkUserLogin(&field, 1);
	if (iRetCode) {
        cout << "[gx adaptor] login failed : " << pUserApi->GetLastErrorText() << endl;
        return -1;
    }

    //等待登陆成功
    for(;!finished;) { sleep(10); }

    if(m_state != LOGGEDON) return -1;

    LOG_INFO("[gx adaptor] logged in successfully");
    
    return 0;
}

void hfs_jt_adaptor::qryThreadFunc()
{
    // while(true) {
        initFile();
        qryAccount();
        m_pos_str = "";
        qryPosition();
        while (!finished)
            sleep(1);
        while (true)
        {
            if (m_pos_str.empty())
            {
                cout << "qryPosition done" << endl;
                break;
            }
            qryPosition();
            while (!finished)
                sleep(1);
        }
        m_pos_str = "";
        qryOrder();
        while (!finished)
            sleep(1);
        while (true)
        {
            if (m_pos_str.empty())
            {
                cout << "qryOrder done" << endl;
                break;
            }
            qryOrder();
            while (!finished)
                sleep(1);
        }
        m_pos_str = "";
        qryTrade();
        while (!finished)
            sleep(1);
        while (true)
        {
            if (m_pos_str.empty())
            {
                cout << "qryTrade done" << endl;
                break;
            }
            qryTrade();
            while (!finished)
                sleep(1);
        }
        // sleep(1000 * 60 * 1);
    // }
}

void hfs_jt_adaptor::qryPosition() {
    // 查询持仓
    sleep(1000);
    cout <<"qryPosition" << endl;
    
    CReqStkQryExpendableSharesField field = {0};
    field.llCuacctCode = m_acctCode;
    field.llCustCode = m_custCode;
    strncpy(field.szQueryPos, m_pos_str.c_str(), sizeof(field.szQueryPos)-1);
    field.chQueryFlag = '0';
    field.iQueryNum = 1000;
    finished = success = false;
    int iRetCode = pUserApi->ReqQryExpendableShares(&field, iRequestID++);
    if (iRetCode) {
        cout << "[jt adaptor] qryPosition failed : " << pUserApi->GetLastErrorText() << endl;
        return;
    }
    //等待持仓请求成功
    for(;!finished;) { sleep(10); }
    LOG_INFO("[jt adaptor] Qry Position done");
}

void hfs_jt_adaptor::qryInstrument() {
    // 查询合约信息  priceTick  
    sleep(1000);
    cout << "qry instrument" << endl;
    
    //等待请求成功
    for(;!finished;) { sleep(10); }
    LOG_INFO("[jt adaptor] Qry InstrumentInfo done");
}

void hfs_jt_adaptor::qryAccount(){
    sleep(1000);
    cout <<"qryAccount" << endl;
    CReqStkQryExpendableFundField field = {0};
    field.llCustCode = m_custCode;
    field.llCuacctCode = m_acctCode;
    field.chCurrency = 0;
    field.iValueFlag = 15;
    finished = success = false;
    int iRetCode = pUserApi->ReqQryExpendableFund(&field, iRequestID++);
    if (iRetCode) {
        cout << "[jt adaptor] qryAccount failed : " << pUserApi->GetLastErrorText() << endl;
        return;
    }
    //等待持仓请求成功
    for(;!finished;) { sleep(10); }
    LOG_INFO("[jt adaptor] Qry Account done");
}
void hfs_jt_adaptor::qryOrder() {
    // 查询委托
    sleep(1000);
    cout <<"qryOrder" << endl;
    
    CReqCosOrderInfoField field = {0};
    sprintf(field.szCuacctCode, "%lld", m_acctCode);
    field.chQueryFlag = '0';   // 向后查询
    strncpy(field.szQueryPos, m_pos_str.c_str(), sizeof(field.szQueryPos)-1);
    field.iQueryNum = 1000;
    field.chFlag = 1;   // 查询正常委托
    finished = success = false;
    int iRetCode = pUserApi->ReqQryOrderInfo(&field, iRequestID++);
    if (iRetCode) {
        cout << "[jt adaptor] qryOrder failed : " << pUserApi->GetLastErrorText() << endl;
        return;
    }
    //等待持仓请求成功
    for(;!finished;) { sleep(10); }
    LOG_INFO("[jt adaptor] Qry Orders done");
}

void hfs_jt_adaptor::qryTrade() {
    // 查询委托
    sleep(1000);
    cout <<"qryTrade" << endl;

    CReqCosMatchField field = {0};
    sprintf(field.szCuacctCode, "%lld", m_acctCode);
    
    field.chQueryFlag = '0';   // 向后查询
    strncpy(field.szQueryPos, m_pos_str.c_str(), sizeof(field.szQueryPos)-1);
    field.iQueryNum = 1000;

    finished = success = false;
    int iRetCode = pUserApi->ReqQryMatch(&field, iRequestID++);
    if (iRetCode) {
        cout << "[jt adaptor] qryTrade failed : " << pUserApi->GetLastErrorText() << endl;
        return;
    }
    //等待持仓请求成功
    for(;!finished;) { sleep(10); }
    LOG_INFO("[jt adaptor] Qry Trade done");
}
int hfs_jt_adaptor::subTopic()
{
    pUserApi->SubTopic("MATCH00", m_trdAcct_SZ.c_str(), '1') ;
    pUserApi->SubTopic("MATCH10", m_trdAcct_SH.c_str(), '1');
    pUserApi->SubTopic("TSU_ORDER", m_trdAcct_SZ.c_str(), '1');
    pUserApi->SubTopic("TSU_ORDER", m_trdAcct_SH.c_str(), '1') ;
}
double hfs_jt_adaptor::myfloor(double price, double tick) {
  return int(price / tick + 0.5) * tick;
}

void hfs_jt_adaptor::sleep(int ms) {
  this_thread::sleep_for(chrono::milliseconds(ms));
}

void hfs_jt_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_jt_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

string hfs_jt_adaptor::ShowErrorInfo(int iRetCode)
{
	switch(iRetCode)
	{
	case -3:
		return ("invalid parameter");
		break;
	case -2:
		return ("invalid handle");
		break;
	case 100:
		return ("no data");
		break;
	case 101:
		return ("timeout");
		break;
	case 102:
		return ("exists");
		break;
	case 103:
		return ("more data");
		break;
	case 500:
		return ("call object function failed");
		break;
	case 501:
		return ("create object failed");
		break;
	case 502:
		return ("initialize object failed ");
		break;
	case 503:
		return ("object uninitiated");
		break;
	case 504:
		return ("create resource failed");
		break;
	case 505:
		return ("dispatch event failed");
		break;
	case 506:
		return ("event  undefined ");
		break;
	case 507:
		return ("register event {@1} from {@2} failed");
		break;
	case 508:
		return ("startup service {@1} failed");
		break;
	case 509:
		return ("init service env {@1} failed");
		break;
	case 510:
		return ("kernel/service env {@1} invalid");
		break;
	case 511:
		return ("service {@1} status not expect");
		break;
	case 512:
		return ("open internal queue {@1} failed");
		break;
	case 513:
		return ("open internal queue {@1} failed");
		break;
	case 514:
		return ("invalid message queue");
		break;
	case 515:
		return ("xml file {@1} format invalid");
		break;
	case 516:
		return ("open runtimedb {@1} failed");
		break;
	case 517:
		return ("create or initialize service function {@1}:{@2} fail ");
		break;
	case 518:
		return ("option {@2} read only");
		break;
	case 519:
		return ("option {@2} unsupported ");
		break;
	case 520:
		return ("purpose access {@2},but not granted");
		break;
	case 521:
		return ("queue {@1} fulled, max depth");
		break;
	case 522:
		return ("xa {@1} undefined");
		break;
	case 523:
		return ("call biz function {@1} exception");
		break;
	case 524:
		return ("timer {@1} callback failed, return");
		break;
	case 525:
		return ("filter expression {@1} invalid");
		break;
	case 526:
		return ("oem {@1} illegal");
		break;	
	case 1000:
		return ("API基本错误");
		break;
	case 1001:
		return ("DLL缺失");
		break;
	case 1002:
		return ("DLL初始化失败(版本不对)");
		break;
	case 1003:
		return ("API实例已存在");
		break;
	case 1101:
		return ("insufficient space expect");
		break;
	case 1102:
		return ("receive packet from {@1} failed");
		break;
	case 1103:
		return ("send packet to {@1} failed");
		break;
	case 1104:
		return ("connect to {@1} failed");
		break;
	case 1105:
		return ("reconnect failed in function");
		break;
	case 1106:
		return ("reconnect {@1} success");
		break;
	case 1107:
		return ("disconnect");
		break;
	case 1100:
		return ("call zmq api {@2} failed");
		break;
	case 1200:
		return ("MA_ERROR_DB_EXCEPTION");
		break;
	case 1201:
		return ("data {@1} unload");
		break;
	case 1202:
		return ("table {@1} cursor {@2} has already opened");
		break;
	case 1203:
		return ("table {@1} cursor {@2} not opened");
		break;
	case 1204:
		return ("database {@1} not opened");
		break;
	case 1205:
		return ("invalid database connect string");
		break;
	case 1250:
		return ("MA_ERROR_DAO_EXCEPTION");
		break;
	case 1500:
		return ("call fix api {@2} failed");
		break;
	case 1501:
		return ("fix parse from {@1} failed");
		break;
	case 1502:
		return ("call kcbp api {@2} failed");
		break;
	case 1503:
		return ("invalid packet {@2} failed");
		break;
	case 1504:
		return ("call json api {@2} failed");
		break;
	case 1600:
		return ("call kcxp api {@2} failed");
		break;
	case 2000:
		return ("API套接字错误");
		break;
	case 2001:
		return ("客户端连接失败(请检查连接参数与服务器是否开启)");
		break;
	case 2002:
		return ("服务器创建失败");
		break;
	case 3000:
		return ("API配置错误");
		break;
	case 3001:
		return ("GTU节点配置文件错误");
		break;
	default:
		return ("尚无详细信息");
		break;

	} 
}