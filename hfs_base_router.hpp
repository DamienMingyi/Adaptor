#pragma once

#include "xml_parser.hpp"
#include "HfsOrder.hpp"
#include "hfs_adaptor_mgr.hpp"

class Communicator;

class hfs_base_router {
protected:    
    Communicator &comm;
    hfs_adaptor_mgr &adMgr;
    a2ftool::XmlNode cfg;

public:
    hfs_base_router(Communicator &comm, hfs_adaptor_mgr& mgr, const a2ftool::XmlNode &_cfg):cfg(_cfg), adMgr(mgr), comm(comm) {
        // ORDER_PROC_FUN f = std::bind(&hfs_base_router::onResponse, this, 
		// 	       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        // mgr.register_rspstream(f);
    }
    virtual ~hfs_base_router(){}
    virtual void LoadAlgoPos(string exch){}
    virtual void rejectpn(string exch){}
    virtual void cancelnew(string exch){}
    virtual void LoadT0Pos(string exch){}
    virtual void RemoveT0Pos(string exch){}
    virtual bool onRequest(int teid, hfs_order_t& order) = 0;
    virtual bool onResponse(int teid, std::string execid, hfs_order_t& order) = 0;
};

