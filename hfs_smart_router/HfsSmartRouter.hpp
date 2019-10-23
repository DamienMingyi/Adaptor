#include "hfs_base_router.hpp"

class Communicator;
//#include "ChildOrdMgr.hpp"
#include "TradePlan.hpp"
//#include "ParentOrdMgr.hpp"

class HfsSmartRouter : public hfs_base_router {
public:
    virtual bool start();
    virtual bool stop();
    virtual bool onRequest(int teid, hfs_order_t& order);
    virtual bool onResponse(int teid, std::string execid, hfs_order_t& order);

    HfsSmartRouter(Communicator &comm, hfs_adaptor_mgr& mgr, const a2ftool::XmlNode &_cfg):hfs_base_router(comm,mgr,_cfg) {start();} 
    virtual ~HfsSmartRouter(){}

    virtual void LoadAlgoPos(string exch);
    virtual void rejectpn(string exch);
    virtual void cancelnew(string exch);
    virtual void LoadT0Pos(string exch);
    virtual void RemoveT0Pos(string exch);
    TradePlan * tradePlan;
   
};

extern "C" {
  hfs_base_router *create(Communicator &comm, hfs_adaptor_mgr& mgr, const a2ftool::XmlNode &_cfg) {
    return new HfsSmartRouter(comm, mgr, _cfg);
  }
};
