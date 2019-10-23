#pragma once

#include "xtp_trader_api.h"
#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
// #include "hfs_trade_utils.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_data_api.hpp"
#include "hfs_risk_control.hpp"
#include "hfs_database.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_data_api.hpp"
#include "hfs_risk_control.hpp"
#include <mutex>
#include <map>

using namespace XTP::API;

#pragma pack(push,1)

struct EntrustNode {
    OrderID nid;
    uint64_t    sid;
    char    tkr[512];
    int     qty;
    float   amt;
    int     eqty;
    float   eamt;
    int     canceled;              //0,表示没有撤单；>0,表示撤单量
    hfs_order_t origOrder;         //委托时的订单，后续填表有用
};

#pragma pack(pop)

typedef map<OrderID, EntrustNode *, OrderIDCmp>   Nid2EntrustMap;
typedef map<uint64_t, EntrustNode *>           Sid2EntrustMap;

class hfs_xtp_adaptor : public hfs_base_adaptor, public TraderSpi{
public:
    hfs_xtp_adaptor(const a2ftool::XmlNode &cfg);
    virtual ~hfs_xtp_adaptor() {};
    virtual int Init();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn(){return m_state == 2;};
    virtual bool onRequest(int teid, hfs_order_t &order);

    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void push_reject_order(int teid, hfs_order_t &order, const char* errmsg);

    bool registerTrader(int teid, const hfs_order_t &order, uint64_t xtp_id);
    EntrustNode *getENode(OrderID& oid);
    EntrustNode *getENode(uint64_t sid);

    std::string getOPStation(int traderid);
public:
    ///当客户端的某个连接与交易后台通信连接断开时，该方法被调用。
    ///@param reason 错误原因，请与错误代码表对应
    ///@param session_id 资金账户对应的session_id，登录时得到
    ///@remark 用户主动调用logout导致的断线，不会触发此函数。api不会自动重连，当断线发生时，请用户自行选择后续操作，可以在此函数中调用Login重新登录，并更新session_id，此时用户收到的数据跟断线之前是连续的
    virtual void OnDisconnected(uint64_t session_id, int reason);

    ///错误应答
    ///@param error_info 当服务器响应发生错误时的具体的错误代码和错误信息,当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@remark 此函数只有在服务器发生错误时才会调用，一般无需用户处理
    virtual void OnError(XTPRI *error_info);

    ///报单通知
    ///@param order_info 订单响应具体信息，用户可以通过order_info.order_xtp_id来管理订单，通过GetClientIDByXTPID() == client_id来过滤自己的订单，order_info.qty_left字段在订单为未成交、部成、全成、废单状态时，表示此订单还没有成交的数量，在部撤、全撤状态时，表示此订单被撤的数量。order_info.order_cancel_xtp_id为其所对应的撤单ID，不为0时表示此单被撤成功
    ///@param error_info 订单被拒绝或者发生错误时错误代码和错误信息，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@remark 每次订单状态更新时，都会被调用，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线，在订单未成交、全部成交、全部撤单、部分撤单、已拒绝这些状态时会有响应，对于部分成交的情况，请由订单的成交回报来自行确认。所有登录了此用户的客户端都将收到此用户的订单响应
    virtual void OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id);

    ///成交通知
    ///@param trade_info 成交回报的具体信息，用户可以通过trade_info.order_xtp_id来管理订单，通过GetClientIDByXTPID() == client_id来过滤自己的订单。对于上交所，exec_id可以唯一标识一笔成交。当发现2笔成交回报拥有相同的exec_id，则可以认为此笔交易自成交了。对于深交所，exec_id是唯一的，暂时无此判断机制。report_index+market字段可以组成唯一标识表示成交回报。
    ///@remark 订单有成交发生的时候，会被调用，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线。所有登录了此用户的客户端都将收到此用户的成交回报。相关订单为部成状态，需要用户通过成交回报的成交数量来确定，OnOrderEvent()不会推送部成状态。
    virtual void OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id);

    ///撤单出错响应
    ///@param cancel_info 撤单具体信息，包括撤单的order_cancel_xtp_id和待撤单的order_xtp_id
    ///@param error_info 撤单被拒绝或者发生错误时错误代码和错误信息，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@remark 此响应只会在撤单发生错误时被回调
    virtual void OnCancelOrderError(XTPOrderCancelInfo *cancel_info, XTPRI *error_info, uint64_t session_id);

    ///请求查询报单响应
    ///@param order_info 查询到的一个报单
    ///@param error_info 查询报单时发生错误时，返回的错误信息，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@param request_id 此消息响应函数对应的请求ID
    ///@param is_last 此消息响应函数是否为request_id这条请求所对应的最后一个响应，当为最后一个的时候为true，如果为false，表示还有其他后续消息响应
    ///@remark 由于支持分时段查询，一个查询请求可能对应多个响应，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线
    virtual void OnQueryOrder(XTPQueryOrderRsp *order_info, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);

    ///请求查询成交响应
    ///@param trade_info 查询到的一个成交回报
    ///@param error_info 查询成交回报发生错误时返回的错误信息，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@param request_id 此消息响应函数对应的请求ID
    ///@param is_last 此消息响应函数是否为request_id这条请求所对应的最后一个响应，当为最后一个的时候为true，如果为false，表示还有其他后续消息响应
    ///@remark 由于支持分时段查询，一个查询请求可能对应多个响应，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线
    virtual void OnQueryTrade(XTPQueryTradeRsp *trade_info, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);

    ///请求查询投资者持仓响应
    ///@param position 查询到的一只股票的持仓情况
    ///@param error_info 查询账户持仓发生错误时返回的错误信息，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@param request_id 此消息响应函数对应的请求ID
    ///@param is_last 此消息响应函数是否为request_id这条请求所对应的最后一个响应，当为最后一个的时候为true，如果为false，表示还有其他后续消息响应
    ///@remark 由于用户可能持有多个股票，一个查询请求可能对应多个响应，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线
    virtual void OnQueryPosition(XTPQueryStkPositionRsp *position, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);

    ///请求查询资金账户响应，需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线
    ///@param asset 查询到的资金账户情况
    ///@param error_info 查询资金账户发生错误时返回的错误信息，当error_info为空，或者error_info.error_id为0时，表明没有错误
    ///@param request_id 此消息响应函数对应的请求ID
    ///@param is_last 此消息响应函数是否为request_id这条请求所对应的最后一个响应，当为最后一个的时候为true，如果为false，表示还有其他后续消息响应
    ///@remark 需要快速返回，否则会堵塞后续消息，当堵塞严重时，会触发断线
    virtual void OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id);
  
private:
    void initFile();
    void qryAccount();
    void qryOrders();
    void qryPosition();
    void qryTrade();
    void qryThreadFunc();
    bool IsErrorInfo(XTPRI *error_info);

    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);
private:
    std::string m_id;
    TraderApi *m_pUserApi;
    int m_state; // 状态 -1 异常， 0 初始状态 ， 1 连接前置， 2 登录成功
    long m_orderRef;
    std::string m_sysinfo;

    int m_frontID;
    uint64_t m_sessionID;
    int m_requestID;
    
    string m_accountid;
    string m_passwd;
    string m_accountKey;
    string m_server;
    int m_clientid;
    int m_port;
    std::mutex m_accLock;

    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    //EntrustInfoVec  evec;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;

    int64_t m_qry_begin_time = 0; // 查询开始时间
    std::string m_export_path;
    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;
    std::ofstream m_account_file;

    RiskControl *m_rc;
    std::unordered_map<int, std::string> m_opstation;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_xtp_adaptor(_cfg);
  }
};
