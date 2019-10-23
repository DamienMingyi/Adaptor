#pragma once
#include "t2sdk_interface.h"
#include "xml_parser.hpp"
#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_data_api.hpp"
#include "hfs_risk_control.hpp"
#include "hfs_database.hpp"
#include <mutex>
#include <map>
#include <unordered_map>

#define FUNC_LOGIN  331100
#define FUNC_QRY_ACCOUNT 332254
#define FUNC_QRY_POSITION 333104
#define FUNC_QRY_ORDER 333101
#define FUNC_QRY_TRADE 333102
#define FUNC_INSERT_ORDER 333002
#define FUNC_CANCEL_ORDER 333017
#define FUNC_HEART_BEAT 620000
#define FUNC_RTN_ORDER 620003
#define FUNC_RTN_TRADE 620003
#define FUNC_RSP_LOGIN 331100
#define FUNC_RSP_ORDER 0
#define FUNC_RSP_POSITION 0
#define FUNC_SUBSCRIBE      620001
#define ISSUE_TYPE_TRADE    12                                              // 订阅证券成交回报
#define ISSUE_TYPE_ORDER    23                                              //订阅委托回报

#pragma pack(push,1)

struct EntrustNode {
    OrderID nid;
    int    sid;
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
typedef unordered_map<int, EntrustNode *>           Sid2EntrustMap;

class hfs_uft_adaptor : public CCallbackInterface , public hfs_base_adaptor
{
private:
    //全局连接对象
    CConnectionInterface * m_lpConnection;
    //全局配置对象
    CConfigInterface * m_lpConfig;
    //全局打包器对象
    IF2Packer * m_lpPacker;
    //全局ESB消息指针
    IBizMessage * m_lpMessage;
    std::string license;
    std::string license_pwd;
    std::string cert_file;
    std::string cert_pwd;
    std::string tradeType;
    std::string client_id;
    std::string branch_no;
    std::string token;
    std::string accountid;
    std::string passwd;
    std::string server;

    std::string m_sysinfo;
    std::string m_id;
    int m_orderRef;
    int m_state; // 状态 -1 异常， 0 初始状态 ， 1 连接前置， 2 登录成功
    bool m_finished = false;

    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    //EntrustInfoVec  evec;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;
    std::unordered_map<std::string, std::string> m_msgIDMap;
    std::unordered_map<std::string, hfs_holdings_t *> m_holdings;

    std::string m_export_path;
    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;
    std::ofstream m_account_file;

    RiskControl *m_rc;
    // hfs_database_mysql *m_sql;
    // std::unordered_map<int, std::string> m_opstation;

public:
    std::atomic<int> *outside_order;
    // 0 成功  -1 失败
    hfs_uft_adaptor(const a2ftool::XmlNode &cfg);
    virtual ~hfs_uft_adaptor();    
    virtual int Init();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn(){return m_state == 2;};
    virtual bool onRequest(int teid, hfs_order_t &order);

private:
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t* order);

    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void push_reject_order(int teid, hfs_order_t &order, const char *errmsg);

    int cancel_order(const char* orderid);

    bool registerTrader(int teid, const hfs_order_t &order, int sid);
    EntrustNode *getENode(OrderID& oid);
    EntrustNode *getENode(int sid);

    void qryThreadFunc();

    std::string getOPStation(int traderid);
    
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);
private:
    int login();

    void subscribeOrder();
    void subscribeDetail();
    int SendToServerSyn(int iFunc, IF2Packer *lpPacker);
    int SendToServerAsy(int iFunc, IF2Packer *lpPacker, long reqID = 0);
    // void unPack(const void * lpBuffer);
    void OnRspLogin(IF2UnPacker* lpUnPack);
    void OnRspOrder(IF2UnPacker* lpUnPack, int senderid);
    void OnRspCancelOrder(IF2UnPacker* lpUnPack);
    void OnRtnOrder(IF2UnPacker* lpUnPack);
    void OnRtnTrade(IF2UnPacker* lpUnPack);

    void qryAccount();
    void OnRspAccount(IF2UnPacker* lpUnPack);
    void qryPosition(string & pos_str);
    void OnRspPosition(IF2UnPacker* lpUnPack);
    void qryOrder(string & pos_str);
    void OnRspQryOrder(IF2UnPacker* lpUnPack);
    void qryTrade(string & pos_str);
    void OnRspQryTrade(IF2UnPacker* lpUnPack);

public:
    unsigned long  FUNCTION_CALL_MODE QueryInterface(const char *iid, IKnown **ppv);
    unsigned long  FUNCTION_CALL_MODE AddRef();
    unsigned long  FUNCTION_CALL_MODE Release();

    // 各种事件发生时的回调方法，实际使用时可以根据需要来选择实现，对于不需要的事件回调方法，可直接return
    // Reserved?为保留方法，为以后扩展做准备，实现时可直接return或return 0。
    void FUNCTION_CALL_MODE OnConnect(CConnectionInterface *lpConnection);
    void FUNCTION_CALL_MODE OnSafeConnect(CConnectionInterface *lpConnection);
    void FUNCTION_CALL_MODE OnRegister(CConnectionInterface *lpConnection);
    void FUNCTION_CALL_MODE OnClose(CConnectionInterface *lpConnection);
    void FUNCTION_CALL_MODE OnSent(CConnectionInterface *lpConnection, int hSend, void *reserved1, void *reserved2, int nQueuingData);
    void FUNCTION_CALL_MODE Reserved1(void *a, void *b, void *c, void *d);
    void FUNCTION_CALL_MODE Reserved2(void *a, void *b, void *c, void *d);
    int  FUNCTION_CALL_MODE Reserved3();
    void FUNCTION_CALL_MODE Reserved4();
    void FUNCTION_CALL_MODE Reserved5();
    void FUNCTION_CALL_MODE Reserved6();
    void FUNCTION_CALL_MODE Reserved7();
    void FUNCTION_CALL_MODE OnReceivedBiz(CConnectionInterface *lpConnection, int hSend, const void *lpUnPackerOrStr, int nResult);
	void FUNCTION_CALL_MODE OnReceivedBizEx(CConnectionInterface *lpConnection, int hSend, LPRET_DATA lpRetData, const void *lpUnpackerOrStr, int nResult);
    void FUNCTION_CALL_MODE OnReceivedBizMsg(CConnectionInterface *lpConnection, int hSend, IBizMessage* lpMsg);
    void RealseAll();
};

void ShowPacket_bl(IF2UnPacker *lpUnPacker);

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_uft_adaptor(_cfg);
  }
};