#include "HfsTEClient.hpp"
#include "HfsCommunicator.hpp"
#include "hfs_log.hpp"
#include "hfs_utils.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

using namespace std;
using namespace boost::asio;

boost::mutex teMutex;
boost::mutex resentMutex;

void TEClient::start() {
	cerr << "New teClient " << socket_.remote_endpoint() << endl;
	boost::asio::async_read(socket_, boost::asio::buffer(&from_te_order, sizeof(hfs_order_t)),
		boost::bind(&TEClient::handle_te_request, shared_from_this(), boost::asio::placeholders::error));
}

void TEClient::handle_te_request(const boost::system::error_code &error) {
	
	if(error) {
		LOG_ERROR ("Stop teClient:{} error value :{}", teid, error.value());
		communicator.unregisterTEClient(shared_from_this());
		return;
	}

	hfs_order_t &ord = from_te_order;
	printOrder("request", ord);

	if(!loggedOn) {
		hfs_order_t &login = from_te_order;
		if(strcmp(login.symbol, "_LOGIN_") != 0) {
			LOG_ERROR ("First order from te should be login order!");
		}
		else {
			if(!init(login.oseq)) {
				LOG_ERROR ("Stop teClient:{} login failed", teid);
				communicator.unregisterTEClient(shared_from_this());
				return;
			}
			if(!communicator.registerTEClient(shared_from_this())) {
				LOG_ERROR ("Stop teClient:{}  regist teClient failed", teid);
				communicator.unregisterTEClient(shared_from_this());
				return;
			}
			loggedOn = true;
		}
	}
	else if(!inited) {
		hfs_order_t &resend = from_te_order;
		if(strcmp(resend.symbol, "_RESEND_") != 0) {
			LOG_ERROR ("Second order from te should be resend order!");
			LOG_ERROR ("Stop teClient {}", teid);
			communicator.unregisterTEClient(shared_from_this());
			return;
		}

		communicator.onRequest(teid, from_te_order);
		inited = true;
	}
	else {
		communicator.onRequest(teid, from_te_order);
	}

	boost::asio::async_read(socket_, boost::asio::buffer(&from_te_order, sizeof(hfs_order_t)),
		boost::bind(&TEClient::handle_te_request, shared_from_this(), boost::asio::placeholders::error));
}

bool TEClient::init(int id) {
	if(id < 0) return false;
	this->teid = id;  

	return true;
}
// ����ѳɽ������ѱ���ί��
bool TEClient::forwardExecution(hfs_order_t *exch_order) {
	boost::mutex::scoped_lock lk(teMutex);
	assert(exch_order);

	try {
		printOrder("forward", *exch_order);
		size_t sz = boost::asio::write(socket_, boost::asio::buffer(exch_order, sizeof(hfs_order_t)));
		if(sz != sizeof(hfs_order_t)) throw;
	}
	catch(...) {
		LOG_ERROR ("TEClient::forwardExecution error!");
		return false;
	}

	return true;  
}

void TEClient::printOrder(const char *tag, hfs_order_t &order) {
	char buf[4096];
	char *p = buf;

	sprintf(p, "[order from %s %d]", tag, current());
	p+= strlen(p);
	sprintf(p, "symbol:%s;", order.symbol);
	p+= strlen(p);
	sprintf(p, "algo:%s;", order.algo);
	p+= strlen(p);
	sprintf(p, "exch:%s;", order.exch);
	p+= strlen(p);
	sprintf(p, "orderid:%s;", order.orderid);
	p+= strlen(p);
	sprintf(p, "idnum:%d;", order.idnum);
	p+= strlen(p);
	sprintf(p, "pid:%d;", order.pid);
	p+= strlen(p);
	sprintf(p, "type:%c;", order.type);
	p+= strlen(p);
	sprintf(p, "side: %c;", order.side);
	p+= strlen(p);
	sprintf(p, "oseq:%d;", order.oseq);
	p+= strlen(p);
	sprintf(p, "tseq:%d;", order.tseq);
	p+= strlen(p);
	sprintf(p, "qty:%d;", order.qty);
	p+= strlen(p);
	sprintf(p, "prc:%.3f;", order.prc);
	p+= strlen(p);
	sprintf(p, "time:%d;", order.tm);
	p+= strlen(p);
	LOG_INFO(buf);
}

