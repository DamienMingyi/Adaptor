#pragma once

#include "HfsOrder.hpp"
#include "HfsPersistence.hpp"
#include "HfsTEClient.hpp"
#include "HfsAdmin.hpp"
#include "hfs_base_router.hpp"
#include "hfs_adaptor_mgr.hpp"
#include "xml_parser.hpp"
#include <boost/asio.hpp>
#include <set>
#include <iostream>

typedef std::set<TEClient *>            TEClientSet;
typedef InfoStore<hfs_order_t>          RequStore;
typedef InfoStore<hfs_order_t>          RespStore;

//当前各个trader的信息，供监控用
struct TraderRecord {
    int   teid;			// tradeEngine id
    int   traderid;    // 账户
    char  symbol[10];	
    char  side;			// 方向？

    int   qtyEntrusted; //委托手数
    int   cntEntrusted; //委托笔数

    int   qtyReported;  
    int   cntReported;

    int   lastUpdTime; 

    const TraderRecord &operator=(const TraderRecord &r) {
        memcpy(this, &r, sizeof(TraderRecord));
        return *this;
    }
};
// 比较函数
class TraderRecordCmp {
public:
    bool operator()(const TraderRecord &l, const TraderRecord &r) {
        return l.teid < r.teid 
            || (l.teid == r.teid && l.traderid < r.traderid);
    }
};

typedef std::vector<TraderRecord> TraderRecordList; 


class Communicator {
    friend class AdminSession;
    friend class TEClient;
    friend class ExchSession;

    static const int MaxTECount = 64; //最多支持2-1=1个TE，其ID范围为 1 ~ 1

public:
    void run();
    bool onResponse(int teid, std::string execid, hfs_order_t& order);
public:
    Communicator(const a2ftool::XmlNode &_cfg);
    ~Communicator();

private:
    bool startAdaptor();
    bool stopAdaptor();

    bool registerTEClient(TEClientPtr session);
    bool unregisterTEClient(TEClientPtr session);

    bool resendTradeHistory(int teid);

    bool sqlInit();
    bool sqlRelease();
    bool sqlSaveReport(int teid, hfs_order_t *order);
    bool sqlSaveOrder(int teid, hfs_order_t *order);
    bool saveReport(int teid, hfs_order_t *xpt);
    bool saveOrder(int teid, const hfs_order_t *order);

    boost::asio::io_service &getIO() { return io_service_; }
    TEClient* getTEClient(int teid) const;
    int  teCount() { return teClients.size(); }
    int  getTSeq(int teid) { return mtseq[teid]++; }


private:
    int  qes_te_server_start();
    void handle_te_accept(TEClientPtr session, const boost::system::error_code &error);

    int  qes_admin_server_start();
    void handle_admin_accept(AdminSessionPtr session, const boost::system::error_code &error);

    bool isRunning() { return running; }

    void getTraderRecordList(TraderRecordList &v);

    bool onRequest(int teid, hfs_order_t& order);
    

private:
    a2ftool::XmlNode                cfg;
    boost::asio::io_service         io_service_; //for public use
    boost::asio::ip::tcp::acceptor *te_acceptor_;
    boost::asio::ip::tcp::acceptor *admin_acceptor_;

    TEClientSet                     teClients;

    // AdaptorPtr                      adaptor;
    hfs_base_router*                router;
    hfs_adaptor_mgr*                adMgr;

    string                          store_root;
    RequStore                      *requStore[MaxTECount];
    RespStore                      *respStore[MaxTECount];
    int                             mtseq[MaxTECount];
    boost::mutex                    mutexRequ[MaxTECount]; 
    boost::mutex                    mutexResp[MaxTECount];

    bool                            started, running;

    int IC_OPEN_CNT, IC_ENTRUST_CNT;
    int IF_OPEN_CNT, IF_ENTRUST_CNT;
    int IH_OPEN_CNT, IH_ENTRUST_CNT; 
    int     CommListenPort;
	int     CommExchPort;
	int     CommAdminPort;
};

