#pragma once 

#include "htsdk_interface.h"
#include "HtClientFiledDefine.h"
#include <stdio.h>
#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "hfs_trade_utils.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_data_api.hpp"
#include "hfs_risk_control.hpp"
#include "hfs_database.hpp"
#include <mutex>
#include <map>

#define MSGTYPE_REQ_INSERT "13010001"
#define MSGTYPE_REQ_CANCEL "13010003"

#define MSGTYPE_QRY_FUND   "13010005"
#define MSGTYPE_QRY_POSITION "13010007"
#define MSGTYPE_QRY_ORDER  "13010009"
#define MSGTYPE_QRY_DERAIL "13010011"

#define MSGTYPE_RSP_INSERT "13010002"
#define MSGTYPE_RSP_CANCEL "13010004"
#define MSGTYPE_RSP_FUND   "13010006"
#define MSGTYPE_RSP_POSITION "13010008"
#define MSGTYPE_RSP_ORDER  "13010010"
#define MSGTYPE_RSP_DETAIL "13010012"

#pragma pack(push,1)

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

#pragma pack(pop)

typedef map<OrderID, EntrustNode *, OrderIDCmp>   Nid2EntrustMap;
typedef unordered_map<std::string, EntrustNode *> Sid2EntrustMap;

class hfs_ht_adaptor : public CCallbackInterface , public hfs_base_adaptor
{
public:
    hfs_ht_adaptor(const a2ftool::XmlNode &cfg);
    virtual ~hfs_ht_adaptor();
    // 0 成功  -1 失败
    virtual int Init();
    virtual int cancel_order(char* order_id);

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn(){return m_state == 2;};
    virtual bool onRequest(int teid, hfs_order_t &order);

    virtual void ProcAsyncCallBack(int iRet, const CHtMessage* pstMsg); 
	virtual void OnConnect();
	virtual void OnClose();
	virtual HT_void OnTokenLoseEfficacy(const CHtMessage* pstMsg);
	virtual HT_void ProcEvent(HT_int32 eventType, const CHtMessage* pstMsg);
    virtual HT_void OnError();

private:
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t* order);

    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void push_reject_order(int teid, hfs_order_t &order, const char *errmsg);

    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    EntrustNode *getENode(OrderID& oid);
    EntrustNode *getENode(std::string sid);

    std::string getOPStation(int traderid);
private:
    int login();

    void subscribeOrder();
    void subscribeDetail();
    int SendToServerSyn(std::string symbol, char side, int qty, float prc, std::string client_id);  // 同步
    int SendToServerAsy(const hfs_order_t &order, std::string client_id);  // 异步
    // void unPack(const void * lpBuffer);
    void OnRspLogin();
    void OnRspOrder();
    void OnRspCancelOrder();
    void OnRtnOrder(CUnPackMessageCommon &unPm);
    void OnRtnTrade(CUnPackMessageCommon &unPm);
    void OnRspOrder(CUnPackMessageCommon &unPm);

    void qryAccount();
    void OnRspAccount();
    std::string  qryPosition();
    void OnRspPosition();

    std::string  qryOrder();
    std::string  qryDetail();

    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);
private:
    std::string tradeType;
    std::string m_id;
    std::string m_userID;
    std::string m_fundAccount;
    std::string m_password;
    std::string m_userToken;
    std::string m_channelID = "link_1";
    std::string m_sysinfo;
    std::string m_license_path = "./cfg";
    std::string m_export_path;

    int m_orderRef;

    int m_state; // 状态 -1 异常， 0 初始状态 ， 1 连接前置， 2 登录成功

    //对于每一组串行操作，都需要下面两个变量作为控制
    bool success, finished;
    std::string m_pos_str;   // 查询定位串

    std::unordered_map<std::string, hfs_holdings_t *> m_holdings;

    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    //EntrustInfoVec  evec;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;
    std::unordered_map<std::string, std::string> m_msgIDMap;

    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;

    RiskControl *m_rc;
    hfs_database_mysql *m_sql = nullptr;
    std::unordered_map<int, std::string> m_opstation;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_ht_adaptor(_cfg);
  }
};