#pragma once 

#include "hfs_base_adaptor.hpp"
#include "redisMgr.hpp"
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

class hfs_cats_adaptor : public hfs_base_adaptor {
public:
    hfs_cats_adaptor(const a2ftool::XmlNode &_cfg);
    virtual ~hfs_cats_adaptor();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn();

    virtual bool onRequest(int teid, hfs_order_t &order);
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t& order);

public:
    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void OnRtnOrder(const std::string &data);
    void OnRtnTrade(const std::string &data);
    void OnAsset(const std::string &data);
    std::string popData();
    std::string getValue(const std::string &data, const std::string &key);

private:
    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    EntrustNode *getENode(int teid, int pid, int oseq);
    EntrustNode *getENode(string sid);

    static void *monitor(void *pInstance);
    void createThread();
    inline std::string trim(std::string &str);
    inline std::string trimExch(std::string &str);
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);

    // 是否正在交易的报单
    bool IsTradingOrder(EntrustNode* node);
private:
    int m_order_ref;
    std::string m_ins_key;
    std::string m_ord_key;
    redisMgr *m_ins_redis;
    redisMgr *m_ord_redis;
    redisMgr *m_cancel_redis;

    bool m_bPosInited;

    std::string m_id;
    std::unordered_map<std::string, hfs_holdings_t *> positions; // 
    std::string accountid;
    std::string passwd;
    std::string server;
    hfs_account_t* accountinfo;

    std::mutex m_muEvec;
    EntrustInfoVec  m_evec;   // 原始委托信息
    Sid2EntrustMap  m_smap;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_cats_adaptor(_cfg);
  }
};
