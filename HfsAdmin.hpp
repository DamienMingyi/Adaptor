#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <vector>

class Communicator;

class AdminSession : public boost::enable_shared_from_this<AdminSession> {
public:
  void start();
  boost::asio::ip::tcp::socket& getSocket() { return socket_; }

public:
  AdminSession(boost::asio::io_service &io, Communicator &communicator)
    : socket_(io),communicator(communicator) {}
  ~AdminSession(){};

private:
  void handle_admin_request(const boost::system::error_code &error);

  bool list_trading_status();

  bool test();
private:
  boost::asio::ip::tcp::socket socket_;
  Communicator &communicator;

  boost::asio::streambuf from_admin_buf;
};
typedef boost::shared_ptr<AdminSession> AdminSessionPtr;


