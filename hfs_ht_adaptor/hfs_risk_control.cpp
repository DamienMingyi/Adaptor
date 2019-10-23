
#include "hfs_risk_control.hpp"

using namespace std;
RiskControl::RiskControl(const a2ftool::XmlNode &cfg) : m_last_ins_cnt(0), m_ins_cnt(0)
{
	m_can_cancel = true;
	m_last_point = chrono::steady_clock::now();
	m_period = chrono::steady_clock::period::den / chrono::steady_clock::period::num;
	auto td = std::thread([this]() {while (true) { this->OnTimer(); sleep(60); } });
	td.detach();
	m_max_speed = cfg.getAttrDefault("max_speed", 300);
	m_max_cancel_rate = cfg.getAttrDefault("cancel_rate", 30);
	m_id = cfg.getAttrDefault("id", "");
	m_max_cnt = cfg.getAttrDefault("max_order_cnt", 500000);
}


RiskControl::~RiskControl()
{
}

bool RiskControl::OnInsertOrder(hfs_order_t* ins) {
	// 统计报单速率
	m_ins_cnt++;
	LOG_INFO("[RiskControl {}] OnInsertOrder ins_cnt:{}, last_ins_cnt:{}" ,m_id, m_ins_cnt, m_last_ins_cnt);
	if (m_ins_cnt - m_last_ins_cnt > m_max_speed) {
		double use_tme = std::chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - m_last_point).count();
		LOG_INFO("[RiskControl {}] {} use tme:{}s",m_id, m_max_speed, use_tme);;
		if ((m_ins_cnt - m_last_ins_cnt) / use_tme > m_max_speed) {
			LOG_INFO("[RiskControl {}] insert order speed exceed", m_id);
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
	if (ins->type == 'X') return true;
	string tkr = ins->symbol;
	string sysid = ins->orderid;

	auto iter = m_tkrOrdMap.find(tkr);
	if (iter != m_tkrOrdMap.end()) {
		for (auto ord : iter->second) {
			if (ord->state != ORD_NONE && ord->state == ORD_PENDING_NEW && ord->state == ORD_NEW && ord->state == ORD_PENDING_CANCEL) 
				continue;
			if (ord->side != ins->side) { // B buy  S sell
				if (ord->side == 'B' && ord->prc >= ins->prc) {
					LOG_INFO("[RiskControl {}] self deal warning exists_symbol:{}, exits_ord_prc:{}, ins_symbol:{}, ins_prc:{}", 
							m_id, ord->symbol, ord->prc, tkr, ins->prc);
					m_ins_cnt--;
					return false;
					//break;
				}
				else if (ord->side == 'S' && ord->prc <= ins->prc) {
					LOG_INFO("[RiskControl {}] self deal warning exists_symbol:{}, exists_ord_prc:{}, ins_symbol:{}, ins_prc:{}", 
							m_id, ord->symbol, ord->prc, tkr, ins->prc);
					m_ins_cnt--;
					return false;
				}
			}
		}
	}
	return true;
	// 	
}

void RiskControl::OnRtnOrder(int teid, const hfs_order_t& ord) {
	string tkr = ord.symbol;
	OrderID oid = OrderID(teid, ord.pid, ord.oseq);
	if (m_ordMap.find(oid) != m_ordMap.end()) {
		hfs_order_t* od_t = m_ordMap[oid];
		// LOG_INFO("update ord symbol:{}, old_tkr:{}", tkr, od_t->symbol);
		memcpy(od_t, &ord, sizeof(hfs_order_t));
	}
	else {
		hfs_order_t* od_t = new hfs_order_t();
		memcpy(od_t, &ord, sizeof(hfs_order_t));
		m_ordMap[oid] = od_t;
		if (m_tkrOrdMap.find(tkr) != m_tkrOrdMap.end()) {
			m_tkrOrdMap[tkr].push_back(od_t);
		}
		else {
			vector<hfs_order_t*> vec;
			vec.push_back(od_t);
			m_tkrOrdMap[tkr] = vec;
		}
	}

}

void RiskControl::OnTimer() {
	// 统计撤单比等
	RiskParam param;
	auto iter = m_ordMap.begin();
	for (; iter != m_ordMap.end(); iter++) {
		hfs_order_t* ord = iter->second;
		param.m_entrust_value += (ord->prc*ord->qty);
		if (ord->state == ORD_FILLED) { // 全部成交
			param.m_filled_cnt++;
			param.m_filled_value += (ord->prc*(ord->qty));
			if (ord->side == 'B') { // 买入
				param.m_buy_value += ((ord->prc)*(ord->qty));
			}
			else {
				param.m_sell_value += ((ord->prc)*(ord->qty));
			}
		}
		else if (ord->state == ORD_CANCELED) { // 撤单
			param.m_filled_value += ((ord->prc)*(ord->qty));
			param.m_cancel_cnt++;
			if (ord->side == 'B') { // 买入
				param.m_buy_value += ((ord->prc)*(ord->qty));
			}
			else {
				param.m_sell_value += ((ord->prc)*(ord->qty));
			}
		}
		else if (ord->state == ORD_REJECTED) {
			param.m_refuse_cnt++;
		}
	}
	LOG_INFO("[RiskControl {}] {}",m_id, param.__str__());
	int total_cnt = param.m_cancel_cnt + param.m_filled_cnt + param.m_refuse_cnt;
	if (total_cnt > m_max_cnt) {
		LOG_INFO("[RiskControl {}] order_cnt:{} > max_cnt:{}",m_id, total_cnt, m_max_cnt);
		m_can_insert = false;
	}
	//if (total_cnt > 100) { // 当委托笔数大于或等100时
		float cancel_rate = param.m_cancel_cnt / (total_cnt + 0.000001);
		float refuse_rate = param.m_refuse_cnt / (total_cnt + 0.000001);
		float fill_rate = param.m_filled_cnt / (total_cnt + 0.000001);
		LOG_INFO("[RiskControl {}] cancel_rate:{}, refuse_rate:{}, fill_rate:{}",m_id, cancel_rate, refuse_rate, fill_rate);
		// cout << "cancel_rate:" << cancel_rate
		// 	<< ", refuse_rate:" << refuse_rate
		// 	<< ", fill_rate:" << fill_rate << endl;
		if (cancel_rate * 100 > m_max_cancel_rate) { // 撤单率超过
			LOG_INFO("[RiskControl {}] cancel_rate:{} > {}",m_id, cancel_rate, m_max_cancel_rate);
			m_can_cancel = false;
		}
	//}
}
