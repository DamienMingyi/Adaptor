#pragma once

#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
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

class hfs_sim_adaptor : public hfs_base_adaptor {
public:
  hfs_sim_adaptor(const a2ftool::XmlNode &_cfg);
  virtual ~hfs_sim_adaptor();

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
  int  seqNo;
  bool loggedon;

  // 配置参数
  int iRequestID;
  int maxRef=1;

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
  int startUserAPI();

  int getNextRef() { return maxRef++; }

  double myfloor(double price, double tick);

  int Login();

  void sleep(int mili);
  void dump(hfs_holdings_t *holding);
  void dump(hfs_order_t *order);
};
extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_sim_adaptor(_cfg);
  }
};
