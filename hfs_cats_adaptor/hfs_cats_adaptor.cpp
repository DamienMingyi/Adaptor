#include "hfs_cats_adaptor.hpp"

using namespace std;
hfs_cats_adaptor::hfs_cats_adaptor(const a2ftool::XmlNode &_cfg) : 
    hfs_base_adaptor(_cfg), m_bPosInited(false)    
{
    m_order_ref = 0;
    m_id = _cfg.getAttrDefault("id", "");
    m_ins_key = _cfg.getAttrDefault("ins_key", "");
    m_ord_key = _cfg.getAttrDefault("ord_key", "");
    string redisIP = cfg.getAttrDefault("redisIp", "192.168.1.16");
    int redisPort = cfg.getAttrDefault("redisPort", 6379);
    m_ins_redis = new redisMgr(redisIP.c_str(), redisPort);
    m_ord_redis = new redisMgr(redisIP.c_str(), redisPort);
    m_cancel_redis = new redisMgr(redisIP.c_str(), redisPort);
    m_ins_redis->delKey(m_ins_key.c_str());
    m_ord_redis->delKey(m_ord_key.c_str());

}

hfs_cats_adaptor::~hfs_cats_adaptor() {

}

bool hfs_cats_adaptor::start() {
    LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
    createThread();
    return true;
}
bool hfs_cats_adaptor::stop() {
    return true;
}
bool hfs_cats_adaptor::isLoggedOn() {
    return true;
}

bool hfs_cats_adaptor::onRequest(int teid, hfs_order_t &order) {
    LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
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
        LOG_INFO ("unexpected order type:{} ", order.type);
        return false;
    };

    // fprintf(stderr, "try saveOrder\n");
    // try {
    //     if(!adap.saveOrder(teid, &order)) throw "saveOrder error!";
    // }
    // catch(const char *estr) {
    //     FLOG << "onRequest: " << estr;
    // }
    return true;
}

bool hfs_cats_adaptor::process_new_entrust(int teid, hfs_order_t& order) {
    assert(order.type == 'O');
    dump(&order);
    char client_id[32] = {0};
    sprintf(client_id, "%02d%03d_%d", teid, order.oseq, order.pid);
    cats_instructs_t *ins = new cats_instructs_t();
    memset(ins, 0, sizeof(cats_instructs_t));
    ins->inst_type = 'O';
    sprintf(ins->client_id, "%s",client_id);
    // strcpy(ins->acct_type, "");
    // strcpy(ins->acct_id, "");
    strcpy(ins->symbol, order.symbol);
    if (order.side == HFS_ORDER_DIRECTION_LONG)
        strcpy(ins->tradeside, "1");
    else
        strcpy(ins->tradeside, "2");
    ins->ord_qty = order.qty;
    ins->ord_price = order.prc;
    ins->ord_type[0] = '0';
    // strcpy(ins->ord_param, tradeType.c_str());
    if (!registerTrader(teid, order, client_id)) {
        LOG_INFO("registe trader : teid:{}, oseq:{}, pid:{}, client_id:{}", teid, order.oseq, order.pid, client_id);
    }
    
    if (-1 == m_ins_redis->pushData(m_ins_key.c_str(), ins)) {
        LOG_INFO("redis push failed : {}", m_ins_key);
        return -1;
    }
    
    //order.type = 'A';

    //hfs_base_adaptor::onResponse(teid, order.exch, order);
    return true;
}

bool hfs_cats_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
    assert(order.type == 'X');
    dump(&order);
    EntrustNode *enode = getENode(teid, order.pid, order.oseq);
    if (!enode) {
        LOG_ERROR("cancel order not exists teid= {} oseq={} pid:{}", teid, order.oseq, order.pid);
		return false;
    }
    
    cats_instructs_t *ins = new cats_instructs_t();
    memset(ins, 0, sizeof(cats_instructs_t));
    ins->inst_type = 'C';
    sprintf(ins->client_id, "%02d%03d_%d", teid, order.oseq, ++m_order_ref);
    strcpy(ins->acct_type, "");
    strcpy(ins->acct_id, "");
    strcpy(ins->order_no, enode->origOrder.orderid);
    strcpy(ins->symbol, order.symbol);
    // strcpy(ins->ord_param, tradeType.c_str());
    if (-1 == m_cancel_redis->pushData(m_ins_key.c_str(), ins)) {
        LOG_ERROR("redis push redis failed : {}", m_ins_key);
        return false;
    }
    return true;
}

void hfs_cats_adaptor::createThread(){
    pthread_t thread_id;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    int max_priority = sched_get_priority_max(SCHED_FIFO);
    struct sched_param sched_value;
    sched_value.sched_priority = max_priority;
    pthread_attr_setschedparam(&thread_attr, &sched_value);
    int ret = pthread_create(&thread_id, &thread_attr, monitor, this);
    pthread_attr_destroy(&thread_attr);
    if (ret != 0)
        LOG_INFO ("create qry thread failed ");
}

void * hfs_cats_adaptor::monitor(void *pInstance) {
    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(27, &mask);

    // if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0){
    //   perror("pthread_setaffinity_np");
    // } 

    hfs_cats_adaptor *cats = (hfs_cats_adaptor *)pInstance;
    while(1) {
        string data = cats->popData();
        if (!data.empty()) 
            LOG_INFO("{}", data) ;
        if (data.find("order") != std::string::npos) {
            cats->OnRtnOrder(data); // update order
        } else if (data.find("asset") != std::string::npos) {
            cats->OnAsset(data);  // update asset, position
        }
    }
    return 0;
}

string hfs_cats_adaptor::popData() {
    return m_ord_redis->popData(m_ord_key.c_str());
}

void hfs_cats_adaptor::OnRtnOrder(const std::string &data) {
    // LOG_INFO("OnRtnOrder:{}",data.c_str());
    // FUNC_CALL
    string err_msg = getValue(data, "err_msg");
    string ord_no = getValue(data, "ord_no");
    string client_id = getValue(data, "client_id").c_str();
    std::lock_guard<std::mutex> lk(m_muEvec);
    EntrustNode* node = getENode(client_id);
    if (node == nullptr) {
        LOG_ERROR("cannot find enode : {}", client_id);
        return;
    }
    int ord_status = atoi(getValue(data, "ord_status").c_str());

    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strncpy(order.symbol, node->origOrder.symbol, sizeof(order.symbol));
    strncpy(order.exch, node->origOrder.exch, sizeof(order.exch));
    order.side = node->origOrder.side;
    order.ocflag = node->origOrder.ocflag;
    order.pid = node->nid.pid;
    order.oseq = node->nid.oseq;
    strncpy(order.orderid, ord_no.c_str(), sizeof(order.orderid));
    strncpy(node->origOrder.orderid, ord_no.c_str(), sizeof(order.orderid));
    if (!IsTradingOrder(node)) {
        LOG_INFO("finished order , get ret :{}", client_id);
    }
    if(ord_status == 0) {
        order.state = ORD_NEW;
        node->origOrder.state = ORD_NEW;
        order.type = 'A';
        order.qty = atoi(getValue(data, "ord_qty").c_str());
    } else if (ord_status == 1) {
        order.state = ORD_NEW;
        node->origOrder.state = ORD_NEW;
        order.type = 'E';
        int filled_qty = atoi(getValue(data, "filled_qty").c_str());
        order.qty = filled_qty - node->eqty;
        order.prc = atof(getValue(data, "avg_px").c_str());
        node->eqty = filled_qty;  // 增量成交
    } else if (ord_status == 2) {
        node->origOrder.state = ORD_FILLED;
        order.state = ORD_FILLED;
        order.type = 'E';
        int filled_qty = atoi(getValue(data, "filled_qty").c_str());
        order.qty = filled_qty - node->eqty;
        order.prc = atof(getValue(data, "avg_px").c_str());
        node->eqty = filled_qty;  // 增量成交
    } else if(ord_status == 3 || ord_status == 4){
        int filled_qty = atoi(getValue(data, "filled_qty").c_str());
        int cxl_qty = atoi(getValue(data, "cxl_qty").c_str());
        if (filled_qty > node->eqty) {            
            order.state = ORD_NEW;
            node->origOrder.state = ORD_NEW;
            order.type = 'E';
            order.qty = filled_qty - node->eqty;
            order.prc = atof(getValue(data, "avg_px").c_str());
            node->eqty = filled_qty;  // 增量成交
            onResponse(node->nid.teid, m_id, order);
        }
        if (node->eqty + cxl_qty < node->origOrder.qty) {
            order.state = ORD_NEW;
            node->origOrder.state = ORD_NEW;
            order.type = 'E';
            order.qty = node->origOrder.qty - node->eqty - cxl_qty;
            order.prc = node->origOrder.prc;
            node->eqty = node->origOrder.qty - cxl_qty; 
            onResponse(node->nid.teid, m_id, order);
        }
        node->origOrder.state = ORD_CANCELED;
        order.state = ORD_CANCELED;
        order.type = 'C';
        order.prc = node->origOrder.prc;
        order.qty = atoi(getValue(data, "cxl_qty").c_str());
    } else if(ord_status == 5 || ord_status == 6) {
        order.state = ORD_REJECTED;
        order.type = 'J';
        order.qty = node->origOrder.qty;
        order.prc = node->origOrder.prc;
        strncpy(order.algo, getValue(data, "err_msg").c_str(), sizeof(order.algo)-1);
    }
    if (!onResponse(node->nid.teid, m_id, order)) {
        LOG_ERROR("onResponse failed, teid:{}, acct:{}, pid:{}, osep:{}", node->nid.teid, node->origOrder.exch, node->nid.pid, node->nid.oseq);
    }
}

void hfs_cats_adaptor::OnAsset(const std::string &data) {
    // LOG_INFO( "OnAsset:{}", data);
    char type = getValue(data, "a_type")[0];
    if (type == 'F') {
        accountinfo->available = atof(getValue(data, "s4").c_str());
    } else if (type == 'P') {
        LOG_INFO("get position:{}", data);
        string symbol = getValue(data, "s1");
        auto iter = positions.find(symbol);
        if (!m_bPosInited) {        
            if (iter == positions.end()) {
                hfs_holdings_t *holding = new hfs_holdings_t();
                memset(holding, 0, sizeof(hfs_holdings_t));
                strcpy(holding->symbol, symbol.c_str());
                // strcpy(holding->exch, pTrade->ExchangeID);
                // holding->cidx = cidx;
                positions.insert(make_pair(symbol, holding));
            }
            hfs_holdings_t *holding = positions[symbol];
            holding->qty_long = atoi(getValue(data, "s2").c_str());
            holding->qty_long_yd = atoi(getValue(data, "s3").c_str());
            holding->avgprc_long = atof(getValue(data, "s4").c_str());
        } else { // 防止持仓初始化失败
            if (iter == positions.end()) {
                LOG_INFO("position inited, but tkr:{} donot find", symbol);
                hfs_holdings_t *holding = new hfs_holdings_t();
                memset(holding, 0, sizeof(hfs_holdings_t));
                strcpy(holding->symbol, symbol.c_str());
                // strcpy(holding->exch, pTrade->ExchangeID);
                // holding->cidx = cidx;
                positions.insert(make_pair(symbol, holding));
                holding->qty_long = atoi(getValue(data, "s2").c_str());
                holding->qty_long_yd = atoi(getValue(data, "s3").c_str());
                holding->avgprc_long = atof(getValue(data, "s4").c_str());
            } else {
                positions[symbol]->qty_long_yd = atoi(getValue(data, "s3").c_str());                
            }
        }
        //dump(positions[symbol]);
    } else if (type == 'E') {// end of one time
        m_bPosInited = true;
    }
}

bool hfs_cats_adaptor::IsTradingOrder(EntrustNode* node) {
    return !(node->origOrder.state != ORD_NEW && node->origOrder.state != ORD_NONE && node->origOrder.state != ORD_PENDING_NEW && 
        node->origOrder.state != ORD_PENDING_CANCEL);
}
std::string hfs_cats_adaptor::getValue(const std::string &data, const std::string &key) {
    try{
        unsigned int p = data.find(key);
        if (p != std::string::npos) {
            string s = data.substr(p + key.size() + 1, data.find(',', p) - p - 1 - key.size());
            return trim(s);
        }
    }
    catch (exception& e) {
        LOG_ERROR("get value : {} failed", key);
    }
    return "";
}

std::string hfs_cats_adaptor::trim(std::string &str) {
    if (str.empty()) {
        return str;
    }

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

void hfs_cats_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_cats_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}


EntrustNode *hfs_cats_adaptor::getENode(int teid, int pid, int oseq) {
	std::lock_guard<std::mutex> lk(m_muEvec);
	EntrustNode *rv = nullptr;
	// EntrustInfoVec::iterator itr = evec.begin();
	for(auto node : m_evec) {
		if(node->nid.teid == teid && node->nid.pid == pid && node->nid.oseq == oseq) {
            rv = node;
            break;
		}
	}
	return rv;
}

EntrustNode *hfs_cats_adaptor::getENode(string sid) {
//   std::lock_guard<std::mutex> lk(muEvec);
	EntrustNode *enode = nullptr;
	auto iter = m_smap.find(sid);
	if (iter != m_smap.end()) {
		enode = m_smap[sid];
	}
	return enode;
}

bool hfs_cats_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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
		std::lock_guard<std::mutex> lk(m_muEvec);
		this->m_evec.push_back(node);
		this->m_smap.insert(make_pair(sid, node));
		LOG_INFO("register trader sid : {}", sid);
		return true;
	}
	catch(std::exception &ex) {
		LOG_ERROR("注册trader失败！原因：{}", ex.what());
		return false;
	}
}
