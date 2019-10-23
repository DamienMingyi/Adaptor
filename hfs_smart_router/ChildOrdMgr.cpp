#include "ChildOrdMgr.hpp"
#include <chrono>
ChildOrdMgr::ChildOrdMgr(hfs_adaptor_mgr & _adMgr,Communicator &_comm)
:adMgr(_adMgr),comm(_comm)
{

}

bool ChildOrdMgr::NewIns(int teid,string exch,int qty, Ord* parentOrd,hfs_order_t& origal_order,char oc)
{
    ChildOrderKey cok;
    cok.exch = exch;
    cok.teid = teid;
    cok.oseq = origal_order.oseq;
    cok.pid = origal_order.pid;

    auto iter = orders.find(cok);
    if (iter == orders.end())
    {
        Ord *ord = new Ord(parentOrd,qty,teid,origal_order);//memery leak
        //ord->status = HFS_ORDER_TYPE_ORDER_ENTER;
        ord->time = parentOrd->time;//TODO:get new time;
        ord->exch = exch;
        ord->ocflag = oc;
        orders.insert(pair<ChildOrderKey,Ord* >(cok,ord));
        //orders[cok] = ord;
        LOG_DEBUG("NewIns: Insert child ord!  exch:{},teid:{},pid:{},oseq:{},{}", cok.exch,cok.teid,cok.pid,cok.oseq,ord->ToString() );
        hfs_order_t hot = ord->ToHfsOrdNew();
        adMgr.onRequest(teid, hot);
        return true;
    }
    else
    {
        LOG_ERROR("Warning: New Ins, Exist child ord!  exch:{},teid:{},pid:{},oseq:{}", cok.exch,cok.teid,cok.pid,cok.oseq);
        return false;
    }
   
}

Ord * ChildOrdMgr::ReceiveNewResp(int teid, string exch,hfs_order_t& order)
{
    ChildOrderKey cok;
    cok.exch = exch;
    cok.teid = teid;
    cok.oseq = order.oseq;
    cok.pid = order.pid;

    auto iter = orders.find(cok);
    if (iter != orders.end())
    {
        Ord * ord = iter->second;
        ord->outerOrderID=order.orderid;
        ord->GetNewResp(order.qty);
        //ord->parentOrd->GetNewResp(order.qty);
        hfs_order_t newResp =  ord->parentOrd->ToHfsNewResp(order.qty);
        LOG_DEBUG("Receive New Resp: found child ord!  exch:{},teid:{},pid:{},oseq:{},{}", cok.exch,cok.teid,cok.pid,cok.oseq,ord->ToString() );
        comm.onResponse(teid, exch, newResp);
        return ord;
    }
    else
    {
        LOG_ERROR("ERROR: Receive New Resp, not exist child ord!  exch:{},teid:{},pid:{},oseq:{}", cok.exch,cok.teid,cok.pid,cok.oseq);
        return nullptr;
    }

}

Ord * ChildOrdMgr::ReceiveFillResp(int teid, string exch,hfs_order_t& order)
{
    ChildOrderKey cok;
    cok.exch = exch;
    cok.teid = teid;
    cok.oseq = order.oseq;
    cok.pid = order.pid;

    auto iter = orders.find(cok);
    if (iter != orders.end())
    {
        Ord * ord = iter->second;
        ord->GetFillResp(order.qty,order.prc);
        ord->parentOrd->GetFillResp(order.qty,order.prc);
        hfs_order_t fillResp = ord->parentOrd->ToHfsFillResp(order.qty);
        LOG_DEBUG("Receive Fill Resp: found child ord!  exch:{},teid:{},pid:{},oseq:{},{}", cok.exch,cok.teid,cok.pid,cok.oseq,ord->ToString() );
        comm.onResponse(teid, exch, fillResp);
        return ord;
    }
    else
    {
        LOG_ERROR("ERROR: Receive Fill Resp, not exist child ord!: {}/{}/{}", cok.exch,cok.teid,cok.pid);
        return nullptr;
    }

}

Ord * ChildOrdMgr::ReceiveCancelResp(int teid, string exch,hfs_order_t& order)
{
    ChildOrderKey cok;
    cok.exch = exch;
    cok.teid = teid;
    cok.oseq = order.oseq;
    cok.pid = order.pid;

    auto iter = orders.find(cok);
    if (iter != orders.end())
    {
        Ord * ord = iter->second;
        if (ord->cqty == 0)
        {
            if (order.qty==0 && ord->nqty > 0)
                order.qty =  ord->nqty;
            else if  (order.qty==0 && ord->pnqty > 0 ) 
                order.qty =  ord->pnqty;
            ord->GetCancelResp(order.qty);
            //ord->parentOrd->GetCancelResp(order.qty);
            hfs_order_t cancelResp = ord->parentOrd->ToHfsCancelResp(order.qty);
            LOG_DEBUG("Receive Cancel Resp: found child ord!  exch:{},teid:{},pid:{},oseq:{},{}", cok.exch,cok.teid,cok.pid,cok.oseq,ord->ToString() );
            comm.onResponse(teid, exch, cancelResp);
            return ord;
        }
        else 
        {
            return nullptr;
        }
    }
    else
    {
        LOG_ERROR("ERROR: Receive Cancel Resp, not not exist child ord!: {}/{}/{}", cok.exch,cok.teid,cok.pid);
        return nullptr; 
    }
}

Ord * ChildOrdMgr::ReceiveFailResp(int teid, string exch,hfs_order_t& order)
{
    ChildOrderKey cok;
    cok.exch = exch;
    cok.teid = teid;
    cok.oseq = order.oseq;
    cok.pid = order.pid;

    auto iter = orders.find(cok);
    if (iter != orders.end())
    {
        Ord * ord = iter->second;
        if (ord->failqty == 0)
        {
            ord->GetFailResp(0,order.algo);
            //int qty = ord->GetFailResp(0,order.algo);
            //ord->parentOrd->GetFailResp(qty,order.algo);
            hfs_order_t failResp = ord->parentOrd->ToHfsFailResp(order.qty);
            LOG_DEBUG("Receive Fail Resp: found child ord!  exch:{},teid:{},pid:{},oseq:{},{}", cok.exch,cok.teid,cok.pid,cok.oseq,ord->ToString() );
            comm.onResponse(teid, exch, failResp);
            return ord;
        }
        else 
        {
            return nullptr;
        }
    }
    else
    {
        LOG_ERROR("ERROR: Receive Fail Resp, not not exist child ord!: {}/{}/{}", cok.exch,cok.teid,cok.pid);
        return nullptr; 
    }
}


void ChildOrdMgr::CancelAll()
{

    for (auto o= orders.begin();o!=orders.end();o++ )
    {
        Ord * ord = o->second;
        if ((ord->nqty>0 || ord->pnqty>0) && ord->pcqty < ord->qty)
        {   
            ord->Cancel();
            hfs_order_t cancelOrder = ord->ToHfsOrdCancel(); 
            adMgr.onRequest(ord->teid, cancelOrder);
        }
    }
}

void ChildOrdMgr::RejectAll()
{
     for (auto o= orders.begin();o!=orders.end();o++ )
    {
        Ord * ord = o->second;
        if ((ord->nqty>0) )
        {   
            ord->GetFailResp(0,"Admin");
            hfs_order_t failResp = ord->parentOrd->ToHfsFailResp(ord->nqty);
            adMgr.onRequest(ord->teid, failResp);
        }
        else  if ((ord->pnqty>0) )
        {   
            ord->GetFailResp(0,"Admin");
            hfs_order_t failResp = ord->parentOrd->ToHfsFailResp(ord->pnqty);
            adMgr.onRequest(ord->teid, failResp);
        } 

    }
}
