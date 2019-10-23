#include "HfsSimpleRouter.hpp"
#include "hfs_log.hpp"
#include "HfsCommunicator.hpp"

bool HfsSimpleRouter::onRequest(int teid, hfs_order_t& order){
    LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
    strcpy(order.exch, "ZXXX_F");
    switch(order.type) {
        case 'O':
            for (auto acc : accounts){
                strcpy(order.exch, acc.c_str());
                adMgr.onRequest(teid, order);
            }            
            //process_new_entrust(teid, order); //将订单拆分到不同的账户，形成不同的新order
            break;
        case 'X':
            for (auto acc : accounts){
                strcpy(order.exch, acc.c_str());
                adMgr.onRequest(teid, order);
            }
            //process_cancel_entrust(teid, order); //试图撤掉所有的外挂订单
            break;
        //case 'H':
            //return adap.resendTradeHistory(teid);
        default:
            LOG_ERROR("unexpected order type: ", order.type);
            return false;
    };

    return true;
}

bool HfsSimpleRouter::onResponse(int teid, std::string execid, hfs_order_t& order){
    //更新这笔主委托的状态，然后回传到Communicator

    // 状态空间：A, J, C, E
    // 假设有两个账户，两个子订单
    // case E, 算作母订单的成交量，返回上层
    // case A, 只要有一个确认，则母订单状态转为A，返回上层
    //    如果另外一个订单是J，则一直不成交，待活跃订单完成后，主动返回C;
    //    这是极少数情况才会遇到：资金（股数）不足、连接不通畅会遇到这种情况
    //case C， 判断另外一个的状态，如果另外一个已经结束，则剩余部分返回撤单成功消息
    
    LOG_INFO("{}::{}",__FILE__, __FUNCTION__);
    comm.onResponse(teid, execid, order);
    return true;
}
