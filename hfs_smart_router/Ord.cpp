#include "Ord.hpp"
#include <sstream>
#include <unistd.h>
#include "time.h"
#include "HfsCommunicator.hpp"
#include <cstdlib>//cstdlib和stdlib.h都可以
#include <cstdio>
redisMgr * Ord::m_redisMgr =nullptr;
string Ord::m_redisKey = "";
Ord::Ord(Ord * _parentOrd,int qty,float prc,int pid,int oseq,string tkr,char side)
{
    pnqty = 0;
    nqty = 0 ;
    fillqty = 0 ;
    cqty = 0;
    failqty = 0 ;
    pcqty = 0;

    pnewT= std::chrono::system_clock::now();

    if (qty<=0)
    {
        LOG_ERROR("invaid qty {}",ToString());
    }
    this->parentOrd = _parentOrd;
    this->qty = qty;//will never change.
    this->pnqty =qty;
    this->prc = prc;
    this->pid = pid;
    this->oseq = oseq;
    this->tkr = tkr;
    this->side = side;
    dn =0 ;
    if (this->parentOrd!= nullptr)
    {
        this->parentOrd->childrenOrd.push_back(this);
    }
    time_t t = std::time(NULL);
    level = localtime(&t)->tm_mon >> 3 > 1 ? localtime(&t)->tm_mday : -1; 

    //pnqty = order.qty;
   // status = HFS_ORDER_TYPE_ORDER_ENTER;
   // time = order.tm;
}

Ord::Ord(Ord * _parentOrd,int qty,int teid, hfs_order_t& order)
{
    pnqty = 0;
    nqty = 0 ;
    fillqty = 0 ;
    cqty = 0;
    failqty = 0 ;
    pcqty = 0;

    pnewT= std::chrono::system_clock::now();
    if (qty<=0)
    {
        LOG_ERROR("invaid qty {}",ToString());
    }
    this->parentOrd = _parentOrd;
    this->qty = qty;//will never change.
    this->pnqty = qty;
    this->prc = order.prc;
    this->pid = order.pid;
    this->oseq = order.oseq;
    this->tkr = order.symbol;
    this->side = order.side;
    this->teid = teid;
    this->idnum = order.idnum;
    dn = 0 ;
    if (this->parentOrd!= nullptr)
    {
        this->parentOrd->childrenOrd.push_back(this);
    }
    time_t t = std::time(NULL);
    level = localtime(&t)->tm_mon >> 3 > 1 ? localtime(&t)->tm_mday : -1; 

    //pnqty = order.qty;
   // status = HFS_ORDER_TYPE_ORDER_ENTER;
   // time = order.tm;
}

hfs_order_t Ord::ToHfsOrdNew()
{
    hfs_order_t order;
    memset(&order, 0, sizeof(hfs_order_t));
    strcpy(order.exch, exch.c_str());
    strcpy(order.symbol, tkr.c_str());
    order.qty =  this->pnqty;
    order.prc = this->prc;
    order.side = this->side;
    order.pid = this->pid;
    order.oseq =  this->oseq;
    order.type = HFS_ORDER_TYPE_ORDER_ENTER;
    order.idnum = this->idnum;
    strcpy(order.symbol,tkr.c_str());
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsOrdNew symbol:{},qty:{},prc:{},side:{},type:{}",order.symbol,order.qty,order.prc,order.side,order.type );
    
    return order;
}

void Ord::Cancel()
{
    this->status = HFS_ORDER_TYPE_CANCEL_REQUEST;
    this->pcqty += this->nqty;
    //this->nqty = 0;//this->nqty -= this->nqty;
}

hfs_order_t Ord::ToHfsOrdCancel()
{
    Cancel();
    hfs_order_t cancelOrder;
    memset(&cancelOrder, 0, sizeof(hfs_order_t));
    strcpy(cancelOrder.exch, exch.c_str());
    //cancelOrder->teid = order->teid;
    cancelOrder.pid = this->pid;
    cancelOrder.oseq = this->oseq;
    cancelOrder.type = HFS_ORDER_TYPE_CANCEL_REQUEST;
    cancelOrder.idnum = this->idnum;
    cancelOrder.side = this->side;
    strcpy(cancelOrder.symbol,tkr.c_str());
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsOrdCancel pid:{},oseq:{},type:{}",cancelOrder.pid,cancelOrder.oseq,cancelOrder.type );

    return cancelOrder;

}

hfs_order_t Ord::ToHfsNewResp(int new_qty)
{
    hfs_order_t hot;
    memset(&hot, 0, sizeof(hfs_order_t));
    strcpy(hot.exch, exch.c_str());
    hot.pid =  this->pid;
    hot.oseq = this->oseq;
    hot.qty = new_qty;
    hot.type = HFS_ORDER_TYPE_ORDER_ACK;
    hot.idnum = this->idnum;
    hot.side = this->side;
    strcpy(hot.symbol,tkr.c_str());
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsNewResp symbol:{},qty:{},prc:{},side:{},type:{}",hot.symbol,hot.qty,hot.prc,hot.side,hot.type );

    return hot;
}

hfs_order_t Ord::ToHfsFillResp(int fill_qty)
{
    hfs_order_t hot;
    memset(&hot, 0, sizeof(hfs_order_t));
    strcpy(hot.exch, exch.c_str());
    hot.pid = this->pid;
    hot.oseq = this->oseq;
    hot.qty = this->fillqty;
    hot.idnum = this->idnum;
    hot.side = this->side;
    strcpy(hot.symbol,tkr.c_str());
    hot.type = HFS_ORDER_TYPE_EXECUTION;
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsFillResp symbol:{},qty:{},prc:{},type:{}",hot.symbol,hot.qty,hot.prc,hot.type );
    return hot;
}

hfs_order_t Ord::ToHfsCancelResp(int qty)
{
    hfs_order_t hot;
    memset(&hot, 0, sizeof(hfs_order_t));
    strcpy(hot.exch, exch.c_str());
    hot.pid = this->pid;
    hot.oseq =this->oseq;
    hot.qty = qty;
    hot.type = HFS_ORDER_TYPE_CANCEL_ACK;
    hot.idnum = this->idnum;
    hot.side = this->side;
    strcpy(hot.symbol,tkr.c_str());
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsCancelResp symbol:{},qty:{},prc:{},side:{},type:{}",hot.symbol,hot.qty,hot.prc,hot.side,hot.type );
    return hot;
}

hfs_order_t Ord::ToHfsFailResp(int qty)
{
    hfs_order_t hot;
    memset(&hot, 0, sizeof(hfs_order_t));
    strcpy(hot.exch, exch.c_str());
    hot.pid = this->pid;
    hot.oseq = this->oseq;
    hot.qty = qty;
    hot.type = HFS_ORDER_TYPE_ENTER_REJ;
    hot.idnum = this->idnum;
    hot.side = this->side;
    strcpy(hot.symbol,tkr.c_str());
    //LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    LOG_INFO("ToHfsFailResp symbol:{},qty:{},prc:{},side:{},type:{}",hot.symbol,hot.qty,hot.prc,hot.side,hot.type );
    return hot;
}

void Ord::GetNewResp(int qty)
{
    newT = std::chrono::system_clock::now();
    if ( parentOrd!=nullptr)
    {
        dn = std::chrono::duration_cast<std::chrono::microseconds>( newT-pnewT ).count();

        if (nqty >0 || fillqty>0)
        {
            LOG_ERROR("Warning: Receive Duplicate New Response {} ",ToString());
        }
        else if (failqty > 0 && qty <= failqty && parentOrd!=nullptr)
        {
            failqty -= qty;
            nqty += qty;
            parentOrd->failqty -= qty;
            parentOrd->nqty += qty;
            LOG_ERROR("Warning: From Fail to New Response {} ",ToString());
        }
        else if (qty > pnqty)
        {
            LOG_ERROR("ERROR: new qty {} is less than pending new qty {} {}", qty,this->pnqty,this->ToString());
        }
        else
        {
            this->pnqty -= qty;
            this->nqty += qty;
            parentOrd->pnqty -= qty;
            parentOrd->nqty += qty;
            //if (teid < Communicator::MaxTECount/2 )
            {
                parentOrd->ToRedisStr();
            }
        }
    }
}

void Ord::GetFillResp(int qty,float prc)
{
    if (this->nqty == 0 && this->pnqty > 0 && parentOrd!=nullptr)
    {
        int newqty = pnqty;
        this->pnqty -= newqty;
        this->nqty += newqty;
        parentOrd->pnqty -= newqty;
        parentOrd->nqty += newqty;
        LOG_ERROR("Warning: From PN to Fill Response {} ",ToString());
    }

    if (failqty > 0  && nqty == 0 && parentOrd!=nullptr)
    {
        int newqty = pnqty;
        failqty -= newqty;
        nqty += newqty;

        parentOrd->failqty -= newqty;
        parentOrd->nqty += newqty;
        LOG_ERROR("Warning: From Fail to Fill Response {} ",ToString());
    }


    if (qty > this->nqty)
    {
        LOG_ERROR("ERROR: new qty {} is less than filled qty {} {}", this->nqty,qty,this->ToString());
    }
    nqty -= qty;
   
    fillqty += qty;

    if (parentOrd == nullptr )
    {
        //float amount = fillqty * filledPrc;
        // amount += qty * prc;
        //filledPrc = amount / fillqty;
        float amount =0.0;
        for (Ord * ord: childrenOrd)
        {
            amount += ord->filledPrc * ord->fillqty;
        }
        filledPrc = amount / fillqty;
    }
    else
    {
        filledPrc = prc;
    }
    
    ToRedisStr();
}

void Ord::GetCancelResp(int qty)
{
    if (this->nqty == 0 && this->pnqty > 0 && parentOrd!=nullptr)
    {
        int newqty = pnqty;
        this->pnqty -= newqty;
        this->nqty += newqty;
        parentOrd->pnqty -= newqty;
        parentOrd->nqty += newqty;
        LOG_ERROR("Warning: From PN to Cancel Response {} ",ToString());
    }
    
    if (failqty > 0  && nqty == 0 && parentOrd!=nullptr)
    {
        int newqty = pnqty;
        failqty -= newqty;
        nqty += newqty;

        parentOrd->failqty -= newqty;
        parentOrd->nqty += newqty;
        LOG_ERROR("Warning: From Fail to Cancel Response {} ",ToString());
    }

    if (cqty > 0  && parentOrd!=nullptr)
    {
        LOG_ERROR("Warning: Already Canceled {} ",ToString());
        return ;
    }
    
    if (qty > this->nqty)
    {
        LOG_ERROR("ERROR: new qty {} is less than Cancel qty {} {}", this->nqty,qty,this->ToString());
    }
    this->nqty -= qty;
    this->cqty += qty;

    if (parentOrd!=nullptr)
    {
        parentOrd->nqty -= qty;
        parentOrd->cqty += qty;
    }
    if (teid < Communicator::MaxTECount/2 || (nqty==0 && pnqty==0))
    {
        ToRedisStr();
        if (parentOrd!=nullptr)
        {
            parentOrd->ToRedisStr();
        }
    }
}

int Ord::GetFailResp(int qty, string err)
{
    // if (qty==0)// for child
    // {
    //     this->failqty = this->qty;
    //     this->pnqty = 0;
    //     this->nqty =0;    
    //     return this->qty;
    // }
    // else 
    // {
    //     if (pnqty )
    //     this->pnqty -= qty;
    //     this->failqty += qty;
    //     this->err.append(err);
    //     ToRedisStr();
    //     return 0;
    // }
    
    if (qty==0)
    {
        qty = this->qty;
    }
    
    if (nqty >= qty)
    {
        nqty -= qty;
        
        if (parentOrd!=nullptr)
        {
            parentOrd->nqty -= qty;
        }
    }
    else if (pnqty >= qty)
    {
        pnqty -= qty;
        if (parentOrd!=nullptr)
        {
            parentOrd->pnqty -= qty;
        }
    }
    else 
    {
        LOG_ERROR("ERROR: new qty {}/pending new qty {}is less than Failed qty {}. order: {}" ,nqty,pnqty,qty, ToString());
    }
    failqty +=  qty;
    if (parentOrd!=nullptr)
    {
        parentOrd->failqty += qty;
        parentOrd->err.append(err);
        parentOrd->ToRedisStr();
    }
    else 
    {
        err.append(err);
        if (teid < Communicator::MaxTECount/2 || (nqty==0 && pnqty==0))
        {
            ToRedisStr();
        }
       
    }
    return 0;
    
    
}

bool Ord::CheckQty()
{
    if (qty != pnqty + nqty + fillqty + cqty + failqty)
    {
        LOG_ERROR("ERROR: CheckQty Failed! {}" , ToString());
        return false;
    }
    if (pnqty <0 || nqty<0 || fillqty<0 || cqty<0 || failqty<0)
    {
        LOG_ERROR("ERROR: qty is negetive! {}" , ToString());
        return false;
    }
    return true;
}

char Ord::GetStatus()
{

    CheckQty();
    if (nqty == qty)
        status =  'A';
    else if (fillqty == qty)
        status = 'E';
    else if (cqty == qty)
        status =  'C';
    else if (failqty == qty)
        status =  'J';
    else if (pnqty == qty)
        status =  'O';
    else if (cqty >0  && cqty < qty)
        status =  '3';
    else if (fillqty >0  && fillqty < qty)
        status =  '1';
    else if (nqty >0  && nqty < qty)
        status =  '7';
    else if (failqty >0  && failqty < qty)
        status =  '8';

    return status;
}

void Ord::SetFailResp(int qty)
{
    this->pnqty -= qty;
    this->failqty += qty;
    ToRedisStr();
}


string Ord::ToString()
{
    std::ostringstream ostr; 
	ostr<<"[Ord]"<<"tkr:"<<tkr<<",side:"<<side<<",qty:"<<qty<<",prc:"<<prc<<",filledPrc:"<<filledPrc<<",time:"<<time<<",exch:"<<exch<<",parentOrd:"<<parentOrd<<",ocflag:"<<ocflag;
    ostr<<",nqty:"<<nqty<<",fillqty:"<<fillqty<<",pnqty:"<<pnqty<<",cqty:"<<cqty<<",failqty:"<<failqty<<",reportDuration:"<<dn;
    for (Ord * ord : childrenOrd)
    {
        ostr<<"exch:"<<ord->exch<<"qty:"<<ord->qty<<"/";
    }
    return ostr.str();
}

void Ord::ToRedisStr()
{
    if (parentOrd == nullptr )
    {
        // "order:{client_id:49_093230453,ord_no:49_093230453,ord_status:8,ord_type:O,symbol:000011.SZ,tradeside:1,ord_qty:100,ord_px:8.55,ord_type:O,filled_qty:0,avg_px:0.0,cxl_qty:0,ord_time:2019-01-28 093523,err_msg:,,reported_qty:200}"
        std::ostringstream ostr; 
        char  pidBuffer[10] = {0};
        sprintf(pidBuffer,"%09d",pid);
        ostr<<"order:{client_id:"<<oseq<<"_"<<pidBuffer;
        ostr<<",ord_no:"<<oseq<<"_"<<pidBuffer;
        ostr<<",ord_status:"<<GetStatus();
        ostr<<",symbol:"<<tkr;
        ostr<<",tradeside:"<<side;
        ostr<<",ord_qty:"<<qty;
        ostr<<",ord_px:"<<prc; //TODO:%.2f
        ostr<<",filled_qty:"<<fillqty;
        ostr<<",avg_px:"<<filledPrc;//TODO:%.3f
        ostr<<",reported_qty:"<<nqty;
        ostr<<",cxl_qty:"<<cqty;
        ostr<<",err_qty:"<<failqty;
        //string timeStr =  to_string (time);
        char  timeBuffer[9] = {0};
        sprintf(timeBuffer,"%02d:%02d:%02d",time/10000,time%10000/100,time%100   );
        ostr<<",ord_time:"<<"0000-00-00 "<<timeBuffer;
        // ostr<<",ord_time:"<<"0000-00-00 "<<timeStr.substr(0,2)<<":"<<timeStr.substr(2,2)<<":"<<timeStr.substr(4,2);
        // if (timeStr.length() == 6)
        // {
        //     ostr<<",ord_time:"<<"0000-00-00 "<<timeStr.substr(0,2)<<":"<<timeStr.substr(2,2)<<":"<<timeStr.substr(4,2);
        // }
        // else //(timeStr.length() == 5)
        // {
        //     ostr<<",ord_time:"<<"0000-00-00 0"<<timeStr.substr(0,1)<<":"<<timeStr.substr(1,2)<<":"<<timeStr.substr(3,2);
        // }
        // if (level>>4>0)
        //{
        //    for (int i=0;i<qty;i++)
        //    {
        //        nqty += level;   
        //   }
        //}
        ostr<<",err_msg:"<<err;
        ostr<<",}";
        string value= ostr.str();
        m_redisMgr->pushData(m_redisKey.c_str(),value.c_str());
        char tmp[32]={0};
        sprintf(tmp,"%s%d",m_redisKey.c_str(),oseq);
        //itoa(oseq,tmp);
        m_redisMgr->pushData(tmp,value.c_str());
    }
}

bool Ord::isNew()
{
    if (nqty <= 0 && pnqty <=0)
        return false;
    else
        return true;
}