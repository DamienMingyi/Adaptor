#pragma once 

#include "hfs_base_router.hpp"
#include "hfs_utils.hpp"
#include <vector>

class Communicator;

class HfsSimpleRouter : public hfs_base_router {
public:
    virtual bool onRequest(int teid, hfs_order_t& order);
    virtual bool onResponse(int teid, std::string execid, hfs_order_t& order);

    HfsSimpleRouter(Communicator &comm, hfs_adaptor_mgr& mgr, const a2ftool::XmlNode &_cfg):hfs_base_router(comm,mgr,_cfg) {
        ORDER_PROC_FUN f = std::bind(&HfsSimpleRouter::onResponse, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        adMgr.register_rspstream(f);

        std::string select = cfg.getAttrDefault("select", "");
        accounts = split(select, "|");
    }
    virtual ~HfsSimpleRouter(){}

    std::vector<std::string>  accounts;
};

extern "C" {
    hfs_base_router *create(Communicator &comm, hfs_adaptor_mgr& mgr, const a2ftool::XmlNode &_cfg) {
        return new HfsSimpleRouter(comm, mgr, _cfg);
    }
};
