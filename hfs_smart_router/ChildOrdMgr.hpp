#pragma once 
#include "Ord.hpp"
#include <map>
#include <iostream>
#include "ParentOrdMgr.hpp"

using namespace std;

struct  ChildOrderKey: public ParentOrderKey
{
    string exch;
    bool operator < (const ChildOrderKey& b) const
    {

        if (teid != b.teid)
            return teid < b.teid;
        else if (oseq != b.oseq)
            return oseq < b.oseq; 
        else if (pid != b.pid)
            return pid < b.pid; 
        else
           return exch < b.exch;
    }
};

class ChildOrdMgr
{
    friend class TradePlan;
public:
    ChildOrdMgr(hfs_adaptor_mgr & _adMgr,Communicator &_comm);
    bool NewIns(int teid,string exch,int qty, Ord* parentOrd,hfs_order_t& origal_order,char oc);
    Ord * ReceiveNewResp(int teid, string exch,hfs_order_t& order);
    Ord * ReceiveFillResp(int teid, string exch,hfs_order_t& order);
    Ord * ReceiveCancelResp(int teid, string exch,hfs_order_t& order);
    Ord * ReceiveFailResp(int teid, string exch,hfs_order_t& order);
    void CancelAll();
    void RejectAll();
private:
    map<ChildOrderKey,Ord *> orders;
    Communicator & comm;
    hfs_adaptor_mgr & adMgr;
};