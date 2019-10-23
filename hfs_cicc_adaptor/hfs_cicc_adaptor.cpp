#include "hfs_cicc_adaptor.hpp"

using namespace std;

hfs_cicc_adaptor::hfs_cicc_adaptor(const a2ftool::XmlNode &cfg) : hfs_base_adaptor(cfg) {
    m_userID = cfg.getAttrDefault("userID", "mapisjyy001");
    m_password = cfg.getAttrDefault("password", "1qaz@WSX");
    // m_fundAccount = cfg.getAttrDefault("fundAccount", "1045679");
    m_server_ip = cfg.getAttrDefault("serverIP", "");
	m_id = cfg.getAttrDefault("id", "");
	m_sysinfo = cfg.getAttrDefault("sysinfo", "");

	m_product_id = cfg.getAttrDefault("productID", "");
	m_productunit_id = cfg.getAttrDefault("productUnitID", "");
	m_protfolio_id = cfg.getAttrDefault("profolioID", "");

	m_export_path = cfg.getAttrDefault("exportPath", "./");

	int key[] = { 1, 2, 3, 4, 5 };//加密字符
	char s[64] = {0};
	strncpy(s, m_password.c_str(), 63);
	decode(s, key);
	m_password = s;
	
	int timex = current();
	m_orderRef = (timex/10000*3600)+((timex%10000)/100*60)+(timex%100);
	m_orderRef = (today()%10)*100 + m_orderRef;
	m_rc = new RiskControl(cfg);
	a2ftool::XmlNode router;
	if (cfg.isRoot()) {
		router = cfg.getChild("router");
	} 
	else router = cfg.getRoot().getChild("router");
	string db_user = router.getAttrDefault("db_user","");
	string db_passwd = router.getAttrDefault("db_pass","");
	string db_addr = router.getAttrDefault("db_addr","");
	// if (!db_addr.empty())
	// 	m_sql = new hfs_database_mysql(db_user.c_str(), db_passwd.c_str(), db_addr.c_str());
	// else m_sql = nullptr;

}

hfs_cicc_adaptor::~hfs_cicc_adaptor() {

}

int hfs_cicc_adaptor::Init() {
	// load opstation
	// if (m_sql) {
	// 	m_sql->start();
	// 	m_sql->query("select traderid, mac, opstation from trader");
	// 	MYSQL_ROW row = nullptr;
	// 	while ((row = m_sql->next()) != nullptr){
	// 		int id = atoi(row[0]);
	// 		if (row[2]) {
	// 			string opstation = row[2];
	// 			m_opstation.insert(make_pair(id, opstation));
	// 			LOG_INFO("[ht adaptor {}] traderid:{}, opstation:{}",m_id, id, opstation);
	// 		}
	// 	}
	// 	m_sql->end();
	// }
    // init api
    if (m_server_ip.empty()) {
        cout << "cicc adaptor server ip not set" << endl;
        return -1;
    }
    m_api = IMSTradeAPI::CreateAPI();
    m_api->SetTradeSPI(this);

    if (!m_api->Initial(m_server_ip.c_str())) {
        LOG_ERROR("[ci adaptor {}] init api failed : {}",m_id, m_api->GetLastError() );
        return -1;
    }
    m_state = 1;
    if (!m_api->TryLogin(m_userID.c_str(), m_password.c_str(), nullptr)) {
        LOG_ERROR("[ci adaptor {}] login failed : {}",m_id, m_api->GetLastError() );
        return -1;
    }
    m_state = 2;

	// query
	qryAccount();
	while (!finished) sleep(2);
	char fname[64] = {0};
	sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(),today());
	m_position_file.open(fname, std::ios::out);
	m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;
    
    qryPosition();
	
	while (!finished) sleep(2);
	sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
	m_entrust_file.open(fname, std::ios::out);
	m_entrust_file << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;
    qryOrder();

	while (!finished) sleep(2);
	sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
	m_detail_file.open(fname, std::ios::out);
	m_detail_file << "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID" << endl;
    qryDetail();
	
    return 0;
}

bool hfs_cicc_adaptor::onRequest(int teid, hfs_order_t &order) {
	switch(order.type) {
	case 'O':
		return process_new_entrust(teid, order);
		break;
	case 'X':
		return process_cancel_entrust(teid, order);
		break;
	case 'H':
		//return adap.resendTradeHistory(teid);
		return true;
		break;
	default:
		LOG_ERROR("[ht adaptor {}] unexpected order type: {}",m_id, order.type);
		return false;
	};
	
}

bool hfs_cicc_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
	assert(order.type == 'O');
	dump(&order);
	if (m_state != 2) {
		LOG_INFO("[ci adaptor {}] adaptor not login, entrust failed", m_id);
		hfs_order_t od;
		memcpy(&od, &order, sizeof(hfs_order_t));
		od.qty = order.qty;
		od.type = 'J';
		od.state = ORD_REJECTED;
		strncpy(od.algo, "adaptor not login", sizeof(od.algo)-1);
		if (!onResponse(teid, m_id, od)) {
			LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
		}
		return false;
	}
	
	if (m_rc->OnInsertOrder(&order) == false || m_rc->IsCanInsert() == false) {
		LOG_INFO("[ci adaptor {}] entrust failed, RiskControl refused", m_id);
		hfs_order_t od;
		memcpy(&od, &order, sizeof(hfs_order_t));
		od.qty = order.qty;
		od.type = 'J';
		od.state = ORD_REJECTED;
		strncpy(od.algo, "RickControl refused", sizeof(od.algo)-1);
		if (!onResponse(teid, m_id, od)) {
			LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
		}
		return false;
	}
	if (order.side == 'S') {
		if (m_holdings.find(order.symbol) != m_holdings.end()) {
			if (m_holdings[order.symbol]->qty_long_yd < order.qty) {
				LOG_ERROR("[ci adaptor {}] entrust failed, not enough holding : {}", m_id, m_holdings[order.symbol]->qty_long_yd);
				hfs_order_t od;
				memcpy(&od, &order, sizeof(hfs_order_t));
				od.qty = order.qty;
				od.type = 'J';
				od.state = ORD_REJECTED;
				strncpy(od.algo, "not enough holding", sizeof(od.algo)-1);
				if (!onResponse(teid, m_id, od)) {
					LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
				}
				return false;
			}
		}
	}
	char buf[10] = {0};
	sprintf(buf, "%d%03d_%d", teid, order.oseq, ++m_orderRef);
    string sid(buf);

	NewOrderField filed;
	memset(&filed, 0, sizeof(NewOrderField));

	strcpy(filed.productId, m_product_id.c_str());
	strcpy(filed.productUnitId, m_productunit_id.c_str());
	strcpy(filed.acctId, m_userID.c_str());
	strcpy(filed.portfolioId, m_protfolio_id.c_str());
    sprintf(filed.clOrderId, "%s", buf);
    string symbol(order.symbol);
    strcpy(filed.stkCode, symbol.substr(0,6).c_str());
    if (symbol.find("SH") != std::string::npos) {
        if (m_bHK)
            strcpy(filed.exchId, "H0");
        else strcpy(filed.exchId, "0");
    } else if (symbol.find("SZ") != std::string::npos) {
        if (m_bHK)
            strcpy(filed.exchId, "H1");
        else strcpy(filed.exchId, "1");
    } else {
        LOG_ERROR("[ci adaptor {}] insert order : unknown exch : {}", m_id, symbol);
        return false;
    }
	sprintf(filed.bsFlag, "%c", order.side);
	filed.orderQty = order.qty;
	filed.orderPrice = order.prc;
	strcpy(filed.orderPriceType, "LIMIT");
	strcpy(filed.currencyId, "00");
	filed.fundType = 1;
	strcpy(filed.businessType, "ORD");

	if (m_api->ReqNewOrder(&filed, ++m_request_id) < 0)
	{
        LOG_ERROR("[ci adaptor {}] req new order failed:{}",m_id, m_api->GetLastError());
    }
    m_request_map[m_request_id] = sid;

	order.tm = current();
	if(!registerTrader(teid, order, sid)) {
    	LOG_ERROR("[ht adaptor {}] register trader failed, teid:{}, pid:{}, oseq:{}, sid:{}",m_id, teid, order.pid, order.oseq, sid);
	}
	return true;
}

bool hfs_cicc_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
	assert(order.type == 'X');
	dump(&order);
	if (m_state != 2) {
		LOG_INFO("[ht adaptor {}] adaptor not login, cancel failed", m_id);
		return false;
	}
	if (m_rc->IsCanCancel() == false) {
		LOG_INFO("[ht adaptor {}] RiskControl refuse cancel request", m_id);
		return false;
	}
	
    OrderID oid(teid, order.pid, order.oseq);
    EntrustNode* node = getENode(oid);
    if (node != nullptr) {
        CancelOrderField ordercancel;
        memset(&ordercancel, 0, sizeof(CancelOrderField));

        strcpy(ordercancel.productId, m_product_id.c_str());
        strcpy(ordercancel.productUnitId, m_productunit_id.c_str());
        strcpy(ordercancel.acctId, m_userID.c_str());
        strcpy(ordercancel.portfolioId, m_protfolio_id.c_str());
        sprintf(ordercancel.clOrderId, "CANCEL%d", ++m_request_id);

        string symbol(order.symbol);
        strcpy(ordercancel.stkCode, symbol.substr(0,6).c_str());
        if (symbol.find("SH") != std::string::npos) {
            if (m_bHK)
                strcpy(ordercancel.exchId, "H0");
            else strcpy(ordercancel.exchId, "0");
        } else if (symbol.find("SZ") != std::string::npos) {
            if (m_bHK)
                strcpy(ordercancel.exchId, "H1");
            else strcpy(ordercancel.exchId, "1");
        } else {
            LOG_ERROR("[ci adaptor {}] insert order : unknown exch : {}", m_id, symbol);
            return false;
        }
        strcpy(ordercancel.orgContractNum, node->origOrder.orderid);//OnRspNewOrder return contractnum
        strcpy(ordercancel.orgclOrderId, node->sid);//original order clorderid

        strcpy(ordercancel.businessType, "ORD");

        CancelOrderField* tmp=&ordercancel;
        if (m_api->ReqCancelOrder(&tmp,1, ++m_request_id) < 0)
        {
            LOG_ERROR("[ci adaptor {}] cancel order failed : {}",m_id, m_api->GetLastError());
        }
    } else {
		LOG_ERROR("[ci adaptor {}] cancel not exists order, teid= {} oseq={}",m_id, teid, order.oseq);
    	return true;
	}
	return true;
}

bool hfs_cicc_adaptor::start() {
	if (Init() != 0) return false;
    return true;
}

bool hfs_cicc_adaptor::stop() {
    return true;
}

void hfs_cicc_adaptor::qryAccount() {
    printf("ExcuAccountQry start\n");
	QryAccountField account;
	memset(&account, '\0', sizeof(QryAccountField));
	strcpy(account.productId, m_product_id.c_str());
	strcpy(account.productUnitId, m_productunit_id.c_str());
	strcpy(account.portfolioId, m_protfolio_id.c_str());
	//int tmpreqid = mReqId++;
	finished = false;
	if (m_api->ReqQryAccount(&account, ++m_request_id) < 0)
	{
		LOG_ERROR("[ci adaptor {}] qry account failed : {}", m_id, m_api->GetLastError());
		finished = true;
	}
}

void hfs_cicc_adaptor::qryPosition() {
    printf("ExcuPosQry start\n");
	QryPositionField position;
	memset(&position, '\0', sizeof(QryPositionField));
	strcpy(position.productId, m_product_id.c_str());
	strcpy(position.productUnitId, m_productunit_id.c_str());
    strcpy(position.portfolioId, m_protfolio_id.c_str());
    position.realPosFlag = false;
    position.authType = 2;
	//int tmpreqid = mReqId++;
	finished = false;
	if (m_api->ReqQryPosition(&position, ++m_request_id) < 0)
	{
		LOG_ERROR("[ci adaptor {}] qry position failed : {}", m_id, m_api->GetLastError());
		finished = true;
	}
}

void hfs_cicc_adaptor::qryOrder(){
    printf("ExcuOrderQry start\n");
	QryOrderField pQuery;
	memset(&pQuery, '\0', sizeof(QryOrderField));
	strcpy(pQuery.productId, m_product_id.c_str());

	finished = false;
	if (m_api->ReqQryOrder(&pQuery, ++m_request_id) < 0)
	{
		LOG_ERROR("[ci adaptor {}] qry orders failed : {}", m_id, m_api->GetLastError());
		finished = true;
	}
}

void hfs_cicc_adaptor::qryDetail() {
    printf("ExcuKnockQry start\n");
	QryKnockField pKnock;
	memset(&pKnock, '\0', sizeof(QryKnockField));
	strcpy(pKnock.productId, m_product_id.c_str());

	finished = false;
	if (m_api->ReqQryKnock(&pKnock, ++m_request_id) < 0)
	{
		LOG_ERROR("[ci adaptor {}] qry details failed : {}", m_id, m_api->GetLastError());
		finished = true;
	}
}

bool hfs_cicc_adaptor::OnRspNewOrder(const NewOrderRspField * pRspOrder, const ErrMsgField * errMsg, int requestId, bool last)
{
    // if (IsErrInfo(errMsg)) return false;
    LOG_INFO("[ci adaptor {}] OnRspNewOrder tkr:{}, side:{}, orderid:{}, client_id:{} ", m_id,
                pRspOrder->stkCode, pRspOrder->bsFlag, pRspOrder->contractNum, pRspOrder->clOrderId);
    // if (m_request_map.find(requestId) == nullptr) {
    //     LOG_ERROR("[ci adaptor {}] OnRspNewOrder cannot find requestid : {}", m_id, requestId);
    //     return false;
    // }
    string client_id = pRspOrder->clOrderId; //m_request_map[requestId];
    EntrustNode* enode = getENode(client_id);
    if (!enode) {
		LOG_ERROR("[ci adaptor {}] OnRspNewOrder cannot find enode : {}", m_id, client_id);
		return true;
    }

    hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	strncpy(enode->origOrder.orderid, pRspOrder->clOrderId, sizeof(order.orderid));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
    order.qty = enode->qty;	
    if (IsErrInfo(errMsg)) {
        order.type = 'J';
		order.state = ORD_REJECTED;
		enode->origOrder.state = ORD_REJECTED;
		strncpy(order.algo, errMsg->errorMsg, sizeof(order.algo)-1);
		LOG_INFO("[ci adaptor {}] OnRspNewOrder get error:{}",m_id, errMsg->errorMsg);
    } else {
		return true;
        // order.type = 'A';
		// order.state = ORD_NEW;
		// enode->origOrder.state = ORD_NEW;
    }
    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
	return true;
}

bool hfs_cicc_adaptor::OnRspCancelOrder(const CancelOrderRspField * pRspCancelOrder, const ErrMsgField * errMsg, int requestId, bool last)
{
	if (errMsg != NULL&&errMsg->errorCode != 0)
	{
		printf("Rtn Cancel Response Err=%s\n", errMsg->errorMsg);
	}
	else
	{
		printf("recv order cancel response, clorderid=%s, contractnum=%s, symbol=%s\n", pRspCancelOrder->clOrderId,
			pRspCancelOrder->contractNum,
			pRspCancelOrder->stkCode);
	}
	return true;
}
bool hfs_cicc_adaptor::OnNotiOrder(const OrderNotiField* pNotiOrd, const ErrMsgField * errMsg) {
	if (IsErrInfo(errMsg)) {
		LOG_INFO("[ci adaptor {}] OnNotiOrder error", m_id);
		return true;
	}
	LOG_INFO("[ci adaptor {}] OnNotiOrder accid:{}, symbol:{}, clientid:{}, orderid:{}, qty:{}, prc:{}, bsflag:{}, filledqty:{}, cancelqty:{}", m_id,
			pNotiOrd->acctId, pNotiOrd->stkCode, pNotiOrd->clOrderId, pNotiOrd->contractNum, pNotiOrd->orderQty, pNotiOrd->orderPrice,
			pNotiOrd->bsflag, pNotiOrd->knockQty, pNotiOrd->withdrawQty);
	
	string client_id = pNotiOrd->clOrderId;
	EntrustNode* enode = getENode(client_id);
    if (!enode) {
		LOG_ERROR("[ci adaptor {}] OnNotiOrder cannot find enode : {}", m_id, client_id);
		return true;
	}
	hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	strncpy(enode->origOrder.orderid, pNotiOrd->contractNum, sizeof(order.orderid));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
	// ALLTRD	全部成交
	// CANCEL	撤单
	// DELETE_F	删除订单-失败状态
	// DELETE_N	删除订单-新状态
	// DELETE_S_E	撤单成功
	// NEW	已接收
	// N_TRD_Q	已报
	// P_TRD_Q	部分成交
	// CANCEL_P	部分撤单
	string status(pNotiOrd->orderStatus);
	if (status == "CANCEL") {
		order.type = 'C';
		order.state = ORD_CANCELED;
		enode->origOrder.state = ORD_CANCELED;
		order.qty = pNotiOrd->withdrawQty;	
	} else if (status == "N_TRD_Q") {
		order.type = 'A';
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
		order.qty = enode->qty;	
	// } else if (status == "CANCEL_P") {
	// 	order.type = 'C';
	// 	order.state = ORD_CANCELED;
	// 	enode->origOrder.state = ORD_CANCELED;
	// 	order.qty = pNotiOrd->withdrawQty;	
	} else {
		return true;
	}
	if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
	return true;
}
bool hfs_cicc_adaptor::OnNotiKnock(const KnockNotiField * pRspKnock, const ErrMsgField * errMsg)
{
	if (IsErrInfo(errMsg)) {
		LOG_ERROR("[ci adaptor {}] OnNotiKnock get error", m_id);
		return true;
	}
	LOG_INFO("[ci adaptor {}] OnNotiKnock symbol:{}, client_id:{}, orderid:{}, qty:{}, prc:{}, tradeqty:{}, tradeprc:{}, filledqty:{}, cxlqty:{}, msg:{}", m_id,
			pRspKnock->stkCode, pRspKnock->clOrderId, pRspKnock->contractNum, 
			pRspKnock->orderQty, pRspKnock->orderPrice, pRspKnock->knockQty, pRspKnock->knockPrice, pRspKnock->cumQty, pRspKnock->withdrawQty, pRspKnock->notiMsg);

	// NEW	新订单
	// P_FILLED	部分成交
	// F_FILLED	全部成交
	// CANCELED	成功撤单
	// REJECTED	拒绝
	// REPLACE	修改订单
	// SUSPEND	挂起
	// ACTIVE	激活
	// CANCEL_REJECTED	撤单被拒
	string client_id = pRspKnock->clOrderId;
	EntrustNode* enode = getENode(client_id);
    if (!enode) {
		LOG_ERROR("[ci adaptor {}] OnNotiKnock cannot find enode : {}", m_id, client_id);
		return true;
	}
	hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	strncpy(enode->origOrder.orderid, pRspKnock->contractNum, sizeof(order.orderid));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
	string execType(pRspKnock->execType);
	if (execType == "P_FILLED") {
		order.type = 'E';
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
		order.qty = pRspKnock->knockQty;
		order.prc = pRspKnock->knockPrice;
	} else if (execType == "F_FILLED") {
		order.type = 'E';
		order.state = ORD_FILLED;
		enode->origOrder.state = ORD_FILLED;
		order.qty = pRspKnock->knockQty;
		order.prc = pRspKnock->knockPrice;
	} else if (execType == "CANCELED") {
		order.type = 'C';
		order.state = ORD_CANCELED;
		enode->origOrder.state = ORD_CANCELED;
		order.qty = pRspKnock->withdrawQty;
	} else if (execType == "REJECTED") {
		order.type = 'J';
		order.state = ORD_REJECTED;
		enode->origOrder.state = ORD_REJECTED;
		order.qty = pRspKnock->orderQty;
		strncpy(order.algo, pRspKnock->notiMsg, sizeof(order.algo));
		LOG_INFO("[ci adaptor {}] OnNotiKnock get error:{}", m_id, pRspKnock->notiMsg);
	} else {
		return true;
	}
	if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ci adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
	// TODO holding
	if (execType == "P_FILLED" || execType == "F_FILLED") {
		int trade_qty = pRspKnock->knockQty;
		char side = pRspKnock->bsFlag[0];
		if (m_holdings.find(order.symbol) == m_holdings.end()) {
			hfs_holdings_t* holding = new hfs_holdings_t();
			memset(holding, 0, sizeof(hfs_holdings_t));
			strcpy(holding->symbol, order.symbol);
			m_holdings.insert(make_pair(order.symbol, holding));
		}
		hfs_holdings_t* holding = m_holdings[order.symbol];
		if (side == 'B') {			
			holding->qty_long += trade_qty;
			holding->avgprc_long = (holding->qty_long * holding->avgprc_long + trade_qty * pRspKnock->knockPrice) / (holding->qty_long + trade_qty + 0.00000001);		
		} else if (side == 'S') {
			holding->qty_long -= trade_qty;
			holding->qty_long_yd -= trade_qty;
			holding->avgprc_long = (holding->qty_long * holding->avgprc_long + trade_qty * pRspKnock->knockPrice) / (holding->qty_long - trade_qty + 0.00000001);	
		}
	}
	return true;
}

bool hfs_cicc_adaptor::OnRspQryOrder(const QryOrderRspField * pRspQryOrder, const ErrMsgField * errMsg, int requestId, bool last)
{
	if (IsErrInfo(errMsg)) {
		LOG_ERROR("[ci adaptor {}] OnRspQryOrder get error ", m_id);
		m_entrust_file.close();
		finished = true;
		return true;
	}
	
	LOG_INFO("[ci adaptpr {}] OnRspQryOrder productid:{}, symbol:{}, clientid:{}, orderid:{}, status:{}, updtime:{}", m_id,
			pRspQryOrder->productId, pRspQryOrder->stkCode, pRspQryOrder->clOrderId, pRspQryOrder->contractNum, pRspQryOrder->orderStatus, pRspQryOrder->orderTime);

	//Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
	if (!m_entrust_file.is_open()) {
		char fname[1024] = {0};
		sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
		m_entrust_file.open(fname, std::ios::out);
		m_entrust_file << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;
	}
	m_entrust_file << pRspQryOrder->acctId << "," << pRspQryOrder->stkCode << ",," << pRspQryOrder->orderTime << ','
				   << pRspQryOrder->contractNum << ',' << pRspQryOrder->clOrderId << ',' << pRspQryOrder->bsflag << ',' 
				   << pRspQryOrder->orderPriceType << ',' << pRspQryOrder->orderStatus << ',' << pRspQryOrder->orderPrice << ','
				   << pRspQryOrder->orderQty << ',' << pRspQryOrder->averagePrice << ',' << pRspQryOrder->knockQty << ','
				   << pRspQryOrder->withdrawQty << ",," << endl;
	m_entrust_file.flush();

	if (last) {
		m_entrust_file.close();
		finished = true;
	}
	return true;
}

bool hfs_cicc_adaptor::OnRspQryKnock(const QryKnockRspField * pRspQryKnock, const ErrMsgField * errMsg, int requestId, bool last)
{
	if (IsErrInfo(errMsg)) {
		finished = true;
		if (m_detail_file.is_open()) {
			m_detail_file.close();
		}
		LOG_ERROR("[ci adaptor {}] OnRspQryKnock get error", m_id);
		return true;
	}
	
	LOG_INFO("[ci adaptor {}] OnRspQryKnock prodid:{}, accid:{}, symbol:{}, clientid:{}, side:{}, tradeqty:{}, tradeprc:{}, tradetme:{} ", 
			m_id,
			pRspQryKnock->productId, pRspQryKnock->acctId, pRspQryKnock->stkCode, pRspQryKnock->clOrderId, pRspQryKnock->bsflag, 
			pRspQryKnock->knockQty, pRspQryKnock->knockPrice, pRspQryKnock->knockTime);
	if (!m_detail_file.is_open()) {
		char fname[1024] = {0};
		sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
		m_detail_file.open(fname, std::ios::out);
		m_detail_file << "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID" << endl;
	}
	//Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID
	m_detail_file << pRspQryKnock->acctId << ',' << pRspQryKnock->stkCode << ",," << pRspQryKnock->knockTime << ',' 
				<< pRspQryKnock->contractNum << ',' << pRspQryKnock->bsflag << ',' << pRspQryKnock->orderPriceType << ',' 
				<< pRspQryKnock->knockPrice << ',' << pRspQryKnock->knockQty << ",," << endl;
	m_detail_file.flush();

	if (last) {
		finished = true;
		if (m_detail_file.is_open()) {
			m_detail_file.close();
		}
	}
	return true;
}

bool hfs_cicc_adaptor::OnRspQryPosition(const QryPositionRspField * pRspQryPosition, const ErrMsgField * errMsg, int requestId, bool last)
{
	if (IsErrInfo(errMsg)) {
		finished = true;
		if (m_position_file.is_open()) {
			m_position_file.close();
		}
		LOG_ERROR("[ci adaptor {}] OnRspQryKnock get error", m_id);
		return true;
	}
	
	LOG_INFO("[ci adaptor {}] OnRspQryPosition prodid:{}, symbol:{}, totqty:{}, avilqty:{}, mktvalue:{}", m_id,
			pRspQryPosition->productId, pRspQryPosition->stkCode, pRspQryPosition->totalQty, pRspQryPosition->usableQty, pRspQryPosition->currentStkValue);
	
	if (!m_position_file.is_open()) {
		char fname[1024] = {0};
		sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
		m_position_file.open(fname, std::ios::out);
		m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;
	}		
	//Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr
	m_position_file << pRspQryPosition->productId << ',' << pRspQryPosition->stkCode << ',' <<pRspQryPosition->totalQty << ',' 
					<< pRspQryPosition->usableQty << ',' << pRspQryPosition->dilutedCost << ',' << pRspQryPosition->currentStkValue<< ','<< endl ;
	m_position_file.flush();

	string symbol(pRspQryPosition->stkCode);
	if (pRspQryPosition->exchId[0] == '0') {  // SH
		symbol.append(".SH");
	} else if (pRspQryPosition->exchId[0] == '1') { // SZ
		symbol.append(".SZ");
	}
	if (m_holdings.find(symbol) == m_holdings.end()) {
		hfs_holdings_t* holding = new hfs_holdings_t();
		memset(holding, 0, sizeof(hfs_holdings_t));
		strcpy(holding->symbol, symbol.c_str());
		m_holdings.insert(make_pair(symbol, holding));
	}
	hfs_holdings_t* holding = m_holdings[symbol];
	holding->qty_long = pRspQryPosition->totalQty;
	holding->qty_long_yd = pRspQryPosition->usableQty;
	holding->avgprc_long = pRspQryPosition->dilutedCost;

	if (last) {
		finished = true;
		if (m_position_file.is_open()) {
			m_position_file.close();
		}
	}

	return true;
}

bool hfs_cicc_adaptor::OnRspQryAccount(const QryAccountRspField * pRspQryAccount, const ErrMsgField * errMsg, int requestId, bool last)
{
	if (errMsg != NULL&&errMsg->errorCode != 0)
	{
		printf("OnRspQryAccount Err=%s\n", errMsg->errorMsg);
	}
	else
	{
		printf("OnRspQryAccount Sucess, currencyId=%s, useamt=%f\n", pRspQryAccount->currencyId, pRspQryAccount->currentAmt);
	}
	if (last)
	{
		printf("OnRspQryAccount Last,requestId=%d\n", requestId);
	}
	finished = true;
	return true;
}

bool hfs_cicc_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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
		LOG_INFO("[ht adaptor {}] register trader : {}",m_id, sid);
		return true;
	}
	catch(std::exception &ex) {
		LOG_ERROR("[ht adaptor {}] register trader {} failed : {}",m_id, sid, ex.what());
		return false;
	}
}

EntrustNode *hfs_cicc_adaptor::getENode(OrderID& oid) {
	std::lock_guard<std::mutex> lk(m_nid_mtx);
	EntrustNode *rv = nullptr;
	if (nmap.find(oid) != nmap.end()) {
		return nmap[oid];
	}
	return rv;
}

EntrustNode *hfs_cicc_adaptor::getENode(string sid) {
	std::lock_guard<std::mutex> lk(m_sid_mtx);
	EntrustNode *enode = nullptr;

	auto iter = smap.find(sid);
	if (iter != smap.end()) {
		enode = smap[sid];
	}
	return enode;
}

bool hfs_cicc_adaptor::IsErrInfo(const ErrMsgField* errMsg) {
    if (errMsg != NULL&&errMsg->errorCode != 0)
	{
        LOG_ERROR("[ci adaptor {}] get errinfo : {}", m_id, errMsg->errorMsg);
		return true;
    }
    return false;
}

void hfs_cicc_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("[ht adaptor {}]holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             m_id, holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_cicc_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("[ht adaptor {}]order:symbol:{},exch:{},pid:{},orderid:{},tm:{},type:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             m_id, order->symbol, order->exch, order->pid, order->orderid, order->tm, (char)order->type, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

//加密
void hfs_cicc_adaptor::encode(char *pstr, int *pkey){
    int len = strlen(pstr);//获取长度
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_cicc_adaptor::decode(char *pstr, int *pkey){
    int len = strlen(pstr);
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

std::string hfs_cicc_adaptor::getOPStation(int traderid) {
	if (m_opstation.find(traderid) != m_opstation.end()) {
		return m_opstation[traderid];
	} 
	return m_sysinfo;
}