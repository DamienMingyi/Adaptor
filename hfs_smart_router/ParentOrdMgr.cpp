#include "ParentOrdMgr.hpp"
#include "stdlib.h"
ParentOrdMgr::ParentOrdMgr(hfs_adaptor_mgr & _adMgr,Communicator &_comm,bool _selfBarginCtl)
:adMgr(_adMgr),comm(_comm),selfBarginCtl(_selfBarginCtl)
{

}

Ord* ParentOrdMgr::NewIns(int teid, hfs_order_t& origal_order)
{
    ParentOrderKey pok;
    pok.teid = teid;
    pok.oseq = origal_order.oseq;
    pok.pid = origal_order.pid;


    auto iter = orders.find(pok);
    if (iter == orders.end())
    {
        comm.onResponse(teid, "PendingNew", origal_order);
        Ord *ord = new Ord(nullptr,origal_order.qty,teid,origal_order);//memery leak

        ord->time = origal_order.tm; //TODO:get current system time;
        orders[pok] = ord;
        LOG_DEBUG("NewIns: Insert parent ord! teid:{},pid:{},oseq:{},{}", pok.teid,pok.pid,pok.oseq,ord->ToString() );
        for(vector<ParentOrderKey>::iterator it = unfinishedOrd.begin();it != unfinishedOrd.end();)
        {
            if(!orders[*it]->isNew())
            {
                it = unfinishedOrd.erase(it);
            }
            else
            {
                if (orders[*it]->tkr.compare(origal_order.symbol) ==0 )
                {
                    if ((origal_order.side=='B' && orders[*it]->side == 'S' && origal_order.prc - orders[*it]->prc > -0.0001)||
                    (origal_order.side=='S' && orders[*it]->side == 'B' &&  orders[*it]->prc - origal_order.prc  > -0.0001))
                    {
                        char err_msg[128];
                        sprintf(err_msg,"Self bargin risk! orderid:%d,traderid:%d",orders[*it]->pid,orders[*it]->oseq);
                        ord->GetFailResp(origal_order.qty,err_msg);     
                        LOG_DEBUG(err_msg);
                        origal_order.state = HFS_ORDER_TYPE_ENTER_REJ;
                        comm.onResponse(teid, "Failed", origal_order);//直接返回错误                  
                        return nullptr;
                    }
                }
                ++it;
            }
        }

        ord->ToRedisStr();
        if (selfBarginCtl)
        {
            unfinishedOrd.push_back(pok);
        }
        return ord;
    }
    else
    {
        
        LOG_ERROR("ERROR: exist parent ord!: teid:{}/oseq:{}/pid:{}", pok.teid, pok.oseq,pok.pid);
        return nullptr;
    }
}

void ParentOrdMgr::CancelIns(int teid,hfs_order_t& origal_order)
{
    ParentOrderKey pok;
    pok.teid = teid;
    pok.oseq = origal_order.oseq;
    pok.pid = origal_order.pid;
    auto iter = orders.find(pok);
    if (iter != orders.end())
    {
        Ord * ord = iter->second;
        ord->Cancel();
        for (Ord* childOrd : ord->childrenOrd)
        {
            if ((childOrd->nqty>0 || childOrd->pnqty>0) && childOrd->pcqty < childOrd->qty)
            {
                childOrd->Cancel();
                hfs_order_t cancelOrder = childOrd->ToHfsOrdCancel(); 
                adMgr.onRequest(teid, cancelOrder);
            }
        }

    }
    else 
    {
        LOG_ERROR("ERROR: does not existed parent ord!teid:{}/oseq:{}/pid:{}", pok.teid, pok.oseq,pok.pid);
        //TODO： order.state = Cancel_Failed; 
        //comm.onResponse(teid, "Failed", origal_order);//直接返回错误
    }
    
}

void ParentOrdMgr::SetNotEnoughResp(int teid,Ord* ord,hfs_order_t& origal_order,int qty)
{
    ord->err.append("Insufficient Position or Balance!");
    ord->SetFailResp(qty);
    origal_order.qty = qty;
    origal_order.state = HFS_ORDER_TYPE_ENTER_REJ;
    origal_order.type = HFS_ORDER_TYPE_ENTER_REJ;
    
    comm.onResponse(teid, "Failed", origal_order);//直接返回错误
}

void ParentOrdMgr::SetFundNotEnoughResp(int teid,Ord* ord,hfs_order_t& origal_order,float availFund)
{
    char tmp[32]={0};
    sprintf(tmp,"%.2f",availFund);
    ord->err.append("Insufficient assigned fund! left:");
    ord->err.append(tmp);
    origal_order.state = HFS_ORDER_TYPE_ENTER_REJ;
    origal_order.type = HFS_ORDER_TYPE_ENTER_REJ;
    ord->SetFailResp(origal_order.qty);
    comm.onResponse(teid, "Failed", origal_order);//直接返回错误
}