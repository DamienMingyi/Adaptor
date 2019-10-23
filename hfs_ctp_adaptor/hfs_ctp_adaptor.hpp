#pragma once

#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "ThostFtdcTraderApi.h"
#include "hfs_base_univ.hpp"
#include <vector>
#include <map>
#include <mutex>

using namespace std;

#pragma pack(push,1)

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

struct CTPConfig {
  string QuoteAddress;
  string TradeAddress;
  string BrokerID;
  string UserName;
  string Password;
  string AuthKey;
};

#pragma pack(pop)

enum CTPSTATE {
  CTP_INIT = 0,
  CTP_DISCONNECTED,
  CTP_CONNECTED,
  CTP_LOGGEDON,
};

struct OrderIDCmp {
  bool operator()(const OrderID &left, const OrderID &right) {
    return left.teid < right.teid
		       || (left.teid == right.teid && left.pid < right.pid)
      || (left.teid == right.teid && left.pid == right.pid && left.oseq < right.oseq);
  }
};

typedef map<string,  EntrustNode *>               Sid2EntrustMap;
typedef map<OrderID, EntrustNode *, OrderIDCmp>   Nid2EntrustMap;
typedef vector<EntrustNode *>                     EntrustInfoVec;

class hfs_ctp_adaptor : public hfs_base_adaptor, public CThostFtdcTraderSpi {
public:
  hfs_ctp_adaptor(const a2ftool::XmlNode &_cfg);
  virtual ~hfs_ctp_adaptor();

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

private:
  // USER_API参数
  CThostFtdcTraderApi* pUserApi;

  CTPConfig params;

  int  seqNo;
  bool loggedon;

  // 配置参数
  int iRequestID;

  // 会话参数
  TThostFtdcFrontIDType FRONT_ID;//前置编号
  TThostFtdcSessionIDType SESSION_ID;//会话编号
  TThostFtdcOrderRefType ORDER_REF;//报单引用
  int maxRef;

  CTPSTATE m_state;

  EntrustInfoVec  evec;
  Sid2EntrustMap  smap;
  Nid2EntrustMap  nmap;

  int  entrustID;
  bool canceled;

  std::mutex mutexSync, mutexOrder, mtxCommon;
  std::mutex mu, muEvec;

  //对于每一组串行操作，都需要下面两个变量作为控制
  bool success, finished;

  std::map<std::string, hfs_instrument *> insInfoMap;
  unordered_map<string, double> priceTickMap;
  std::map<std::string, int> tkr2idnum;
  std::map<std::string, hfs_holdings_t *> holdings;
  std::string m_id;  // 账户ID
  std::string m_position_file;
private:
  
  /*****************************************************************************************************************************
   * 报单响应函数,
   * Thost收到报单指令，如果没有通过参数校验，拒绝接受报单指令。用户就会收到OnRspOrderInsert消息，其中包含了错误编码和错误消息。
   * 如果Thost接受了报单指令，用户不会收到OnRspOrderInser，而会收到OnRtnOrder，用来更新委托状态。
   * 交易所收到报单后，通过校验。用户会收到OnRtnOrder、OnRtnTrade。
   * 如果交易所认为报单错误，用户就会收到OnErrRtnOrder。

   * 撤单响应函数,
   * Thost收到撤单指令，如果没有通过参数校验，拒绝接受撤单指令。用户就会收到OnRspOrderAction消息，其中包含了错误编码和错误消息。
   * 如果Thost接受了撤单指令，用户不会收到OnRspOrderAction，而会收到OnRtnOrder，用来更新委托状态。
   * 交易所收到撤单后，通过校验，执行了撤单操作。用户会收到OnRtnOrder。
   * 如果交易所认为报单错误，用户就会收到OnErrRtnOrderAction。
   *****************************************************************************************************************************/
  ///报单录入请求响应：如果没有通过参数校验，拒绝接受报单指令，则会收到此消息
  virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

  ///报单录入错误回报：如果交易所认为报单错误，则会收到此消息
  virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

  ///报单操作请求响应：如果没有通过参数校验，拒绝接受撤单指令，则会收到此消息
  virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

  ///报单操作错误回报
  virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);
  
  ///报单通知
  virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

  ///成交通知
  virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

  /********************************************************************************************************************************
   * 连接、登录、查询函数
   ********************************************************************************************************************************/
  ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  virtual void OnFrontConnected();
  virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  ///登录请求响应
  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

  ///投资者结算结果确认响应
  virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

  /// 查询持仓响应
  virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
   
   /// 查询合约响应
  virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  ///错误应答
  virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

  ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
  virtual void OnFrontDisconnected(int nReason);

  /********************************************************************************************************************************
   * 其它辅助函数
   ********************************************************************************************************************************/
  ///心跳超时警告。当长时间未收到报文时，该方法被调用。
  virtual void OnHeartBeatWarning(int nTimeLapse);

private:
  // 是否收到成功的响应
  bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);

  // 是否我的报单回报
  bool IsMyOrder(CThostFtdcOrderField *pOrder);

  // 是否正在交易的报单
  bool IsTradingOrder(CThostFtdcOrderField *pOrder);

private:
  int startUserAPI();

  int getNextRef() { return maxRef++; }

  void qryPosition();  // 查询持仓
  void qryInstrument(); // 查询合约信息

  void printFtdcOrder(CThostFtdcOrderField *pOrder);
  
  void printTrade(CThostFtdcTradeField *pTrade);
  void printInputOrder(CThostFtdcInputOrderField *pOrder);
  // void printHolding(CThostFtdcInvestorPositionField *pos);

  double myfloor(double price, double tick);

  int Login();

  void sleep(int mili);
  void dump(hfs_holdings_t *holding);
  void dump(hfs_order_t *order);
};
extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_ctp_adaptor(_cfg);
  }
};
