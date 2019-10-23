#include "HfsSmartRouter.hpp"
#include "hfs_log.hpp"

#include <iostream>
#include "Ord.hpp"
using namespace std;


bool HfsSmartRouter::start() {
    LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
    ORDER_PROC_FUN f = std::bind(&HfsSmartRouter::onResponse, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    adMgr.register_rspstream(f);
    LOG_INFO("Start SmartRouter!");
    tradePlan = new TradePlan(adMgr,comm,cfg);
    return true;
}

bool HfsSmartRouter::stop() {
    return true;
}


bool HfsSmartRouter::onRequest(int teid, hfs_order_t& order)
{
    if(order.qty<0)
        return false;
    switch(order.type) {
        case 'O':
            return tradePlan->OnReceiveIns( teid, order);
            //process_new_entrust(teid, order); //将订单拆分到不同的账户，形成不同的新order
            break;
        case 'X':
            return tradePlan->OnReceiveCancel( teid, order);
            //process_cancel_entrust(teid, order); //试图撤掉所有的外挂订单
            break;
        default:
            LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
            LOG_ERROR("unexpected order type: ", order.type);
            return false;
    };

    return true;
}

bool HfsSmartRouter::onResponse(int teid, std::string execid, hfs_order_t& order){
    //更新这笔主委托的状态，然后回传到Communicator

    // 状态空间：A, J, C, E
    // 假设有两个账户，两个子订单
    // case E, 算作母订单的成交量，返回上层
    // case A, 只要有一个确认，则母订单状态转为A，返回上层
    //    如果另外一个订单是J，则一直不成交，待活跃订单完成后，主动返回C;
    //    这是极少数情况才会遇到：资金（股数）不足、连接不通畅会遇到这种情况
    //case C， 判断另外一个的状态，如果另外一个已经结束，则剩余部分返回撤单成功消息
    if(order.qty<0)
        return false;
    switch(order.type) {
        case 'A':
            return tradePlan->OnReceiveNewResp(teid,execid,order);
        case 'J':
            return tradePlan->OnReceiveFailResp(teid,execid,order);
        case 'C':
            return tradePlan->OnReceiveCancelResp(teid,execid,order);
        case 'E':
            return tradePlan->OnReceiveFillResp(teid,execid,order);
        default:
            LOG_DEBUG("{}::{}",__FILE__, __FUNCTION__);
            LOG_ERROR("unexpected order type: ", order.type);
            return false;
    }

    return true;
}

void HfsSmartRouter::LoadAlgoPos(string exch)
{
    tradePlan->LoadAlgoPos(exch);
}

void HfsSmartRouter::rejectpn(string exch)
{
    tradePlan->RejectPendingNew(exch);
}

void HfsSmartRouter::cancelnew(string exch)
{
    tradePlan->CancelNew(exch);
}

void HfsSmartRouter::LoadT0Pos(string exch)
{
    tradePlan->LoadT0Pos(exch);
}

void HfsSmartRouter::RemoveT0Pos(string exch)
{
    tradePlan->RemoveT0Pos(exch);
}