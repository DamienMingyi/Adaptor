#pragma once

#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_log.hpp"
#include "hfs_utils.hpp"
#include "hfs_risk_control.hpp"
#include "quickfix/FileStore.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/Log.h"
#include "quickfix/FileLog.h"
#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Values.h"
#include "quickfix/Mutex.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"
#include "quickfix/fix42/Logon.h"
#include "quickfix/fix42/Logout.h"
#include "quickfix/fix42/ListStatusRequest.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>

using namespace std;

#pragma pack(push,1)

// struct OrderID {
//     int teid;
//     int pid;
//     int oseq;
//     OrderID() {};
//     OrderID(int teid_, int pid_, int oseq_) {
//         teid = teid_; pid = pid_; oseq = oseq_;
//     };
//     OrderID(const OrderID& other) {
//         this->teid = other.teid;
//         this->pid = other.pid;
//         this->oseq = other.oseq;
//     };
// };

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

// struct OrderIDCmp {
//     bool operator()(const OrderID &left, const OrderID &right) {
//         return left.teid < right.teid
//             || (left.teid == right.teid && left.pid < right.pid)
//             || (left.teid == right.teid && left.pid == right.pid && left.oseq < right.oseq);
//     }
// };

typedef map<string,  EntrustNode *>               Sid2EntrustMap;
typedef map<OrderID, EntrustNode *, OrderIDCmp>   Nid2EntrustMap;
typedef vector<EntrustNode>                       EntrustInfoVec;

class hfs_fix_adaptor : 
    public hfs_base_adaptor,
    public FIX::Application,
    public FIX::MessageCracker {

public:
    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn();

    virtual bool onRequest(int teid, hfs_order_t &order);
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t& order);

    void login();
    void logout();

public:
    hfs_fix_adaptor(const a2ftool::XmlNode &_cfg);
    ~hfs_fix_adaptor();

private:
    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
    bool IsTradingOrder(EntrustNode* node);
    void qry_order();
    void qry_position();
    void qry_detail();
    void qry_thread();

    void init_file();

private:
    void onCreate(const FIX::SessionID&);
	void onLogon(const FIX::SessionID& sessionID);
	void onLogout(const FIX::SessionID& sessionID);
    void toAdmin(FIX::Message &, const FIX::SessionID &) throw(FIX::DoNotSend);
    void toApp(FIX::Message&, const FIX::SessionID&)
		throw(FIX::DoNotSend);
	void fromAdmin(const FIX::Message&, const FIX::SessionID&)
		throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon);
	void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
		throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType);

	void onMessage(const FIX42::ExecutionReport&, const FIX::SessionID&);
	void onMessage(const FIX42::OrderCancelReject&, const FIX::SessionID&);
	void onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID&);
	void onMessage(const FIX42::OrderCancelRequest& message, const FIX::SessionID&);
	void onMessage(const FIX42::Reject& message, const FIX::SessionID& sessionID);

    void onRspQryOrder(const FIX::Message &);
    void onRspQryDetail(const FIX::Message &);
    void onRspQryPosition(const FIX::Message &);

    std::string  newSeq();

    EntrustNode *getENode(int teid, int pid, int oseq);
    EntrustNode *getENode(string& sid);

    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    
    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);
private:
    FIX::SocketInitiator *m_initiator;
    std::string m_id;
    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    //EntrustInfoVec  evec;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;

    int  seqNo;

    bool loggedon;

    std::string m_account_id;
	std::string m_user;
	std::string m_passwd;
	std::string m_server;
	std::string m_sender_subid;
	std::string m_sender_compid;
	std::string m_target_compid;
	std::string m_fix_cfg;
    std::string m_sysinfo;
    bool m_bHK;
    
    long m_orderRef = 0;
    RiskControl *m_rc;

    std::string m_export_path;
    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_fix_adaptor(_cfg);
  }
};

bool file_exists(const char *fname);