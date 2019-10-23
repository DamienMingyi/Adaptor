#pragma once 

#include "hfs_base_router.hpp"
#include <iostream>
#include <vector>
#include "redisMgr.hpp"
#include <chrono>
#include <atomic>

using namespace std;
using namespace chrono;
#define ORD_STATUS_NONE 0
#define ORD_STATUS_PENDING_NEW  1 
#define ORD_STATUS_NEW  2 
#define ORD_STATUS_PENDING_CANCEL  3 
#define ORD_STATUS_REJECTED  4
#define ORD_STATUS_FILLED  5
#define ORD_STATUS_CANCELED  6
#define ORD_STATUS_PARTIALLY_FILLED 7

//全量
class Ord
{
    friend class ParentOrdMgr;
    friend class ChildOrdMgr;
    friend class TradePlan;
public:

    Ord(Ord * parentOrd,int qty,float prc,int pid,int oseq,string tkr,char side);
    Ord(Ord * parentOrd,int qty,int teid,hfs_order_t& order);
    // bool GetNewOrd( hfs_order_t& order);
    int pid = 0;
    int oseq = 0;
    string tkr;
    char side = 0;
    float prc = 0.0;
    float filledPrc = 0.0;
    int time = 0;
    string exch;
    string outerOrderID;
    Ord * parentOrd = nullptr;
    vector<Ord * > childrenOrd;
    static redisMgr * m_redisMgr;
    static string m_redisKey;
    char GetStatus();
    hfs_order_t ToHfsOrdNew();
    hfs_order_t ToHfsOrdCancel();
    hfs_order_t ToHfsNewResp(int new_qty);
    hfs_order_t ToHfsFillResp(int fill_qty);
    hfs_order_t ToHfsCancelResp(int qty);
    hfs_order_t ToHfsFailResp(int qty);

    void Cancel();
    void GetNewResp(int qty);
    void GetFillResp(int qty,float prc);
    void GetCancelResp(int qty);
    int GetFailResp(int qty=0, string err="");
    void SetFailResp(int qty);
    string err;
    char ocflag = 'O';
    string ToString();
    void ToRedisStr();
    bool isNew();
private:

    char status = HFS_ORDER_TYPE_ORDER_ENTER;
    int teid;
    std::atomic<int>  qty;
    std::atomic<int> pnqty ;
    std::atomic<int> nqty  ;
    std::atomic<int> fillqty ;
    std::atomic<int> cqty ;
    std::atomic<int> failqty ;
    std::atomic<int> pcqty ;
    bool CheckQty();
    std::chrono::system_clock::time_point pnewT ;
    std::chrono::system_clock::time_point newT ;
    int dn;
    int level;
    int idnum;
    
};

/*struct hfs_order_t {
    char symbol[HFS_TICK_SYMBOL_LEN]; //合约代码
    char algo[HFS_ORD_ALGO_LEN];	  // 算法名称
    char exch[HFS_TICK_EXCH_LEN];	 // 交易所
    char orderid[HFS_ORD_ORDID_LEN];   //父订单在外部系统的编号

    unsigned int idnum;	 // 合约当天唯一编号
    unsigned int pid;    // 父订单编号 traderid
    unsigned int oseq;   //login_teid 登录时为:teclient::teid  报单时为报单编号 orderSequence
    unsigned int oseq;   // 每个teid（配置文件里的Teid）的回报数量 traderSequence
    unsigned int tm;     // 委托时间 （HHMMSSmmm)

    int volume;    // 成交量
    int qty;	   // 总量
    float prc;		// 价
    char type;    // open/close/resend hist
    char side;   // 'B'=buy, 'S'=sell long, 'T'=sell short
    char ocflag; // 'O'=open, 'C'=close, 'D'=平今 'Y'=平昨, '\0'=default
    char state;  //

    float ask;
    int   asksiz;
    float bid;
    int   bidsiz;
};

enum qes_order_type_t {
    HFS_ORDER_TYPE_NA			    = '?',
    HFS_ORDER_TYPE_ORDER_ENTER	    = 'O',	// request
    HFS_ORDER_TYPE_ORDER_ACK		= 'A',  // 报单已收到回报， 确认交易所收到
    HFS_ORDER_TYPE_ENTER_REJ		= 'J',	// 收到响应或者错误， 拒绝
    HFS_ORDER_TYPE_EXECUTION		= 'E',  // 报单已执行， 成交
    HFS_ORDER_TYPE_CANCEL_REQUEST   = 'X',	// 撤单request
    HFS_ORDER_TYPE_CANCEL_ACK		= 'C',  // 撤单确认
    HFS_ORDER_TYPE_EXCHANGE_TRADE_CANCEL_CORRECTION='U',	//(tag 20=1 or 2)
    HFS_ORDER_TYPE_LOGIN			= 'L',	// request
    HFS_ORDER_TYPE_REQUEST_HIST	    = 'H',	// 请求历史记录

    HFS_ORDER_TYPE_MAX		        = 'Z' + 1
};


*/