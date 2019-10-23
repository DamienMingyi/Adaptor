#pragma once 

#include "hfs_base_adaptor.hpp"
#include "hfs_log.hpp"
#include "hfs_utils.hpp"
#include <atomic>

struct OrderID {
  int teid;
  int pid;
  int oseq;
};

struct EntrustNode {
  OrderID nid;
  char    sid[512];
  char    tkr[512];
  int     qty;
  float   amt;
  int     eqty;
  float   eamt;
  int     canceled;              //0,表示没有撤单；>0,表示撤单量
  hfs_order_t origOrder;         //委托时的订单，后续填表有用
};

typedef map<string,  EntrustNode *>               Sid2EntrustMap;
typedef vector<EntrustNode *>                     EntrustInfoVec;

class hfs_matic_adaptor : public hfs_base_adaptor {
    hfs_matic_adaptor(const a2ftool::XmlNode &_cfg);
    virtual ~hfs_matic_adaptor();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn();

    virtual bool onRequest(int teid, hfs_order_t &order);

public:
    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void OnRtnOrder(const std::string &data);
    void OnRtnTrade(const std::string &data);

private:
    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    EntrustNode *getENode(int teid, int pid, int oseq);
    EntrustNode *getENode(string sid);

    void monitor_entrust();
    void monitor_entrust_rq();
    void monitor_detail();
    void monitor_result();

    inline std::string trim(std::string &str);
    inline std::string trimExch(std::string &str);
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);

    // 是否正在交易的报单
    bool IsTradingOrder(EntrustNode* node);

    std::vector<std::string> split(const  std::string& s, const std::string& delim, std::vector<std::string> &elems) ;
private:    
    std::mutex m_muEvec;
    EntrustInfoVec  m_evec;   // 原始委托信息
    Sid2EntrustMap  m_smap;

    std::string m_id;
    std::string m_account;
    string instruct_filepath;
	string entrust_filepath;
	string details_filepath;
    string asset_prop;
    
    atomic_int m_order_ref;
    bool m_running;

    std::mutex m_ordmap_lock;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_matic_adaptor(_cfg);
  }
};