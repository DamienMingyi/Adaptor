#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include "redisMgr.hpp"
#include "HfsOrder.hpp"
#include "xml_parser.hpp"
using namespace std;

struct RiskParam {
  int m_cancel_cnt;
  int m_filled_cnt;
  int m_refuse_cnt;
  int m_new_cnt;
  double m_entrust_value;
  double m_filled_value;
  double m_buy_value;
  double m_sell_value;

  RiskParam() {
    m_cancel_cnt = 0;
    m_filled_cnt = 0;
    m_refuse_cnt = 0;
    m_new_cnt = 0;
    m_entrust_value = 0;
    m_filled_value = 0;
    m_buy_value = 0;
    m_sell_value = 0;
  }
  std::string __str__() {
    std::ostringstream oss;  //创建一个格式化输出流
    oss << "new_cnt:" << m_new_cnt << ",cancel_cnt:" << m_cancel_cnt
        << ", filled_cnt:" << m_filled_cnt << ", refuse_cnt:" << m_refuse_cnt
        << ", entrust_value:" << m_entrust_value << ", filled_value:"
        << m_filled_value << ", buy_value:" << m_buy_value << ", sell_value:"
        << m_sell_value;
    return oss.str();
  }
};
struct OrderID {
  int teid;
  int pid;
  int oseq;
  OrderID() {
  }
  OrderID(int teid_, int pid_, int oseq_) {
    teid = teid_;
    pid = pid_;
    oseq = oseq_;
  }
  OrderID(const OrderID &other) {
    this->teid = other.teid;
    this->pid = other.pid;
    this->oseq = other.oseq;
  }
};
struct OrderIDCmp {
  bool operator()(const OrderID &left, const OrderID &right) {
    return left.teid < right.teid
        || (left.teid == right.teid && left.pid < right.pid)
        || (left.teid == right.teid && left.pid == right.pid
            && left.oseq < right.oseq);
  }
};
class RiskControl {
 private:
  std::map<std::string, hfs_order_t*> m_insMap;  //  -instructs
  std::map<OrderID, hfs_order_t*, OrderIDCmp> m_ordMap;  // cats_id - ord
  std::map<std::string, std::vector<hfs_order_t*>> m_tkrOrdMap;  // tkr - ords
  int m_ins_cnt;  // 指令数量
  int m_last_ins_cnt;  // 指令数量
  std::chrono::steady_clock::time_point m_last_point;  // 上次统计时间
  int m_period;
  bool m_can_cancel = true;   // 是否可以撤单
  bool m_can_insert = true;    // 是否可报单
  int m_max_speed;       // 报单速度限制
  int m_max_cancel_rate;  // 撤单率
  int m_max_cnt;
  std::string m_id;

 public:
  std::map<std::string, hfs_holdings_t *> m_holdings;
  hfs_account_t m_accountinfo;

  RiskControl(const a2ftool::XmlNode &cfg)
      :
      m_last_ins_cnt(0),
      m_ins_cnt(0) {
    m_can_cancel = true;
    m_last_point = chrono::steady_clock::now();
    m_period = chrono::steady_clock::period::den
        / chrono::steady_clock::period::num;

    m_max_speed = cfg.getAttrDefault("max_speed", 300);
    m_max_cancel_rate = cfg.getAttrDefault("cancel_rate", 30);
    m_id = cfg.getAttrDefault("id", "");
    m_max_cnt = cfg.getAttrDefault("max_order_cnt", 500000);

    auto td = std::thread([this]() {
      while (true) {
        this->OnTimer();
        sleep(60);
      }
    });
    td.detach();
  }
  ~RiskControl() {
  }

  bool OnInsertOrder(hfs_order_t *ins) {
    // 统计报单速率
    m_ins_cnt++;
    LOG_INFO("[RiskControl {}] OnInsertOrder ins_cnt:{}, last_ins_cnt:{}", m_id,
             m_ins_cnt, m_last_ins_cnt);
    if (m_ins_cnt - m_last_ins_cnt > m_max_speed) {
      double use_tme = std::chrono::duration_cast<chrono::duration<double>>(
          chrono::steady_clock::now() - m_last_point).count();
      LOG_INFO("[RiskControl {}] {} use tme:{}s", m_id, m_max_speed, use_tme);
      ;
      if ((m_ins_cnt - m_last_ins_cnt) / use_tme > m_max_speed) {
        LOG_INFO("[RiskControl {}] insert order speed exceed {} > {}", m_id,
                 (m_ins_cnt - m_last_ins_cnt) / use_tme, m_max_speed);
        cout << "insert order speed exceed" << endl;
        m_ins_cnt--;
        m_last_ins_cnt = m_ins_cnt;
        m_last_point = chrono::steady_clock::now();
        return false;
      }
      m_last_ins_cnt = m_ins_cnt;
      m_last_point = chrono::steady_clock::now();
    }
    // 判断自成交
    if (ins->type == 'X')
      return true;
    string tkr = ins->symbol;
    string sysid = ins->orderid;

    auto iter = m_tkrOrdMap.find(tkr);
    if (iter != m_tkrOrdMap.end()) {
      for (auto ord : iter->second) {
        if (ord->state != ORD_NONE && ord->state != ORD_PENDING_NEW
            && ord->state != ORD_NEW && ord->state != ORD_PENDING_CANCEL)
          continue;
        if (ord->side != ins->side) {  // B buy  S sell
          if (ord->side == 'B' && ord->prc >= ins->prc) {
            LOG_INFO(
                "[RiskControl {}] self deal warning exists_symbol:{}, exits_ord_prc:{}, ins_symbol:{}, ins_prc:{}",
                m_id, ord->symbol, ord->prc, tkr, ins->prc);
            m_ins_cnt--;
            return false;
            //break;
          } else if (ord->side == 'S' && ord->prc <= ins->prc) {
            LOG_INFO(
                "[RiskControl {}] self deal warning exists_symbol:{}, exists_ord_prc:{}, ins_symbol:{}, ins_prc:{}",
                m_id, ord->symbol, ord->prc, tkr, ins->prc);
            m_ins_cnt--;
            return false;
          }
        }
      }
    }
    return true;
  }
  void OnRtnOrder(int teid, const hfs_order_t &ord) {
    string tkr = ord.symbol;
    OrderID oid = OrderID(teid, ord.pid, ord.oseq);
    if (m_ordMap.find(oid) != m_ordMap.end()) {
      hfs_order_t *od_t = m_ordMap[oid];
      if (od_t->state != ORD_NONE && od_t->state != ORD_PENDING_NEW
          && od_t->state != ORD_NEW && od_t->state != ORD_PENDING_CANCEL)
        return;
      memcpy(od_t, &ord, sizeof(hfs_order_t));
    } else {
      hfs_order_t *od_t = new hfs_order_t();
      memcpy(od_t, &ord, sizeof(hfs_order_t));
      m_ordMap[oid] = od_t;
      if (m_tkrOrdMap.find(tkr) != m_tkrOrdMap.end()) {
        m_tkrOrdMap[tkr].push_back(od_t);
      } else {
        vector<hfs_order_t*> vec;
        vec.push_back(od_t);
        m_tkrOrdMap[tkr] = vec;
      }
    }
    LOG_INFO("update ord symbol:{}, orderid:{}, status:{}", tkr, ord.orderid,
             (int) ord.state);
  }
  hfs_order_t* get_ord_by_sysID(std::string sysid) {
    return nullptr;
  }
  void OnTimer() {
    // 统计撤单比等
    RiskParam param;
    auto iter = m_ordMap.begin();
    for (; iter != m_ordMap.end(); iter++) {
      hfs_order_t *ord = iter->second;
      param.m_entrust_value += (ord->prc * ord->qty);
      if (ord->state == ORD_FILLED) {  // 全部成交
        param.m_filled_cnt++;
        param.m_filled_value += (ord->prc * (ord->qty));
        if (ord->side == 'B') {  // 买入
          param.m_buy_value += ((ord->prc) * (ord->qty));
        } else {
          param.m_sell_value += ((ord->prc) * (ord->qty));
        }
      } else if (ord->state == ORD_CANCELED) {  // 撤单
        param.m_filled_value += ((ord->prc) * (ord->qty));
        param.m_cancel_cnt++;
        if (ord->side == 'B') {  // 买入
          param.m_buy_value += ((ord->prc) * (ord->qty));
        } else {
          param.m_sell_value += ((ord->prc) * (ord->qty));
        }
      } else if (ord->state == ORD_REJECTED) {
        param.m_refuse_cnt++;
      } else {
        param.m_new_cnt++;
      }
    }
    LOG_INFO("[RiskControl {}] {}", m_id, param.__str__());
    int total_cnt = param.m_cancel_cnt + param.m_filled_cnt + param.m_refuse_cnt
        + param.m_new_cnt;
    if (total_cnt > m_max_cnt) {
      LOG_INFO("[RiskControl {}] order_cnt:{} > max_cnt:{}", m_id, total_cnt,
               m_max_cnt);
      m_can_insert = false;
    }
    if (total_cnt > 100) {  // 当委托笔数大于或等100时
      float cancel_rate = param.m_cancel_cnt / (total_cnt + 0.000001);
      float refuse_rate = param.m_refuse_cnt / (total_cnt + 0.000001);
      float fill_rate = param.m_filled_cnt / (total_cnt + 0.000001);
      LOG_INFO("[RiskControl {}] cancel_rate:{}, refuse_rate:{}, fill_rate:{}",
               m_id, cancel_rate, refuse_rate, fill_rate);
      // cout << "cancel_rate:" << cancel_rate
      // 	<< ", refuse_rate:" << refuse_rate
      // 	<< ", fill_rate:" << fill_rate << endl;
      if (cancel_rate * 100 > m_max_cancel_rate) {  // 撤单率超过
        LOG_INFO("[RiskControl {}] cancel_rate:{} > {}", m_id,
                 cancel_rate * 100, m_max_cancel_rate);
        m_can_cancel = false;
      } else if (!m_can_cancel) {
        m_can_cancel = true;
      }
    }
  }

  bool VerifyPosition(hfs_order_t *order) {
    if (order->side == 'S') {
      auto holding = m_holdings.find(order->symbol);
      if (holding == m_holdings.end()) {
        order->type = 'J';
        order->state = ORD_REJECTED;
        LOG_INFO("[RiskControl {}] no holdings for {}", m_id, order->symbol);
        return false;
      } else if (holding->second->qty_long_yd < order->qty) {
        order->type = 'J';
        order->state = ORD_REJECTED;
        LOG_INFO(
            "[RiskControl {}] insufficient holdings for {}, position quantity:{}, order quantity:{}",
            m_id, order->symbol, holding->second->qty_long_yd, order->qty);
        return false;
      }
    }
    return true;
  }

  bool VerifyAsset(hfs_order_t *order) {
    if (order->side == 'B') {
      double orderPrice = order->qty * order->prc;
      if (m_accountinfo.available <= orderPrice) {
        order->type = 'J';
        order->state = ORD_REJECTED;
        LOG_INFO(
            "[RiskControl {}] insufficient assets, available asset:{}, order price:{}",
            m_id, m_accountinfo.available, orderPrice);
        return false;
      }
    }
    return true;
  }

  void OnShiftHoldings(const hfs_order_t &order) {
    if (order.side == 'S') {
      auto holding = m_holdings.find(order.symbol);
      holding->second->qty_long_yd -= order.qty;
      holding->second->qty_long -= order.qty;
      if (holding->second->qty_long == 0)
        m_holdings.erase(holding);
    } else if (order.side == 'B') {
      auto holding = m_holdings.find(order.symbol);
      if (holding != m_holdings.end()) {
        holding->second->qty_long += order.qty;
      } else {
        hfs_holdings_t *holding = new hfs_holdings_t();
        memset(holding, 0, sizeof(hfs_holdings_t));
        strncpy(holding->symbol, order.symbol, 6);
        strncpy(holding->exch, order.symbol + 7, 2);
        holding->cidx = -1;
        holding->qty_long += order.qty;
        holding->avgprc_long = order.prc;
        m_holdings.insert(make_pair(order.symbol, holding));
      }
    }
  }

  bool IsCanCancel() {
    return m_can_cancel;
  }
  bool IsCanInsert() {
    return m_can_insert;
  }
};

