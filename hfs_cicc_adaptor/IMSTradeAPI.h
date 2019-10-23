#pragma once

#include <string>
#include <stdint.h>
#if defined(WIN32)
#ifdef LIB_TRADER_API_EXPORT
#define TRADER_API_EXPORT __declspec(dllexport)
#else
#define TRADER_API_EXPORT __declspec(dllimport)
#endif
#else
#ifdef LIB_TRADER_API_EXPORT
#define TRADER_API_EXPORT __attribute__ ((visibility("default")))
#else
#define TRADER_API_EXPORT
#endif
#endif

struct ErrMsgField
{
	int errorCode;//错误编号，0正常
	char errorMsg[256];	//错误信息
};

// Algo普通委托
struct NewOrderField
{
	char productId[16];			// 合约代码
	char productUnitId[16];		// 资产单元代码
	char portfolioId[16];			// 投资组合代码
	char acctId[16];				// 资金账号
	char stkCode[32];				// 合约代码
	char exchId[4];				// 市场代码
	char bsFlag[4];				// 买卖方向
	int64_t orderQty;				// 委托数量
	double orderPrice;			// 委托价格
	char orderPriceType[8];		// 委托类型 Limit-限价 Any-市价
	char matchCondition[8];		// 市价类型
	char currencyId[8];			// 币种代码
	int32_t fundType;				// 资金类型
	char businessType[16];			// 业务类型
	char offsetFlag[8];			// 开平方向
	char hedgeFlag[8];			// 投机套保标志 
	char clOrderId[16];			// 自定义合同号
	char strategyId[16];			// 客户自定义策略ID--API discarded
};

//Algo子单撤单
struct CancelOrderField
{
	char productId[16];			// 合约代码
	char productUnitId[16];		// 资产单元代码
	char portfolioId[16];			// 投资组合代码
	char acctId[16];				// 资金账号
	char stkCode[32];				// 合约代码
	char exchId[4];				// 市场
	char clOrderId[16];			// 自定义合同号
	char orgclOrderId[16];			// 原单的自定义合同号
	char orgContractNum[32];		// 原单柜台合同号
	char orgSerialNum[32];			// 原IMS生成合同序列号
	char businessType[16];			// 业务类型
};

//子单确认
struct NewOrderRspField
{
	char exchId[4];				// 市场代码
	char stkCode[32];				// 合约代码
	char bsFlag[4];				// 买(B)卖(S)
	char clOrderId[16];			// 自定义合同号
	char contractNum[32];			// 生成的柜台合同号
	char serialNum[32];			// IMS生成合同序列号
	double totalAmt;				// 总金额
	double commisionAmt;			// 清算金额
	int64_t orderQty;				// 委托数量
	char strategyId[16];			// 客户自定义策略ID--API discarded
};

//撤单确认
struct CancelOrderRspField
{
	char exchId[4];				// 市场代码
	char stkCode[32];				// 合约代码
	char clOrderId[16];			// 自定义合同号
	char contractNum[32];			// 生成的柜台合同号
	char orgclOrderId[32];			// 原单的自定义合同号
	char serialNum[32];			// IMS生成合同序列号
	char businessType[16];			// 业务类型
};

//成交推送
struct KnockNotiField
{
	char stkCode[32];				// 合约代码
	char exchId[4];				// 市场
	char bsFlag[4];				// 买(B)卖(S)
	int64_t orderQty;				// 委托数量
	double orderPrice;			// 委托价格
	char contractNum[32];			// 柜台合同号
	char serialNum[32];			// IMS生成合同序列号
	char clOrderId[16];			// 自定义合同号
	char execType[16];			// 成交类型
	char orderRejectFlag[16];		// 交易所拒单标志
	int32_t  knockQty;			// 成交数量
	double knockPrice;			// 成交价格
	char knockCode[32];			// 成交编号
	char knockTime[16];			// 成交时间
	int32_t leaveQty;				// 剩余数量
	char orderStatus[32];			// 委托状态
	char offsetFlag[8];			// 开平方向
	char hedgeFlag[8];			// 投机套保 
	int32_t cumQty;				// 累计成交量
	double avgPrice;				// 平均成交量
	double knockAmt;				// 成交金额
	double reckoningAmt;			// 清算金额
	int32_t  withdrawQty;			// 撤单数量
	char strategyId[16];			// 客户自定义策略ID--API discarded
	char notiMsg[256];			// 被柜台拒单或被撤单的原因
	char productId[16];			// 产品
	char productUnitId[16];		// 资产单元
	char portfolioId[16];			// 投资组合
	char acctId[16];				// 资金账号
	char advisorId[16];			// 投顾
	char basketId[32];			// 篮子ID
	char algoId[32];				// ALGOID
	char instructionId[32];		// 指令
	char auditId[32];				// 审计
	char pnltraderId[32];			// 交易员
	char currencyId[4];			// 币种
	char orderTime[16];			// 委托时间
	char memo[64];				// 备注
};

//委托推送
struct OrderNotiField
{
	char lastUpdateTime[16];//上次更新时间
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char acctId[16];//资金帐号
	char portfolioId[16];//投资组合ID
	char regId[16];//股东代码
	char stkCode[32];//合约代码
	char exchId[4];//市场
	char orderTime[16];//委托时间
	char contractNum[32];//柜台券商委托号
	int32_t orderQty;//委托数量
	double orderPrice;//委托价格
	char bsflag[8];//买卖方向
	int32_t knockQty; //成交量
	int32_t leaveQty;  //剩余量
	char orderStatus[8]; //订单状态
	char orderOwner[32]; //订单发送人
	char serialNum[32]; //序列号
	char actionFlag[8]; //订单类型 actionflag
	char orderPriceType[16]; //订单价格类型 
	double totalAmt; //订单金额
	char hedgeFlag[8];  //投机套保标志
	char offsetFlag[8]; //开平方向
	char matchCondition[8];  //市价类型
	char basketId[32]; //篮子Id
	double averagePrice;	//成交均价
	int32_t withdrawQty;  //撤单数量
	char instructionId[32];	//指令ID
	char batchNum[32];	   //委托批号
	double knockAmt; //成交金额
	char clOrderId[16];//指令ID
	char strategyId[16];//客户自定义策略ID--API discarded
	char exchangeErrMsg[256];//被交易所或柜台拒单时返回的错误信息
	// 02/14/2019 新增字段
	char occurTime[16];//业务发生时间
	char advisorId[16];//投顾
	char currencyId[4];//币种
	char memo[64];//备注
	char algoId[32];//ALGOID
	char auditId[32];//审计
	char pnltraderId[32];//交易员
};

struct QryOrderField
{
	char productId[16];//产品ID
	char orderStatus[16];//委托状态
	char exchId[4];//市场
	char stkCode[32];//合约代码
	char lastUpdateTime[16];//上次更新时间
	char clOrderId[16];//fix订单号
	char contractNum[32];//柜台券商委托号
};

struct QryOrderRspField
{
	char lastUpdateTime[16];//上次更新时间
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char acctId[16];//资金帐号
	char portfolioId[16];//投资组合ID
	char regId[16];//股东代码
	char stkCode[32];//合约代码
	char exchId[4];//市场
	char orderTime[16];//委托时间
	char contractNum[32];//柜台券商委托号
	int32_t orderQty;//委托数量
	double orderPrice;//委托价格
	char bsflag[8];//买卖方向
	int32_t knockQty; //成交量
	int32_t leaveQty;  //剩余量
	char orderStatus[8]; //订单状态
	char orderOwner[32]; //订单发送人
	char serialNum[32]; //序列号
	char actionFlag[8]; //订单类型 actionflag
	char orderPriceType[16]; //订单价格类型 
	double totalAmt; //订单金额
	char hedgeFlag[8];  //投机套保标志
	char offsetFlag[8]; //开平方向
	char matchCondition[8];  //市价类型
	char basketId[32]; //篮子Id
	double averagePrice;	//成交均价
	int32_t withdrawQty;  //撤单数量
	char instructionId[32];	//指令ID
	char batchNum[32];	   //委托批号
	double knockAmt; //成交金额
	char clOrderId[16];//指令ID
	char strategyId[16];//客户自定义策略ID--API discarded
	char exchangeErrMsg[256];//被交易所或柜台拒单时返回的错误信息
	// 02/14/2019 新增字段
	char occurTime[16];//业务发生时间
	char advisorId[16];//投顾
	char currencyId[4];//币种
	char memo[64];//备注
	char algoId[32];//ALGOID
	char auditId[32];//审计
	char pnltraderId[32];//交易员
};

struct QryKnockField
{
	char productId[16];//产品ID
	char exchId[4];//市场
	char lastUpdateTime[16];//上次更新时间
	char clOrderId[16];//fix订单号
	char serialNum[32];
	char contractNum[32];//柜台券商委托号
};

struct QryKnockRspField
{
	char lastUpdateTime[16];//上次更新时间
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char acctId[16];//资金帐号
	char portfolioId[16];//投资组合ID
	char exchId[4];//市场
	char regId[16];//股东代码
	char stkCode[32];//合约代码
	char orderTime[16];//委托时间
	char contractNum[32];//柜台券商委托号
	int32_t orderQty;//委托数量
	double orderPrice;//委托价格
	char bsflag[8];//买卖方向
	int32_t knockQty; //成交量
	double knockPrice; //成交价格
	char knockCode[32]; //成交编号
	char knockTime[32]; //成交时间
	int32_t leaveQty;  //剩余量
	char orderStatus[8]; //订单状态 
	char orderOwner[32]; //订单发送人
	char serialNum[32]; //序列号
	char actionFlag[8]; //订单类型 
	char orderPriceType[16]; //订单价格类型 
	double totalAmt; //订单金额
	char hedgeFlag[8];  //投机套保标志
	char offsetFlag[8]; //开平方向
	char matchCondition[8];  //市价类型
	char basketId[32]; //篮子Id
	double averagePrice;	//成交均价
	int32_t withdrawQty;  //撤单数量
	char instructionId[32];	//指令ID
	int32_t cumQty; //累计成交量
	char batchNum[32];	   //委托批号
	double knockAmt; //成交金额
	char clOrderId[16];//指令ID
	char strategyId[16];//客户自定义策略ID--API discarded
};

struct QryPositionField
{
	char productId[16];//产品ID
	char productUnitId[16]; //资产单元代码
	char portfolioId[16];//投资组合代码     
	char occurtime[32];//发生时间
	char stkCode[32];//合约代码
	char exchId[4];//市场	
	bool realPosFlag;//true：返回当前可用持仓; false:返回全部持仓
	int32_t authType;//权限类型：1, 查询; 2, 交易
};

struct QryPositionRspField
{
	char productId[16];//产品ID
	char productName[128];//产品名称
	char productUnitId[16]; //资产单元代码	
	char portfolioId[16];//投资组合代码      
	char portfolioName[128]; //投资组合名称      
	char exchId[4];//市场
	char stkCode[32];//合约代码
	int64_t fundShares;//产品份额
	char processFlag[8];//产品指令流程
	char productUnitName[128]; //资产单元名称
	int32_t masterFlag; //主资产单元标志 1主资产单元  0非主资产	
	char bsflag[8];//买卖方向
	char hedgeFlag[8];  //投机套保标志
	int32_t fundType;//资金类型
	int32_t totalQty;//总持仓数
	int32_t usableQty;//持仓可用数	/ 可平仓
	int32_t frozenQty; //冻结数量 = 今日冻结+昨日冻结
	int32_t ydUsableQty; //昨仓可用
	int32_t tdUsableQty; //今仓可用
	int32_t ydOffFrozenQty;	//昨平仓冻结
	int32_t tdOffFrozenQty;	//今平仓冻结
	int32_t openFrozPositionQty;	//开仓冻结
	int32_t offFrozPositionQty;		//平仓冻结 = 今日平仓冻结 + 昨日平仓冻结
	double currentStkValue;		//市值
	double realtimeIncome;		//现货卖出收益
	int32_t bondPledgeQty;			//质押券余额
	int32_t currentQty;			//上日余额
	int32_t unsaleableqty;		//非流通持仓					
	double plAmt;				//盈亏金额
	double dilutedCost;		//摊薄成本
	char currencyId[8];		//币种
};

struct QryAccountField
{
	char productId[16];//产品ID
	char productUnitId[16]; //资产单元代码
	char portfolioId[16];//投资组合代码     
	bool realFundNavFlag;		//是否需要实时计算产品单位净值
	int32_t authType;//权限类型：1, 查询; 2, 交易
};

struct QryAccountRspField
{
	char productId[16];//产品ID
	char productName[128];//产品名称
	char productUnitId[16]; //资产单元代码	
	char acctId[16];
	int64_t fundShares;//产品份额
	double assetAmt;//总资产
	double fundNav;	//实时单位净值
	char processFlag[8];//产品指令流程
	double productNav;	//产品净值
	double preAssetAmt;		//上日总资产
	double preProductNav;   //上日净值
	char productUnitName[128]; //资产单元名称
	int32_t masterFlag; //主资产单元标志 1主资产单元  0非主资产
	int32_t fundType;//资金类型
	double usable; //资金可用
	double frozenAmt; //资金冻结
	double marketValue; //资金市值
	double previousAmt; //上日余额
	double floatPnl;	  //浮动盈亏,期货现货
	double marginAmt; //保证金 占用
	double marginFrozenAmt;	//保证金冻结
	double ydMarginAmt; //上日保证金
	double realtimeAmt;//总权益
	double realtimePnl; //盯市盈亏
	double risk;		//风险度 = marginAmt/realtimeAmt
	double fetchAmt;	//可取金额 = 可出金（F_CanCashOutAmt） ＋ 预约可取金额
	double currentAmt;	//当前余额
	char currencyId[8];	//币种信息
	double exceptFrozenAmt;	//特殊冻结金额
	double shHkUsableAmt;		//港股通可用
	double equityValue;		//股票市值
	double fundValue;			//基金市值
	double bondValue;			//债券市值
	double realtimeAmtByClosePric;//使用收盘价计算的总权益
	double premium;//权利金
};

struct RecallField
{
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char portfolioId[16];//投资组合ID
	char stkCode[32];//合约代码
	char exchId[4];//市场代码
	int64_t recallQty;//Recall数量
};

struct QryEqualAccountField
{
	char productUnitId[16]; //资产单元代码
	int32_t fundType;//资金类型
	char currencyId[8];//币种
	char acctId[16];//柜台的资金账号
};

struct QryEqualAccountRspField
{
	char productUnitId[16]; //资产单元代码	
	int32_t fundType;//资金类型
	char currencyId[8];//币种
	char acctId[16];//柜台的资金账号
	double equalUsable; //汇率转换之后资金可用
	double euqalFrozenAmt; //汇率转换之后的资金冻结
	double equalPreviousAmt; //汇率转换之后的上日余额
	double equalCurrentAmt;	//汇率转换之后的当前余额
	double equalTotalAmt;	//汇率转换之后的总额，保留
};

struct QryPositionLimitField
{
	char productUnitId[16]; //资产单元代码
	char queryType[8];//查询类型1：PTH额度，2：初始融券额度
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
};

struct QryPositionLimitRspField
{
	char productUnitId[16]; //资产单元代码
	char acctId[16];//柜台的资金账号
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
	int64_t positionLimit;//当前持仓额度
};

struct QryPositionQuantityField
{
	char productUnitId[16]; //资产单元代码
	char queryType[8];//查询类型1：自有持仓数量，2：已交易融券数量
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
	char occurtime[32];//日期，20181225000000
};

struct QryPositionQuantityRspField
{
	char productUnitId[16]; //资产单元代码
	char acctId[16];//柜台的资金账号
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
	char occurtime[32];//日期，20181225000000
	int64_t positionQuantity;//当前持仓额度
};


struct BasketOrderDetailField
{
	char basketDetailId[16];//篮子内订单标识
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
	char bsFlag[4];//买(B)卖(S)
	int64_t orderQty;//委托数量
	double orderPrice;//委托价格，算法篮子时表示价格限制，0表示无限制
	double orderAmt;//委托金额
	char orderPriceType[8];//委托类型 Limit-限价 Any-市价
	char matchCondition[8]; //市价类型
	char offsetFlag[8];//开平方向
	char hedgeFlag[8];//投机套保 
	char currencyId[8];//币种
};

struct NewBasketOrderField
{
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char portfolioId[16];//投资组合ID
	int32_t fundType;//资金类型
	char description[128];//篮子备注
	//char localCurrencyId[8];//本地货币种类
	int32_t basketOrderCount;//篮子里的子单数
	BasketOrderDetailField** basketDetail;//篮子子单的详细信息
};

struct NewBasketOrderRspField
{
	char basketId[32];//篮子编号
	int32_t orderSuccessFlag;//该笔篮子子单是否成功，0-失败，1-成功
	char errMsg[128];//该笔篮子子单失败时的错误原因
	char contractNum[32];//生成的柜台合同号
	char basketDetailId[16];//篮子内订单标识
	char batchNum[16];//篮子内部批量号
};

/*struct NewAlgoBasketOrderField
{
	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char portfolioId[16];//投资组合ID
	int32_t fundType;//资金类型
	char description[128];//篮子备注
	char startTime[8];//hhmmss 启动时间
	char stopTime[8];//hhmmss Algo停止时间
	char algoType[8];//Algo策略类型，TWAP-TWAP，VWAP-VWAP
	double maxMarketShare;//最大市场占比，(0-50]， 默认可以填33
	//char localCurrencyId[8];//本地货币种类
	int32_t basketOrderCount;//篮子里的子单数
	BasketOrderDetailField** basketDetail;//篮子子单的详细信息
};*/

struct NewAlgoBasketOrderRspField
{
	char basketId[32];			// 篮子编号
	char algoParentOrderId[32];	// 算法母单代码
	char basketDetailId[16];		// 篮子内订单序列号
	char memo[64];				// 子单备注
};

//Algo 母单状态推送
struct AlgoBasketOrderNotiField
{
	char basketId[32];//篮子编号
	char algoParentOrderId[32];//Algo母单ID
	char basketDetailId[16];//篮子内订单标识

	char productId[16];//产品ID
	char productUnitId[16];//资产单元代码
	char portfolioId[16];//投资组合ID
	char exchId[4];//市场代码
	char stkCode[32];//合约代码
	char bsFlag[4];//买(B)卖(S)
	int64_t orderQty;//委托数量

	int32_t algoStatus;//Algo母单状态 需要参考码表
	char algoOrderErrmsg[128];//Algo母单被拒绝或停止的原因
	double knockProcess;//成交进度
	int32_t knockQty;//累计成交量
	double knockAmt;//累计成交金额
	double reckoningAmt; //清算金额
	double averagePrice;//累计成交均价
	int infoSerialNum;//推送消息序列号，需要以此过滤推送信息
};

// Algo指令操作
struct AlgoParentOrderActField
{
	char productId[16];			// 产品ID
	char productUnitId[16];		// 资产单元ID
	char portfolioId[16];			// 投资组合ID
	char actionFlag[4];			// ALGO操作指令
	char algoParentOrderId[32];	// Algo母单ID
};

struct AlgoParentOrderActRspField
{
	char algoParentOrderId[32];	// Algo母单ID
};

// Algo指令查询
struct QryAlgoParentOrderField
{
	char algoParentOrderId[32];	// Algo母单ID
	char queryFlag[4];			// 0-普通Algo 1-组策略Algo
};

// Algo指令查询回报
struct QryAlgoParentOrderRspField
{
	char productId[16];			// 产品ID
	char productUnitId[16];		// 资产单元ID
	char portfolioId[16];			// 投资组合ID
	char algoParentOrderId[32];	// Algo母单ID
	char bsFlag[4];				// 买卖方向
	char algoType[8];				// Algo策略类型
	int32_t algoStatus;			// Algo母单状态
	char exchId[4];				// 市场代码
	char stkCode[32];				// 合约代码
	char startTime[8];			// 启动时间 hhmmss
	char stopTime[8];				// 停止时间 hhmmss
	int64_t orderQty;				// 委托数量
	double orderAmt;				// 委托总额
	double maxMarketShare;		// 最大市场占比，(0-50]， 默认可以填33
	char algoOrderErrmsg[128];		// Algo母单被拒绝或停止的原因
	double knockProcess;			// 成交进度
	int32_t knockQty;				// 累计成交量
	double knockAmt;				// 累计成交金额
	double reckoningAmt;			// 清算金额
	double averagePrice;			// 累计成交均价
};

// 篮子子单查询回报
struct QryBasketOrderDetailRspField
{
	char basketDetailId[16];	// 篮子子单ID
	char bsFlag[4];			// 买卖方向
	char exchId[4];			// 市场代码
	char stkCode[32];			// 合约代码
	char currencyId[8];		// 币种
	double orderPrice;		// 委托价
	double referPrice;		// 参考价
	int64_t orderQty;			// 委托数量
	double orderAmt;			// 委托金额
	int64_t knockQty;			// 成交数量
	double knockAmt;			// 成交金额
	char orderStatus[8];		// 委托状态
	char offsetFlag[8];		// 开平方向
	char hedgeFlag[8];		// 投机套保
	char matchCondition[8];	// 市价类型
	char orderPriceType[8];	// 委托类型 Limit-限价 Any-市价
};

// 篮子查询回报
struct QryBasketOrderRspField
{
	char basketId[32];		// 篮子ID
	char productId[16];		// 产品ID
	char productUnitId[16];	// 资产单元ID
	char portfolioId[16];		// 投资组合ID
	int32_t basketDetailCount;	// 篮子子单数量
	QryBasketOrderDetailRspField** basketDetail;	// 篮子子单列表
};

struct TwapChildOrder
{
	char exchId[4];			// 市场代码
	char stkCode[32];			// 合约代码
	char bsFlag[4];			// 买卖方向
	char offsetFlag[8];		//开平方向
	char hedgeFlag[8];		//投机套保 
	double priceLimit;		// 价格限制
	int64_t orderQty;			// 委托数量
	double orderAmt;
	char currencyId[8];		// 币种
	char memo[64];
};

struct NewTwapBasketOrder
{
	char productId[16];			// 产品ID
	char productUnitId[16];		// 资产单元ID
	char portfolioId[16];			// 投资组合ID
	char startTime[8];//hhmmss 启动时间
	char stopTime[8];//hhmmss Algo停止时间
	double maxMarketShare;		// 最大市场占比，(0-0.50]， 默认可以填0.33
	int32_t basketDetailCount;// 篮子子单数量
	TwapChildOrder** basketDetail;
};

struct QuickBasketChildOrder
{
	char exchId[4];			// 市场代码
	char stkCode[32];			// 合约代码
	char bsFlag[4];			// 买卖方向
	double priceLimit;
	int64_t orderQty;			// 委托数量
	char currencyId[8];		// 币种
	char memo[64];
};

struct NewQuickBasketOrder
{
	char productId[16];			// 产品ID
	char productUnitId[16];		// 资产单元ID
	char portfolioId[16];			// 投资组合ID
	char startTime[8];//hhmmss 启动时间
	char stopTime[8];//hhmmss Algo停止时间
	double maxMarketShare;		// 最大市场占比，(0-0.50]， 默认可以填0.33
	int32_t basketDetailCount;// 篮子子单数量
	QuickBasketChildOrder** basketDetail;
};

typedef TwapChildOrder VwapChildOrder;
typedef NewTwapBasketOrder NewVwapBasketOrder;

typedef NewAlgoBasketOrderRspField NewVwapOrderRspField;
typedef NewAlgoBasketOrderRspField NewTwapOrderRspField;
typedef NewAlgoBasketOrderRspField NewQuickBasketOrderRspField;

class IMSTradeSPI
{
public:
	virtual const char* GetLastError() = 0;						// 返回最后一次内部错误

public:
	virtual bool OnRecall(const RecallField* pRspRecall) = 0;	// Recall信息响应
	// 委托类回调
	virtual bool OnRspNewOrder(const NewOrderRspField* pRspNewOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;							// 报单请求响应
	virtual bool OnRspCancelOrder(const CancelOrderRspField* pRspCnclOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;				// 撤单请求响应
	virtual bool OnRspNewBasketOrder(const NewBasketOrderRspField* pRspNewBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;				// 篮子响应
	virtual bool OnRspNewAlgoBasketOrder(const NewAlgoBasketOrderRspField* pRspNewAlgoBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;	// Algo篮子响应
	virtual bool OnRspAlgoParentOrderAct(const AlgoParentOrderActRspField* pRspAlgoPrntOrdAct, const ErrMsgField* errMsg, int requestId) = 0;				// Algo暂停请求响应
	virtual bool OnRspNewTwapOrder(const NewTwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	virtual bool OnRspNewVwapOrder(const NewVwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	virtual bool OnRspNewQuickBasketOrder(const NewQuickBasketOrderRspField* pRspQuickBsktOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	// 推送类回调
	virtual bool OnNotiOrder(const OrderNotiField* pNotiOrd, const ErrMsgField * errMsg) = 0;								// 委托推送通知
	virtual bool OnNotiKnock(const KnockNotiField* pNotiKnk, const ErrMsgField * errMsg) = 0;								// 成交通知
	virtual bool OnNotiAlgoBasketOrder(const AlgoBasketOrderNotiField* pNotiAlgoBsktOrd, const ErrMsgField * errMsg) = 0;	// Algo状态推送回调
	// 委托查询类回调
	virtual bool OnRspQryOrder(const QryOrderRspField* pRspQryOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;								// 查询委托请求响应
	virtual bool OnRspQryKnock(const QryKnockRspField* pRspQryKnk, const ErrMsgField * errMsg, int requestId, bool last) = 0;								// 查询成交请求响应
	virtual bool OnRspQryBasketOrder(const QryBasketOrderRspField* pRspQryBsktOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;				// 查询篮子请求响应
	virtual bool OnRspQryAlgoParentOrder(const QryAlgoParentOrderRspField* pRspQryAlgoPrntOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;	// 查询Algo指令请求响应
	// 持仓资金查询类回调
	virtual bool OnRspQryPosition(const QryPositionRspField* pRspQryPos, const ErrMsgField * errMsg, int requestId, bool last) = 0;						// 查询持仓请求响应
	virtual bool OnRspQryAccount(const QryAccountRspField* pRspQryAcct, const ErrMsgField * errMsg, int requestId, bool last) = 0;						// 查询资金请求响应
	virtual bool OnRspQryEqualAccount(const QryEqualAccountRspField* pRspQryEqAcct, const ErrMsgField * errMsg, int requestId, bool last) = 0;			// 查询汇率转换后的资金请求响应
	virtual bool OnRspQryPositionLimit(const QryPositionLimitRspField* pRspQryPosLim, const ErrMsgField * errMsg, int requestId, bool last) = 0;	// 查询当前额度请求响应
	virtual bool OnRspQryPositionQuantity(const QryPositionQuantityRspField* pRspQryPosQuant, const ErrMsgField * errMsg, int requestId, bool last) = 0;		// 查询自有持仓或已交易融券数量请求响应
};

class TRADER_API_EXPORT IMSTradeAPI
{
public:
	virtual const char * GetLastError() = 0;				// 请求最新错误信息
	virtual bool SetTradeSPI(IMSTradeSPI* pSpi) = 0;		// 注册回调接口
	virtual bool Initial(const char* addr) = 0;				// 初始化IMS接入站点
	virtual bool TryLogin(const char* userId, const char * password, const char* pLicence) = 0;	// 登录IMS
    TRADER_API_EXPORT static IMSTradeAPI* CreateAPI();						// 创建API实例
    TRADER_API_EXPORT static void ReleaseAPI(IMSTradeAPI* pApi);				// 销毁API实例

public:
	// 委托类请求
	virtual int ReqNewOrder(NewOrderField* pNewOrd, int requestId) = 0;										// 报单请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqCancelOrder(CancelOrderField** pCnclOrd, int cancelOrderCount, int requestId) = 0;	// 撤单请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqNewBasketOrder(NewBasketOrderField* pBsktOrd, int requestId) = 0;										// 下篮子单: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqAlgoParentOrderAct(AlgoParentOrderActField* pAlgoPrntOrdAct, int requestId) = 0;// Algo指令请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqNewTwapBasketOrder(NewTwapBasketOrder* pTwapOrd, int requestId) = 0;
	virtual int ReqNewVwapBasketOrder(NewVwapBasketOrder* pVwapOrd, int requestId) = 0;
	virtual int ReqNewQuickBasketOrder(NewQuickBasketOrder* pQuickOrd, int requestId) = 0;
	// 委托查询类请求
	virtual int ReqQryOrder(QryOrderField* pQryOrd, int requestId) = 0;											// 查询委托请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryKnock(QryKnockField* pQryKnk, int requestId) = 0;											// 查询成交请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryAlgoParentOrder(QryAlgoParentOrderField* pQryAlgoPrntOrd, int requestId) = 0;				// Algo母单查询请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryBasketOrderList(int requestId) = 0;															// 篮子查询请求: 返回值小于0 发生错误，通过GetLastError得到错误
	// 持仓资金类查询请求
	virtual int ReqQryPosition(QryPositionField* pQryPos, int requestId) = 0;									// 查询持仓请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryAccount(QryAccountField* pQryAcct, int requestId) = 0;										// 查询资金请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryEqualAccount(QryEqualAccountField* pQryEqAcct, int requestId) = 0;						// 查询汇率转换后的资金请求: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryPositionLimit(QryPositionLimitField* pQryPosLim, int requestId) = 0;			// 查询当前额度: 返回值小于0 发生错误，通过GetLastError得到错误
	virtual int ReqQryPositionQuantity(QryPositionQuantityField* pQryPosQuant, int requestId) = 0;				// 查询自有持仓或已交易融券数量: 返回值小于0 发生错误，通过GetLastError得到错误
};

