#pragma once
#include <map>
#include "xml_parser.hpp"
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
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "redisMgr.hpp"


using namespace std;

class hfs_fix2bsim : 
    public FIX::Application,
    public FIX::MessageCracker {

public:
    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn();

    void login();
    void logout();
    void monitor_redis();
    int run();

public:
    hfs_fix2bsim(const a2ftool::XmlNode &_cfg);
    ~hfs_fix2bsim();

private:
    bool process_new_entrust(const std::string &instrument_id, const std::string &exchange_id, double price, int volume, char direction, char offset, std::string &client_id);
    bool process_cancel_entrust(cats_order_t *order);

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

    cats_order_t *get_order(string& sid);

    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);

private:
    FIX::SocketInitiator *m_initiator;
    std::string m_id;

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
    int m_teid;
    bool m_bHK;
    
    long m_orderRef = 0;
   // RiskControl *m_rc;

    std::vector<std::string> m_order_keys;
    std::vector<std::string> m_ins_keys;
    std::string m_redis_ip;
    int m_redis_port;
    redisMgr* m_ord_redis;
    bool m_bRunning;
    bool m_clear_redis = false;
    bool m_bLogoned = false;
    //std::ofstream m_entrust_file;
    std::string m_export_path;

    std::unordered_map<std::string, cats_order_t*> m_order_map;
    std::unordered_map<std::string,std::string> m_client2orderno;
    std::mutex m_order_lock;

    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;
};

bool file_exists(const char *fname);


//extern "C" {
// // hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
//    return new hfs_fix2bsim(_cfg);
//  }
//};
