#include "HfsCommunicator.hpp"
#include "HfsTEClient.hpp"
#include "HfsAdmin.hpp"
#include "hfs_log.hpp"
#include "hfs_utils.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <algorithm>
#include <cassert>

//#include <mysql_connection.h>
//#include <cppconn/driver.h>
//#include <cppconn/exception.h>
//#include <cppconn/resultset.h>
//#include <cppconn/statement.h>
//#include <cppconn/prepared_statement.h>

using namespace std;
using boost::asio::ip::tcp;
using namespace boost::posix_time;

boost::mutex mutexClients; //实现对TESet的并发访问控制
boost::mutex mutexAdmin;   //处理admin指令时，应该暂停整个交易

const int MAX_REQU_COUNT = 1024 * 1024; //100w  最大请求数量 
const int MAX_RESP_COUNT = 1024 * 1024 * 10; //1000w  最大回应数量

const char eot = 0x04;

const int cf_max_open_cnt = 8; 

//sql::Driver *driver;
//sql::Connection *con;
//sql::Statement *stmt;
//sql::ResultSet *res;
//sql::PreparedStatement *pstmt;

Communicator::Communicator(const a2ftool::XmlNode &_cfg)
	:cfg(_cfg), te_acceptor_(NULL), admin_acceptor_(NULL), started(false), running(false) {
		// adaptor = AdaptorPtr(new Adaptor(*this, HfsParams::get_mutable_instance().AdaptorType)); //选择接口类型

		CommListenPort = _cfg.getAttrDefault("CommListenPort", 0);
		CommExchPort = _cfg.getAttrDefault("CommExchPort", 0);
		CommAdminPort = _cfg.getAttrDefault("CommAdminPort", 0);
		store_root = _cfg.getAttrDefault("StoreRoot", "./");
		memset(requStore, 0, sizeof(requStore));
		memset(respStore, 0, sizeof(respStore));

		for(int i=0;i<MaxTECount;i++) {
			mtseq[i] = 1; //tseq从1开始计数
		}
		adMgr = new hfs_adaptor_mgr(_cfg);

		IC_OPEN_CNT = 0; // 中证500 开仓数量
		IF_OPEN_CNT = 0; // 沪深300
		IH_OPEN_CNT = 0; // 上证50
		// 委托数量
		IC_ENTRUST_CNT = 0; 
		IF_ENTRUST_CNT = 0;
		IH_ENTRUST_CNT = 0;
}

Communicator::~Communicator() { }

bool Communicator::startAdaptor() {
	a2ftool::XmlNode node = cfg.getChild("router");
	string path = node.getAttrDefault("path", "");
	if (path.empty()) {
		cerr << "router path is empty " << endl;
		return false;
	}
	void * pHandle = dlopen(path.c_str(), RTLD_LAZY);
    if(!pHandle) {
      	printf("dlopen - %s\n", dlerror());
      	throwException(string("load ") + path + " failed!");
	}
	hfs_base_router * (* create)(Communicator &, hfs_adaptor_mgr&, const a2ftool::XmlNode &) = (hfs_base_router * (*)(Communicator &, hfs_adaptor_mgr&, const a2ftool::XmlNode &))dlsym(pHandle, "create");
	router = create(*this, *adMgr, node);

	if(!adMgr->start()) {
		LOG_ERROR("启动adaptor失败！");
		started = false;
		return false;
	}

	started = true;
	return true;
}

bool Communicator::stopAdaptor() {
	started = false;  

	return true;
}

void Communicator::run() {
	if(!sqlInit()) {
		LOG_ERROR("初始化数据库失败！");
		return;
	}

	//线程1，监听ExchConnector的连接，并负责维护
	startAdaptor();

	//线程2，监听trade engine的消息，接受te的登录
	qes_te_server_start();

	//线程3，监听外部的控制命令
	qes_admin_server_start();

	io_service_.run();

	cerr << "io_service_.run finished!" << endl;

	stopAdaptor(); 
}

int Communicator::qes_te_server_start() {
	tcp::endpoint te_edp(tcp::v4(), CommListenPort); 
	te_acceptor_    = new tcp::acceptor(io_service_, te_edp);
	if(!te_acceptor_) {
		LOG_ERROR("创建te_acceptor失败");
		exit(-1);
	}
	try {
		TEClientPtr new_session(new TEClient(io_service_, *this, *router));
		te_acceptor_->async_accept(new_session->getSocket(), boost::bind(&Communicator::handle_te_accept, this, 
			new_session, boost::asio::placeholders::error)); // 异步等待接受链接

		cerr << "TE连接监听已启动||地址:" << te_acceptor_->local_endpoint() << endl;
	}
	catch(...) {
		LOG_ERROR ("TEClient::qes_te_server_start error!");
		return -1;
	}
	return 0;
}

void Communicator::handle_te_accept(TEClientPtr session, const boost::system::error_code &error) {
	if(error) {
		LOG_ERROR("handle_te_accept error! {}", error.message());
		return;
	}
	try{
		cerr << "handle_te_accept: " << session->getSocket().remote_endpoint() << endl;

		session->start();
		// 继续等待下一个连接
		TEClientPtr new_session(new TEClient(io_service_, *this, *router));
		te_acceptor_->async_accept(new_session->getSocket(), boost::bind(&Communicator::handle_te_accept, this,
			new_session, boost::asio::placeholders::error));
	}
	catch(...) {
		LOG_ERROR ("TEClient::qes_te_server_start error!");
	}
}

int Communicator::qes_admin_server_start() {
	tcp::endpoint admin_edp(tcp::v4(), CommAdminPort);
	admin_acceptor_ = new tcp::acceptor(io_service_, admin_edp);

	AdminSessionPtr new_session(new AdminSession(io_service_, *this));
	admin_acceptor_->async_accept(new_session->getSocket(), boost::bind(&Communicator::handle_admin_accept, this,
		new_session, boost::asio::placeholders::error));

	cerr << "Admin连接监听已启动||地址:" << admin_acceptor_->local_endpoint() << endl;

	return 0;
}

void Communicator::handle_admin_accept(AdminSessionPtr session, const boost::system::error_code &error) {
	if(error) {
		LOG_ERROR("handle_admin_accept error! {} ", error.message());
		return;
	}
	cerr << "handle_admin_accept: " << session->getSocket().remote_endpoint() << endl;

	session->start();

	AdminSessionPtr new_session(new AdminSession(io_service_, *this));
	admin_acceptor_->async_accept(new_session->getSocket(), boost::bind(&Communicator::handle_admin_accept, this,
		new_session, boost::asio::placeholders::error));
}

bool Communicator::registerTEClient(TEClientPtr session) {
	LOG_INFO("{}",__FUNCTION__);
	boost::mutex::scoped_lock lk(mutexClients);  

	int teid = session.get()->getTEID();
	try {
		if(requStore[teid] == NULL) {
			char fname[512];
			sprintf(fname, "%s/request_%d_%d.bin", store_root.c_str(), today(), teid);
			requStore[teid] = new RequStore(MAX_REQU_COUNT, fname); 
		}
		if(respStore[teid] == NULL) {
			char fname[512];
			sprintf(fname, "%s/response_%d_%d.bin", store_root.c_str(), today(), teid);
			respStore[teid] = new RespStore(MAX_RESP_COUNT, fname);
		}
		if(!requStore[teid] || !respStore[teid]) {
			LOG_ERROR ("创建store失败! teid:{}", teid);
			exit(-1);
		}
	}
	catch(...) {
		cerr << "registerTEClient exception!" << endl;
		return false;
	}

	teClients.insert(session.get());

	return true;
}

bool Communicator::unregisterTEClient(TEClientPtr session) {
	LOG_INFO("{}",__FUNCTION__);
	boost::mutex::scoped_lock lk(mutexClients);
	TEClientSet::iterator itr = teClients.find(session.get());
	if(itr != teClients.end()) {
		teClients.erase(itr);
	}
	return true;
}

bool Communicator::resendTradeHistory(int teid) {
	if(teid > MaxTECount) return false;

	TEClient *tecli = getTEClient(teid);
	if(!tecli) return false;

	RespStore *store = respStore[teid];
	if(!store) return false;

	boost::mutex::scoped_lock lk(store->getMutex());
	for(int i=0;i<store->getCount();i++) {
		hfs_order_t *xpt = store->getData() + i;
		mtseq[teid] = xpt->tseq; //更新tseq，使tseq保持到最新状态
		switch(xpt->type) {
		case 'E':
		case 'U':
			tecli->forwardExecution(xpt);
			break;
		default:
			break;
		}
	}

	return true;
}

TEClient* Communicator::getTEClient(int teid) const {
	//LOG_INFO("{}",__FUNCTION__);
	boost::mutex::scoped_lock lk(mutexClients);
	TEClientSet::iterator itr = teClients.begin();
	for(;itr!=teClients.end();++itr) {
		if((*itr)->getTEID() == teid) return *itr;
	}
	return NULL;
}

void Communicator::getTraderRecordList(TraderRecordList &vec) {
	boost::mutex::scoped_lock lk(mutexAdmin);

	vec.clear();

	for(int teid=0;teid<MaxTECount;teid++) {
		RequStore *store = requStore[teid];
		if(!store) continue;

		for(int i=0;i<store->getCount();i++) {
			hfs_order_t *req = store->getData() + i;
			if(req->type != HFS_ORDER_TYPE_ORDER_ENTER) continue;
			// 找到对应的 teid
			int k = -1;
			for(int j=0;j<(int)vec.size();j++) {
				if(vec[j].teid == teid && vec[j].traderid == (int)req->pid) {
					k = j;
					break;
				}
			}
			if(k >= 0) {
				vec[k].qtyEntrusted+=  req->qty;
				vec[k].cntEntrusted+=  1;
			}
			else {
				TraderRecord record;
				memset(&record, 0, sizeof(TraderRecord));
				record.teid     = teid;
				record.traderid = req->pid;
				strncpy(record.symbol, req->symbol, sizeof(record.symbol));
				record.side     = req->side;
				vec.push_back(record);
			}
		}
	}

	//respStore
	for(int teid=0;teid<MaxTECount;teid++) {
		RespStore *store = respStore[teid];
		if(!store) continue;

		for(int i=0;i<store->getCount();i++) {
			hfs_order_t *xpt = store->getData() + i;
			if(xpt->type != HFS_ORDER_TYPE_EXECUTION || xpt->type != HFS_ORDER_TYPE_CANCEL_ACK) continue;

			int k = -1;
			for(int j=0;j<(int)vec.size();j++) {
				if(vec[j].teid == teid && vec[j].traderid == (int)xpt->pid) {
					k = j;
					break;
				}
			}
			if(k >= 0) { //反馈一定是针对请求的
				vec[k].qtyReported+=   xpt->qty;
				vec[k].cntReported+=   1;
				vec[k].lastUpdTime =   xpt->tm;
			}
		}
	}

	sort(vec.begin(), vec.end(), TraderRecordCmp());
}
// 处理tec请求
bool Communicator::onRequest(int teid, hfs_order_t& order) {
  //股指日内交易，不用c++接口
	// bool valid = true;
	// if(strcmp(order.symbol, "IC") >= 0 && order.ocflag == 'O' && IC_OPEN_CNT + IC_ENTRUST_CNT + order.qty >= cf_max_open_cnt) {
	// 	valid = false;
	// }
	// else if(strcmp(order.symbol, "IF") >= 0 && order.ocflag == 'O' && IF_OPEN_CNT + IF_ENTRUST_CNT + order.qty >= cf_max_open_cnt) {
	// 	valid = false;
	// }
	// if(strcmp(order.symbol, "IH") >= 0 && order.ocflag == 'O' && IH_OPEN_CNT + IH_ENTRUST_CNT + order.qty >= cf_max_open_cnt) {
	// 	valid = false;
	// }
	// if(!valid) {
	// 	FLOG << "dangerous IC IF IH:" << IC_OPEN_CNT + IC_ENTRUST_CNT << ' ' << IF_OPEN_CNT + IF_ENTRUST_CNT << ' ' << IH_OPEN_CNT + IH_ENTRUST_CNT;
	// 	throw "dangerous!";
	// }
	// ILOG << "COMM IC IF IH:" << IC_OPEN_CNT << ' ' << IF_OPEN_CNT << ' ' << IH_OPEN_CNT;

	// if(strcmp(order.symbol, "IC") >= 0 && order.ocflag == 'O') {
	// 	IC_ENTRUST_CNT+= order.qty;
	// }
	// else if(strcmp(order.symbol, "IF") >= 0 && order.ocflag == 'O') {
	// 	IF_ENTRUST_CNT+= order.qty;
	// }
	// if(strcmp(order.symbol, "IH") >= 0 && order.ocflag == 'O') {
	// 	IH_ENTRUST_CNT+= order.qty;
	// }
	// LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
	return router->onRequest(teid, order);
}
// 处理交易API响应
bool Communicator::onResponse(int teid, std::string execid, hfs_order_t& order) {
	try {
		TEClient *teclient = getTEClient(teid);
		if(!teclient) throw "Can't find TEClient!";

		for(;!teclient->isInited();) {
			//boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            sleep(1);
		}

		{
			boost::mutex::scoped_lock resplk(mutexResp[teid]);
			order.tseq = getTSeq(teid);
		}

		if(strcmp(order.symbol, "IC") >= 0 && order.ocflag == 'O' && order.type == 'E') {
			IC_OPEN_CNT+=order.qty; IC_ENTRUST_CNT-= order.qty;
		}
		else if(strcmp(order.symbol, "IF") >= 0 && order.ocflag == 'O' && order.type == 'E') {
			IF_OPEN_CNT+=order.qty; IF_ENTRUST_CNT-= order.qty;
		}
		if(strcmp(order.symbol, "IH") >= 0 && order.ocflag == 'O' && order.type == 'E') {
			IH_OPEN_CNT+=order.qty; IH_ENTRUST_CNT-= order.qty;
		}
		if(!teclient->forwardExecution(&order)) throw "forward Execution error!";

		if(!saveReport(teid, &order)) throw "saveReport error!";
	}
	catch(const char *estr) {
		LOG_ERROR("onResponse error! {}", estr);
		return false;
	}

	return true;
}


bool Communicator::sqlInit() {

	// try {
	// 	driver = get_driver_instance();
	// 	if(!driver) {
	// 		FLOG << "获取sql driver失败！";
	// 		return false;
	// 	}
	// 	con = driver->connect(HfsParams::get_mutable_instance().SQLAddress, HfsParams::get_mutable_instance().SQLUser, HfsParams::get_mutable_instance().SQLPassword); 
	// 	con->setSchema("trading"); //选择数据库

	// 	stmt = con->createStatement();
	// } catch (sql::SQLException &e) {
	// 	FLOG << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() <<	" )" << endl;
	// 	return false;
	// }

	return true;
}

bool Communicator::sqlRelease() {
	//if(stmt) {
	//	delete stmt;
	//	stmt = NULL;
	//}
	//if(con) {
	//	con->close();
	//	delete con;
	//	con = NULL;
	//}
	return true;
}

bool Communicator::saveReport(int teid, hfs_order_t *xpt) {
	boost::mutex::scoped_lock adminlk(mutexAdmin);
	assert(xpt);

	if(1>teid || MaxTECount <= teid) { //teid不应该为0,0表示异常值
		LOG_ERROR ("不合法的teid = {}", teid);
		return false;
	}

	// store must have been created for this teid                                                                                   
	if(0==respStore[teid]) {
		LOG_ERROR("注册teid={} 前必须注册相应的store!" ,teid);      
		return false;
	}        

	boost::mutex::scoped_lock resplk(mutexResp[teid]);
	hfs_order_t *ord = respStore[teid]->append(xpt);
	if(!ord) {
		LOG_ERROR ("Failed to append respStore!");
		return false;
	}

	//向mysql添加交易记录
	/*try {
	char sql[1024];
	if(xpt->side=='\0')
	xpt->side=' ';
	if(xpt->ocflag=='\0')
	xpt->ocflag=' ';

	time_t lt = time(NULL);
	struct tm*ptr = localtime(&lt);
	char now[128];
	strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", ptr);


	sprintf(sql, 
	"INSERT INTO childorderresult(dte, teid, pid, tkr, algo, exchangename, operatetype, "
	"side, ocflag, qty, price, tkrid, oseq, tseq, tme, ask, asksiz, bid, bidsiz, updatetime, inserttime) "
	"VALUES (%d, %d, %d, \'%s\', \'%s\', \'%s\', \'%c\', \'%c\', \'%c\', %d, %f, %d, %d, %d, %d, %f, %d, %f, %d, \'%s\', \'%s\')", 
	today(), teid, xpt->pid, xpt->symbol, xpt->algo, xpt->exch, xpt->type, xpt->side, xpt->ocflag,
	xpt->qty, xpt->prc, xpt->idnum, xpt->oseq, xpt->tseq, xpt->tm, 
	xpt->ask, xpt->asksiz, xpt->bid, xpt->bidsiz, now, now);
	ILOG << "\nsql: " << sql << endl;
	stmt->execute(sql);
	}
	catch(sql::SQLException &e) {
	FLOG << "插入失败！ " << e.getErrorCode() << ' ' << e.getSQLState() << ' ' << e.what() << '\n';
	return false;
	}
	catch(std::exception &ex) {
	FLOG << "插入失败！" << ex.what();
	return false;
	}*/

	return(true);
}

bool Communicator::saveOrder(int teid, const hfs_order_t *order) {
	boost::mutex::scoped_lock adminlk(mutexAdmin);
	assert(order);

	if(1>teid || MaxTECount <= teid) { //teid不应该为0,0表示异常值
		LOG_ERROR("不合法的teid = {}", teid);
		return false;
	}

	// store must have been created for this teid                                                                                   
	if(0==requStore[teid]) {
		LOG_ERROR("注册teid={} 前必须注册相应的store!", teid );       
		return false;
	}        

	boost::mutex::scoped_lock requlk(mutexRequ[teid]);
	hfs_order_t *ord = requStore[teid]->append(order);
	if(!ord) {
		LOG_ERROR ("Failed to append requStore! teid:{}", teid);
		return false;
	}

	//向mysql添加交易记录
	//try {
	//	time_t lt = time(NULL);
	//	struct tm*ptr = localtime(&lt);
	//	char now[128];
	//	strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", ptr);

	//	char sql[1024];
	//	sprintf(sql, 
	//		"INSERT INTO childorderentrust(dte, teid, pid, tkr, algo, exchangename, operatetype, "
	//		"side, ocflag, qty, price, tkrid, oseq, tseq, tme, ask, asksiz, bid, bidsiz, updatetime, inserttime) "
	//		"VALUES (%d, %d, %d, \'%s\', \'%s\', \'%s\', \'%c\', \'%c\', \'%c\', %d, %f, %d, %d, %d, %d, %f, %d, %f, %d, \'%s\', \'%s\')", 
	//		today(), teid, order->pid, order->symbol, order->algo, order->exch, order->type, order->side, order->ocflag,
	//		order->qty, order->prc, order->idnum, order->oseq, order->tseq, order->tm,
	//		order->ask, order->asksiz, order->bid, order->bidsiz, now, now);
	//	ILOG << "\nsql: " << sql << endl;
	//	stmt->execute(sql);
	//}
	//catch(sql::SQLException &e) {
	//	FLOG << "插入失败！ " << e.getErrorCode() << ' ' << e.getSQLState() << ' ' << e.what() << '\n';
	//	return false;
	//}
	//catch(std::exception &ex) {
	//	FLOG << "插入失败！" << ex.what();
	//	return false;
	//}

	return(true);
}
