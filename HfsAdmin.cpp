#include "HfsAdmin.hpp"
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp> 
#include <boost/regex.hpp>
#include <iostream>
#include "HfsCommunicator.hpp"
#include "hfs_log.hpp"

using namespace std;

const char eot = 0x04;

void AdminSession::start() {
    cerr << "New AdminSession " << socket_.remote_endpoint() << endl;
    boost::asio::async_read_until(socket_, from_admin_buf, boost::regex("\r\n"),
                                  boost::bind(&AdminSession::handle_admin_request, 
                                  shared_from_this(), boost::asio::placeholders::error));
}

void AdminSession::handle_admin_request(const boost::system::error_code &error) {
    LOG_INFO("{}",__FUNCTION__);
    if(error) {
        LOG_ERROR("handle_admin_request error: {}", error.message());
        return;
    }

    string cmd;
    std::istream is(&from_admin_buf);
    getline(is, cmd);
    cmd = cmd.substr(0, cmd.length() > 1 ? (cmd.length()-1) : 0);

    if(cmd.length() > 0) {
        cerr << "handle_admin_request cmd:[" << cmd << "]" << endl;
    
        if(cmd == "list") {
            list_trading_status();
        }
        else if(cmd == "test") {
            const char *sbuf = "cmd == test\n";
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
        }
        else if (cmd.find("LoadAlgoPos") != std::string::npos)
        {
            string exch = cmd.substr(11);
            char sbuf[64]={0};
            if (exch.length()>0)
            {
                int pos = exch.find(" ");
                exch = exch.substr(pos+1);
                sprintf (sbuf, "LoadAlgoPos %s",exch.c_str());
            }
            else 
            {
                sprintf (sbuf, "LoadAlgoPos All");  
            }
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
            communicator.router->LoadAlgoPos(exch);
        }
        else if (cmd.find("RejectPn") != std::string::npos)
        {
            string exch = cmd.substr(8);
            char sbuf[64]={0};
            if (exch.length()>0)
            {
                int pos = exch.find(" ");
                exch = exch.substr(pos+1);
                sprintf (sbuf, "RejectPn %s",exch.c_str());
            }
            else 
            {
                sprintf (sbuf, "RejectPn All");  
            }
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
            communicator.router->rejectpn(exch);
        }
        else if (cmd.find("CancelNew") != std::string::npos)
        {
            string exch = cmd.substr(9);
            char sbuf[64]={0};
            if (exch.length()>0)
            {
                int pos = exch.find(" ");
                exch = exch.substr(pos+1);
                sprintf (sbuf, "CancelNew %s",exch.c_str());
            }
            else 
            {
                sprintf (sbuf, "CancelNew All");  
            }
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
            communicator.router->cancelnew(exch);
        }
        else if (cmd.find("LoadT0Pos") != std::string::npos)
        {
            string exch = cmd.substr(9);
            char sbuf[64]={0};
            if (exch.length()>0)
            {
                int pos = exch.find(" ");
                exch = exch.substr(pos+1);
                sprintf (sbuf, "LoadT0Pos %s",exch.c_str());
            }
            else 
            {
                sprintf (sbuf, "LoadT0Pos All");  
            }
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
            communicator.router->LoadT0Pos(exch);
        }
        else if (cmd.find("RemoveT0Pos") != std::string::npos)
        {
            string exch = cmd.substr(11);
            char sbuf[64]={0};
            if (exch.length()>0)
            {
                int pos = exch.find(" ");
                exch = exch.substr(pos+1);
                sprintf (sbuf, "RemoveT0Pos %s",exch.c_str());
            }
            else 
            {
                sprintf (sbuf, "RemoveT0Pos All");  
            }
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
            communicator.router->RemoveT0Pos(exch);
        }
        else {
            const char *sbuf = "cmd == unkown\n";
            socket_.write_some(boost::asio::buffer(sbuf, strlen(sbuf)+1));
        }
    }

    boost::asio::async_read_until(socket_, from_admin_buf,  boost::regex("\r\n"),
                                   boost::bind(&AdminSession::handle_admin_request, 
                                   shared_from_this(), boost::asio::placeholders::error));
}

bool AdminSession::test() {
    boost::asio::io_service &io = socket_.get_io_service();
    boost::asio::ip::tcp::endpoint edp(boost::asio::ip::address_v4::from_string("10.1.1.125"), 50001);
    boost::asio::ip::tcp::socket sk(io);

    try {
        sk.connect(edp);
        char cmd[1024];  //cmd = 'load 600641 B 1000000 10.5'
        sprintf(cmd, "load 600641 B 1000000 10.5");
        cerr << "send_new_scheme_cmd:" << cmd << endl;
        cmd[strlen(cmd)] = eot;
        int cmdLen = strlen(cmd) + 1;
        int sz = boost::asio::write(sk, boost::asio::buffer(cmd, cmdLen));
        if(sz != cmdLen) {
            cerr << "write socket error!" << endl;
            sk.close();
            return false;
        }

        char buf[1024];
        boost::system::error_code errcd;
        boost::asio::streambuf recvbuf;
        sz = boost::asio::read_until(sk, recvbuf, eot, errcd);
        if(errcd) {
            LOG_ERROR ("send_new_scheme_cmd:read_until error:", errcd.message());
            sk.close();
            return false;
        }
        cerr << "send_new_scheme_cmd recv " << sz << " char! bufsize: " << recvbuf.size() << endl;
        std::istream is(&recvbuf);
        is.get(buf, sz);
        if(sz < 2) {
            LOG_ERROR("te doesn't accept the position!");
            sk.close();
            return false;
        }
        buf[sz-2] = '\0';
        cerr << "buf:" << buf << endl;

        int traderid = atoi(buf);
        cerr << "Trade Engine assigned traderid:" << traderid << endl;

        sk.close();
    }
    catch(std::exception &e) {
        cerr << "dispatch position error!" << endl;
        return false;
    }

    return true;
}

bool AdminSession::list_trading_status() {
    char buf[64 * 1024]; //64k
    char *p = buf;

    sprintf(p, "����״̬����\n");
    p+= strlen(p);

    int n = communicator.teClients.size();
    sprintf(p, "TE����: %d \n", n);
    p+= strlen(p);

    TraderRecordList vec;
    communicator.getTraderRecordList(vec);

    sprintf(p, "%5s %8s %10s %10s %5s %10s %5s %5s %10s\n", 
                    "teid", "traderid", "symbol", "side", "qtyE", "cntE", "qtyR", "cntR", "time");
    p+= strlen(p);

    for(int i=0;i<(int)vec.size();i++) {
        TraderRecord &record = vec[i];
        sprintf(p, "%5d %8d %10s %10c %5d %10d %5d %5d %10d\n", 
                        record.teid, record.traderid, record.symbol, record.side, record.qtyEntrusted, record.cntEntrusted,
                        record.qtyReported, record.cntReported, record.lastUpdTime);
        p+= strlen(p);
    }

    socket_.write_some(boost::asio::buffer(buf, strlen(buf)+1));

    return true;
}
