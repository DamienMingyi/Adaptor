#include "hfs_matic_adaptor.cpp"
#include "hfs_log.hpp"

using namespace std;

hfs_matic_adaptor::hfs_matic_adaptor(const a2ftool::XmlNode &_cfg) : 
    hfs_base_adaptor(_cfg)  
{
    m_id = cfg.getAttrDefault("id", "");
    m_account = cfg.getAttrDefault("account", "7040988");
	instruct_filepath = cfg.getAttrDefault("instructpath", "./");
	entrust_filepath = cfg.getAttrDefault("entrustfilepath", "./");
	details_filepath = cfg.getAttrDefault("detailsfilepath", "./");
    asset_prop = cfg.getAttrDefault("asset_prop", "7");
    
    m_order_ref = 0;
}

hfs_matic_adaptor::~hfs_matic_adaptor() {

}

bool hfs_matic_adaptor::start() {
    LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
    m_running = true;
    createThread();
    return true;
}
bool hfs_matic_adaptor::stop() {
    m_running = false;
    return true;
}
bool hfs_matic_adaptor::isLoggedOn() {
    return true;
}

bool hfs_matic_adaptor::onRequest(int teid, hfs_order_t &order) {
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

bool hfs_matic_adaptor::process_new_entrust(int teid, hfs_order_t& order) {
    assert(order.type == 'O');
    dump(&order);
    char client_id[32] = {0};
    sprintf(client_id, "%02d%03d_%d", teid, order.oseq, ++m_order_ref);
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
    if (-1 == m_ins_redis->pushData(m_ins_key.c_str(), ins)) {
        LOG_INFO("redis push failed : {}", m_ins_key);
        return -1;
    }
    if (!registerTrader(teid, order, client_id)) {
        LOG_INFO("registe trader : teid:{}, oseq:{}, pid:{}, client_id:{}", teid, order.oseq, order.pid, client_id);
    }
    //order.type = 'A';

    //hfs_base_adaptor::onResponse(teid, order.exch, order);
    return true;
}

bool hfs_matic_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
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
    sprintf(ins->client_id, "%02d%03d_%d", teid, enode->origOrder.oseq, enode->origOrder.pid);
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

void hfs_matic_adaptor::createThread(){
    if (asset_prop == "7") {  // 
        auto entrust_td = std::thread(std::bind(&hfs_matic_adaptor::monitor_entrust_rq, this));
        entrust_td.detach();
    } else {
        auto entrust_td = std::thread(std::bind(&hfs_matic_adaptor::monitor_entrust, this));
        entrust_td.detach();
    }
    auto rst_td = std::thread(std::bind(&hfs_matic_adaptor::monitor_result, this));
    rst_td.detach();

    auto detail_td = std::thread(std::bind(&hfs_matic_adaptor::monitor_detail, this));
    detail_td.detach();

}

void hfs_matic_adaptor::monitor_entrust(){
	ifstream fs;
	string entrustName;
	entrustName = entrust_filepath + to_string(today()) + ".csv";
    cout << entrustName << endl;
    while (true)
	{		
		fs.open(entrustName, ios::in);
		if (fs.is_open()) break;
		else {
			cout << "ord file not exists" << endl;
			fs.close();
			Sleep(1000);
		}
	}
	cout << "start monitor order file " << entrustName << endl;
    // load his
    string dataline;
    vector<string> result;
    //id,init_date,fund_account,batch_no,entrust_no,exchange_type,stock_account,stock_code,stock_name,entrust_bs,entrust_price,entrust_amount,business_amount,business_price,entrust_type,entrust_status,entrust_time,entrust_date,entrust_prop,cancel_info,withdraw_amount,position_str,report_no,report_time,orig_order_id,trade_plat,max_price_levels,trade_time_type,mac,client_version,tag,business_balance,instance_id
    while(getline(fs, dataline)) {        
        if (dataline.find("id") != std::string::npos) {
            continue;
        }
        result.clear();
		split(dataline, ",", result);
		if (result.size() < 33) {
			continue;
        }
        string client_id = result[30];
        if (client_id.empty()) continue;
        string ord_no = result[4];
        string business_amount = result[12];
        string business_price = result[13];
        string withdraw_amount = result[20];
        string entrust_status = result[16];

        EntrustNode* node = getENode(client_id);
        if (!node) {
            int teid = atoi(client_id.substr(0,2).c_str());
            int oseq = atoi(client_id.substr(2,3).c_str());
            int pid = atoi(client_id.substr(4).c_str());
            
            hfs_order_t od;
            memset(&od, 0, sizeof(od));
            int tmp = atoi(result[7].c_str());
            sprintf(od.symbol, "%06d", tmp);
            if (od.symbol[0] == '0' || od.symbol[0] == '3') {
                sprintf(od.symbol+6, "%s", ".SZ");
            } else {
                sprintf(od.symbol+6, "%s", ".SH");
            }
            strncpy(od.exch, m_id.c_str(), sizeof(od.exch)-1);
            strncpy(od.orderid, ord_no.c_str(), sizeof(od.orderid)-1);

            od.pid = pid;
            od.oseq = oseq;
            
            od.qty = atoi(result[11].c_str());
            od.prc = atof(result[10].c_str());
            od.type = 'A';
            od.side = result[9]=="1"?'B':'S';
            od.ocflag = 'O';
            od.state = ORD_NEW;
            registerTrader(teid, od, client_id);
            LOG_INFO("load order : clientid:{}, teid:{}, oseq:{}, pid:{}", client_id, teid, oseq, pid);
        }
        node = getENode(client_id);
        if (!node) continue;
        std::lock_guard<std::mutex> lk(m_muEvec);
        node->eqty = atoi(business_amount.c_str());
        node->canceled = atoi(withdraw_amount.c_str());
        node->eamt = atoi(business_amount.c_str()) * atof(business_price.c_str());

        if (entrust_status == "2") {
            node->origOrder.type = 'A';
            node->origOrder.state = ORD_NEW;
        }
        else if (entrust_status == "6") {
            if (node->origOrder.state == ORD_CANCELED) continue;
            node->origOrder.type = 'C';
            node->origOrder.state = ORD_CANCELED;
        }
        else if (entrust_status == "7")
        {
            node->origOrder.type = 'E';
            node->origOrder.state = ORD_NEW;    
        }
        else if (entrust_status == "8")
        {
            node->origOrder.type = 'E';
            node->origOrder.state = ORD_FILLED;  
        }
        else if (entrust_status == "5")
        {
            if (node->qty <= node->eqty + node->canceled)
            {
                if (node->origOrder.state == ORD_CANCELED) {                    
                    continue;
                }
                node->origOrder.state = ORD_CANCELED;
            }
        }
        else if (entrust_status == '9')
        {
            if (node->origOrder.state == ORD_REJECTED) {                    
                continue;
            }
            node->origOrder.state = ORD_REJECTED;
        }

    }
	while(m_running) {		
        
			//0:已报，1：部分成交，2：已经成交，3：撤单，4：撤单，5：拒单，6：拒单
			std::lock_guard <std::mutex> lck(m_ord_mapLock);
			if (strtag.length() > 0) //strcmp(strtag.c_str(), "") != 0
			{
				map<string, cats_order_t>::iterator iter;

				iter = r_ord_map.find(strtag);
				if (iter != r_ord_map.end())
				{
					strcpy(iter->second.avg_px, business_price.c_str());
					strcpy(iter->second.cxl_qty, withdraw_amount.c_str());

					if (strcmp(iter->second.ord_no, "0") == 0){}
						strcpy(iter->second.ord_no, entrust_no.c_str());

					if (entrust_status == '2' || entrust_status == '6' || entrust_status == '7' || entrust_status == '8')
					{
						if (entrust_status == '2') {
							strcpy(iter->second.err_msg, "");
							strcpy(iter->second.ord_status, "0");
						}
						else if (entrust_status == '6') {
							if (strcmp(iter->second.ord_status, "4") == 0) continue;
							strcpy(iter->second.err_msg, "");
							strcpy(iter->second.ord_status, "4");
							strcpy(iter->second.filled_qty, business_amount.c_str());
						}
						else if (entrust_status == '7')
						{
							strcpy(iter->second.ord_status, "1");
							strcpy(iter->second.err_msg, "");
							strcpy(iter->second.filled_qty, business_amount.c_str());

						}
						else if (entrust_status == '8')
						{
							strcpy(iter->second.ord_status, "2");
							strcpy(iter->second.err_msg, "");
							strcpy(iter->second.filled_qty, business_amount.c_str());
						}

						redis->pushData(m_order_keys, &iter->second);
						NLOG_INFO << "ENTRUST RECEIVE:id:" << iter->first << "," << iter->second.ord_no << ",status:" << iter->second.ord_status << "," << iter->second.symbol << "," << iter->second.ord_qty << "," << iter->second.tradeside;
						cout << iter->first << " : " << iter->second.ord_no<< " " << "status:" << iter->second.ord_status << endl;
					}
					else if (entrust_status == '5')
					{
						int entrust = stoi(entrust_amount);
						int business = stoi(business_amount);
						int withdraw = stoi(withdraw_amount);
						if (entrust == business + withdraw)
						{
							if (strcmp(iter->second.ord_status, "3") == 0) {
								strcpy(iter->second.filled_qty, business_amount.c_str());
								continue;
							}
							strcpy(iter->second.ord_status, "3");
							strcpy(iter->second.err_msg, "");
							strcpy(iter->second.filled_qty, business_amount.c_str());
							redis->pushData(m_order_keys, &iter->second);
							NLOG_INFO << "ENTRUST RECEIVE:" << iter->first << "," << iter->second.ord_no << ",status:" << iter->second.ord_status << "," << iter->second.symbol << "," << iter->second.ord_qty << "," << iter->second.tradeside;
							cout << iter->first << " : " << iter->second.ord_no << "  " << iter->second.ord_status << endl;
						}
					}
					else if (entrust_status == '9')
					{
						if (strcmp(iter->second.ord_status, "5") != 0)
						{
							strcpy(iter->second.ord_status, "5");
							strcpy(iter->second.err_msg, "");
							redis->pushData(m_order_keys, &iter->second);
							NLOG_INFO << "ENTRUST RECEIVE:" << iter->first << "," << iter->second.ord_no << ",status:" << iter->second.ord_status << "," << iter->second.symbol << "," << iter->second.ord_qty << "," << iter->second.tradeside ;
							cout << iter->first << " : " << iter->second.ord_no << " " << iter->second.ord_status << endl;
						}
					}
					NLOG_INFO << "T0ENTRUST RECEIVE:id:" << iter->first << "," << iter->second.ord_no << ",status:" << iter->second.ord_status << "," << iter->second.symbol << "," << iter->second.ord_qty << "," << iter->second.tradeside <<","<<iter->second.ord_time;
				}
			}
			//m_ord_mapLock.unlock();
		}
	}
}

void hfs_matic_adaptor::monitor_entrust_rq() {

}

void hfs_matic_adaptor::monitor_detail() {

}

void hfs_matic_adaptor::monitor_result() {
    
}

bool hfs_matic_adaptor::IsTradingOrder(EntrustNode* node) {
    return !(node->origOrder.state != ORD_NEW && node->origOrder.state != ORD_NONE && node->origOrder.state != ORD_PENDING_NEW && 
        node->origOrder.state != ORD_PENDING_CANCEL);
}

std::string hfs_matic_adaptor::trim(std::string &str) {
    if (str.empty()) {
        return str;
    }

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

void hfs_matic_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_matic_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}


EntrustNode *hfs_matic_adaptor::getENode(int teid, int pid, int oseq) {
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

EntrustNode *hfs_matic_adaptor::getENode(string sid) {
//   std::lock_guard<std::mutex> lk(muEvec);
	EntrustNode *enode = nullptr;
	auto iter = m_smap.find(sid);
	if (iter != m_smap.end()) {
		enode = m_smap[sid];
	}
	return enode;
}

bool hfs_matic_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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


std::vector<std::string> hfs_matic_adaptor::split(const  std::string& s, const std::string& delim, std::vector<std::string> &elems) {
    //std::vector<std::string> elems;
	size_t pos = 0;
	size_t len = s.length();
	size_t delim_len = delim.length();
	if (delim_len == 0) return elems;
	while (pos < len)
	{
		int find_pos = s.find(delim, pos);
		if (find_pos < 0)
		{
			elems.push_back(s.substr(pos, len - pos));
			break;
		}
		elems.push_back(s.substr(pos, find_pos - pos));
		pos = find_pos + delim_len;
	}
	return elems;
}