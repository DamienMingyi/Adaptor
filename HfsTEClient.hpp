#ifndef __QES_HS_TE_CLIENT_H__
#define __QES_HS_TE_CLIENT_H__

#include "HfsOrder.hpp"
#include "hfs_base_router.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/condition.hpp>
#include <boost/asio.hpp>
#include <queue>

class Communicator;

class TEClient : public boost::enable_shared_from_this<TEClient> {
public:
  void start();
  bool forwardExecution(hfs_order_t *exch_order);
  int  getTEID() { return teid; }
  bool isInited() const { return inited; }

  boost::asio::ip::tcp::socket& getSocket() { return socket_; }
  
public:
  TEClient(boost::asio::io_service &io, Communicator& communicator, hfs_base_router &adaptor)
    :communicator(communicator), adaptor(adaptor), loggedOn(false), inited(false),socket_(io) { 
    
  }
  ~TEClient(){};

private:
  bool init(int id);
  void handle_te_request(const boost::system::error_code &error);

  void printOrder(const char *tag, hfs_order_t &order);

private:
  Communicator& communicator;
  hfs_base_router &   adaptor;
  int           teid;

  hfs_order_t   from_te_order;
  hfs_order_t   to_te_order;

  bool          loggedOn;
  bool          inited;

  boost::asio::ip::tcp::socket socket_;
};
typedef boost::shared_ptr<TEClient>     TEClientPtr;

#endif
