#pragma once

#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "maCliCosTradeApi.h"
#include "maCliStkTradeApiStruct.h"
#include "maCliMktData.h"
#include "MaThirdParty.h"
#include "hfs_base_univ.hpp"
#include "hfs_risk_control.hpp"
#include <vector>
#include <map>
#include <mutex>

using namespace std;
USE_NAMESPACE_MACLI;

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

struct Config {
    string Server;
    int Port;
    string AcctType;
    string UserName;
    string Password;
    string AuthData;
    char   Channal;
    string SysInfo;
    string Remark;
    string cos_user;
    string cos_password;
    string cos_authdata;
};

#pragma pack(pop)

enum STATE {
    INIT = 0,
    DISCONNECTED,
    CONNECTED,
    LOGGEDON,
};

typedef map<string,  EntrustNode *>               Sid2EntrustMap;
typedef map<OrderID, EntrustNode *, OrderIDCmp>   Nid2EntrustMap;
// typedef vector<EntrustNode *>                     EntrustInfoVec;

class hfs_jt_adaptor : public hfs_base_adaptor, public CCliCosTradeSpi {
public:
    hfs_jt_adaptor(const a2ftool::XmlNode &_cfg);
    virtual ~hfs_jt_adaptor();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn();
    virtual bool onRequest(int teid, hfs_order_t &order);
    
private:
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t* order);

    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);

    bool readConfig();    //读取配置变量
    
    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    EntrustNode *getENode(int teid, int pid, int oseq);
    EntrustNode *getENode(string sid);
    EntrustNode *getENode(OrderID& oid);
    std::string ShowErrorInfo(int iRetCode);

    int cancelOrderByBsn(int bsn);  // 按批次号撤单
    int subTopic();

    void push_reject_order(int teid, hfs_order_t &order, const char *errmsg);

private:
    // USER_API参数
    CCliCosTradeApi* pUserApi;

    Config params;

    int  seqNo;
    bool loggedon;

    // 配置参数
    int iRequestID;
    int m_dte;

    // 会话参数

    int m_orderRef;
    int m_order_bsn;  // 当前委托批次号

    STATE m_state;

    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;

    int  entrustID;
    bool canceled;

    std::mutex mutexSync, mutexOrder, mtxCommon;

    //对于每一组串行操作，都需要下面两个变量作为控制
    bool success, finished;

    std::map<std::string, hfs_instrument *> insInfoMap;
    unordered_map<string, double> priceTickMap;
    std::map<std::string, int> tkr2idnum;
    std::map<std::string, hfs_holdings_t *> holdings;
    std::map<LONGLONG, std::string> request2clientid;
    std::string m_id;  // 账户ID
    std::string m_export_path;
    std::string m_prefix;
    LONGLONG m_custCode;
    LONGLONG m_acctCode;
    std::string m_session_id;
    std::string m_trdAcct_SZ;
    std::string m_trdAcct_SH;
    std::string m_pos_str;

    std::ofstream m_account_file;
    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;

    RiskControl *m_rc;

private:
    virtual int OnConnected(void);
    virtual int OnDisconnected(int p_nReason, const char *p_pszErrInfo);
    virtual int OnRspHeartBeat(CFirstSetField *p_pFirstSet, CRspHeartBeatField *p_pRspHeartBeatField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);
    virtual int OnRtnSubTopic(CRspSubTopicField* p_pRspSubTopicField);
    virtual int OnRtnUnsubTopic(CRspUnsubTopicField* p_pRspUnsubTopicField);
    // 用户登录请求响应
    virtual int OnRspStkUserLogin(CFirstSetField *p_pFirstSet, CRspStkUserLoginField *p_pRspUserLoginField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
    virtual int OnRspCosLogin(CFirstSetField *p_pFirstSet, CRspCosLoginField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);
    virtual int OnRspAutoLogin(CFirstSetField *p_pFirstSet, CRspCosAutoLoginField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);

    virtual int OnRtnTradeRisk(CRtnTradeRiskInfoField* p_pRspField) ;

    virtual int OnRspMaxTradeQty(CFirstSetField *p_pFirstSet, CRspStkMaxTradeQtyField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);
    // 买卖委托响应
    virtual int OnRspOrder(CFirstSetField *p_pFirstSet, CRspCosOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);
    virtual int OnRtnTsuOrder(CRtnTsuOrderField *p_pOrderField);
    // 委托撤单响应
    virtual int OnRspCancelOrder(CFirstSetField *p_pFirstSet, CRspCosCancelOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
    // 批量委托响应
    virtual int OnRspQuantityOrder(CFirstSetField *p_pFirstSetField, CRspStkQuantityOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
    // 批量撤单响应
    virtual int OnRspQuantityCancelOrder(CFirstSetField *p_pFirstSetField, CRspStkQuantityCancelOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
    // 可用资金查询响应
    virtual int OnRspQryExpendableFund(CFirstSetField *p_pFirstSetField, CRspStkQryExpendableFundField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);
    // 当日委托查询响应
    virtual int OnRspQryOrderInfo(CFirstSetField *p_pFirstSet, CRspCosOrderInfoField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
    // 当日成交查询响应
    virtual int OnRspQryMatch(CFirstSetField *p_pFirstSet, CRspCosMatchField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);

    // 成交回报
    virtual int OnRtnOrder(CRtnOrderField * p_pOrderField);
    // 
   // 可用股份查询响应
    virtual int OnRspQryExpendableShares(CFirstSetField *p_pFirstSet, CRspStkQryExpendableSharesField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex);

    //量化可撤单委托查询请求响应spi
    virtual int OnRspQryCanWithdrawOrder(CFirstSetField *p_pFirstSet, CRspCosCanWithdrawOrderField *p_pRspField, LONGLONG  p_llRequestId, int p_iFieldNum, int p_iFieldIndex) ;
private:
    // 是否收到成功的响应
    bool IsErrorRspInfo(CFirstSetField *pRspInfo, std::string prefix);

    // 是否我的报单回报
    bool IsMyOrder(CRtnStkOrderConfirmField *pOrder);

    // 是否正在交易的报单
    bool IsTradingOrder(CRtnStkOrderConfirmField *pOrder);

private:
    int startUserAPI();

    int getNextRef() { return m_orderRef++; }

    void qryPosition();  // 查询持仓
    void qryInstrument(); // 查询合约信息
    void qryAccount();
    void qryOrder();
    void qryTrade();
    void initFile();
    void qryThreadFunc();
    double myfloor(double price, double tick);

    int Login();   // cos 登录
    int StkLogin(); // 股票登录

    void sleep(int mili);
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
};
extern "C" {
    hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
        return new hfs_jt_adaptor(_cfg);
    }
};
