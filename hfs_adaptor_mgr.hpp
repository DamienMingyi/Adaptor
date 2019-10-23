#pragma once

#include "HfsOrder.hpp"
#include "hfs_utils.hpp"
#include "hfs_log.hpp"
#include "hfs_base_adaptor.hpp"
#include <dlfcn.h>
#include <string>
#include <map>

using namespace std;

class hfs_adaptor_mgr {
public:
	hfs_adaptor_mgr(const a2ftool::XmlNode &_cfg);
	~hfs_adaptor_mgr();

	bool start();
	bool stop();
	
	bool onRequest(int teid, hfs_order_t &order);
	bool onResponse(int teid, std::string execid, hfs_order_t& order);

	void loadAdaptor();

	bool register_rspstream(ORDER_PROC_FUN f){rspfunc = f; return true;};

private:
	a2ftool::XmlNode cfg;
	map<string, hfs_base_adaptor *> adMap; //exchid -> adaptor
	ORDER_PROC_FUN rspfunc;
};



