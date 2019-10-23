#ifndef __HT_CLIENT_FILED_DEFINEE_H__
#define __HT_CLIENT_FILED_DEFINEE_H__
#include "htsdk_interface.h"

//修改日期20180920
//因功能号中所有字段均采用的数字编码，为方便客户编写程序，整理了“HtClientFiledDefine.h”字典，客户可采用，也可根据需要自行编订

namespace HtField
{
	const HT_int32 MsgType = 35;
	const HT_int32 AvgPx = 6;//成交均价
	const HT_int32 CumQty = 14;//成交数量
	const HT_int32 Currency = 15;//币种
	const HT_int32 ExecID = 17;//成交编号
	const HT_int32 LastPx = 31;//成交价格
	const HT_int32 LastQty = 32;//成交数量
	const HT_int32 OrderID = 37;//委托编号
	const HT_int32 OrderQty = 38;//委托数量
	const HT_int32 OrdStatus = 39;//委托状态
	const HT_int32 Price = 44;//委托价格
	const HT_int32 SecurityID = 48;//证券代码
	const HT_int32 OrderSide = 54;//买卖方向
	const HT_int32 Text = 58;//失败原因
	const HT_int32 SecurityType = 167;//证券类别
	const HT_int32 SubscriptionRequestType = 263;//订阅标识
	const HT_int32 Password = 554;//用户登录密码
	const HT_int32 AvailableMargin = 899;//可用保证金
	const HT_int32 SecurityExchange = 1301;// 交易类别
	const HT_int32 Token = 10002;//用户口令
	const HT_int32 SubGroupNumber = 10007;//记录数
	const HT_int32 RequestExeStatus = 10008;//请求执行状态
	const HT_int32 LoginUserID = 10035;//用户名
	const HT_int32 SecurityAccount = 10115;//证券账号
	const HT_int32 ReMarks = 10121;//用户登录预留信息
	const HT_int32 EntrustProp = 10130;//委托属性
	const HT_int32 OrderStation = 11072;//站点地址
	const HT_int32 OrderSoldQty = 11074;//今日委托卖出数量
	const HT_int32 TradeDate = 11418;//交易日期
	const HT_int32 CompactType = 12532;//合约类型
	const HT_int32 YearInterestRate = 12541;//合约年利率
	const HT_int32 FinanceQuotaCapacity = 12551;//融资最高额度
	const HT_int32 SecurityLoanQuotaCapacity = 12552;//融券最高额度
	const HT_int32 FinanceOccupyQuota = 12571;//融资已用额度
	const HT_int32 OpenCompactQuota = 12574;//期初合约金额
	const HT_int32 OpenCompactQuantity = 12575;//期初合约数量
	const HT_int32 OpenCompactCommission = 12576;//期初合约费用
	const HT_int32 RealtimeCompactQuota = 12577;//未还合约金额
	const HT_int32 RealtimeCompactQuantity = 12578;//未还合约数量
	const HT_int32 RealtimeCompactCommission = 12579;//未还合约费用
	const HT_int32 RealtimeCompactInterest = 12580;//未还合约利息
	const HT_int32 ReturnTotalInterest = 12584;//已还利息
	const HT_int32 ReturnCloseDate = 12597;//归还截止日
	const HT_int32 CompactState = 12599;//合约状态
	const HT_int32 CompactOrigin = 12600;//合约来源
	const HT_int32 FinanceAvailableQuota = 12616;//融资可用额度
	const HT_int32 FinanceOccupyMargin = 12617;//融资已用保证金额
	const HT_int32 FinanceCompactQuota = 12618;//融资合约金额
	const HT_int32 FinanceCompactCommission = 12619;//融资合约费用
	const HT_int32 FinanceCompactInterest = 12620;//融资合约利息
	const HT_int32 FinanceMarketValue = 12621;//融资市值
	const HT_int32 FinanceCompactRevenue = 12622;//融资合约盈亏
	const HT_int32 SecurityLoanAvailableQuota = 12623;//融券可用额度
	const HT_int32 SecurityLoanOccupyQuota = 12624;//融券已用额度
	const HT_int32 SecurityLoanOccupyMargin = 12625;//融券已用保证金额
	const HT_int32 SecurityLoanCompactQuota = 12626;//融券合约金额
	const HT_int32 SecurityLoanCompactCommission = 12627;//融券合约费用
	const HT_int32 SecurityLoanCompactInterest = 12628;//融券合约利息
	const HT_int32 SecurityLoanMarketValue = 12629;//融券市值
	const HT_int32 SecurityLoanCompactRevenue = 12630;//融券合约盈亏
	const HT_int32 FundAccount = 15806;//资金账号
	const HT_int32 RequestNumber = 10009;//请求行数
	const HT_int32 PositionStr = 10011;//定位串
	const HT_int32 CreateTime = 10012;//委托时间
	const HT_int32 OrderDir = 10034;//排序方式
	const HT_int32 SubscriptionTopic = 10069;//订阅主题
	const HT_int32 ExchangeExecType = 10131;//成交类别
	const HT_int32 ExchangeExecStat = 10132;//成交状态
	const HT_int32 InitialHoldingQty = 10166;//期初持有数量
	const HT_int32 UnBoughtQty = 10167;//今买入未成数量
	const HT_int32 UnSoldQty = 10168;//今卖出未成数量
	const HT_int32 OccupyMargin = 11020;//已用保证金
	const HT_int32 SettledSoldAmount = 11025;//累计卖出金额
	const HT_int32 SettledSoldQty = 11026;//累计卖出数量
	const HT_int32 SettledBoughtAmount = 11027;//累计买入金额
	const HT_int32 SettleBoughtQty = 11028;//累计买入数量
	const HT_int32 IntradayBoughtAmount = 11029;//今买入已成金额
	const HT_int32 IntradayBoughtQty = 11030;//今买入已成数量
	const HT_int32 IntradaySoldAmount = 11031;//今卖出已成金额
	const HT_int32 IntradaySoldQty = 11032;//今卖出已成数量
	const HT_int32 TradeableQty = 11033;//可用数量
	const HT_int32 AverageCost = 11034;//持仓成本
	const HT_int32 HoldingQty = 11035;//当前数量
	const HT_int32 SettledCash = 11037;//期初资金余额
	const HT_int32 TradeableCash = 11038;//可用资金余额
	const HT_int32 HoldingCash = 11039;//当前资金余额
	const HT_int32 EntrustType = 11067;//委托类别
	const HT_int32 ExchangeMatchDate = 11076;//成交时间
	const HT_int32 OccurQuantity = 11371;//发生数量
	const HT_int32 LastAmount = 11802;//成交金额
	const HT_int32 CumAmount = 11803;//成交金额
	const HT_int32 CancelledQty = 11814;//撤单数量
	const HT_int32 AssetBalance = 11816;//总资产
	const HT_int32 OrigOrderID = 11820;//原委托编号
	const HT_int32 ParentOrderID = 11825;//批次号
	const HT_int32 CompactNo = 12500;//合约编号 
	const HT_int32 PositionProperty = 12501;//头寸性质
	const HT_int32 CounterpartySecurityAccount = 12503;//普通证券账号 
	const HT_int32 MaintainValue = 12530;//维持担保比例
	const HT_int32 CashAsset = 12608;//现金资产
	const HT_int32 SecurityMarketValue = 12609;//证券市值
	const HT_int32 AssureAsset = 12610;//担保资产
	const HT_int32 TotalLiability = 12611;//总负债
	const HT_int32 CollateralAvailableFund = 12612;//买担保品可用资金
	const HT_int32 FinanceAvailableFund = 12613;//买融资标的可用资金
	const HT_int32 SecurityAvailableFund = 12614;//买券还券可用资金
	const HT_int32 CashAvailableFund = 12615;//现金还款可用资金
	const HT_int32 TransferAsset = 12631;//可转出资产
	const HT_int32 CompactTotalInterest = 12632;//合约总利息
	const HT_int32 NetAsset = 12633;//净资产
	const HT_int32 WithdrawQuota = 12634;//可取资金余额
	const HT_int32 OccurQuota = 12635;//发生金额
	const HT_int32 OccurCommission = 12636;//发生费用
	const HT_int32 OccurInterest = 12637;//发生利息
	const HT_int32 RepaymentAmount = 12644;//还款金额
	const HT_int32 HoldingProfit = 15405;//盈亏金额
	const HT_int32 SecurityName = 17511;//证券名称
	const HT_int32 CostPrice = 58003;//成本价
	const HT_int32 CounterpartyFundAccount = 58004;//普通资金账号
	const HT_int32 CliengMsgID = 10029;
}


#endif

