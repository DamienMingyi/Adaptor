#include "hfs_adaptor_mgr.hpp"

hfs_adaptor_mgr::hfs_adaptor_mgr(const a2ftool::XmlNode &_cfg) : cfg(_cfg) {
	loadAdaptor();
}

void hfs_adaptor_mgr::loadAdaptor() {
	a2ftool::XmlNode accounts = cfg.getChild("accounts");
	vector<a2ftool::XmlNode> nodes = accounts.getChildren("account");
	for (auto node : nodes) {
		string path = node.getAttrDefault("path", "");
		string accid = node.getAttrDefault("id", "");
		void * pHandle = dlopen(path.c_str(), RTLD_LAZY);
		if (!pHandle) {
			cerr << "load " << path << " failed! " << dlerror() << endl;
			throwException(string("load ") + path + " failed!");
		}
		hfs_base_adaptor * (* create)(const a2ftool::XmlNode &) = (hfs_base_adaptor * (*)(const a2ftool::XmlNode &))dlsym(pHandle, "create");
		hfs_base_adaptor* ad = create(node);
		adMap.insert(make_pair(accid, ad));
	}
}

bool hfs_adaptor_mgr::start() {
	ORDER_PROC_FUN f = std::bind(&hfs_adaptor_mgr::onResponse, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	for (auto iter = adMap.begin(); iter != adMap.end(); iter++) {
		iter->second->register_rspstream(f);
		iter->second->start();
	}
	return true;
}

bool hfs_adaptor_mgr::stop() {
	for (auto iter = adMap.begin(); iter != adMap.end(); iter++) {
		iter->second->stop();
	}
}

bool hfs_adaptor_mgr::onRequest(int teid, hfs_order_t &order) {
	/*
	 * case O, 根据exchid判断发送到哪个账户，调用相应的so文件
	 */
	LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
	string accid = order.exch;
	if (adMap.find(accid) == adMap.end()) {
		LOG_ERROR("cannot find {} adaptor", accid);
		return false;
	}
	return adMap[accid]->onRequest(teid, order);
}

bool hfs_adaptor_mgr::onResponse(int teid, std::string execid, hfs_order_t& order) {
	LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
	// return router->onResponse(teid,  execid, order);
	return rspfunc(teid, execid, order);
}