#include "hfs_uft_adaptor.hpp"

using namespace std;

hfs_uft_adaptor::hfs_uft_adaptor(const a2ftool::XmlNode& cfg):hfs_base_adaptor(cfg), m_state(-1) {
    accountid = cfg.getAttrDefault("account", "");
    passwd = cfg.getAttrDefault("password", "");
    server = cfg.getAttrDefault("server", "60.191.116.105:8066");
    license = cfg.getAttrDefault("license", "license.dat");
    license_pwd = cfg.getAttrDefault("license_pwd", "888888");
    cert_file = cfg.getAttrDefault("cert", "license.dat");
    cert_pwd = cfg.getAttrDefault("cert_pwd", "888888");

    m_id = cfg.getAttrDefault("id", "");
	m_sysinfo = cfg.getAttrDefault("sysinfo", "");
    m_export_path = cfg.getAttrDefault("export_path", "./export/");

    //int timex = current();
	//m_orderRef = (timex/10000*3600)+((timex%10000)/100*60)+(timex%100);
    //m_orderRef = (today()%10)*100 + m_orderRef;

    m_lpPacker = NewPacker(2);
    m_lpPacker->AddRef();
    m_lpMessage = NewBizMessage();
    m_lpMessage->AddRef();

    m_rc = new RiskControl(cfg);
    // a2ftool::XmlNode router;
    // if (cfg.isRoot()) {
    //     router = cfg.getChild("router");
    // } else {
    //     router = cfg.getRoot().getChild("router");
    // }

	// string db_user = router.getAttrDefault("db_user","");
	// string db_passwd = router.getAttrDefault("db_pass","");
	// string db_addr = router.getAttrDefault("db_addr","");
	// if (!db_addr.empty())
	// 	m_sql = new hfs_database_mysql(db_user.c_str(), db_passwd.c_str(), db_addr.c_str());
	// else m_sql = nullptr;
}

hfs_uft_adaptor::~hfs_uft_adaptor() {
    RealseAll();
}

int hfs_uft_adaptor::Init() {
    //通过T2SDK的引出函数，来获取一个新的CConfig对象
    //此对象在创建连接对象时被传递，用于配置所创建的连接对象的各种属性（比如服务器IP地址、安全模式）
    m_lpConfig = NewConfig();
    //通过T2SDK的引出函数NewXXXX返回的对象，需要调用对象的Release方法释放，而不能直接用delete
    //因为t2sdk.dll和调用程序可能是由不同的编译器、编译模式生成，delete可能会导致异常
    //为了适应Delphi等使用（Delphi对接口自动调用AddRef方法），用C/C++开发的代码，需要在NewXXXX之后调用一下AddRef
    //以保证引用计数正确
    m_lpConfig->AddRef();
    //if (access("./t2sdk.ini", 0)!=0) {
        //[t2sdk] servers指定需要连接的IP地址及端口
        m_lpConfig->SetString("t2sdk", "servers", server.c_str());        
        //[t2sdk] license_file指定许可证文件
        m_lpConfig->SetString("t2sdk", "license_file", license.c_str());        
        //[t2sdk] send_queue_size指定T2_SDK的发送队列大小
        m_lpConfig->SetString("t2sdk", "send_queue_size", "1000");
        //在此设置一下就可以支持自动重连
        m_lpConfig->SetString("t2sdk", "auto_reconnect", "1");        
        //接收缓存的初始大小，单位字节，实际接收到服务端的数据时，可能会扩大（如果需要）
        m_lpConfig->SetString("t2sdk", "init_recv_buf_size", "512");        
        //每块发送缓存的初始大小，单位字节，该大小也会根据需要动态扩大
        m_lpConfig->SetString("t2sdk", "init_send_buf_size", "512");            
        //发送队列的大小，该大小不会动态变化，若该配置项很小，且连接发包很频繁，则可能
        //因为发送队列满而造成发送失败
        m_lpConfig->SetString("t2sdk", "send_queue_size", "512");          
        //用于指定T2_SDK错误信息的语言ID（2052为简体中文，1033为英文）若不配置该配置项，默认为2052
        m_lpConfig->SetString("t2sdk", "lang", "2052");         
        //客户端给服务端发送心跳的间隔时间，单位为秒，最小值为5秒; 不配或者配<=0，表示不开启客户端心跳
        m_lpConfig->SetString("t2sdk", "heartbeat_time", "30");         
        //第三方许可证的密码，统一接入专用, 实际上需不需要请与财通技术人员联系
        m_lpConfig->SetString("t2sdk", "license_pwd", license_pwd.c_str());         
        //连接的安全模式，可以选择明文（none），通信密码（pwd），SSL（ssl）; 注意大小写敏感
        m_lpConfig->SetString("safe", "safe_level", "none");
        //cert_file配置被服务端校验的证书路径和密码
        // m_lpConfig->SetString("safe", "cert_file", cert_file.c_str());          
        //cert_file证书密码
        // m_lpConfig->SetString("safe", "cert_pwd", cert_pwd.c_str());      
        //是否校验服务端，可不配，若不配，默认校验服务端
        // m_lpConfig->SetString("safe", "check_server_cert", "1");         
        ////其它配置项同上配置，可参考文档《财通UFX统一接入平台_开发手册_标准版.pdf》

        ///可以把配置信息保存到文件，这样以后只需要把文件Load进来即可
        //m_lpConfig->Save("t2sdk.ini");
    //} else {
        //m_lpConfig->Load("./t2sdk.ini");
    //}
    //通过T2SDK的引出函数，来获取一个新的CConnection对象
    m_lpConnection = NewConnection(m_lpConfig);
    m_lpConnection->AddRef();
    int iRet = 0;

    //初始化连接对象，返回0表示初始化成功，注意此时并没开始连接服务器
    if (0 == (iRet = m_lpConnection->Create2BizMsg(this))) //注意异步模式必须要有g_callback，同步模式此处可填NULL
    {
        //正式开始连接注册，参数1000为超时参数，单位是ms
        iRet = m_lpConnection->Connect(1000);
        if (iRet != 0)
        {
            //若连接/注册失败，打印失败原因
            cout << "connect failed : " << iRet  << (m_lpConnection->GetErrorMsg(iRet)) << endl;
        }
    }
    else
    {
        cout << (m_lpConnection->GetErrorMsg(iRet)) << endl;
    }
    while(m_state != 1) {
        sleep(1);
    }
    login();  
    while(m_state != -1 && m_state != 2) {
        cout << "connecting..." << endl;
        sleep(1);
    }
    if (m_state == -1)
        return -1;
    auto qry_thread = std::thread(std::bind(&hfs_uft_adaptor::qryThreadFunc, this));
    qry_thread.detach();

    subscribeOrder();
    subscribeDetail();
    sleep(5);

    return iRet;
}
void hfs_uft_adaptor::qryThreadFunc() {
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

        sprintf(fname, "%s/STOCK_ACCOUNT_%d.csv", m_export_path.c_str(), today());
        if (m_account_file.is_open())
        {
            m_account_file.close();
        }
        m_account_file.open(fname, std::ios::out);
        m_account_file << "Account,Available,MarketValue,AssetBalance" << endl;
        string pos_str("");
        qryAccount();
        while (m_finished == false)
            sleep(2);
        qryPosition(pos_str);
        while (m_finished == false)
            sleep(2);
        qryOrder(pos_str);
        while (m_finished == false)
            sleep(2);
        qryTrade(pos_str);
        while (m_finished == false)
            sleep(2);

        sleep(5 * 60);  // 每隔5分钟查询一次全量数据
    }
}

void hfs_uft_adaptor::subscribeOrder(){
    ///加入字段名
    m_lpPacker->BeginPack();
	m_lpPacker->AddField("branch_no", 'I', 5);			
	m_lpPacker->AddField("fund_account", 'S', 18);		
	// m_lpPacker->AddField("op_branch_no");	
	// m_lpPacker->AddField("op_entrust_way");
	// m_lpPacker->AddField("op_station");
	// m_lpPacker->AddField("client_id");
	// m_lpPacker->AddField("password");
	// m_lpPacker->AddField("user_token");
	m_lpPacker->AddField("issue_type");  
	
	///加入对应的字段值
	m_lpPacker->AddInt(atoi(branch_no.c_str()));					
	m_lpPacker->AddStr(accountid.c_str());				
	// m_lpPacker->AddStr("0");							
	// m_lpPacker->AddStr("");	
	// m_lpPacker->AddStr("0");	//op_station
	// m_lpPacker->AddStr(client_id.c_str());		
	// m_lpPacker->AddStr(passwd.c_str());					
	// m_lpPacker->AddStr(token.c_str());		
	m_lpPacker->AddStr("23");			
	///结束打包
	m_lpPacker->EndPack();

    m_lpMessage->SetSequeceNo(23);
    m_lpMessage->SetIssueType(23);
	m_lpMessage->SetPacketType(REQUEST_PACKET);
    m_lpMessage->SetFunction(620001);
    m_lpMessage->SetKeyInfo(m_lpPacker->GetPackBuf(), m_lpPacker->GetPackLen());	
    
    int iRet = m_lpConnection->SendBizMsg(m_lpMessage, 1);
    if (iRet<=0)   //失败
    {
    	cout << (m_lpConnection->GetErrorMsg(iRet)) << endl;
    	return;
    }
}
void hfs_uft_adaptor::subscribeDetail() {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("issue_type");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("exchange_type");
    m_lpPacker->AddField("stock_code");
    m_lpPacker->AddField("fund_account");   		
    
    m_lpPacker->AddStr("12");  //issue_type     12 为成交回报订阅
    m_lpPacker->AddStr(branch_no.c_str());  //branch_no   
    m_lpPacker->AddStr("*");  //exchange_type       
    m_lpPacker->AddStr("*");  //stock_code        
    m_lpPacker->AddStr(accountid.c_str());  //fund_account         
    m_lpPacker->EndPack();

    m_lpMessage->SetPacketType(REQUEST_PACKET);
    m_lpMessage->SetIssueType(12);
    m_lpMessage->SetFunction(620001);
    m_lpMessage->SetKeyInfo(m_lpPacker->GetPackBuf(), m_lpPacker->GetPackLen());	
    
    int iRet = m_lpConnection->SendBizMsg(m_lpMessage, 1);
    if (iRet<=0)   //失败
    {
    	cout << (m_lpConnection->GetErrorMsg(iRet)) << endl;
    	return;
    }
}

bool hfs_uft_adaptor::onRequest(int teid, hfs_order_t &order) {
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
    };
    return true;
}

bool hfs_uft_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
	assert(order.type == 'O');
	dump(&order);
	if (m_state != 2) {
		LOG_INFO("[uft adaptor {}] adaptor not login, entrust failed", m_id);
		push_reject_order(teid, order, "adaptor not login");
		return false;
	}
	
	if (m_rc->OnInsertOrder(&order) == false || m_rc->IsCanInsert() == false) {
		LOG_INFO("[uft adaptor {}] entrust failed, RiskControl refused", m_id);
		push_reject_order(teid, order, "RickControl refused");
		return false;
	}

    m_lpPacker->BeginPack();
    // m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("password_type");
    m_lpPacker->AddField("user_token");
    m_lpPacker->AddField("exchange_type", 'S');
    m_lpPacker->AddField("stock_code");
    m_lpPacker->AddField("entrust_amount", 'I', 10);
    m_lpPacker->AddField("entrust_price", 'F', 11);
    m_lpPacker->AddField("entrust_bs", 'C', 1);
    m_lpPacker->AddField("entrust_prop");
    m_lpPacker->AddField("batch_no", 'I', 8);

    m_lpPacker->AddStr("Z");   // op_entrust_way
    m_lpPacker->AddStr(m_sysinfo.c_str());   // op_station
    m_lpPacker->AddStr(branch_no.c_str());  // branch_no
    m_lpPacker->AddStr(client_id.c_str());  // client_id
    m_lpPacker->AddStr(accountid.c_str());  // fund_account
    m_lpPacker->AddStr(passwd.c_str());  // password
    m_lpPacker->AddStr("2");  // password_type
    m_lpPacker->AddStr(token.c_str());  // user_token
    string symbol(order.symbol);
    if (symbol.find("SH") != std::string::npos) {
        m_lpPacker->AddStr("1");  // exchange_type
    } else {
        m_lpPacker->AddStr("2");  // exchange_type
    }
    
    m_lpPacker->AddStr(symbol.substr(0,6).c_str());   //stock_code
    m_lpPacker->AddInt(order.qty);  // entrust_amount
    m_lpPacker->AddDouble(round(order.prc/0.01)*0.01);  // entrust_price
    if (order.side == HFS_ORDER_DIRECTION_LONG) {
        m_lpPacker->AddChar('1');  // entrust_bs
    } else 
        m_lpPacker->AddChar('2');  // entrust_bs
    m_lpPacker->AddInt(0);  //entrust_prop

    int batch_no = teid * 1000 + ++m_orderRef;
    m_lpPacker->AddInt(batch_no); //batch_no
    m_lpPacker->EndPack();

    int ret = SendToServerAsy(FUNC_INSERT_ORDER, m_lpPacker, batch_no);
    if (ret <= 0) {
        LOG_INFO("[uft adaptor {}] entrust failed, send order failed", m_id);
		push_reject_order(teid, order, "send order failed");
		return false;
    }

	order.tm = current();
	if(!registerTrader(teid, order, batch_no)) {
    	LOG_ERROR("[uft adaptor {}] register trader failed, teid:{}, pid:{}, oseq:{}, sid:{}",m_id, teid, order.pid, order.oseq, m_orderRef);
    }
    return true;
}

bool hfs_uft_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
	assert(order.type == 'X');
	dump(&order);
	if (m_state != 2) {
		LOG_INFO("[uft adaptor {}] adaptor not login, cancel failed", m_id);
		return false;
	}
	if (m_rc->IsCanCancel() == false) {
		LOG_INFO("[uft adaptor {}] RiskControl refuse cancel request", m_id);
		return false;
	}
	
	OrderID oid(teid, order.pid, order.oseq);
	EntrustNode* node = getENode(oid);
	if (node) {
		m_lpPacker->BeginPack();
        m_lpPacker->AddField("op_entrust_way");
        m_lpPacker->AddField("op_station");
        m_lpPacker->AddField("branch_no");
        m_lpPacker->AddField("client_id");
        m_lpPacker->AddField("fund_account");
        m_lpPacker->AddField("password");
        m_lpPacker->AddField("password_type");
        m_lpPacker->AddField("user_token");
        m_lpPacker->AddField("entrust_no");
        m_lpPacker->AddField("batch_flag");

        m_lpPacker->AddStr("Z");   // op_entrust_way
        m_lpPacker->AddStr(m_sysinfo.c_str());   // op_station
        m_lpPacker->AddStr(branch_no.c_str());  // branch_no
        m_lpPacker->AddStr(client_id.c_str());  // client_id
        m_lpPacker->AddStr(accountid.c_str());  // fund_account
        m_lpPacker->AddStr(passwd.c_str());  // password
        m_lpPacker->AddStr("2");  // password_type
        m_lpPacker->AddStr(token.c_str());  // user_token
        m_lpPacker->AddStr(node->origOrder.orderid);
        m_lpPacker->AddStr("0");
        m_lpPacker->EndPack();
        
        int ret = SendToServerAsy(FUNC_CANCEL_ORDER, m_lpPacker);
        if (ret <= 0) {
            LOG_ERROR("[uft adaptor {}] cancel order failed : {}", m_id, ret);
            return false;
        }
	} else {
		LOG_ERROR("[uft adaptor {}] cancel not exists order, teid= {} oseq={}",m_id, teid, order.oseq);
    	return true;
	}
	return true;
}
int hfs_uft_adaptor::cancel_order(const char* orderid) {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("password_type");
    m_lpPacker->AddField("user_token");
    m_lpPacker->AddField("entrust_no");
    m_lpPacker->AddField("batch_flag");

    m_lpPacker->AddStr("Z");   // op_entrust_way
    m_lpPacker->AddStr(m_sysinfo.c_str());   // op_station
    m_lpPacker->AddStr(branch_no.c_str());  // branch_no
    m_lpPacker->AddStr(client_id.c_str());  // client_id
    m_lpPacker->AddStr(accountid.c_str());  // fund_account
    m_lpPacker->AddStr(passwd.c_str());  // password
    m_lpPacker->AddStr("2");  // password_type
    m_lpPacker->AddStr(token.c_str());  // user_token
    m_lpPacker->AddStr(orderid);
    m_lpPacker->AddStr("0");
    m_lpPacker->EndPack();
    
    int ret = SendToServerAsy(FUNC_CANCEL_ORDER, m_lpPacker);
    if (ret <= 0) {
        LOG_ERROR("[uft adaptor {}] cancel order failed : {}", m_id, ret);
        return -1;
    }
    return 0;
}
unsigned long hfs_uft_adaptor::QueryInterface(const char *iid, IKnown **ppv)
{
    return 0;
}

unsigned long hfs_uft_adaptor::AddRef()
{
    return 0;
}

unsigned long hfs_uft_adaptor::Release()
{
    return 0;
}

bool hfs_uft_adaptor::start() {
	if (Init() != 0) return false;
    return true;
}

bool hfs_uft_adaptor::stop() {
    return true;
}

// 以下各回调方法的实现仅仅为演示使用
void hfs_uft_adaptor::OnConnect(CConnectionInterface *lpConnection)
{
    cout << ("hfs_uft_adaptor::OnConnect") << endl;
    m_state = 1;
}

void hfs_uft_adaptor::OnSafeConnect(CConnectionInterface *lpConnection)
{
    cout << ("hfs_uft_adaptor::OnSafeConnect") << endl;
}

void hfs_uft_adaptor::OnRegister(CConnectionInterface *lpConnection)
{
    cout << ("hfs_uft_adaptor::OnRegister") << endl;
    m_state = 1;    
}

void hfs_uft_adaptor::OnClose(CConnectionInterface *lpConnection)
{
    cout << ("hfs_uft_adaptor::OnClose") << endl;
}

int hfs_uft_adaptor::login() {
    // 登录用同步
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("password_type");
    m_lpPacker->AddField("input_content");
    m_lpPacker->AddField("account_content");
    m_lpPacker->AddField("content_type");
    m_lpPacker->AddField("comm_password");
    m_lpPacker->AddField("auth_product_type");
    m_lpPacker->AddField("auth_key");			
    
    m_lpPacker->AddStr("0");  //op_branch_no     
    m_lpPacker->AddStr("Z");  //op_entrust_way
    m_lpPacker->AddStr(m_sysinfo.c_str()); //op_station
    m_lpPacker->AddStr("0");  //branch_no        
    m_lpPacker->AddStr(passwd.c_str());  //password
    m_lpPacker->AddStr("2");
    m_lpPacker->AddStr("1");                //input_content
    m_lpPacker->AddStr(accountid.c_str());  //account_content  
    m_lpPacker->AddStr("0");  //content_type     
    m_lpPacker->AddStr("");  //comm_password    
    m_lpPacker->AddStr("");  //auth_product_type
    m_lpPacker->AddStr("");  //auth_key   
    m_lpPacker->EndPack();       
    
    SendToServerAsy(331100, m_lpPacker); 
    return 0; 
}

void hfs_uft_adaptor::OnSent(CConnectionInterface *lpConnection, int hSend, void *reserved1, void *reserved2, int nQueuingData)
{
}

void hfs_uft_adaptor::Reserved1(void *a, void *b, void *c, void *d){}
void hfs_uft_adaptor::Reserved2(void *a, void *b, void *c, void *d){}
int  hfs_uft_adaptor::Reserved3(){return 0;}
void hfs_uft_adaptor::Reserved4(){}
void hfs_uft_adaptor::Reserved5(){}
void hfs_uft_adaptor::Reserved6(){}
void hfs_uft_adaptor::Reserved7(){}

void hfs_uft_adaptor::OnReceivedBizEx(CConnectionInterface *lpConnection, int hSend, LPRET_DATA lpRetData, const void *lpUnpackerOrStr, int nResult)
{
}

void hfs_uft_adaptor::OnReceivedBiz(CConnectionInterface *lpConnection, int hSend, const void *lpUnPackerOrStr, int nResult)
{
}

void hfs_uft_adaptor::OnReceivedBizMsg(CConnectionInterface *lpConnection, int hSend, IBizMessage* lpMsg)
{
    int iFunctionID = lpMsg->GetFunction();
    int senderid = lpMsg->GetSenderId();
    char * lpKeyRevData = NULL;
    IF2UnPacker* lpUnPack = NULL;
    int iKeyRevLen = 0;
    cout << "funcid : " << iFunctionID << endl;
    switch (iFunctionID) {
        case FUNC_HEART_BEAT:
            lpMsg->ChangeReq2AnsMessage();
            lpConnection->SendBizMsg(lpMsg,1);	
            break;
        case FUNC_RTN_ORDER:{    
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();
                if (lpMsg->GetIssueType() == ISSUE_TYPE_TRADE) {
                    OnRtnTrade(lpUnPack);
                } else if (lpMsg->GetIssueType() == ISSUE_TYPE_ORDER) {
                    OnRtnOrder(lpUnPack);
                }
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_RSP_LOGIN:{
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();
                OnRspLogin(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_SUBSCRIBE:{
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();
                if (lpUnPack->GetInt("error_no") != 0){
                    LOG_INFO("sub failed: error_no:{}, error_msg:{}",lpUnPack->GetInt("error_no"), lpUnPack->GetStr("error_info"));             
                }
                else{    
                    if (lpMsg->GetIssueType() == ISSUE_TYPE_TRADE) {
                        LOG_INFO("sub trade success ");
                    } else if (lpMsg->GetIssueType() == ISSUE_TYPE_ORDER) {
                        LOG_INFO("sub order success");
                    }        
                    // ShowPacket_bl(lpUnPack);
                }
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_INSERT_ORDER:{
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                OnRspOrder(lpUnPack, senderid);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_QRY_ACCOUNT: {
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                OnRspAccount(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_QRY_POSITION: {
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                OnRspPosition(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_CANCEL_ORDER: {
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                // OnRspCancelOrder(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_QRY_ORDER :{
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                OnRspQryOrder(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        case FUNC_QRY_TRADE :{
            lpKeyRevData = (char *)lpMsg->GetContent(iKeyRevLen);
            lpUnPack = NewUnPacker(lpKeyRevData,iKeyRevLen);
            if (lpUnPack)
            {                
                lpUnPack->AddRef();                
                OnRspQryTrade(lpUnPack);
                lpUnPack->Release();
            }
            break;
        }
        default:
            break;
    }
}
void hfs_uft_adaptor::OnRspLogin(IF2UnPacker* lpUnPack) {
    cout << "login" << endl;
    if (lpUnPack->GetInt("error_no") != 0)
    {
        LOG_ERROR("[uft adaptor {}] OnRspLogin get error:{}", m_id, lpUnPack->GetStr("error_info"));
        cout << "login error:" << lpUnPack->GetStr("error_info") << endl;
        m_finished = true;
        m_state = -1;
        return;
    }
    if (lpUnPack->GetStr("branch_no") != NULL)
        branch_no = lpUnPack->GetStr("branch_no");
    if (lpUnPack->GetStr("client_id") != NULL)
        client_id = lpUnPack->GetStr("client_id");
    if (lpUnPack->GetStr("user_token") != NULL)
        token = lpUnPack->GetStr("user_token");  
    m_state = 2;
    LOG_INFO("[uft adaptor {}] branch_no:{}, client_id:{}, token:{}",m_id, branch_no, client_id, token);  
}
void hfs_uft_adaptor::OnRspOrder(IF2UnPacker* lpUnPack, int senderid) {
    // ShowPacket_bl(lpUnPack);
    LOG_INFO("[uft adaptor {}] OnRspOrder batch_no:{}, entrust_no:{}, error_no:{}, senderid:{}", m_id, lpUnPack->GetInt("batch_no"), lpUnPack->GetInt("entrust_no"), lpUnPack->GetInt("error_no"), senderid);
    int batch_no = lpUnPack->GetInt("batch_no");
    if (batch_no == 0) batch_no = senderid;
    EntrustNode* enode = getENode(batch_no);
    if (!enode) {
        LOG_INFO("[uft adaptor {}] cannot find enode:{}", m_id, batch_no);
        return;
    }
    hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	sprintf(enode->origOrder.orderid, "%d", lpUnPack->GetInt("entrust_no"));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;
    order.qty = enode->qty;	
    
    if (lpUnPack->GetInt("error_no") != 0){
        order.type = 'J';
		order.state = ORD_REJECTED;
		enode->origOrder.state = ORD_REJECTED;
		strncpy(order.algo, lpUnPack->GetStr("error_info"), sizeof(order.algo)-1);
		LOG_INFO("[uft adaptor {}] OnRspOrder get error:{}",m_id, lpUnPack->GetStr("error_info"));
    } else {
        order.type = 'A';
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
    }
    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[uft adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
}
void hfs_uft_adaptor::OnRtnOrder(IF2UnPacker* lpUnPack) {
    // 更新外挂订单信息， 更新资金
    LOG_INFO("[uft adaptor {}]OnRtnOrder:stock_code:{}, batch_no:{}, OrderStatus:{}, time:{}, type:{}, report_no:{}, entrust_no:{}", m_id,
             lpUnPack->GetStr("stock_code"), lpUnPack->GetStr("batch_no"), lpUnPack->GetChar("entrust_status"), lpUnPack->GetStr("report_time"), lpUnPack->GetChar("entrust_type"), lpUnPack->GetStr("report_no"),lpUnPack->GetStr("entrust_no"));
    char type = lpUnPack->GetChar("entrust_type");
    if (type == '2') { // 撤单委托回报， 不处理
        return ;
    }
    int batch_no = lpUnPack->GetInt("batch_no");
    char status = lpUnPack->GetChar("entrust_status");
    EntrustNode* enode = getENode(batch_no);
    if (!enode) {
        LOG_INFO("[uft adaptor {}] cannot find enode:{}", m_id, batch_no);
        return;
    }
    hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	sprintf(enode->origOrder.orderid, "%d", lpUnPack->GetInt("entrust_no"));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
	order.oseq = enode->nid.oseq;

    if (status == '2') {
        order.type = 'A';
        order.qty = enode->qty;	
		order.state = ORD_NEW;
		enode->origOrder.state = ORD_NEW;
    }
    else if (status == '9') {
        order.type = 'J';
        order.qty = enode->qty;	
		order.state = ORD_REJECTED;
        enode->origOrder.state = ORD_REJECTED;
        strncpy(order.algo, lpUnPack->GetStr("error_info"), sizeof(order.algo)-1);
		LOG_INFO("[uft adaptor {}] OnRtnOrder get error:{}",m_id, lpUnPack->GetStr("error_info"));
    }
    else if (status == '5' || status == '6') {
        enode->origOrder.state = ORD_CANCELED;
        order.state = ORD_CANCELED;
        order.type = 'C';
        order.prc = enode->origOrder.prc;
        // order.qty = qty;
    }
    else {
        LOG_ERROR("[uft adaptor {}] OnRtnOrder unknown status:{}", m_id, status);
        return;
    }

    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[uft adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);    
}
void hfs_uft_adaptor::OnRtnTrade(IF2UnPacker* lpUnPack) {
    LOG_INFO("[uft adaptor {}] OnRtnTrade batch_no:{}, symbol:{}, trade_qty:{}, trade_price:{}, type:{}, real_status:{}, entrust_status:{} ",
                m_id, lpUnPack->GetInt("batch_no"), lpUnPack->GetStr("stock_code"), 
                lpUnPack->GetInt("business_amount"), lpUnPack->GetDouble("business_price"), lpUnPack->GetChar("real_type"), lpUnPack->GetChar("real_status"), lpUnPack->GetChar("entrust_status"));

    int batch_no = lpUnPack->GetInt("batch_no");
    char type = lpUnPack->GetChar("real_type");
    char status = lpUnPack->GetChar("real_status");
    int qty = lpUnPack->GetInt("business_amount");
    double prc = lpUnPack->GetDouble("business_price");

    EntrustNode* enode = getENode(batch_no);
    if (!enode) {
        LOG_INFO("[uft adaptor {}] cannot find enode:{}", m_id, batch_no);
        return;
    }
    hfs_order_t order;
	memset(&order, 0, sizeof(hfs_order_t));
	strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
	sprintf(enode->origOrder.orderid, "%d", lpUnPack->GetInt("entrust_no"));
	order.idnum = enode->origOrder.idnum;
	order.side = enode->origOrder.side;
	order.ocflag = enode->origOrder.ocflag;
	order.pid = enode->nid.pid;
    order.oseq = enode->nid.oseq;

    if (type == '0' && status == '0') { // 成交
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
    } else if (type == '0' && status == '2') { // 废单
        enode->origOrder.state = ORD_REJECTED;
        order.state = ORD_REJECTED;
        order.type = 'J';
        order.prc = enode->origOrder.prc;
        order.qty = abs(qty);
        strncpy(order.algo, lpUnPack->GetStr("error_info"), sizeof(order.algo));
        LOG_ERROR("[uft adaptor {}] OnRtnTrade get error : {}", m_id, lpUnPack->GetStr("error_info"));
    } else if (type == '2' && status == '0') { // 撤单
        enode->origOrder.state = ORD_CANCELED;
        order.state = ORD_CANCELED;
        order.type = 'C';
        order.prc = enode->origOrder.prc;
        order.qty = abs(qty);
    } else {
        LOG_ERROR("[uft adaptor {}] OnRtnTrade unknown exec_type:{}", m_id, type);
        return;
    }
    if (!onResponse(enode->nid.teid, m_id, order)) {
        LOG_ERROR("[uft adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
	}
	m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);   
    //TODO holdings
    // if (lpUnPack->GetChar("real_type") == '0' && lpUnPack->GetChar("real_status") == '0') {  // 成交信息
    //     string symbol = lpUnPack->GetStr("stock_code");
    //     if (symbol.at(0) == '6') 
    //         symbol += ".SH";
    //     else 
    //         symbol += ".SZ";
    //     auto iter2 = positions.find(symbol);
    //     if (iter2 == positions.end()) {
    //         hfs_holdings_t *holding = new hfs_holdings_t();
    //         memset(holding, 0, sizeof(hfs_holdings_t));
    //         strcpy(holding->symbol, symbol.c_str());
    //         // strcpy(holding->exch, );
    //         // holding->cidx = cidx;
    //         positions.insert(make_pair(symbol, holding));
    //     }
    //     hfs_holdings_t *holding = positions[symbol];
    //     char bs = lpUnPack->GetChar("entrust_bs");
    //     int trade_qty = lpUnPack->GetInt("business_amount");
    //     double trade_prc = lpUnPack->GetDouble("business_price");

    //     if (bs == '1') { // buy
    //         holding->avgprc_long = (holding->avgprc_long * holding->qty_long + trade_qty * trade_prc) / (holding->qty_long + trade_qty + 0.00000001);
    //         holding->qty_long += trade_qty;
    //         accountinfo->available -= trade_qty * trade_prc;
    //     } else if (bs == '2') { // sell
    //         holding->avgprc_long = (holding->avgprc_long * holding->qty_long - trade_qty * trade_prc) / (holding->qty_long - trade_qty + 0.00000001);
    //         holding->qty_long -= trade_qty; 
    //         accountinfo->available += trade_qty * trade_prc;
    //     }
    //     dump(holding);
    // }
    // dump(od);
}

void hfs_uft_adaptor::qryAccount() {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("user_token");

    m_lpPacker->AddStr("0");
    m_lpPacker->AddStr("Z");
    m_lpPacker->AddStr(m_sysinfo.c_str());
    m_lpPacker->AddStr(branch_no.c_str());
    m_lpPacker->AddStr(client_id.c_str());
    m_lpPacker->AddStr(accountid.c_str());
    m_lpPacker->AddStr(passwd.c_str());
    m_lpPacker->AddStr(token.c_str());
    m_lpPacker->EndPack();
    m_finished = false;
    int ret = SendToServerAsy(FUNC_QRY_ACCOUNT, m_lpPacker);
    if (ret <= 0) {
        LOG_INFO ("[uft adaptor {}] qry account failed, {} ", m_id, ret );
        m_finished = true;
        return ;
    }
}
void hfs_uft_adaptor::OnRspAccount(IF2UnPacker* lpUnPack) {
    LOG_INFO("[uft adaptor {}] OnRspAccount current:{}, enable:{}, fetch_:{}", m_id,
            lpUnPack->GetDouble("current_balance"), lpUnPack->GetDouble("enable_balance"), lpUnPack->GetDouble("fetch_balance"));
    //Account,Available,MarketValue,AssetBalance
    m_account_file << accountid << "," << lpUnPack->GetDouble("enable_balance") << ",," << endl;
    m_finished = true;
}
void hfs_uft_adaptor::qryPosition(string &pos_str) {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("user_token");
    m_lpPacker->AddField("query_mode"); // 0 明细  1 汇总
    m_lpPacker->AddField("position_str");
    m_lpPacker->AddField("request_num");

    m_lpPacker->AddStr("0");
    m_lpPacker->AddStr("Z");
    m_lpPacker->AddStr(m_sysinfo.c_str());
    m_lpPacker->AddStr(branch_no.c_str());
    m_lpPacker->AddStr(client_id.c_str());
    m_lpPacker->AddStr(accountid.c_str());
    m_lpPacker->AddStr(passwd.c_str());
    m_lpPacker->AddStr(token.c_str());
    m_lpPacker->AddStr("1");
    m_lpPacker->AddStr(pos_str.c_str());
    m_lpPacker->AddStr("500");
    m_lpPacker->EndPack();

    int ret = SendToServerAsy(FUNC_QRY_POSITION, m_lpPacker);
    if (ret <= 0) {
        LOG_INFO ("[uft adaptor {}] qry position failed, {} ", m_id, ret );
        m_finished = true;
        return ;
    }
}
void hfs_uft_adaptor::OnRspPosition(IF2UnPacker* lpUnPack) {
    if (lpUnPack->GetInt("error_no") != 0)
	{
        LOG_ERROR("[uft adaptor {}] OnRspPosition get error:{}", m_id, lpUnPack->GetStr("error_info"));
        m_finished = true;
		return;
	}
	else{
        int ct = lpUnPack->GetRowCount();
		string pos = "";
		while (!lpUnPack->IsEOF())
		{
			const char* lpStrPos = lpUnPack->GetStr("position_str");
			if( lpStrPos == 0 )
				pos = "";
			else
				pos = lpStrPos;
            LOG_INFO("[uft adaptor {}] OnRspPosition account:{}, symbol:{}, name:{}, hold_qty:{}, curr_qty:{}, enable_qty:{}, cost_prc:{}, mktvalue:{}, pos_str:{}", m_id,
                     lpUnPack->GetStr("fund_account"), lpUnPack->GetStr("stock_code"), lpUnPack->GetStr("stock_name"), lpUnPack->GetInt("hold_amount"), lpUnPack->GetInt("current_amount"),
                     lpUnPack->GetInt("enable_amount"), lpUnPack->GetInt("hold_amount"), lpUnPack->GetDouble("cost_price"), lpUnPack->GetDouble("market_value"), lpUnPack->GetStr("position_str"));
            string symbol = lpUnPack->GetStr("stock_code");
            string exch = lpUnPack->GetStr("exchange_type");
            if (exch == "1") {
                symbol.append(".SH");
            } else if (exch == "2") {
                symbol.append(".SZ");
            } else {
                LOG_INFO("[uft adaptor] unknown exch:{}", exch);
                continue;
            }
            if (m_holdings.find(symbol) == m_holdings.end()) {
                hfs_holdings_t* holding = new hfs_holdings_t();
                memset(holding, 0, sizeof(hfs_holdings_t));
                strcpy(holding->symbol, symbol.c_str());
                m_holdings.insert(make_pair(symbol, holding));
            }
            hfs_holdings_t* holding = m_holdings[symbol];
            holding->qty_long = lpUnPack->GetInt("hold_amount");
            holding->qty_long_yd = lpUnPack->GetInt("enable_amount");
            holding->avgprc_long = lpUnPack->GetDouble("cost_price");
            // Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr
            m_position_file << lpUnPack->GetStr("fund_account") << "," << lpUnPack->GetStr("stock_code") << ","
                            << lpUnPack->GetInt("current_amount") << "," << lpUnPack->GetInt("enable_amount") << ","
                            << lpUnPack->GetDouble("cost_price") << "," << lpUnPack->GetDouble("market_value") << ","
                            << lpUnPack->GetStr("position_str") << endl;
            lpUnPack->Next();
		}

		if (pos.length() != 0)
		{
			qryPosition(pos);
		} else {
            cout << "qry position done " << endl;
            m_finished = true;
        }
    }
}
void hfs_uft_adaptor::qryOrder(string &pos_str) {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("user_token");
    m_lpPacker->AddField("position_str");
    m_lpPacker->AddField("request_num");

    m_lpPacker->AddStr("0");
    m_lpPacker->AddStr("Z");
    m_lpPacker->AddStr(m_sysinfo.c_str());
    m_lpPacker->AddStr(branch_no.c_str());
    m_lpPacker->AddStr(client_id.c_str());
    m_lpPacker->AddStr(accountid.c_str());
    m_lpPacker->AddStr(passwd.c_str());
    m_lpPacker->AddStr(token.c_str());
    m_lpPacker->AddStr(pos_str.c_str());
    m_lpPacker->AddStr("1000");
    m_lpPacker->EndPack();

    int ret = SendToServerAsy(FUNC_QRY_ORDER, m_lpPacker);
    if (ret <= 0) {
        LOG_INFO ("[uft adaptor {}] qry account failed, {} ", m_id, ret );
        m_finished = true;
        return ;
    }
}
void hfs_uft_adaptor::OnRspQryOrder(IF2UnPacker* lpUnPacker) {
    if (lpUnPacker->GetInt("error_no") != 0)
	{
        LOG_ERROR("[uft adaptor {}] OnRspQryOrder get error:{}", m_id, lpUnPacker->GetStr("error_info"));
        m_finished = true;
		return;
	}
	else{
        int ct = lpUnPacker->GetRowCount();
        m_orderRef += ct;
        string pos = "";
		while (!lpUnPacker->IsEOF())
		{
			const char* lpStrPos = lpUnPacker->GetStr("position_str");
			if( lpStrPos == 0 )
				pos = "";
			else
                pos = lpStrPos;
            LOG_INFO("[uft adaptor {}] OnRspQryOrder init_date:{}, account:{}, batch_no,:{}, entrust_no:{}, stock_code:{}, entrust_bs:{}, entrust_price:{}, entrust_amount:{}, business_price:{}, buiness_amount:{}, entrust_status:{}, withdraw_amount:{}, pos_str:{}", m_id,
            lpUnPacker->GetInt("init_date"), lpUnPacker->GetStr("fund_account"), lpUnPacker->GetInt("batch_no"), lpUnPacker->GetInt("entrust_no"), lpUnPacker->GetStr("stock_code"), lpUnPacker->GetChar("entrust_bs"), lpUnPacker->GetDouble("entrust_price"), lpUnPacker->GetInt("entrust_amount"), lpUnPacker->GetDouble("business_price"), lpUnPacker->GetInt("business_amount"), lpUnPacker->GetChar("entrust_status"), lpUnPacker->GetInt("withdraw_amount"), lpUnPacker->GetStr("position_str"));
            //Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
            m_entrust_file << lpUnPacker->GetStr("fund_account") << ',' << lpUnPacker->GetStr("stock_code") << ',' << lpUnPacker->GetInt("init_date") << ',' << lpUnPacker->GetInt("entrust_time") << ',' << lpUnPacker->GetInt("entrust_no") << ',' << lpUnPacker->GetInt("batch_no") << ',' << lpUnPacker->GetChar("entrust_bs") << ',' << lpUnPacker->GetChar("entrust_type") << lpUnPacker->GetChar("entrust_status") << ',' << lpUnPacker->GetDouble("entrust_price") << ',' << lpUnPacker->GetInt("entrust_amount") << ',' << lpUnPacker->GetDouble("business_price") << ',' << lpUnPacker->GetInt("business_amount") << ',' << lpUnPacker->GetInt("withdraw_amount") << ',' << lpUnPacker->GetStr("cancel_info") << ',' << lpUnPacker->GetStr("position_str") << ',' << endl;
            lpUnPacker->Next();
        }
        if (pos.length() != 0)
		{
			qryOrder(pos);
		} else {
            cout << "qry order done " << endl;
            m_entrust_file.close();
            m_finished = true;
        }
    }
}
void hfs_uft_adaptor::qryTrade(string &pos_str) {
    m_lpPacker->BeginPack();
    m_lpPacker->AddField("op_branch_no");
    m_lpPacker->AddField("op_entrust_way");
    m_lpPacker->AddField("op_station");
    m_lpPacker->AddField("branch_no");
    m_lpPacker->AddField("client_id");
    m_lpPacker->AddField("fund_account");
    m_lpPacker->AddField("password");
    m_lpPacker->AddField("user_token");
    m_lpPacker->AddField("position_str");
    m_lpPacker->AddField("request_num");

    m_lpPacker->AddStr("0");
    m_lpPacker->AddStr("Z");
    m_lpPacker->AddStr(m_sysinfo.c_str());
    m_lpPacker->AddStr(branch_no.c_str());
    m_lpPacker->AddStr(client_id.c_str());
    m_lpPacker->AddStr(accountid.c_str());
    m_lpPacker->AddStr(passwd.c_str());
    m_lpPacker->AddStr(token.c_str());
    m_lpPacker->AddStr(pos_str.c_str());
    m_lpPacker->AddStr("1000");
    m_lpPacker->EndPack();

    int ret = SendToServerAsy(FUNC_QRY_TRADE, m_lpPacker);
    if (ret <= 0) {
        LOG_INFO ("[uft adaptor {}] qry trade failed, {} ", m_id, ret );
        m_finished = true;
        return ;
    }
}
void hfs_uft_adaptor::OnRspQryTrade(IF2UnPacker* lpUnPack) {
    if (lpUnPack->GetInt("error_no") != 0)
	{
        LOG_ERROR("[uft adaptor {}] OnRspQryTrade get error:{}", m_id, lpUnPack->GetStr("error_info"));
        m_finished = true;
		return;
	}
	else{
        int ct = lpUnPack->GetRowCount();
		string pos = "";
		while (!lpUnPack->IsEOF())
		{
			const char* lpStrPos = lpUnPack->GetStr("position_str");
			if( lpStrPos == 0 )
				pos = "";
			else
                pos = lpStrPos;
            LOG_INFO("[uft adaptor {}] OnRspQryTrade date:{}, account:{}, stock_code:{}, entrust_no:{}, entrust_bs:{}, business_price:{}, business_amount:{}, real_type:{}, real_status:{}, business_time:{}, pos_str:{}", m_id,
                        lpUnPack->GetInt("date"), lpUnPack->GetStr("fund_account"), lpUnPack->GetStr("stock_code"), lpUnPack->GetStr("entrust_no"), lpUnPack->GetChar("entrust_bs"), lpUnPack->GetDouble("business_price"), lpUnPack->GetInt("business_amount"), lpUnPack->GetChar("real_type"), lpUnPack->GetChar("real_status"), lpUnPack->GetInt("business_time"), lpUnPack->GetStr("position_str"));
            // Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID
            m_detail_file << lpUnPack->GetStr("fund_account") << ',' << lpUnPack->GetStr("stock_code") << ',' << lpUnPack->GetInt("date") << ',' << lpUnPack->GetInt("business_time") << ',' << lpUnPack->GetStr("entrust_no") << ',' << lpUnPack->GetChar("entrust_bs") << ',' << lpUnPack->GetChar("real_type") << ',' << lpUnPack->GetDouble("business_price") << ',' << lpUnPack->GetInt("business_amount") << ',' << ',' << endl;
            lpUnPack->Next();
        }
        
        if (pos.length() != 0)
		{
			qryTrade(pos);
		} else {
            cout << "qry trade done " << endl;
            m_detail_file.close();
            m_finished = true;
        }
    }
}
//异步发送, 应答包是通过CCallback::OnReceivedBiz接收的
int hfs_uft_adaptor::SendToServerAsy(int iFunc, IF2Packer *lm_lpPacker, long reqID)
{
	int iRet = 0;
	
	if (m_lpMessage==NULL)
	{   
		m_lpMessage = NewBizMessage();
		m_lpMessage->AddRef();
	}
	
	m_lpMessage->SetPacketType(REQUEST_PACKET);
    m_lpMessage->SetFunction(iFunc);
    m_lpMessage->SetContent(lm_lpPacker->GetPackBuf(),lm_lpPacker->GetPackLen());
    m_lpMessage->SetSenderId(reqID);
    iRet = m_lpConnection->SendBizMsg(m_lpMessage, 1);
    if (iRet<=0)   //失败
    {
    	LOG_INFO("asy send failed : {}",m_lpConnection->GetErrorMsg(iRet));
    }

	return iRet;
}

//同步发送与接收
int hfs_uft_adaptor::SendToServerSyn(int iFunc, IF2Packer *lm_lpPacker)
{
	int iRet = 0;

	if (m_lpMessage==NULL)
	{   
		m_lpMessage = NewBizMessage();
		m_lpMessage->AddRef();
	}	

	m_lpMessage->SetPacketType(REQUEST_PACKET);
    m_lpMessage->SetFunction(iFunc);
    m_lpMessage->SetContent(lm_lpPacker->GetPackBuf(),lm_lpPacker->GetPackLen());	
	
    iRet = m_lpConnection->SendBizMsg(m_lpMessage, 0);
    if (iRet<=0)   //失败
    {
    	LOG_INFO("syn send failed : {}", m_lpConnection->GetErrorMsg(iRet));
    }
    else
    {
    	//注意：lpBizMessageRecv不能释放
    	IBizMessage* lpBizMessageRecv = NULL;
    	//iRet = g_lpConnection->RecvBiz(iRet, &Pointer);
    	iRet = m_lpConnection->RecvBizMsg(iRet, &lpBizMessageRecv, 5000);
    	if (iRet==0) //接收成功
    	{
    		if (lpBizMessageRecv->GetErrorNo() ==0) //业务操作成功
			{
				int iLen = 0;
				const void * lpBuffer = lpBizMessageRecv->GetContent(iLen);
				IF2UnPacker * lpUnPacker = NewUnPacker((void *)lpBuffer,iLen);
    
                ShowPacket_bl((IF2UnPacker *)lpUnPacker);
			}
			else  //业务操作有错误信息
			{ 
			    LOG_INFO("syn recv failed1 : {}", lpBizMessageRecv->GetErrorInfo());
			}
    	}
    	else //接收失败
    	{
    		LOG_INFO("syn recv failed2 : {}", m_lpConnection->GetErrorMsg(iRet));
    	}
    }
            
	return iRet;
}

//释放资源
void hfs_uft_adaptor::RealseAll()
{    
    if (m_lpConnection!=NULL)
    {
    	m_lpConnection->Close();
    	m_lpConnection->Release();
    }
    	
    if (m_lpConfig!=NULL)  
    {	  	
        m_lpConfig->Release();
    }
        
    if (m_lpPacker!=NULL)  
    {  	
        //释放打包器，不释放会引起内存泄露，后果严重
        m_lpPacker->FreeMem(m_lpPacker->GetPackBuf());
        m_lpPacker->Release();       
    }
    
    if (m_lpMessage!=NULL)
    {
    	m_lpMessage->Release();
    }
}
void ShowPacket_bl(IF2UnPacker *lpUnPacker)
{
    int i = 0, t = 0, j = 0, k = 0;

    for (i = 0; i < lpUnPacker->GetDatasetCount(); ++i)
    {
        //设置当前结果集
    	lpUnPacker->SetCurrentDatasetByIndex(i);

        //打印字段
        for (t = 0; t < lpUnPacker->GetColCount(); ++t)
        {
            printf("%20s", lpUnPacker->GetColName(t));
        }

        putchar('\n');

        //打印所有记录
        for (j = 0; j < (int)lpUnPacker->GetRowCount(); ++j)
        {
            //打印每条记录
            for (k = 0; k < lpUnPacker->GetColCount(); ++k)
            {
                switch (lpUnPacker->GetColType(k))
                {
                    case 'I':
                        printf("%20d", lpUnPacker->GetIntByIndex(k));
                        break;
                    
                    case 'C':
                        printf("%20c", lpUnPacker->GetCharByIndex(k));
                        break;
                    
                    case 'S':
                        printf("%20s", lpUnPacker->GetStrByIndex(k));
                        break;
                    
                    case 'F':
                        printf("%20f", lpUnPacker->GetDoubleByIndex(k));
                        break;
                    
                    case 'R':
                    {
                        int nLength = 0;
                        void *lpData = lpUnPacker->GetRawByIndex(k, &nLength);
                    
                        //对2进制数据进行处理
                        break;
                    }
                    
                    default:
                        //未知数据类型
                        printf("未知数据类型。\n");
                        break;
                }
            }

            cout << endl;

            lpUnPacker->Next();
        }
        
        putchar('\n');
    }
}

bool hfs_uft_adaptor::registerTrader(int teid, const hfs_order_t &order, int sid) {
	try {
		EntrustNode *node = new EntrustNode();
		memset(node, 0, sizeof(EntrustNode));
		node->nid.teid = teid;
		node->nid.pid  = order.pid;
		node->nid.oseq = order.oseq;
		node->sid = sid;
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
		LOG_INFO("[ht adaptor {}] register trader : {},{},{},{}",m_id, teid, order.pid, order.oseq, sid);
		return true;
	}
	catch(std::exception &ex) {
		LOG_ERROR("[ht adaptor {}] register trader {} failed : {}",m_id, sid, ex.what());
		return false;
	}
}

void hfs_uft_adaptor::push_reject_order(int teid, hfs_order_t &order, const char *errmsg) {
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

EntrustNode *hfs_uft_adaptor::getENode(OrderID& oid) {
	std::lock_guard<std::mutex> lk(m_nid_mtx);
	EntrustNode *rv = nullptr;
	if (nmap.find(oid) != nmap.end()) {
		return nmap[oid];
	}
	return rv;
}

EntrustNode *hfs_uft_adaptor::getENode(int sid) {
	std::lock_guard<std::mutex> lk(m_sid_mtx);
	EntrustNode *enode = nullptr;

	auto iter = smap.find(sid);
	if (iter != smap.end()) {
		enode = smap[sid];
	}
	return enode;
}

void hfs_uft_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("[ht adaptor {}]holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             m_id, holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_uft_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("[ht adaptor {}]order:symbol:{},exch:{},pid:{},orderid:{},tm:{},type:{},qty:{},prc:{},side:{},ocflag:{},m_state:{}",
             m_id, order->symbol, order->exch, order->pid, order->orderid, order->tm, (char)order->type, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

//加密
void hfs_uft_adaptor::encode(char *pstr, int *pkey){
    int len = strlen(pstr);//获坖长度
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_uft_adaptor::decode(char *pstr, int *pkey){
    int len = strlen(pstr);
    for (int i = 0; i < len; i++)
        *(pstr + i) = *(pstr + i) ^ i;
}

std::string hfs_uft_adaptor::getOPStation(int traderid) {
	// if (m_opstation.find(traderid) != m_opstation.end()) {
	// 	return m_opstation[traderid];
	// } 
	return m_sysinfo;
}