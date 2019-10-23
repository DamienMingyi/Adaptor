#pragma once

#include "xml_parser.hpp"
#include "HfsOrder.hpp"

typedef std::function<bool(int, std::string, hfs_order_t&)> ORDER_PROC_FUN;

class hfs_base_adaptor {
protected:
    a2ftool::XmlNode cfg;

public:
    hfs_base_adaptor(const a2ftool::XmlNode &_cfg) :cfg(_cfg){}

    virtual ~hfs_base_adaptor() {}

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool isLoggedOn() = 0;

    virtual bool onRequest(int teid, hfs_order_t &order) = 0;

    bool onResponse(int teid, std::string execid, hfs_order_t& order) {return rspfunc(teid, execid, order);};
    bool register_rspstream(ORDER_PROC_FUN f){rspfunc = f; return true;};

private:
    ORDER_PROC_FUN rspfunc;
};

