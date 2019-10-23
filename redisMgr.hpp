#pragma once

#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <mutex>
#include "hfs_log.hpp"

// tradeside (交易方向)
// 数值	含义
// 1	买入
// 2	卖出
// A	融资买入
// B	融券卖出
// C	买券还券
// D	卖券还款
// E	先买券还券，再担保品买入
// F	ETF申购
// G	ETF赎回
// FA	开仓买入
// FB	开仓卖出
// FC	平仓买入
// FD	平仓卖出
// FG	平今买入
// FH	平今卖出
// FI	平昨买入
// FJ	平昨卖出
struct cats_instructs_t {
    char inst_type; // O / C
    char client_id[16];  // 用户自定义ID
    char acct_type[8]; // 账户类型 0 集中交易  F0 深证快速交易  SHF0 上海快速交易
    char acct_id[16];
    char order_no[32];   // 撤单需要
    char symbol[32];     // 
    char tradeside[8];   // 1 买入  2 卖出  
    int ord_qty;   
    float ord_price;
    char ord_type[8];   // 委托类型 0 限价  U 市价 
    char ord_param[32];
};
// order_status (订单状态)
// 数值	含义
// 0	已报
// 1	部分成交
// 2	全部成交
// 3	部分撤单
// 4	全部撤单
// 5	交易所拒单
// 6	柜台未接受
struct cats_order_t {
    char client_id[16];
    char ord_no[32];
    char ord_status[4];
    char acct_type[8];
    char acct_id[16];
    char cats_acct[16];
    char symbol[16];
    char tradeside[8];
    char ord_qty[9];
    char ord_px[8];
    char ord_type[8];
    char ord_param[32];
    char corr_type[32];  // 下单来源
	char corr_id[32];
    char filled_qty[9];
    char avg_px[8];     //成交均价
    char cxl_qty[9];   //撤单数量
    char ord_time[24]; // yyyy-MM-dd HH:mm:ss
    char err_msg[32];
    char write_time[32]; // 
};
// 资金的s1-s10含义
// 字段名	含义	备注
// s1	资金账户	暂时和acct一样，预留字段
// s2	币种	参见数据字典章节
// s3	当前余额	
// s4	可用余额	
// s5	当前保证金	期货账户时才有意义
// s6	冻结保证金	期货账户时才有意义

// 持仓的s1-s10含义
// 字段名	含义	备注
// s1	标的代码	
// s2	当前数量	
// s3	可用数	
// s4	成本价	
// s5	持仓方向	期货账户时才有意义
// s6	投机套保标记	期货账户时才有意义
// s7	昨仓数	期货账户时才有意义
// s8	标的名称		
// s9	市值
struct cats_asset_t {
    char a_type[8];
    char acct_type[8];
    char acct_id[32];
    char s1[32];
    char s2[32];
    char s3[32];
    char s4[32];
    char s5[32];
    char s6[32];
    char s7[32];
    char s8[32];
    char s9[32];
    char s10[32];
};
#include <mutex>
class redisMgr 
{
public:
    redisMgr(const char *hostname, int port=6379){
        struct timeval timeout = {2, 0}; //2s的超时时间
        m_context = (redisContext *)redisConnectWithTimeout(hostname, port, timeout);
        if ((NULL == m_context) || (m_context->err))
        {
            if (m_context)
            {
                std::cout << "connect error:" << m_context->errstr << std::endl;
            }
            else
            {
                std::cout << "connect error: can't allocate redis context." << std::endl;
            }
        }
        if (redisSetTimeout(m_context, timeout) != REDIS_OK)
        {
            LOG_ERROR("set timeout fail");
        }
        redisEnableKeepAlive(m_context);
    }
    ~redisMgr(){
        redisFree(m_context);
    };

    int pushData(const char *key, cats_instructs_t *data) {
        char buf[512] = {0};
        sprintf(buf, "instructs:{inst_type:%c,client_id:%s,acct_type:%s,acct_id:%s,order_no:%s,symbol:%s,tradeside:%s,ord_qty:%d,ord_price:%f,ord_type:%s,ord_param:%s,}", 
                    data->inst_type, data->client_id, data->acct_type, data->acct_id, data->order_no, data->symbol, data->tradeside, 
                    data->ord_qty, data->ord_price, data->ord_type, data->ord_param);
        //m_redisLock.lock();
        redisReply *pRedisReply = (redisReply *)redisCommand(m_context, "LPUSH %s %s", key, buf);
        // m_redisLock.unlock();
        if (!(pRedisReply->type == REDIS_REPLY_INTEGER && pRedisReply->integer > 0)) {
            return -1;
        }
        freeReplyObject(pRedisReply);
        // if (redisAppendCommand(m_context, "LPUSH %s %s", key, buf) != REDIS_OK){
        //     LOG_INFO("cannot append command ");
        //     return -1;
        // }
        return 0;
    }
	
    int pushData(const char *key, const char *data) {
        m_redisLock.lock();
        redisReply *pRedisReply = (redisReply *)redisCommand(m_context, "LPUSH %s %s", key, data);
        m_redisLock.unlock();
        if (!(pRedisReply->type == REDIS_REPLY_INTEGER && pRedisReply->integer > 0)) {
            return -1;
        }
        freeReplyObject(pRedisReply);
        return 0;
    }
    
    std::string popData(const char * key) {
        //m_redisLock.lock();
        redisReply *pRedisReply = (redisReply *)redisCommand(m_context, "RPOP %s",key);
        //m_redisLock.unlock();
        std::string data = (pRedisReply && pRedisReply->len>0)? pRedisReply->str:"";
        freeReplyObject(pRedisReply);
        return data;
    }

    int delKey(const char* key) {
        redisReply *pRedisReply = (redisReply *)redisCommand(m_context, "del %s", key);

        if (!(pRedisReply->type == REDIS_REPLY_INTEGER && pRedisReply->integer > 0)) {
            return -1;
        }
        freeReplyObject(pRedisReply);
        return 0;
    }

  private:
    redisContext *m_context;
    redisReply *m_reply;
    std::mutex m_redisLock;
};


