#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include "redisMgr.hpp"
#include "HfsOrder.hpp"
#include "xml_parser.hpp"

struct RiskParam {
	int m_cancel_cnt;
	int m_filled_cnt;
	int m_refuse_cnt;
	double m_entrust_value;
	double m_filled_value;
	double m_buy_value;
	double m_sell_value;

	RiskParam() {
		m_cancel_cnt = 0;
		m_filled_cnt = 0;
		m_refuse_cnt = 0;
		m_entrust_value = 0;
		m_filled_value = 0;
		m_buy_value = 0;
		m_sell_value = 0;
	}
	std::string __str__() {
		std::ostringstream oss;  //创建一个格式化输出流
		oss << "cancel_cnt:" << m_cancel_cnt << ", filled_cnt:" << m_filled_cnt << ", refuse_cnt:" << m_refuse_cnt
			<< ", entrust_value:" << m_entrust_value << ", filled_value:" << m_filled_value << ", buy_value:" << m_buy_value << ", sell_value:" << m_sell_value;
		return oss.str();
	}
};
struct OrderID {
    int teid;
    int pid;
    int oseq;
    OrderID() {};
    OrderID(int teid_, int pid_, int oseq_) {
        teid = teid_; pid = pid_; oseq = oseq_;
    };
    OrderID(const OrderID& other) {
        this->teid = other.teid;
        this->pid = other.pid;
        this->oseq = other.oseq;
    };
};
struct OrderIDCmp {
  bool operator()(const OrderID &left, const OrderID &right) {
    return left.teid < right.teid
		       || (left.teid == right.teid && left.pid < right.pid)
      || (left.teid == right.teid && left.pid == right.pid && left.oseq < right.oseq);
  }
};
class RiskControl
{
public:
	RiskControl(const a2ftool::XmlNode &cfg);
	~RiskControl();

	bool OnInsertOrder(hfs_order_t* ins);
	void OnRtnOrder(int teid, const hfs_order_t& ord); 
	hfs_order_t* get_ord_by_sysID(std::string sysid);
	void OnTimer();
	bool IsCanCancel() {
		return m_can_cancel;
	};
private:
	std::map<std::string, hfs_order_t*> m_insMap; //  -instructs
	std::map<OrderID, hfs_order_t *, OrderIDCmp> m_ordMap; // cats_id - ord
	std::map<std::string, std::vector<hfs_order_t*>> m_tkrOrdMap; // tkr - ords
	int m_ins_cnt;  // 指令数量
	int m_last_ins_cnt; // 指令数量
	std::chrono::steady_clock::time_point m_last_point;  // 上次统计时间
	int m_period;
	bool  m_can_cancel;   // 是否可以撤单
	int m_max_speed;       // 报单速度限制
	int m_max_cancel_rate; // 撤单率
	std::string m_id;
};

