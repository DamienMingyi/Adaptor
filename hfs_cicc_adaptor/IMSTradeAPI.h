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
	int errorCode;//�����ţ�0����
	char errorMsg[256];	//������Ϣ
};

// Algo��ͨί��
struct NewOrderField
{
	char productId[16];			// ��Լ����
	char productUnitId[16];		// �ʲ���Ԫ����
	char portfolioId[16];			// Ͷ����ϴ���
	char acctId[16];				// �ʽ��˺�
	char stkCode[32];				// ��Լ����
	char exchId[4];				// �г�����
	char bsFlag[4];				// ��������
	int64_t orderQty;				// ί������
	double orderPrice;			// ί�м۸�
	char orderPriceType[8];		// ί������ Limit-�޼� Any-�м�
	char matchCondition[8];		// �м�����
	char currencyId[8];			// ���ִ���
	int32_t fundType;				// �ʽ�����
	char businessType[16];			// ҵ������
	char offsetFlag[8];			// ��ƽ����
	char hedgeFlag[8];			// Ͷ���ױ���־ 
	char clOrderId[16];			// �Զ����ͬ��
	char strategyId[16];			// �ͻ��Զ������ID--API discarded
};

//Algo�ӵ�����
struct CancelOrderField
{
	char productId[16];			// ��Լ����
	char productUnitId[16];		// �ʲ���Ԫ����
	char portfolioId[16];			// Ͷ����ϴ���
	char acctId[16];				// �ʽ��˺�
	char stkCode[32];				// ��Լ����
	char exchId[4];				// �г�
	char clOrderId[16];			// �Զ����ͬ��
	char orgclOrderId[16];			// ԭ�����Զ����ͬ��
	char orgContractNum[32];		// ԭ����̨��ͬ��
	char orgSerialNum[32];			// ԭIMS���ɺ�ͬ���к�
	char businessType[16];			// ҵ������
};

//�ӵ�ȷ��
struct NewOrderRspField
{
	char exchId[4];				// �г�����
	char stkCode[32];				// ��Լ����
	char bsFlag[4];				// ��(B)��(S)
	char clOrderId[16];			// �Զ����ͬ��
	char contractNum[32];			// ���ɵĹ�̨��ͬ��
	char serialNum[32];			// IMS���ɺ�ͬ���к�
	double totalAmt;				// �ܽ��
	double commisionAmt;			// ������
	int64_t orderQty;				// ί������
	char strategyId[16];			// �ͻ��Զ������ID--API discarded
};

//����ȷ��
struct CancelOrderRspField
{
	char exchId[4];				// �г�����
	char stkCode[32];				// ��Լ����
	char clOrderId[16];			// �Զ����ͬ��
	char contractNum[32];			// ���ɵĹ�̨��ͬ��
	char orgclOrderId[32];			// ԭ�����Զ����ͬ��
	char serialNum[32];			// IMS���ɺ�ͬ���к�
	char businessType[16];			// ҵ������
};

//�ɽ�����
struct KnockNotiField
{
	char stkCode[32];				// ��Լ����
	char exchId[4];				// �г�
	char bsFlag[4];				// ��(B)��(S)
	int64_t orderQty;				// ί������
	double orderPrice;			// ί�м۸�
	char contractNum[32];			// ��̨��ͬ��
	char serialNum[32];			// IMS���ɺ�ͬ���к�
	char clOrderId[16];			// �Զ����ͬ��
	char execType[16];			// �ɽ�����
	char orderRejectFlag[16];		// �������ܵ���־
	int32_t  knockQty;			// �ɽ�����
	double knockPrice;			// �ɽ��۸�
	char knockCode[32];			// �ɽ����
	char knockTime[16];			// �ɽ�ʱ��
	int32_t leaveQty;				// ʣ������
	char orderStatus[32];			// ί��״̬
	char offsetFlag[8];			// ��ƽ����
	char hedgeFlag[8];			// Ͷ���ױ� 
	int32_t cumQty;				// �ۼƳɽ���
	double avgPrice;				// ƽ���ɽ���
	double knockAmt;				// �ɽ����
	double reckoningAmt;			// ������
	int32_t  withdrawQty;			// ��������
	char strategyId[16];			// �ͻ��Զ������ID--API discarded
	char notiMsg[256];			// ����̨�ܵ��򱻳�����ԭ��
	char productId[16];			// ��Ʒ
	char productUnitId[16];		// �ʲ���Ԫ
	char portfolioId[16];			// Ͷ�����
	char acctId[16];				// �ʽ��˺�
	char advisorId[16];			// Ͷ��
	char basketId[32];			// ����ID
	char algoId[32];				// ALGOID
	char instructionId[32];		// ָ��
	char auditId[32];				// ���
	char pnltraderId[32];			// ����Ա
	char currencyId[4];			// ����
	char orderTime[16];			// ί��ʱ��
	char memo[64];				// ��ע
};

//ί������
struct OrderNotiField
{
	char lastUpdateTime[16];//�ϴθ���ʱ��
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char acctId[16];//�ʽ��ʺ�
	char portfolioId[16];//Ͷ�����ID
	char regId[16];//�ɶ�����
	char stkCode[32];//��Լ����
	char exchId[4];//�г�
	char orderTime[16];//ί��ʱ��
	char contractNum[32];//��̨ȯ��ί�к�
	int32_t orderQty;//ί������
	double orderPrice;//ί�м۸�
	char bsflag[8];//��������
	int32_t knockQty; //�ɽ���
	int32_t leaveQty;  //ʣ����
	char orderStatus[8]; //����״̬
	char orderOwner[32]; //����������
	char serialNum[32]; //���к�
	char actionFlag[8]; //�������� actionflag
	char orderPriceType[16]; //�����۸����� 
	double totalAmt; //�������
	char hedgeFlag[8];  //Ͷ���ױ���־
	char offsetFlag[8]; //��ƽ����
	char matchCondition[8];  //�м�����
	char basketId[32]; //����Id
	double averagePrice;	//�ɽ�����
	int32_t withdrawQty;  //��������
	char instructionId[32];	//ָ��ID
	char batchNum[32];	   //ί������
	double knockAmt; //�ɽ����
	char clOrderId[16];//ָ��ID
	char strategyId[16];//�ͻ��Զ������ID--API discarded
	char exchangeErrMsg[256];//�����������̨�ܵ�ʱ���صĴ�����Ϣ
	// 02/14/2019 �����ֶ�
	char occurTime[16];//ҵ����ʱ��
	char advisorId[16];//Ͷ��
	char currencyId[4];//����
	char memo[64];//��ע
	char algoId[32];//ALGOID
	char auditId[32];//���
	char pnltraderId[32];//����Ա
};

struct QryOrderField
{
	char productId[16];//��ƷID
	char orderStatus[16];//ί��״̬
	char exchId[4];//�г�
	char stkCode[32];//��Լ����
	char lastUpdateTime[16];//�ϴθ���ʱ��
	char clOrderId[16];//fix������
	char contractNum[32];//��̨ȯ��ί�к�
};

struct QryOrderRspField
{
	char lastUpdateTime[16];//�ϴθ���ʱ��
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char acctId[16];//�ʽ��ʺ�
	char portfolioId[16];//Ͷ�����ID
	char regId[16];//�ɶ�����
	char stkCode[32];//��Լ����
	char exchId[4];//�г�
	char orderTime[16];//ί��ʱ��
	char contractNum[32];//��̨ȯ��ί�к�
	int32_t orderQty;//ί������
	double orderPrice;//ί�м۸�
	char bsflag[8];//��������
	int32_t knockQty; //�ɽ���
	int32_t leaveQty;  //ʣ����
	char orderStatus[8]; //����״̬
	char orderOwner[32]; //����������
	char serialNum[32]; //���к�
	char actionFlag[8]; //�������� actionflag
	char orderPriceType[16]; //�����۸����� 
	double totalAmt; //�������
	char hedgeFlag[8];  //Ͷ���ױ���־
	char offsetFlag[8]; //��ƽ����
	char matchCondition[8];  //�м�����
	char basketId[32]; //����Id
	double averagePrice;	//�ɽ�����
	int32_t withdrawQty;  //��������
	char instructionId[32];	//ָ��ID
	char batchNum[32];	   //ί������
	double knockAmt; //�ɽ����
	char clOrderId[16];//ָ��ID
	char strategyId[16];//�ͻ��Զ������ID--API discarded
	char exchangeErrMsg[256];//�����������̨�ܵ�ʱ���صĴ�����Ϣ
	// 02/14/2019 �����ֶ�
	char occurTime[16];//ҵ����ʱ��
	char advisorId[16];//Ͷ��
	char currencyId[4];//����
	char memo[64];//��ע
	char algoId[32];//ALGOID
	char auditId[32];//���
	char pnltraderId[32];//����Ա
};

struct QryKnockField
{
	char productId[16];//��ƷID
	char exchId[4];//�г�
	char lastUpdateTime[16];//�ϴθ���ʱ��
	char clOrderId[16];//fix������
	char serialNum[32];
	char contractNum[32];//��̨ȯ��ί�к�
};

struct QryKnockRspField
{
	char lastUpdateTime[16];//�ϴθ���ʱ��
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char acctId[16];//�ʽ��ʺ�
	char portfolioId[16];//Ͷ�����ID
	char exchId[4];//�г�
	char regId[16];//�ɶ�����
	char stkCode[32];//��Լ����
	char orderTime[16];//ί��ʱ��
	char contractNum[32];//��̨ȯ��ί�к�
	int32_t orderQty;//ί������
	double orderPrice;//ί�м۸�
	char bsflag[8];//��������
	int32_t knockQty; //�ɽ���
	double knockPrice; //�ɽ��۸�
	char knockCode[32]; //�ɽ����
	char knockTime[32]; //�ɽ�ʱ��
	int32_t leaveQty;  //ʣ����
	char orderStatus[8]; //����״̬ 
	char orderOwner[32]; //����������
	char serialNum[32]; //���к�
	char actionFlag[8]; //�������� 
	char orderPriceType[16]; //�����۸����� 
	double totalAmt; //�������
	char hedgeFlag[8];  //Ͷ���ױ���־
	char offsetFlag[8]; //��ƽ����
	char matchCondition[8];  //�м�����
	char basketId[32]; //����Id
	double averagePrice;	//�ɽ�����
	int32_t withdrawQty;  //��������
	char instructionId[32];	//ָ��ID
	int32_t cumQty; //�ۼƳɽ���
	char batchNum[32];	   //ί������
	double knockAmt; //�ɽ����
	char clOrderId[16];//ָ��ID
	char strategyId[16];//�ͻ��Զ������ID--API discarded
};

struct QryPositionField
{
	char productId[16];//��ƷID
	char productUnitId[16]; //�ʲ���Ԫ����
	char portfolioId[16];//Ͷ����ϴ���     
	char occurtime[32];//����ʱ��
	char stkCode[32];//��Լ����
	char exchId[4];//�г�	
	bool realPosFlag;//true�����ص�ǰ���óֲ�; false:����ȫ���ֲ�
	int32_t authType;//Ȩ�����ͣ�1, ��ѯ; 2, ����
};

struct QryPositionRspField
{
	char productId[16];//��ƷID
	char productName[128];//��Ʒ����
	char productUnitId[16]; //�ʲ���Ԫ����	
	char portfolioId[16];//Ͷ����ϴ���      
	char portfolioName[128]; //Ͷ���������      
	char exchId[4];//�г�
	char stkCode[32];//��Լ����
	int64_t fundShares;//��Ʒ�ݶ�
	char processFlag[8];//��Ʒָ������
	char productUnitName[128]; //�ʲ���Ԫ����
	int32_t masterFlag; //���ʲ���Ԫ��־ 1���ʲ���Ԫ  0�����ʲ�	
	char bsflag[8];//��������
	char hedgeFlag[8];  //Ͷ���ױ���־
	int32_t fundType;//�ʽ�����
	int32_t totalQty;//�ֲܳ���
	int32_t usableQty;//�ֲֿ�����	/ ��ƽ��
	int32_t frozenQty; //�������� = ���ն���+���ն���
	int32_t ydUsableQty; //��ֿ���
	int32_t tdUsableQty; //��ֿ���
	int32_t ydOffFrozenQty;	//��ƽ�ֶ���
	int32_t tdOffFrozenQty;	//��ƽ�ֶ���
	int32_t openFrozPositionQty;	//���ֶ���
	int32_t offFrozPositionQty;		//ƽ�ֶ��� = ����ƽ�ֶ��� + ����ƽ�ֶ���
	double currentStkValue;		//��ֵ
	double realtimeIncome;		//�ֻ���������
	int32_t bondPledgeQty;			//��Ѻȯ���
	int32_t currentQty;			//�������
	int32_t unsaleableqty;		//����ͨ�ֲ�					
	double plAmt;				//ӯ�����
	double dilutedCost;		//̯���ɱ�
	char currencyId[8];		//����
};

struct QryAccountField
{
	char productId[16];//��ƷID
	char productUnitId[16]; //�ʲ���Ԫ����
	char portfolioId[16];//Ͷ����ϴ���     
	bool realFundNavFlag;		//�Ƿ���Ҫʵʱ�����Ʒ��λ��ֵ
	int32_t authType;//Ȩ�����ͣ�1, ��ѯ; 2, ����
};

struct QryAccountRspField
{
	char productId[16];//��ƷID
	char productName[128];//��Ʒ����
	char productUnitId[16]; //�ʲ���Ԫ����	
	char acctId[16];
	int64_t fundShares;//��Ʒ�ݶ�
	double assetAmt;//���ʲ�
	double fundNav;	//ʵʱ��λ��ֵ
	char processFlag[8];//��Ʒָ������
	double productNav;	//��Ʒ��ֵ
	double preAssetAmt;		//�������ʲ�
	double preProductNav;   //���վ�ֵ
	char productUnitName[128]; //�ʲ���Ԫ����
	int32_t masterFlag; //���ʲ���Ԫ��־ 1���ʲ���Ԫ  0�����ʲ�
	int32_t fundType;//�ʽ�����
	double usable; //�ʽ����
	double frozenAmt; //�ʽ𶳽�
	double marketValue; //�ʽ���ֵ
	double previousAmt; //�������
	double floatPnl;	  //����ӯ��,�ڻ��ֻ�
	double marginAmt; //��֤�� ռ��
	double marginFrozenAmt;	//��֤�𶳽�
	double ydMarginAmt; //���ձ�֤��
	double realtimeAmt;//��Ȩ��
	double realtimePnl; //����ӯ��
	double risk;		//���ն� = marginAmt/realtimeAmt
	double fetchAmt;	//��ȡ��� = �ɳ���F_CanCashOutAmt�� �� ԤԼ��ȡ���
	double currentAmt;	//��ǰ���
	char currencyId[8];	//������Ϣ
	double exceptFrozenAmt;	//���ⶳ����
	double shHkUsableAmt;		//�۹�ͨ����
	double equityValue;		//��Ʊ��ֵ
	double fundValue;			//������ֵ
	double bondValue;			//ծȯ��ֵ
	double realtimeAmtByClosePric;//ʹ�����̼ۼ������Ȩ��
	double premium;//Ȩ����
};

struct RecallField
{
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char portfolioId[16];//Ͷ�����ID
	char stkCode[32];//��Լ����
	char exchId[4];//�г�����
	int64_t recallQty;//Recall����
};

struct QryEqualAccountField
{
	char productUnitId[16]; //�ʲ���Ԫ����
	int32_t fundType;//�ʽ�����
	char currencyId[8];//����
	char acctId[16];//��̨���ʽ��˺�
};

struct QryEqualAccountRspField
{
	char productUnitId[16]; //�ʲ���Ԫ����	
	int32_t fundType;//�ʽ�����
	char currencyId[8];//����
	char acctId[16];//��̨���ʽ��˺�
	double equalUsable; //����ת��֮���ʽ����
	double euqalFrozenAmt; //����ת��֮����ʽ𶳽�
	double equalPreviousAmt; //����ת��֮����������
	double equalCurrentAmt;	//����ת��֮��ĵ�ǰ���
	double equalTotalAmt;	//����ת��֮����ܶ����
};

struct QryPositionLimitField
{
	char productUnitId[16]; //�ʲ���Ԫ����
	char queryType[8];//��ѯ����1��PTH��ȣ�2����ʼ��ȯ���
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
};

struct QryPositionLimitRspField
{
	char productUnitId[16]; //�ʲ���Ԫ����
	char acctId[16];//��̨���ʽ��˺�
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
	int64_t positionLimit;//��ǰ�ֲֶ��
};

struct QryPositionQuantityField
{
	char productUnitId[16]; //�ʲ���Ԫ����
	char queryType[8];//��ѯ����1�����гֲ�������2���ѽ�����ȯ����
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
	char occurtime[32];//���ڣ�20181225000000
};

struct QryPositionQuantityRspField
{
	char productUnitId[16]; //�ʲ���Ԫ����
	char acctId[16];//��̨���ʽ��˺�
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
	char occurtime[32];//���ڣ�20181225000000
	int64_t positionQuantity;//��ǰ�ֲֶ��
};


struct BasketOrderDetailField
{
	char basketDetailId[16];//�����ڶ�����ʶ
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
	char bsFlag[4];//��(B)��(S)
	int64_t orderQty;//ί������
	double orderPrice;//ί�м۸��㷨����ʱ��ʾ�۸����ƣ�0��ʾ������
	double orderAmt;//ί�н��
	char orderPriceType[8];//ί������ Limit-�޼� Any-�м�
	char matchCondition[8]; //�м�����
	char offsetFlag[8];//��ƽ����
	char hedgeFlag[8];//Ͷ���ױ� 
	char currencyId[8];//����
};

struct NewBasketOrderField
{
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char portfolioId[16];//Ͷ�����ID
	int32_t fundType;//�ʽ�����
	char description[128];//���ӱ�ע
	//char localCurrencyId[8];//���ػ�������
	int32_t basketOrderCount;//��������ӵ���
	BasketOrderDetailField** basketDetail;//�����ӵ�����ϸ��Ϣ
};

struct NewBasketOrderRspField
{
	char basketId[32];//���ӱ��
	int32_t orderSuccessFlag;//�ñ������ӵ��Ƿ�ɹ���0-ʧ�ܣ�1-�ɹ�
	char errMsg[128];//�ñ������ӵ�ʧ��ʱ�Ĵ���ԭ��
	char contractNum[32];//���ɵĹ�̨��ͬ��
	char basketDetailId[16];//�����ڶ�����ʶ
	char batchNum[16];//�����ڲ�������
};

/*struct NewAlgoBasketOrderField
{
	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char portfolioId[16];//Ͷ�����ID
	int32_t fundType;//�ʽ�����
	char description[128];//���ӱ�ע
	char startTime[8];//hhmmss ����ʱ��
	char stopTime[8];//hhmmss Algoֹͣʱ��
	char algoType[8];//Algo�������ͣ�TWAP-TWAP��VWAP-VWAP
	double maxMarketShare;//����г�ռ�ȣ�(0-50]�� Ĭ�Ͽ�����33
	//char localCurrencyId[8];//���ػ�������
	int32_t basketOrderCount;//��������ӵ���
	BasketOrderDetailField** basketDetail;//�����ӵ�����ϸ��Ϣ
};*/

struct NewAlgoBasketOrderRspField
{
	char basketId[32];			// ���ӱ��
	char algoParentOrderId[32];	// �㷨ĸ������
	char basketDetailId[16];		// �����ڶ������к�
	char memo[64];				// �ӵ���ע
};

//Algo ĸ��״̬����
struct AlgoBasketOrderNotiField
{
	char basketId[32];//���ӱ��
	char algoParentOrderId[32];//Algoĸ��ID
	char basketDetailId[16];//�����ڶ�����ʶ

	char productId[16];//��ƷID
	char productUnitId[16];//�ʲ���Ԫ����
	char portfolioId[16];//Ͷ�����ID
	char exchId[4];//�г�����
	char stkCode[32];//��Լ����
	char bsFlag[4];//��(B)��(S)
	int64_t orderQty;//ί������

	int32_t algoStatus;//Algoĸ��״̬ ��Ҫ�ο����
	char algoOrderErrmsg[128];//Algoĸ�����ܾ���ֹͣ��ԭ��
	double knockProcess;//�ɽ�����
	int32_t knockQty;//�ۼƳɽ���
	double knockAmt;//�ۼƳɽ����
	double reckoningAmt; //������
	double averagePrice;//�ۼƳɽ�����
	int infoSerialNum;//������Ϣ���кţ���Ҫ�Դ˹���������Ϣ
};

// Algoָ�����
struct AlgoParentOrderActField
{
	char productId[16];			// ��ƷID
	char productUnitId[16];		// �ʲ���ԪID
	char portfolioId[16];			// Ͷ�����ID
	char actionFlag[4];			// ALGO����ָ��
	char algoParentOrderId[32];	// Algoĸ��ID
};

struct AlgoParentOrderActRspField
{
	char algoParentOrderId[32];	// Algoĸ��ID
};

// Algoָ���ѯ
struct QryAlgoParentOrderField
{
	char algoParentOrderId[32];	// Algoĸ��ID
	char queryFlag[4];			// 0-��ͨAlgo 1-�����Algo
};

// Algoָ���ѯ�ر�
struct QryAlgoParentOrderRspField
{
	char productId[16];			// ��ƷID
	char productUnitId[16];		// �ʲ���ԪID
	char portfolioId[16];			// Ͷ�����ID
	char algoParentOrderId[32];	// Algoĸ��ID
	char bsFlag[4];				// ��������
	char algoType[8];				// Algo��������
	int32_t algoStatus;			// Algoĸ��״̬
	char exchId[4];				// �г�����
	char stkCode[32];				// ��Լ����
	char startTime[8];			// ����ʱ�� hhmmss
	char stopTime[8];				// ֹͣʱ�� hhmmss
	int64_t orderQty;				// ί������
	double orderAmt;				// ί���ܶ�
	double maxMarketShare;		// ����г�ռ�ȣ�(0-50]�� Ĭ�Ͽ�����33
	char algoOrderErrmsg[128];		// Algoĸ�����ܾ���ֹͣ��ԭ��
	double knockProcess;			// �ɽ�����
	int32_t knockQty;				// �ۼƳɽ���
	double knockAmt;				// �ۼƳɽ����
	double reckoningAmt;			// ������
	double averagePrice;			// �ۼƳɽ�����
};

// �����ӵ���ѯ�ر�
struct QryBasketOrderDetailRspField
{
	char basketDetailId[16];	// �����ӵ�ID
	char bsFlag[4];			// ��������
	char exchId[4];			// �г�����
	char stkCode[32];			// ��Լ����
	char currencyId[8];		// ����
	double orderPrice;		// ί�м�
	double referPrice;		// �ο���
	int64_t orderQty;			// ί������
	double orderAmt;			// ί�н��
	int64_t knockQty;			// �ɽ�����
	double knockAmt;			// �ɽ����
	char orderStatus[8];		// ί��״̬
	char offsetFlag[8];		// ��ƽ����
	char hedgeFlag[8];		// Ͷ���ױ�
	char matchCondition[8];	// �м�����
	char orderPriceType[8];	// ί������ Limit-�޼� Any-�м�
};

// ���Ӳ�ѯ�ر�
struct QryBasketOrderRspField
{
	char basketId[32];		// ����ID
	char productId[16];		// ��ƷID
	char productUnitId[16];	// �ʲ���ԪID
	char portfolioId[16];		// Ͷ�����ID
	int32_t basketDetailCount;	// �����ӵ�����
	QryBasketOrderDetailRspField** basketDetail;	// �����ӵ��б�
};

struct TwapChildOrder
{
	char exchId[4];			// �г�����
	char stkCode[32];			// ��Լ����
	char bsFlag[4];			// ��������
	char offsetFlag[8];		//��ƽ����
	char hedgeFlag[8];		//Ͷ���ױ� 
	double priceLimit;		// �۸�����
	int64_t orderQty;			// ί������
	double orderAmt;
	char currencyId[8];		// ����
	char memo[64];
};

struct NewTwapBasketOrder
{
	char productId[16];			// ��ƷID
	char productUnitId[16];		// �ʲ���ԪID
	char portfolioId[16];			// Ͷ�����ID
	char startTime[8];//hhmmss ����ʱ��
	char stopTime[8];//hhmmss Algoֹͣʱ��
	double maxMarketShare;		// ����г�ռ�ȣ�(0-0.50]�� Ĭ�Ͽ�����0.33
	int32_t basketDetailCount;// �����ӵ�����
	TwapChildOrder** basketDetail;
};

struct QuickBasketChildOrder
{
	char exchId[4];			// �г�����
	char stkCode[32];			// ��Լ����
	char bsFlag[4];			// ��������
	double priceLimit;
	int64_t orderQty;			// ί������
	char currencyId[8];		// ����
	char memo[64];
};

struct NewQuickBasketOrder
{
	char productId[16];			// ��ƷID
	char productUnitId[16];		// �ʲ���ԪID
	char portfolioId[16];			// Ͷ�����ID
	char startTime[8];//hhmmss ����ʱ��
	char stopTime[8];//hhmmss Algoֹͣʱ��
	double maxMarketShare;		// ����г�ռ�ȣ�(0-0.50]�� Ĭ�Ͽ�����0.33
	int32_t basketDetailCount;// �����ӵ�����
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
	virtual const char* GetLastError() = 0;						// �������һ���ڲ�����

public:
	virtual bool OnRecall(const RecallField* pRspRecall) = 0;	// Recall��Ϣ��Ӧ
	// ί����ص�
	virtual bool OnRspNewOrder(const NewOrderRspField* pRspNewOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;							// ����������Ӧ
	virtual bool OnRspCancelOrder(const CancelOrderRspField* pRspCnclOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;				// ����������Ӧ
	virtual bool OnRspNewBasketOrder(const NewBasketOrderRspField* pRspNewBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;				// ������Ӧ
	virtual bool OnRspNewAlgoBasketOrder(const NewAlgoBasketOrderRspField* pRspNewAlgoBsktOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;	// Algo������Ӧ
	virtual bool OnRspAlgoParentOrderAct(const AlgoParentOrderActRspField* pRspAlgoPrntOrdAct, const ErrMsgField* errMsg, int requestId) = 0;				// Algo��ͣ������Ӧ
	virtual bool OnRspNewTwapOrder(const NewTwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	virtual bool OnRspNewVwapOrder(const NewVwapOrderRspField* pRspNewTwapOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	virtual bool OnRspNewQuickBasketOrder(const NewQuickBasketOrderRspField* pRspQuickBsktOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;
	// ������ص�
	virtual bool OnNotiOrder(const OrderNotiField* pNotiOrd, const ErrMsgField * errMsg) = 0;								// ί������֪ͨ
	virtual bool OnNotiKnock(const KnockNotiField* pNotiKnk, const ErrMsgField * errMsg) = 0;								// �ɽ�֪ͨ
	virtual bool OnNotiAlgoBasketOrder(const AlgoBasketOrderNotiField* pNotiAlgoBsktOrd, const ErrMsgField * errMsg) = 0;	// Algo״̬���ͻص�
	// ί�в�ѯ��ص�
	virtual bool OnRspQryOrder(const QryOrderRspField* pRspQryOrd, const ErrMsgField * errMsg, int requestId, bool last) = 0;								// ��ѯί��������Ӧ
	virtual bool OnRspQryKnock(const QryKnockRspField* pRspQryKnk, const ErrMsgField * errMsg, int requestId, bool last) = 0;								// ��ѯ�ɽ�������Ӧ
	virtual bool OnRspQryBasketOrder(const QryBasketOrderRspField* pRspQryBsktOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;				// ��ѯ����������Ӧ
	virtual bool OnRspQryAlgoParentOrder(const QryAlgoParentOrderRspField* pRspQryAlgoPrntOrd, const ErrMsgField* errMsg, int requestId, bool last) = 0;	// ��ѯAlgoָ��������Ӧ
	// �ֲ��ʽ��ѯ��ص�
	virtual bool OnRspQryPosition(const QryPositionRspField* pRspQryPos, const ErrMsgField * errMsg, int requestId, bool last) = 0;						// ��ѯ�ֲ�������Ӧ
	virtual bool OnRspQryAccount(const QryAccountRspField* pRspQryAcct, const ErrMsgField * errMsg, int requestId, bool last) = 0;						// ��ѯ�ʽ�������Ӧ
	virtual bool OnRspQryEqualAccount(const QryEqualAccountRspField* pRspQryEqAcct, const ErrMsgField * errMsg, int requestId, bool last) = 0;			// ��ѯ����ת������ʽ�������Ӧ
	virtual bool OnRspQryPositionLimit(const QryPositionLimitRspField* pRspQryPosLim, const ErrMsgField * errMsg, int requestId, bool last) = 0;	// ��ѯ��ǰ���������Ӧ
	virtual bool OnRspQryPositionQuantity(const QryPositionQuantityRspField* pRspQryPosQuant, const ErrMsgField * errMsg, int requestId, bool last) = 0;		// ��ѯ���гֲֻ��ѽ�����ȯ����������Ӧ
};

class TRADER_API_EXPORT IMSTradeAPI
{
public:
	virtual const char * GetLastError() = 0;				// �������´�����Ϣ
	virtual bool SetTradeSPI(IMSTradeSPI* pSpi) = 0;		// ע��ص��ӿ�
	virtual bool Initial(const char* addr) = 0;				// ��ʼ��IMS����վ��
	virtual bool TryLogin(const char* userId, const char * password, const char* pLicence) = 0;	// ��¼IMS
    TRADER_API_EXPORT static IMSTradeAPI* CreateAPI();						// ����APIʵ��
    TRADER_API_EXPORT static void ReleaseAPI(IMSTradeAPI* pApi);				// ����APIʵ��

public:
	// ί��������
	virtual int ReqNewOrder(NewOrderField* pNewOrd, int requestId) = 0;										// ��������: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqCancelOrder(CancelOrderField** pCnclOrd, int cancelOrderCount, int requestId) = 0;	// ��������: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqNewBasketOrder(NewBasketOrderField* pBsktOrd, int requestId) = 0;										// �����ӵ�: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqAlgoParentOrderAct(AlgoParentOrderActField* pAlgoPrntOrdAct, int requestId) = 0;// Algoָ������: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqNewTwapBasketOrder(NewTwapBasketOrder* pTwapOrd, int requestId) = 0;
	virtual int ReqNewVwapBasketOrder(NewVwapBasketOrder* pVwapOrd, int requestId) = 0;
	virtual int ReqNewQuickBasketOrder(NewQuickBasketOrder* pQuickOrd, int requestId) = 0;
	// ί�в�ѯ������
	virtual int ReqQryOrder(QryOrderField* pQryOrd, int requestId) = 0;											// ��ѯί������: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryKnock(QryKnockField* pQryKnk, int requestId) = 0;											// ��ѯ�ɽ�����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryAlgoParentOrder(QryAlgoParentOrderField* pQryAlgoPrntOrd, int requestId) = 0;				// Algoĸ����ѯ����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryBasketOrderList(int requestId) = 0;															// ���Ӳ�ѯ����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	// �ֲ��ʽ����ѯ����
	virtual int ReqQryPosition(QryPositionField* pQryPos, int requestId) = 0;									// ��ѯ�ֲ�����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryAccount(QryAccountField* pQryAcct, int requestId) = 0;										// ��ѯ�ʽ�����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryEqualAccount(QryEqualAccountField* pQryEqAcct, int requestId) = 0;						// ��ѯ����ת������ʽ�����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryPositionLimit(QryPositionLimitField* pQryPosLim, int requestId) = 0;			// ��ѯ��ǰ���: ����ֵС��0 ��������ͨ��GetLastError�õ�����
	virtual int ReqQryPositionQuantity(QryPositionQuantityField* pQryPosQuant, int requestId) = 0;				// ��ѯ���гֲֻ��ѽ�����ȯ����: ����ֵС��0 ��������ͨ��GetLastError�õ�����
};

