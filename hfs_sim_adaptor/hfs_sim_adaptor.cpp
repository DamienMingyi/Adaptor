#include "hfs_sim_adaptor.hpp"
#include "hfs_log.hpp"
#include <thread>

hfs_sim_adaptor::hfs_sim_adaptor(const a2ftool::XmlNode &_cfg)
  :hfs_base_adaptor(_cfg) {
    iRequestID = 0;
}

hfs_sim_adaptor::~hfs_sim_adaptor() {

}

bool hfs_sim_adaptor::start(){
    return true;
}

bool hfs_sim_adaptor::stop() {
    return true;
}

bool hfs_sim_adaptor::isLoggedOn() {
    return true;
}

bool hfs_sim_adaptor::onRequest(int teid, hfs_order_t &order) {
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


bool hfs_sim_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
    assert(order.type == 'O');
    
    dump(&order);
    char buf[16] = {0};
    sprintf(buf, "%d_%d_%d", teid, order.pid, getNextRef());
    string sid(buf);
    if(!registerTrader(teid, order, sid)) {
        LOG_ERROR("注册trader失败！{} {} {} {}", teid, order.pid, order.oseq, sid);
    }
    // 模拟撮合 1-100 全成 100-~ 部成
    if (order.qty < 100) {
        hfs_order_t od;
        // memset(&order, 0, sizeof(hfs_order_t));
        memcpy(&od, &order, sizeof(hfs_order_t));
        od.type = 'E';
        od.qty  = order.qty;
        od.prc  = order.prc;

        if(!onResponse(teid, m_id, od)) {
            LOG_ERROR("转发消息失败！");
            return true;
        }
    } else {
        hfs_order_t od;
        // memset(&order, 0, sizeof(hfs_order_t));
        memcpy(&od, &order, sizeof(hfs_order_t));
        od.type = 'E';
        od.qty  = order.qty / 2;
        od.prc  = order.prc;

        if(!onResponse(teid, m_id, od)) {
            LOG_ERROR("转发消息失败！");
            return true;
        }
    }
    return true;
}

bool hfs_sim_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
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
    hfs_order_t od;
    memset(&od, 0, sizeof(hfs_order_t));
    strncpy(od.symbol, enode->origOrder.symbol, sizeof(od.symbol));
    od.idnum = enode->origOrder.idnum;
    od.side = enode->origOrder.side;
    od.ocflag = enode->origOrder.ocflag;
    od.pid = enode->nid.pid;
    od.oseq = enode->nid.oseq;
    od.type = 'C';
    od.state = ORD_CANCELED;
    if(!onResponse(teid, m_id, od)) {
        LOG_ERROR("转发消息失败！");
        return true;
    }
    return true;
}

bool hfs_sim_adaptor::registerTrader(int teid, const hfs_order_t &order, string sid) {
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

EntrustNode *hfs_sim_adaptor::getENode(int teid, int pid, int oseq) {
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

EntrustNode *hfs_sim_adaptor::getENode(string sid) {
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

bool hfs_sim_adaptor::readConfig() {
    return true;
}

double hfs_sim_adaptor::myfloor(double price, double tick) {
  return int(price / tick + 0.5) * tick;
}

void hfs_sim_adaptor::sleep(int ms) {
  this_thread::sleep_for(chrono::milliseconds(ms));
}


void hfs_sim_adaptor::dump(hfs_holdings_t *holding){
    if (!holding)
        return;
    LOG_INFO("holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
             holding->symbol, holding->cidx, holding->exch, holding->qty_long, holding->qty_short, holding->qty_long_yd ,holding->qty_short_yd, holding->avgprc_long, holding->avgprc_short);
}

void hfs_sim_adaptor::dump(hfs_order_t *order) {
    if (!order)
        return;

    LOG_INFO("order:symbol:{},exch:{},pid:{},orderid:{},tm:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
             order->symbol, order->exch, order->pid, order->orderid, order->tm, order->qty, order->prc,
             (char)order->side, (char)order->ocflag, (char)order->state);
}

// void hfs_sim_adaptor::printHolding(CThostFtdcInvestorPositionField *pos) {
//   LOG_INFO("[tradingtool] OnRspQryInvestorPosition:instrumentID:{},PosiDirection:{},PositionDate:{},YdPosition:{},Position:{}",
//              pos->InstrumentID, (char)pos->PosiDirection,
//              pos->PositionDate, pos->YdPosition, pos->Position);
// }
