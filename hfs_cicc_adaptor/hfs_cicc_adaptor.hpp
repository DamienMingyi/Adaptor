#pragma once 

#include <stdio.h>
#include "hfs_base_adaptor.hpp"
#include "xml_parser.hpp"
#include "hfs_trade_utils.hpp"
#include "hfs_base_univ.hpp"
#include "hfs_data_api.hpp"
#include "hfs_risk_control.hpp"
#include "hfs_database.hpp"
#include "IMSTradeAPI.h"
#include <mutex>
#include <map>

#pragma pack(push,1)

struct EntrustNode {
    OrderID nid;
    char    sid[512];
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
typedef map<std::string, EntrustNode *>           Sid2EntrustMap;


class hfs_cicc_adaptor : public IMSTradeSPI , public hfs_base_adaptor
{
public:
    hfs_cicc_adaptor(const a2ftool::XmlNode &cfg);
    virtual ~hfs_cicc_adaptor();
    // 0 成功  -1 失败
    virtual int Init();

    virtual bool start();
    virtual bool stop();
    virtual bool isLoggedOn(){return m_state == 2;};
    virtual bool onRequest(int teid, hfs_order_t &order);

    virtual const char* GetLastError() {return "";};
    virtual bool OnRecall(const RecallField* pRspRecall) override { return true; }	// Recall信息响应
	// 委托类回调
	virtual bool OnRspNewOrder(const NewOrderRspField* pRspNewOrd, const ErrMsgField* errMsg, int requestId, bool last) override ;// 报单请求响应
	virtual bool OnRspCancelOrder(const CancelOrderRspField* pRspCnclOrd, const ErrMsgField * errMsg, int requestId, bool last) override ;// 撤单请求响应
	virtual bool OnRspNewBasketOrder(const NewBasketOrderRspField* pRspNewBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) override { return true; }				// 篮子响应
	virtual bool OnRspNewAlgoBasketOrder(const NewAlgoBasketOrderRspField* pRspNewAlgoBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) override { return true; }	// Algo篮子响应
	virtual bool OnRspAlgoParentOrderAct(const AlgoParentOrderActRspField* pRspAlgoPrntOrdAct, const ErrMsgField* errMsg, int requestId) override { return true; }				// Algo暂停请求响应
	// 推送类回调
	virtual bool OnNotiOrder(const OrderNotiField* pNotiOrd, const ErrMsgField * errMsg);						// 委托推送通知
	virtual bool OnNotiKnock(const KnockNotiField* pNotiKnk, const ErrMsgField * errMsg) override ;		// 成交通知
	virtual bool OnNotiAlgoBasketOrder(const AlgoBasketOrderNotiField* pNotiAlgoBsktOrd, const ErrMsgField * errMsg) override { return true; }// Algo状态推送回调
	// 委托查询类回调
	virtual bool OnRspQryOrder(const QryOrderRspField* pRspQryOrd, const ErrMsgField * errMsg, int requestId, bool last) override ;			// 查询委托请求响应
	virtual bool OnRspQryKnock(const QryKnockRspField* pRspQryKnk, const ErrMsgField * errMsg, int requestId, bool last) override ;			// 查询成交请求响应
	virtual bool OnRspQryBasketOrder(const QryBasketOrderRspField* pRspQryBsktOrd, const ErrMsgField* errMsg, int requestId, bool last) override { return true; }			// 查询篮子请求响应
	virtual bool OnRspQryAlgoParentOrder(const QryAlgoParentOrderRspField* pRspQryAlgoPrntOrd, const ErrMsgField* errMsg, int requestId, bool last) override { return true; }	// 查询Algo指令请求响应
	// 持仓资金查询类回调
	virtual bool OnRspQryPosition(const QryPositionRspField* pRspQryPos, const ErrMsgField * errMsg, int requestId, bool last) override ;						// 查询持仓请求响应
	virtual bool OnRspQryAccount(const QryAccountRspField* pRspQryAcct, const ErrMsgField * errMsg, int requestId, bool last) override ;					// 查询资金请求响应
	virtual bool OnRspQryEqualAccount(const QryEqualAccountRspField* pRspQryEqAcct, const ErrMsgField * errMsg, int requestId, bool last) override { return true; }		// 查询汇率转换后的资金请求响应
	virtual bool OnRspQryPositionLimit(const QryPositionLimitRspField* pRspQryPosLim, const ErrMsgField * errMsg, int requestId, bool last) override { return true; }	// 查询当前额度请求响应
    virtual bool OnRspQryPositionQuantity(const QryPositionQuantityRspField* pRspQryPosQuant, const ErrMsgField * errMsg, int requestId, bool last) override { return true; }		// 查询自有持仓或已交易融券数量请求响应
    
    virtual bool OnRspNewTwapOrder(const NewTwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last){return true;};
	virtual bool OnRspNewVwapOrder(const NewVwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last){return true;};
	virtual bool OnRspNewQuickBasketOrder(const NewQuickBasketOrderRspField* pRspQuickBsktOrd, const ErrMsgField* errMsg, int requestId, bool last){return true;};

private:
    // virtual bool onResponse(int teid, std::string execid, hfs_order_t* order);

    bool process_new_entrust(int teid, hfs_order_t &order);
    bool process_cancel_entrust(int teid, hfs_order_t &order);
    void push_reject_order(int teid, hfs_order_t &order);

    bool registerTrader(int teid, const hfs_order_t &order, string sid);
    EntrustNode *getENode(OrderID& oid);
    EntrustNode *getENode(std::string sid);

    std::string getOPStation(int traderid);
private:
    void qryAccount();
    void  qryPosition();

    void  qryOrder();
    void  qryDetail();

    bool IsErrInfo(const ErrMsgField * errMsg);
    
    void dump(hfs_holdings_t *holding);
    void dump(hfs_order_t *order);
    void decode(char *pstr, int *pkey);
    void encode(char *pstr, int *pkey);
private:
    std::string tradeType;
    std::string m_id;
    std::string m_userID;
    std::string m_password;
    std::string m_sysinfo;
    std::string m_server_ip;
    std::string m_product_id;
    std::string m_productunit_id;
    std::string m_protfolio_id;
    std::string m_export_path;

    int m_orderRef;
    hfs_data_api *dpi;
    hfs_base_univ *base_univ;
    int m_state; // 状态 -1 异常， 0 初始状态 ， 1 连接前置， 2 登录成功

    int m_request_id = 0;

    //对于每一组串行操作，都需要下面两个变量作为控制
    bool success, finished;

    bool m_bHK;   // 是否是沪股通交易

    std::map<std::string, hfs_holdings_t *> m_holdings;
    std::unordered_map<int, std::string > m_request_map;

    std::mutex  m_sid_mtx;
    std::mutex  m_nid_mtx;
    //EntrustInfoVec  evec;
    Sid2EntrustMap  smap;
    Nid2EntrustMap  nmap;

    std::ofstream m_position_file;
    std::ofstream m_detail_file;
    std::ofstream m_entrust_file;

    RiskControl *m_rc;
    hfs_database_mysql *m_sql;
    std::unordered_map<int, std::string> m_opstation;

    IMSTradeAPI* m_api;
};

extern "C" {
  hfs_base_adaptor *create(const a2ftool::XmlNode &_cfg) {
    return new hfs_cicc_adaptor(_cfg);
  }
};
