#include "hfs_ht_adaptor.hpp"

using namespace std;

hfs_ht_adaptor::hfs_ht_adaptor(const a2ftool::XmlNode &cfg) : hfs_base_adaptor(cfg) {
    m_userID = cfg.getAttrDefault("userID", "mapisjyy001");
    m_password = cfg.getAttrDefault("password", "1qaz@WSX");
	m_fundAccount = cfg.getAttrDefault("fundAccount", "1045679");
	m_id = cfg.getAttrDefault("id", "");
	m_sysinfo = cfg.getAttrDefault("sysinfo", "");
	m_license_path = cfg.getAttrDefault("licensePath", "./cfg");
	m_export_path = cfg.getAttrDefault("export_path", "./export/");

	int key[] = {1, 2, 3, 4, 5}; //加密字符
	char s[64] = {0};
	strncpy(s, m_password.c_str(), 63);
	decode(s, key);
	m_password = s;
	
	int timex = current();
	if (timex > 93000) {
		m_orderRef = (timex / 10000 * 3600) + ((timex % 10000) / 100 * 60) + (timex % 100);
	} else {
		m_orderRef = 0;
	}
	
	m_rc = new RiskControl(cfg);
	// a2ftool::XmlNode router = cfg.getParent().getParent().getChild("router");

	// string db_user = router.getAttrDefault("db_user","");
	// string db_passwd = router.getAttrDefault("db_pass","");
	// string db_addr = router.getAttrDefault("db_addr","");
	// if (!db_addr.empty())
	// 	m_sql = new hfs_database_mysql(db_user.c_str(), db_passwd.c_str(), db_addr.c_str());
	// else m_sql = nullptr;
}

hfs_ht_adaptor::~hfs_ht_adaptor() {

}

int hfs_ht_adaptor::Init() {
	// load opstation
	if (m_sql) {
		m_sql->start();
		m_sql->query("select traderid, mac, opstation from trader");
		MYSQL_ROW row = nullptr;
		while ((row = m_sql->next()) != nullptr){
			int id = atoi(row[0]);
			if (row[2]) {
				string opstation = row[2];
				m_opstation.insert(make_pair(id, opstation));
				LOG_INFO("[ht adaptor {}] traderid:{}, opstation:{}",m_id, id, opstation);
			}
		}
		m_sql->end();
	}
	// init api
	int iRet = HtClientManagerInit(m_license_path.c_str(), 10000, m_channelID.c_str());
	if (0 != iRet)
	{
		cout << "client manager init error:" << iRet << endl;
		return -1;
    }
    if (NULL == HtSendMsgInterfaceInit(this, m_channelID.c_str()))
	{
		cout << "HtSendMsgInterfaceInit error" << endl;
		return -1;
    }
    login();
	if (m_state != 2) return -1;
	subscribeDetail();
	subscribeOrder();
	// query
	char fname[64] = {0};
	sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
	m_position_file.open(fname, std::ios::out);
	m_position_file << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" << endl;
    qryAccount();
    m_pos_str = "0";
    string pos_str = qryPosition();
    while (pos_str != m_pos_str) {
        m_pos_str = pos_str;
        qryPosition();
	}
	
	m_pos_str = "0";
	sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
	m_entrust_file.open(fname, std::ios::out);
	m_entrust_file << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID" << endl;
    pos_str = qryOrder();
    while (pos_str != m_pos_str) {
        m_pos_str = pos_str;
        qryOrder();
	}
	
	m_pos_str = "0";
	sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
	m_detail_file.open(fname, std::ios::out);
	m_detail_file << "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID" << endl;
    pos_str = qryDetail();
    while (pos_str != m_pos_str) {
        m_pos_str = pos_str;
        qryDetail();
    }
	
    return 0;
}

bool hfs_ht_adaptor::onRequest(int teid, hfs_order_t &order) {
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
		LOG_ERROR("[ht adaptor {}] unexpected order type: {}",m_id, order.type);
		return false;
	}
	return true;
}

bool hfs_ht_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
	assert(order.type == 'O');
	dump(&order);
	if (m_state != 2) {
		LOG_INFO("[ht adaptor {}] adaptor not login, entrust failed", m_id);
		push_reject_order(teid, order, "adaptor not login");
		return false;
	}
	
	if (m_rc->OnInsertOrder(&order) == false || m_rc->IsCanInsert() == false) {
		LOG_INFO("[ht adaptor {}] entrust failed, RiskControl refused", m_id);
		push_reject_order(teid, order, "RickControl refused");
		return false;
	}

	char buf[10] = {0};
	sprintf(buf, "%d", teid * 100000 + (++m_orderRef));
	string sid(buf);
	if (SendToServerAsy(order, sid) != 0) {
		push_reject_order(teid, order, "SendToServerAsy failed");
	}
	order.tm = current();
	if(!registerTrader(teid, order, sid)) {
    	LOG_ERROR("[ht adaptor {}] register trader failed, teid:{}, pid:{}, oseq:{}, sid:{}",m_id, teid, order.pid, order.oseq, sid);
  	}
	return true;
}

bool hfs_ht_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
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
	if (node) {
		cancel_order(node->origOrder.orderid);
	} else {
		LOG_ERROR("[ht adaptor {}] cancel not exists order, teid= {} oseq={}",m_id, teid, order.oseq);
    	return true;
	}
	return true;
}

int hfs_ht_adaptor::cancel_order(char* order_id) {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010003");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::OrderID, order_id);
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[1024] = { 0 };
		if (HtMsgPtrToString(response, jstr, 1024) > 0)//以json形弝输出结果
		{
			LOG_INFO("[ht adaptor {}] cancel response:{}",m_id, jstr);
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			// cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { 
			LOG_ERROR("[ht adaptor {}] getvalue RequestExeStatus error!", m_id);
			return -1; 
		}

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { LOG_ERROR("[ht adaptor {}] getvalue Text error!", m_id); return -1; }
			LOG_ERROR("[ht adaptor {}] cancel failed:{}", m_id, error);
		}
		else
		{
			int orderID = 0;
			if (0 != unPm.GetValue(HtField::OrderID, orderID)) { LOG_ERROR("[ht adaptor {}] getvalue OrderID error!", m_id); return -1; }
			LOG_ERROR("[ht adaptor {}] cancel order:{}", m_id, orderID);
		}

		HtDecreaseMsgPtr(response);
	}
	else
	{
		LOG_ERROR("[ht adaptor {}] cancel {} failed", m_id, order_id);
    }
    return 0;
}

bool hfs_ht_adaptor::start() {
	if (Init() != 0) return false;
    return true;
}

bool hfs_ht_adaptor::stop() {
    return true;
}

int hfs_ht_adaptor::login() {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "11000001");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	char encode[256] = { 0 };
	CPackMessageCommon::Encode(this->m_password.c_str(), encode, 256);
	pm.AddValue(HtField::Password, encode);//密砝加密坎传输密文
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

	CHtMessage* response = NULL;
	int result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);

	if (0 == result && NULL != response)
	{
		char jstr[1024] = { 0 };
		if (HtMsgPtrToString(response, jstr, 1024) > 0)//以json形弝输出结果
		{
			LOG_INFO("[ht adaptor {} login response:{}]",m_id, jstr);
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			// cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { LOG_ERROR("[ht adaptor {}] getvalue RequestExeStatus error!", m_id); return -1; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { LOG_ERROR("[ht adaptor {}] getvalue Text error!", m_id); return -1; }
            LOG_ERROR("[ht adaptor {}] login error:{}",m_id, error);
            m_state = -1;            
		}
		else
		{
			const char *token = NULL;
			if (0 != unPm.GetValue(HtField::Token, token)) { LOG_ERROR("[ht adaptor {}] getvalue Token error!", m_id); return -1; }
			this->m_userToken = token;
            LOG_ERROR("[ht adaptor {}] login success, get token:{}",m_id, token);
            m_state = 2;
		}

		HtDecreaseMsgPtr(response);
    }
    return 0;
}

void hfs_ht_adaptor::subscribeDetail() {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13990001");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());
	pm.AddValue(HtField::SubscriptionRequestType, "1");
	pm.AddValue(HtField::SubscriptionTopic, "1");

	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[10240] = { 0 };
		if (HtMsgPtrToString(response, jstr, 10240) > 0)//以json形弝输出结果
		{
			cout << jstr << endl;
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			// cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { LOG_ERROR("[ht adaptor {}] getvalue RequestExeStatus error!", m_id); return; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { LOG_ERROR("[ht adaptor {}] getvalue Text error!", m_id); return; }
			cout << "subscribeDeal error:" << error << endl;
		}
		else
		{
			cout << "subscribe success" << endl;
		}


		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "subscribeDeal error!" << result << endl;
	}
}

void hfs_ht_adaptor::subscribeOrder() {
	CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13990001");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());
	pm.AddValue(HtField::SubscriptionRequestType, "1");
	pm.AddValue(HtField::SubscriptionTopic, "2");

	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[10240] = { 0 };
		if (HtMsgPtrToString(response, jstr, 10240) > 0)//以json形弝输出结果
		{
			cout << jstr << endl;
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			// cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { cout << "get status value error!" << endl; return; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { cout << "getvalue error!" << endl; return; }
			cout << "subscribeOrder error:" << error << endl;
		}
		else
		{
			cout << "subscribe success" << endl;
		}


		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "subscribeOrder error!" << result << endl;
	}
}

int hfs_ht_adaptor::SendToServerSyn(std::string symbol, char side, int qty, float prc, std::string client_id) {
    int orderId = 0;
	CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010001");

	//新建一行存放输入坂数l
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str()); 
    pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
    if (symbol.find("SH") != std::string::npos) {
        pm.AddValue(HtField::SecurityExchange, "1");
    } else {
        pm.AddValue(HtField::SecurityExchange, "2");
    }
	
	//pm.AddValue(HtField::SecurityAccount, "A274752370");
	pm.AddValue(HtField::SecurityID, symbol.substr(0,6).c_str());
	pm.AddValue(HtField::OrderQty, qty);
	pm.AddValue(HtField::Price, prc);
	pm.AddValue(HtField::OrderSide, side=='B'?'1':'2');
	pm.AddValue(HtField::EntrustProp, "0");
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());
	pm.AddValue(HtField::ParentOrderID, "150006");  // 批次坷  TODO 坯坖交易日期

	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);

	if (0 == result && NULL != response)
	{

		char jstr[1024] = { 0 };
		if (HtMsgPtrToString(response, jstr, 1024) > 0)//以json形弝输出结果
		{
			LOG_INFO("[ht adaptor {}] SendToServerSyn response:{}",m_id, jstr);
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);
		
		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			// cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { LOG_ERROR("[ht adaptor {}] getvalue RequestExeStatus error!", m_id); return 0; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { LOG_ERROR("[ht adaptor {}] getvalue Text error!", m_id); return 0; }
			LOG_ERROR("[ht adaptor {}] SendToServerSyn error:{}", m_id, error);
		}
		else
		{
			int orderID = 0;
			if (0 != unPm.GetValue(HtField::OrderID, orderID)) { LOG_ERROR("[ht adaptor {}] getvalue OrderID error!", m_id); return 0; }
			orderId = orderID;
			LOG_INFO("[ht adaptor {}] SendToServerSyn orderid:{}", m_id, orderID);
		}

		HtDecreaseMsgPtr(response);
	}
	else
	{
		LOG_ERROR("[ht adaptor {}] SendToServerSyn error!", m_id);
	}
	return orderId;
}

int hfs_ht_adaptor::SendToServerAsy(const hfs_order_t &order, std::string client_id) {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010001");
	string symbol = order.symbol;
	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	if (symbol.find("SH") != std::string::npos) {
        pm.AddValue(HtField::SecurityExchange, "1");
    } else {
        pm.AddValue(HtField::SecurityExchange, "2");
    }	
	//pm.AddValue(HtField::SecurityAccount, "A274752370");
	pm.AddValue(HtField::SecurityID, symbol.substr(0,6).c_str());
	pm.AddValue(HtField::OrderQty, order.qty);
	pm.AddValue(HtField::Price, order.prc);
	pm.AddValue(HtField::OrderSide, order.side=='B'?'1':'2');
	pm.AddValue(HtField::EntrustProp, "0");
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());
	pm.AddValue(HtField::ParentOrderID, client_id.c_str());  // client_id  

	int ret = HtGetSendMsgInterface(m_channelID.c_str())->Send(pm.GetPackedMsg());
	if (0 != ret) {
		LOG_INFO("[ht adaptor {}] SendToServerAsy sengmsg failed, ret:{}",m_id, ret);
		return -1;
	}
	
	const char* msgID = NULL;
	HtGetClientMsgID(pm.GetPackedMsg(), msgID);
	LOG_INFO("[ht adaptor {}] async_entrust msgID:{}", m_id, msgID);
	m_msgIDMap[msgID] = client_id;
	return 0;
}

void hfs_ht_adaptor::qryAccount() {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010005");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::Currency, "0");
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[1024] = { 0 };
		if (HtMsgPtrToString(response, jstr, 1024) > 0)//以json形弝输出结果
		{
			cout << jstr << endl;
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		const char* msgType = NULL;
		if (0 == unPm.GetValue(HtField::MsgType, msgType))
		{
			cout << "msgType:" << msgType << endl;
		}

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { cout << "getvalue error!" << endl; return; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { cout << "getvalue error!" << endl; return; }
			cout << "queryFund error:" << error << endl;
		}
		else
		{
			double enableCash = 0;
			if (0 != unPm.GetValue(HtField::TradeableCash, enableCash)) { cout << "getvalue error!" << endl; return; }
			cout << "enableCash:" << enableCash << endl;
		}


		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "queryFund error!" << endl;
	}
}

std::string hfs_ht_adaptor::qryPosition() {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010007");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());

	pm.AddValue(HtField::SecurityAccount, "");
	pm.AddValue(HtField::SecurityExchange, "");
	pm.AddValue(HtField::SecurityID, "");
	pm.AddValue(HtField::PositionStr, m_pos_str.c_str());
	pm.AddValue(HtField::RequestNumber, 900);
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

    string pos_str("0");
	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		// char jstr[10240] = { 0 };

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { cout << "get status value error!" << endl; return pos_str; }
        
		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { LOG_ERROR("get text value error!"); return pos_str; }
			cout << "queryPosition error:" << error << endl;
		}
		else
		{
			int num = 0;
			if (0 != unPm.GetValue(HtField::SubGroupNumber, num)) { cout << "getvalue error!" << endl; return pos_str; }
			if (num == 0) m_position_file.close();

			//读坖坎续挝仓数杮
			int i = 2;
			if (m_pos_str != "0") i=3;
			for (; i < 2+num; i++)
			{
				unPm.SetCurrentRow(i);
				const char *stockCode = NULL;
                if (0 != unPm.GetValue(HtField::SecurityID, stockCode)) { cout << "getvalue SecurityID error!" << endl; return pos_str; }
                const char *pos = NULL;
                if (0 != unPm.GetValue(HtField::PositionStr, pos)) { cout << "getvalue PositionStr error!" << endl; return pos_str; }
				pos_str = pos;
				if (!m_position_file.is_open()) {LOG_ERROR("holding file not opened"); return pos_str; };
				//"Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr" 
				const char *HQty = nullptr;
				if (0 != unPm.GetValue(HtField::HoldingQty, HQty)) { cout << "getvalue HoldingQty error!" << endl; return pos_str; }
				const char *TQty = nullptr;
				if (0 != unPm.GetValue(HtField::TradeableQty, TQty)) { cout << "getvalue TradeableQty error!" << endl; return pos_str; }
				const char *CostPrice = nullptr;
				if (0 != unPm.GetValue(HtField::CostPrice, CostPrice)) { cout << "getvalue CostPrice error!" << endl; return pos_str; }
				const char *MValue = nullptr;
				if (0 != unPm.GetValue(HtField::SecurityMarketValue, MValue)) { cout << "getvalue SecurityMarketValue error!" << endl; return pos_str; }
				const char *account = nullptr;
				if (0 != unPm.GetValue(HtField::FundAccount, account)) { cout << "getvalue FundAccount error!" << endl; return pos_str; }
				m_position_file << account << "," << stockCode << "," << HQty << "," << TQty << "," << CostPrice << "," << MValue << ","
								<< pos << endl;
				m_position_file.flush();

				string symbol(stockCode);
				const char *SecurityExchange = nullptr;
				if (0 != unPm.GetValue(HtField::SecurityExchange, SecurityExchange)) { cout << "getvalue SecurityExchange error!" << endl; return pos_str; }
				if (SecurityExchange[0] == '1') {
					symbol.append(".SZ");
				} else if (SecurityExchange[0] == '2') {
					symbol.append(".SH");
				}
				if (m_holdings.find(symbol) == m_holdings.end()) {
					hfs_holdings_t* holding = new hfs_holdings_t();
					memset(holding, 0, sizeof(hfs_holdings_t));
					strcpy(holding->symbol, symbol.c_str());
					m_holdings.insert(make_pair(symbol, holding));
				}
				hfs_holdings_t* holding = m_holdings[symbol];
				holding->qty_long = atoi(HQty);
				holding->qty_long_yd = atoi(TQty);
				holding->avgprc_long = atof(CostPrice);
			}

		}
		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "queryPosition error!" << result << endl;
	}
    return pos_str;
}

std::string hfs_ht_adaptor::qryOrder(){
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010009");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::SecurityAccount, "");
	pm.AddValue(HtField::SecurityExchange, "");
	pm.AddValue(HtField::SecurityID, "");
	pm.AddValue(HtField::OrderID, "");
	pm.AddValue(HtField::OrderDir, "0");
	pm.AddValue(HtField::PositionStr, m_pos_str.c_str());
	pm.AddValue(HtField::RequestNumber, 500);
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

    string pos_str("0");
	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[10240] = { 0 };
		if (HtMsgPtrToString(response, jstr, 10240) > 0)//以json形弝输出结果
		{
			cout << jstr << endl;
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		// const char* msgType = NULL;
		// if (0 == unPm.GetValue(HtField::MsgType, msgType))
		// {
		// 	cout << "msgType:" << msgType << endl;
		// }

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { cout << "get status value error!" << endl; return pos_str; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { cout << "getvalue error!" << endl; return pos_str; }
			cout << "queryEntrust error:" << error << endl;
		}
		else
		{
			int num = 0;
			if (0 != unPm.GetValue(HtField::SubGroupNumber, num)) { cout << "getvalue error!" << endl; return pos_str; }
			if (num == 0) m_entrust_file.close();

			//读坖坎续数杮
			int i = 2;
			if (m_pos_str != "0") i=3;
			for (; i < 2 + num; i++)
			{
				unPm.SetCurrentRow(i);
				int orderID = 0;
                if (0 != unPm.GetValue(HtField::OrderID, orderID)) { cout << "getvalue error!" << endl; return pos_str; }
                const char *pos = NULL;
                if (0 != unPm.GetValue(HtField::PositionStr, pos)) { cout << "getvalue error!" << endl; return pos_str; }                
				pos_str = pos;
				
				const char *account = nullptr;
				if (0 != unPm.GetValue(HtField::FundAccount, account)) { cout << "getvalue FundAccount error!" << endl; return pos_str; }
				const char *dte = nullptr;
				if (0 != unPm.GetValue(HtField::TradeDate, dte)) { cout << "getvalue TradeDate error!" << endl; return pos_str; }
				const char *tme = nullptr;
				if (0 != unPm.GetValue(HtField::CreateTime, tme)) { cout << "getvalue CreateTime error!" << endl; return pos_str; }
				const char *code = nullptr;
				if (0 != unPm.GetValue(HtField::SecurityID, code)) { cout << "getvalue SecurityID error!" << endl; return pos_str; }
				const char *clientID = nullptr;
				if (0 != unPm.GetValue(HtField::ParentOrderID, clientID)) { cout << "getvalue ParentOrderID error!" << endl; return pos_str; }
				const char *side = nullptr;
				if (0 != unPm.GetValue(HtField::OrderSide, side)) { cout << "getvalue OrderSide error!" << endl; return pos_str; }
				const char *type = nullptr;
				if (0 != unPm.GetValue(HtField::EntrustType, type)) { cout << "getvalue EntrustType error!" << endl; return pos_str; }
				const char *status = nullptr;
				if (0 != unPm.GetValue(HtField::OrdStatus, status)) { cout << "getvalue OrdStatus error!" << endl; return pos_str; } 
				const char *price = nullptr;
				if (0 != unPm.GetValue(HtField::Price, price)) { cout << "getvalue Price error!" << endl; return pos_str; }
				const char *qty = nullptr;
				if (0 != unPm.GetValue(HtField::OrderQty, qty)) { cout << "getvalue OrderQty error!" << endl; return pos_str; }
				const char *AvgPx = nullptr;
				if (0 != unPm.GetValue(HtField::AvgPx, AvgPx)) { cout << "getvalue AvgPx error!" << endl; return pos_str; }
				const char *filledQty = nullptr;
				if (0 != unPm.GetValue(HtField::CumQty, filledQty)) { cout << "getvalue CumQty error!" << endl; return pos_str; }
				const char *cancelledQty = nullptr;
				if (0 != unPm.GetValue(HtField::CancelledQty, cancelledQty)) { cout << "getvalue CancelledQty error!" << endl; return pos_str; }
				const char *text = nullptr;
				if (0 != unPm.GetValue(HtField::Text, text)) { cout << "getvalue Text error!" << endl; return pos_str; }
				const char *origID = nullptr;
				if (0 != unPm.GetValue(HtField::OrigOrderID, origID)) { cout << "getvalue OrigOrderID error!" << endl; return pos_str; }
				//"Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID"
				m_entrust_file << account << "," << code << "," << dte << "," << tme << "," << orderID << "," 
								<< clientID << "," << side << "," << type << "," << status << "," << price << "," << qty << ","
								<< AvgPx << "," << filledQty << "," << cancelledQty << "," << text << "," << pos_str << "," << origID << endl;
			}

		}
		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "queryEntrust error!" << result << endl;
    }
    return pos_str;
}

std::string hfs_ht_adaptor::qryDetail() {
    CPackMessageCommon pm;
	//消杯类型放在首行
	pm.AddValue(HtField::MsgType, "13010011");

	//新建一行存放输入坂数
	pm.SetCurrentRow(1);
	pm.AddValue(HtField::LoginUserID, this->m_userID.c_str());
	pm.AddValue(HtField::Token, this->m_userToken.c_str());
	pm.AddValue(HtField::FundAccount, m_fundAccount.c_str());
	pm.AddValue(HtField::SecurityAccount, "");
	pm.AddValue(HtField::SecurityExchange, "");
	pm.AddValue(HtField::SecurityID, "");
	pm.AddValue(HtField::PositionStr, m_pos_str.c_str());
	pm.AddValue(HtField::RequestNumber, 500);
	pm.AddValue(HtField::OrderStation, m_sysinfo.c_str());

    string pos_str("0");
	CHtMessage* response = NULL;
	HT_int32 result = HtGetSendMsgInterface(m_channelID.c_str())->Send_Sync(pm.GetPackedMsg(), response);
	if (0 == result && NULL != response)
	{
		char jstr[10240] = { 0 };
		if (HtMsgPtrToString(response, jstr, 10240) > 0)//以json形弝输出结果
		{
			cout << jstr << endl;
		}

		CUnPackMessageCommon unPm;
		unPm.SetPackedMsg(response);

		//从首行坖出消杯类型
		// const char* msgType = NULL;
		// if (0 == unPm.GetValue(HtField::MsgType, msgType))
		// {
		// 	cout << "msgType:" << msgType << endl;
		// }

		//从第一行读坖结果
		unPm.SetCurrentRow(1);
		int status = 0;//获坖执行状思
		if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { cout << "get status value error!" << endl; return pos_str; }

		if (0 != status)
		{
			const char* error = NULL;//获坖失败原因
			if (0 != unPm.GetValue(HtField::Text, error)) { cout << "getvalue error!" << endl; return pos_str; }
			cout << "queryDeal error:" << error << endl;
		}
		else
		{
			int num = 0;
			if (0 != unPm.GetValue(HtField::SubGroupNumber, num)) { cout << "getvalue error!" << endl; return pos_str; }
			if (num == 0) m_detail_file.close();

			//读坖坎续挝仓数杮
			int i = 2;
			if (m_pos_str != "0") i=3;
			for (; i < 2 + num; i++)
			{
				unPm.SetCurrentRow(i);
				const char *execID = NULL;
                if (0 != unPm.GetValue(HtField::OrderID, execID)) { cout << "getvalue error!" << endl; return pos_str; }
                const char *pos = NULL;
                if (0 != unPm.GetValue(HtField::PositionStr, pos)) { cout << "getvalue error!" << endl; return pos_str; }
				pos_str = pos;
				//Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID
				const char *account = nullptr;
				if (0 != unPm.GetValue(HtField::FundAccount, account)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *code = nullptr;
				if (0 != unPm.GetValue(HtField::SecurityID, code)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *dte = nullptr;
				if (0 != unPm.GetValue(HtField::TradeDate, dte)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *tme = nullptr;
				if (0 != unPm.GetValue(HtField::ExchangeMatchDate, tme)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *orderID = nullptr;
				if (0 != unPm.GetValue(HtField::OrderID, orderID)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *side = nullptr;
				if (0 != unPm.GetValue(HtField::OrderSide, side)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *qty = nullptr;
				if (0 != unPm.GetValue(HtField::LastQty, qty)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *type = nullptr;
				if (0 != unPm.GetValue(HtField::ExchangeExecType, type)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *prc = nullptr;
				if (0 != unPm.GetValue(HtField::LastPx, prc)) { cout << "getvalue error!" << endl; return pos_str; }
				const char *origID = nullptr;
				if (0 != unPm.GetValue(HtField::OrigOrderID, origID)) { cout << "getvalue error!" << endl; return pos_str; }

				m_detail_file << account << "," << code << "," << dte << "," << tme << "," << orderID << ","
							 << side << "," << type << "," << prc << "," << qty << "," << pos << "," << origID << endl;
			}

		}
		HtDecreaseMsgPtr(response);
	}
	else
	{
		cout << "queryDeal error!" << result << endl;
    }
    return pos_str;
}

void hfs_ht_adaptor::ProcAsyncCallBack(int iRet, const CHtMessage* pstMsg)
{
	if (0 != iRet)
	{
		cout << "ProcAsyncCallBack error!" << endl;
		return;
	}

	CUnPackMessageCommon unPm;
	unPm.SetPackedMsg(pstMsg);

	//从首行坖出消杯类型
	const char* msgType = NULL;
	if (0 == unPm.GetValue(HtField::MsgType, msgType))
	{
		cout << "msgType:" << msgType << endl;
	}

	if (0 == strcmp(msgType, "13990004"))
	{
		LOG_INFO("[ht adaptor {}] ProcAsyncCallBack Deal", m_id);
		OnRtnTrade(unPm);
	}
	else if (0 == strcmp(msgType, "13990008"))
	{
		LOG_INFO("[ht adaptor {}] ProcAsyncCallBack order", m_id);
		OnRtnOrder(unPm);
	}
	else if (0 == strcmp(msgType, "13010002")) 
	{
		OnRspOrder(unPm);
	}

	char jstr[1024] = { 0 };
	if (HtMsgPtrToString(pstMsg, jstr, 1024) > 0)//以json形弝输出结果
	{
		LOG_INFO("[ht adaptor {}] ProcAsyncCallBack {}",m_id, jstr);
	}

	if (pstMsg != NULL)
	{
		HtDecreaseMsgPtr(pstMsg);//释放应答消杯内存
	}

}

void hfs_ht_adaptor::OnRspOrder(CUnPackMessageCommon &unPm) {
	unPm.SetCurrentRow(1);
	const char *msgID = nullptr;
	if (0 != unPm.GetValue(HtField::CliengMsgID, msgID)) { LOG_ERROR("[ht adaptor {}] OnRspOrder getvalue CliengMsgID failed", m_id); return; }	
	string strID(msgID);
	if (m_msgIDMap.find(strID) == m_msgIDMap.end()) {
		LOG_INFO("[ht adaptor {}] OnRspOrder cannot find detal msgID:{}",m_id, strID);
		return;
	}
	string client_id = m_msgIDMap[strID];
	int orderID = 0;
	if (0 != unPm.GetValue(HtField::OrderID, orderID)) { LOG_ERROR("[ht adaptor {}]OnRspOrder getvalue OrderID failed", m_id); return; }
	EntrustNode* enode = getENode(client_id);
	if (enode == nullptr) {
		LOG_INFO("[ht adaptor {}]OnRspOrder cannot find enode : {}",m_id, client_id);
		return ;
	}
	hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	sprintf(enode->origOrder.orderid, "%d", orderID);
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
	order.qty = enode->qty;	
	int status = 0;
	if (0 != unPm.GetValue(HtField::RequestExeStatus, status)) { LOG_ERROR("[ht adaptor {}][OnRspOrder] getvalue RequestExeStatus failed",m_id); return; }	
	if (status == 0) {
		order.type = 'A';
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
	}
	else {
		order.type = 'J';
		order.state = ORD_REJECTED;
		enode->origOrder.state = ORD_REJECTED;
		const char *errmsg = nullptr;
		if (0 != unPm.GetValue(HtField::Text, errmsg)) { LOG_ERROR("[ht adaptor {}][OnRspOrder] getvalue Text failed",m_id); return; }
		strncpy(order.algo, errmsg, sizeof(order.algo)-1);
		LOG_INFO("[ht adaptor {}][ht adaptor OnRspOrder] get error:{}",m_id, errmsg);
	}

	if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ht adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
}

void hfs_ht_adaptor::OnRtnOrder(CUnPackMessageCommon &unPm) {
	unPm.SetCurrentRow(1);
	const char *client_id=NULL;
	if (0 != unPm.GetValue(HtField::ParentOrderID, client_id)) { LOG_ERROR("[ht adaptor {}]OnRtnOrder getvalue ParentOrderID failed", m_id); return; }	
	EntrustNode* enode = getENode(string(client_id));
	if (enode == nullptr) {
		LOG_INFO("[ht adaptor {}]OnRtnOrder cannot find enode : {}", m_id, client_id);
		return ;
	}
	int orderID = 0;
	if (0 != unPm.GetValue(HtField::OrderID, orderID)) { LOG_ERROR("[ht adaptor {}]OnRtnOrder getvalue OrderID failed", m_id); return; }
	char status;
	if (0 != unPm.GetValue(HtField::OrdStatus, status)) { LOG_ERROR("[ht adaptor {}]OnRtnOrder getvalue OrdStatus failed", m_id); return; }

	hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
	sprintf(enode->origOrder.orderid, "%d", orderID);
	//委托戝功，ordStatus为已报；委托失败，ordStatus为废坕。
	if (status == '2') {
		order.qty = enode->qty;
		order.type = 'A';
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
	} else if (status == '9') {
		order.qty = enode->qty;
		order.type = 'J';
		order.state = ORD_REJECTED;
		enode->origOrder.state = ORD_REJECTED;
		const char *errmsg = nullptr;
		if (0 != unPm.GetValue(HtField::Text, errmsg)) { LOG_ERROR("[ht adaptor {}] OnRtnOrder getvalue Text failed",m_id); return; }
		strncpy(order.algo, errmsg, sizeof(order.algo)-1);
	}
	if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ht adaptor {}]onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
}

void hfs_ht_adaptor::OnRtnTrade(CUnPackMessageCommon &unPm) {
	unPm.SetCurrentRow(1);
	const char *client_id=NULL;
	if (0 != unPm.GetValue(HtField::ParentOrderID, client_id)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue ParentOrderID failed", m_id); return; }
	// if (m_msgIDMap.find(msgID) == m_msgIDMap.end()) {
	// 	LOG_INFO("cannot find detal msgID:{}", msgID);
	// 	return;
	// }
	// string client_id = m_msgIDMap[msgID];
	EntrustNode* enode = getENode(client_id);
	if (enode == nullptr) {
		LOG_INFO("[ht_adaptor {}] OnRtnTrade cannot find enode : {}", m_id, client_id);
		return ;
	}
	int orderID = 0;
	if (0 != unPm.GetValue(HtField::OrderID, orderID)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue OrderID failed", m_id); return; }
	char execType;
	if (0 != unPm.GetValue(HtField::ExchangeExecType, execType)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue ExchangeExecType failed", m_id); return; }
	char execStatus;
	if (0 != unPm.GetValue(HtField::ExchangeExecStat, execStatus)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue ExchangeExecStat failed", m_id); return; }
	int qty;
	if (0 != unPm.GetValue(HtField::LastQty, qty)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue LastQty failed", m_id); return; }
	double prc ;
	if (0 != unPm.GetValue(HtField::LastPx, prc)) { LOG_ERROR("[ht_adaptor {}]OnRtnTrade getvalue LastPx failed", m_id); return; }
	hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
	sprintf(enode->origOrder.orderid, "%d", orderID);
	if (execType == '0' && execStatus=='0') { // 买卖成交
		prc = (enode->eamt + qty * prc) / (qty + enode->eqty + 0.000001);
		order.state = ORD_NEW;		
        enode->origOrder.state = ORD_NEW;
        order.type = 'E';
        order.qty = qty;
        order.prc = prc;
		enode->eqty += qty;  // 增針戝交
		enode->eamt += qty * prc;
		if (enode->eqty >= enode->qty) {
			enode->origOrder.state = ORD_FILLED;
		}
	} else if (execType == '2' && execStatus=='0') { // 撤单成交
        enode->origOrder.state = ORD_CANCELED;
        order.state = ORD_CANCELED;
        order.type = 'C';
        order.prc = enode->origOrder.prc;
        order.qty = qty;
	} else {
		LOG_INFO("[ht_adaptor {}] OnRtnTrade unkonwn execType:{}",m_id, execType);
		return ;
	}
	if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[ht_adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
	// TODO  handle holding
}

/*
网络断线針连戝功坎自动调用
*/

HT_void hfs_ht_adaptor::OnTokenLoseEfficacy(const CHtMessage* pstMsg)
{
	cout << "on OnTokenLoseEfficacy relogin" << endl;
	m_state = -1;
	login();
}

void hfs_ht_adaptor::OnConnect()
{
	//因断线会导致订阅失效，请务必在此針新订阅
	cout << "on connect" << endl;
	login();
	subscribeDetail();
	subscribeOrder();
}

/*
网络断线坎自动调用
*/
void hfs_ht_adaptor::OnClose()
{
	cout << "network error!" << endl;
	m_state = -1;
}

HT_void hfs_ht_adaptor::ProcEvent(HT_int32 eventType, const CHtMessage* pstMsg)
{
	cout << "on ProcEvent" << endl;
}

HT_void hfs_ht_adaptor::OnError()
{
	cout << "on error" << endl;
}

bool hfs_ht_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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

void hfs_ht_adaptor::push_reject_order(int teid, hfs_order_t &order, const char *errmsg) {
	hfs_order_t od;
	memcpy(&od, &order, sizeof(hfs_order_t));
	// od.qty = order.qty;
	od.type = 'J';
	od.state = ORD_REJECTED;
	strncpy(od.algo, errmsg, sizeof(od.algo)-1);
	if (!onResponse(teid, m_id, od)) {
		LOG_ERROR("[ht adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, teid, order.pid, order.oseq);
	}
}

EntrustNode *hfs_ht_adaptor::getENode(OrderID& oid) {
	std::lock_guard<std::mutex> lk(m_nid_mtx);
	EntrustNode *rv = nullptr;
	if (nmap.find(oid) != nmap.end()) {
		return nmap[oid];
	}
	return rv;
}

EntrustNode *hfs_ht_adaptor::getENode(string sid) {
	std::lock_guard<std::mutex> lk(m_sid_mtx);
	EntrustNode *enode = nullptr;

	auto iter = smap.find(sid);
	if (iter != smap.end()) {
		enode = smap[sid];
	}
	return enode;
}

void hfs_ht_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("[ht adaptor {}]holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             m_id, holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_ht_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("[ht adaptor {}]order:symbol:{},exch:{},pid:{},orderid:{},tm:{},type:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             m_id, order->symbol, order->exch, order->pid, order->orderid, order->tm, (char)order->type, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

//加密
void hfs_ht_adaptor::encode(char *pstr, int *pkey){
    int len = strlen(pstr);//获坖长度
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_ht_adaptor::decode(char *pstr, int *pkey){
    int len = strlen(pstr);
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

std::string hfs_ht_adaptor::getOPStation(int traderid) {
	if (m_opstation.find(traderid) != m_opstation.end()) {
		return m_opstation[traderid];
	} 
	return m_sysinfo;
}