#ifndef __HT_CLIENT_FILED_DEFINEE_H__
#define __HT_CLIENT_FILED_DEFINEE_H__
#include "htsdk_interface.h"

//�޸�����20180920
//���ܺ��������ֶξ����õ����ֱ��룬Ϊ����ͻ���д���������ˡ�HtClientFiledDefine.h���ֵ䣬�ͻ��ɲ��ã�Ҳ�ɸ�����Ҫ���бඩ

namespace HtField
{
	const HT_int32 MsgType = 35;
	const HT_int32 AvgPx = 6;//�ɽ�����
	const HT_int32 CumQty = 14;//�ɽ�����
	const HT_int32 Currency = 15;//����
	const HT_int32 ExecID = 17;//�ɽ����
	const HT_int32 LastPx = 31;//�ɽ��۸�
	const HT_int32 LastQty = 32;//�ɽ�����
	const HT_int32 OrderID = 37;//ί�б��
	const HT_int32 OrderQty = 38;//ί������
	const HT_int32 OrdStatus = 39;//ί��״̬
	const HT_int32 Price = 44;//ί�м۸�
	const HT_int32 SecurityID = 48;//֤ȯ����
	const HT_int32 OrderSide = 54;//��������
	const HT_int32 Text = 58;//ʧ��ԭ��
	const HT_int32 SecurityType = 167;//֤ȯ���
	const HT_int32 SubscriptionRequestType = 263;//���ı�ʶ
	const HT_int32 Password = 554;//�û���¼����
	const HT_int32 AvailableMargin = 899;//���ñ�֤��
	const HT_int32 SecurityExchange = 1301;// �������
	const HT_int32 Token = 10002;//�û�����
	const HT_int32 SubGroupNumber = 10007;//��¼��
	const HT_int32 RequestExeStatus = 10008;//����ִ��״̬
	const HT_int32 LoginUserID = 10035;//�û���
	const HT_int32 SecurityAccount = 10115;//֤ȯ�˺�
	const HT_int32 ReMarks = 10121;//�û���¼Ԥ����Ϣ
	const HT_int32 EntrustProp = 10130;//ί������
	const HT_int32 OrderStation = 11072;//վ���ַ
	const HT_int32 OrderSoldQty = 11074;//����ί����������
	const HT_int32 TradeDate = 11418;//��������
	const HT_int32 CompactType = 12532;//��Լ����
	const HT_int32 YearInterestRate = 12541;//��Լ������
	const HT_int32 FinanceQuotaCapacity = 12551;//������߶��
	const HT_int32 SecurityLoanQuotaCapacity = 12552;//��ȯ��߶��
	const HT_int32 FinanceOccupyQuota = 12571;//�������ö��
	const HT_int32 OpenCompactQuota = 12574;//�ڳ���Լ���
	const HT_int32 OpenCompactQuantity = 12575;//�ڳ���Լ����
	const HT_int32 OpenCompactCommission = 12576;//�ڳ���Լ����
	const HT_int32 RealtimeCompactQuota = 12577;//δ����Լ���
	const HT_int32 RealtimeCompactQuantity = 12578;//δ����Լ����
	const HT_int32 RealtimeCompactCommission = 12579;//δ����Լ����
	const HT_int32 RealtimeCompactInterest = 12580;//δ����Լ��Ϣ
	const HT_int32 ReturnTotalInterest = 12584;//�ѻ���Ϣ
	const HT_int32 ReturnCloseDate = 12597;//�黹��ֹ��
	const HT_int32 CompactState = 12599;//��Լ״̬
	const HT_int32 CompactOrigin = 12600;//��Լ��Դ
	const HT_int32 FinanceAvailableQuota = 12616;//���ʿ��ö��
	const HT_int32 FinanceOccupyMargin = 12617;//�������ñ�֤���
	const HT_int32 FinanceCompactQuota = 12618;//���ʺ�Լ���
	const HT_int32 FinanceCompactCommission = 12619;//���ʺ�Լ����
	const HT_int32 FinanceCompactInterest = 12620;//���ʺ�Լ��Ϣ
	const HT_int32 FinanceMarketValue = 12621;//������ֵ
	const HT_int32 FinanceCompactRevenue = 12622;//���ʺ�Լӯ��
	const HT_int32 SecurityLoanAvailableQuota = 12623;//��ȯ���ö��
	const HT_int32 SecurityLoanOccupyQuota = 12624;//��ȯ���ö��
	const HT_int32 SecurityLoanOccupyMargin = 12625;//��ȯ���ñ�֤���
	const HT_int32 SecurityLoanCompactQuota = 12626;//��ȯ��Լ���
	const HT_int32 SecurityLoanCompactCommission = 12627;//��ȯ��Լ����
	const HT_int32 SecurityLoanCompactInterest = 12628;//��ȯ��Լ��Ϣ
	const HT_int32 SecurityLoanMarketValue = 12629;//��ȯ��ֵ
	const HT_int32 SecurityLoanCompactRevenue = 12630;//��ȯ��Լӯ��
	const HT_int32 FundAccount = 15806;//�ʽ��˺�
	const HT_int32 RequestNumber = 10009;//��������
	const HT_int32 PositionStr = 10011;//��λ��
	const HT_int32 CreateTime = 10012;//ί��ʱ��
	const HT_int32 OrderDir = 10034;//����ʽ
	const HT_int32 SubscriptionTopic = 10069;//��������
	const HT_int32 ExchangeExecType = 10131;//�ɽ����
	const HT_int32 ExchangeExecStat = 10132;//�ɽ�״̬
	const HT_int32 InitialHoldingQty = 10166;//�ڳ���������
	const HT_int32 UnBoughtQty = 10167;//������δ������
	const HT_int32 UnSoldQty = 10168;//������δ������
	const HT_int32 OccupyMargin = 11020;//���ñ�֤��
	const HT_int32 SettledSoldAmount = 11025;//�ۼ��������
	const HT_int32 SettledSoldQty = 11026;//�ۼ���������
	const HT_int32 SettledBoughtAmount = 11027;//�ۼ�������
	const HT_int32 SettleBoughtQty = 11028;//�ۼ���������
	const HT_int32 IntradayBoughtAmount = 11029;//�������ѳɽ��
	const HT_int32 IntradayBoughtQty = 11030;//�������ѳ�����
	const HT_int32 IntradaySoldAmount = 11031;//�������ѳɽ��
	const HT_int32 IntradaySoldQty = 11032;//�������ѳ�����
	const HT_int32 TradeableQty = 11033;//��������
	const HT_int32 AverageCost = 11034;//�ֲֳɱ�
	const HT_int32 HoldingQty = 11035;//��ǰ����
	const HT_int32 SettledCash = 11037;//�ڳ��ʽ����
	const HT_int32 TradeableCash = 11038;//�����ʽ����
	const HT_int32 HoldingCash = 11039;//��ǰ�ʽ����
	const HT_int32 EntrustType = 11067;//ί�����
	const HT_int32 ExchangeMatchDate = 11076;//�ɽ�ʱ��
	const HT_int32 OccurQuantity = 11371;//��������
	const HT_int32 LastAmount = 11802;//�ɽ����
	const HT_int32 CumAmount = 11803;//�ɽ����
	const HT_int32 CancelledQty = 11814;//��������
	const HT_int32 AssetBalance = 11816;//���ʲ�
	const HT_int32 OrigOrderID = 11820;//ԭί�б��
	const HT_int32 ParentOrderID = 11825;//���κ�
	const HT_int32 CompactNo = 12500;//��Լ��� 
	const HT_int32 PositionProperty = 12501;//ͷ������
	const HT_int32 CounterpartySecurityAccount = 12503;//��֤ͨȯ�˺� 
	const HT_int32 MaintainValue = 12530;//ά�ֵ�������
	const HT_int32 CashAsset = 12608;//�ֽ��ʲ�
	const HT_int32 SecurityMarketValue = 12609;//֤ȯ��ֵ
	const HT_int32 AssureAsset = 12610;//�����ʲ�
	const HT_int32 TotalLiability = 12611;//�ܸ�ծ
	const HT_int32 CollateralAvailableFund = 12612;//�򵣱�Ʒ�����ʽ�
	const HT_int32 FinanceAvailableFund = 12613;//�����ʱ�Ŀ����ʽ�
	const HT_int32 SecurityAvailableFund = 12614;//��ȯ��ȯ�����ʽ�
	const HT_int32 CashAvailableFund = 12615;//�ֽ𻹿�����ʽ�
	const HT_int32 TransferAsset = 12631;//��ת���ʲ�
	const HT_int32 CompactTotalInterest = 12632;//��Լ����Ϣ
	const HT_int32 NetAsset = 12633;//���ʲ�
	const HT_int32 WithdrawQuota = 12634;//��ȡ�ʽ����
	const HT_int32 OccurQuota = 12635;//�������
	const HT_int32 OccurCommission = 12636;//��������
	const HT_int32 OccurInterest = 12637;//������Ϣ
	const HT_int32 RepaymentAmount = 12644;//������
	const HT_int32 HoldingProfit = 15405;//ӯ�����
	const HT_int32 SecurityName = 17511;//֤ȯ����
	const HT_int32 CostPrice = 58003;//�ɱ���
	const HT_int32 CounterpartyFundAccount = 58004;//��ͨ�ʽ��˺�
	const HT_int32 CliengMsgID = 10029;
}


#endif

