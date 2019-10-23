#if !defined(__MA_CLI_MKT_DATA_STRUCT_H__)
#define __MA_CLI_MKT_DATA_STRUCT_H__

#include "maMktData.h"
#include "maCliTradeApi.h"
#include "maCliStkTradeApiStruct.h"
#include "maCliOptTradeApiStruct.h"
#include "maCliFislTradeApiStruct.h"
#include "maCliFutureTradeApiStruct.h"
#include "maCliCosTradeApiStruct.h"

BGN_NAMESPACE_MACLI

struct MACLIAPI CReqSubMktDataField:public  CReqSubTopicField
{
  //char          szMktCode[24+1];  //标的代码  MKT_CODE  CHAR(24)  X 订阅行情的标的代码，如SH.600446
};

struct MACLIAPI CRspSubMktDataField:public  CRspSubTopicField
{
  //char          szMktCode[24+1];
};
struct MACLIAPI CReqUnsubMktDataField:public CReqUnsubTopicField
{
  //LONGLONG llCuacctCode;
};
struct MACLIAPI CRspUnsubMktDataField:public CRspUnsubTopicField
{

};
 struct MACLIAPI CReqPlayBackField
 {
  char chPlayCtrl;      //播放控制  PLAY_CTRL CHAR(1) √  9080  ‘0’—停止播放 ‘1’—开始播放
  char szTrdDate[8+1];    //TRD_DATE              8844
  int  iBgnTime;        //BGN_TIME              916
  int  iEndTime;        //END_TIME              917
  char chPlayBackType;    //PLAY_BACK_TYPE        8996
  int  iPlayInterval;     //回放时间间隔  PLAY_INTERVAL INTEGER X 9081  播放单位行情时间后的间隔时间(PLAY_INTERVAL以毫秒为单位)
  char szStkCode[128+1];    //股票代码  STK_CODE  VARCHAR(128)  X 48  播放行情代码用逗号（，）分割，如000001,000002,000003….
 };
 struct MACLIAPI CRspPlayBackField
 {
  //none
 };
 //自选股编辑
 struct MACLIAPI CReqMktDataPortfolioField
 {
   char     szUserCode[16+1]; //用户代码 USER_CODE VARCHAR(16) Y 8902 cos用户代码
   char     chStkex;          //交易所   STKEX     CHAR(1)     Y 207
   char     szStkbd[2+1];     //板块     STKBD     CHAR(2)     Y 625
   char     szTrdCodes[512+1];//品种代码 TRD_CODES VARCHAR(512)X 48  竖线分割(逐条添加或删除时必传)
   char     chOperType;       //操作类型 OPER_TYPE CHAR(1)     Y 8844;0:添加；1:逐条删除；2:范围删除；其它为参数错误
   char     szBegCode[8+1];   //开始代码 BEG_CODE  VARCHAR(8)  X 9000范围删除时必传(此范围包含开始代码和结束代码本身)
   char     szEndCode[8+1];   //结束代码 END_CODE  VARCHAR(8)  X 9001范围删除时必传
 };

 struct MACLIAPI CRspMktDataPortfolioField
 {
   int      iAmount;          //数量AMOUNTINTEGER8900返回插入或删除的数量
 };
 //自选股查询
 struct MACLIAPI CReqMktDataPortfolioInfoField
 {
   char     szUserCode[16+1]; //用户代码 USER_CODE  VARCHAR(16) Y 8902 cos用户代码
   char     chStkex;         //交易所    STKEX      CHAR(1)     Y 207
   char     szStkbd[2+1];    //板块      STKBD      CHAR(2)     Y 625
   char     chQueryFlag;     //查询方式  QUERY_FLAG CHAR(1)     Y 9080;0：按位置向后查询1：按位置向前查询2：按范围查询其它参数错误
   char     szStkCode[8+1];  //查询位置  STK_CODE   VARCHAR(8)  X 8991按位置查询时必传
   int      iQueryNum;       //查询行数  QUERY_NUM  INTEGER     X 8992按位置查询时必传，最多1000条
   char     szBegCode[8+1];  //开始代码  BEG_CODE   VARCHAR(8)  X 9000范围查询时必传(此范围包含开始代码和结束代码本身)
   char     szEndCode[8+1];  //结束代码  END_CODE   VARCHAR(8)  X 9001范围查询时必传
 };

 struct MACLIAPI CRspMktDataPortfolioInfoField
 {
   char     szUserCode[16+1]; //用户代码 USER_CODE   VARCHAR(16)   8902 cos用户代码
   char     chStkex;           //交易所   STKEX      CHAR(1)       207
   char     szStkbd[2+1];      //板块    STKBD       CHAR(2)       625
   int      iAmount;           //数量    AMOUNT      INTEGER       8900
   char     szStkCodes[1536+1];//代码(复数)STK_CODES VARCHAR(512)  48  |分割多个代码
 };
//  代码表查询
//   struct MACLIAPI CReqMktDataCodeTableField
//   {
//     char     chStkex;           //交易所STKEXCHAR(1)Y 207交易所
//     char     szStkbd[2+1];      //板块 STKBDCHAR(2)Y 625板块
//   };
//
//   struct MACLIAPI CRspMktDataCodeTableField
//   {
//     char     chStkex;           //交易所  STKEX CHAR(1)       207
//     char     szStkbd[2+1];      //板块   STKBD CHAR(2)        625
//     int      iAmount;           //数量   AMOUNTINTEGER        8901
//     char     szCodes[512000+1]; //代码及属性CODES VARCHAR(512000)8902{code|CNNname|ENName|type}{ code|CNNname|ENName|type }....
//   };
//   //代码信息查询
//   struct MACLIAPI CReqMktDataCodeInfoField
//   {
//     char     chStkex;           //交易所 STKEX   CHAR(1)    Y 207
//     char     szStkbd[2+1];      //板块  STKBD   CHAR(2)    Y 625
//     char     szStkCode[12+1];   //证券代码STK_CODEVARCHAR(12)Y 48
//   };
//
//   struct MACLIAPI CRspMktDataCodeInfoField
//   {
//     char     chStkex;          //交易所  STKEX CHAR(1)    207
//     char     szCode[8+1];      //交易所代码CODE  VARCHAR(8) 8901
//     char     szCnname[32+1];   //中文名称 CNNAMEVARCHAR(32)8902
//     char     szEnname[32+1];   //英文名称 ENNAMEVARCHAR(32)8903
//     int      iType;            //代码类型 TYPE  INTEGER    8904
//   };

 //k线数据
 struct MACLIAPI CReqMktDataKDataField
 {
   char     chMarket;        //市场  MARKET       CHAR(1)     Y 8899
   char     szCode[8+1];     //代码  CODE         VARCHAR(8)  Y 8900
   char     chCqFlag;        //除权标志CQ_FLAG      CHAR(1)   Y 89010：不复权（仅支持）1：向前复权（从现在向过去）2：向后复权（从过去向现在）
   int      iCqDate;         //复权日期CQ_DATE      INTEGER   Y 8902
   char     chQjFlag;        //全价标志QJ_FLAG      CHAR(1)   Y 89030:净价；1：全价
   char     szCycType[2+1];  //数据周期CYC_TYPE     VARCHAR(2)Y 89040：秒（不支持）1：分2：日3：周4：月5：季6：半年7：年8：tickbar（不支持）9：5分10：15分11：30分12：60分
   int      iCycDef;         //周期数量CYC_DEF      INTEGER   X 8905
   char     chAutoComplete;  //自动补齐AUTO_COMPLETECHAR(1)   Y 89060：不补齐其它补齐
   int      iBegDate;        //开始日期BEG_DATE     INTEGER   Y 8907 <0:上市日期开始0：当天开始
   int      iEndDate;        //结束日期END_DATE     INTEGER   X 8908
   int      iBegTime;        //开始时间BEG_TIME     INTEGER   Y 8909
   int      iEndTime;        //结束时间END_TIME     INTEGER   Y 8910
 };

 struct MACLIAPI CRspMktDataKDataField
 {
   char     chMarket;         //市场  MARKET  CHAR(1)          8899
   char     szCode[8+1];      //代码  CODE    VARCHAR(8)       8900
   int      iAmount;          //数量  AMOUNT  INTEGER          8902数据条数
   int      iBegDate;         //开始日期      BEG_DATEINTEGER  8907
   int      iEndDate;         //结束日期      END_DATEINTEGER  8908
   int      iBegTime;         //开始时间      BEG_TIMEINTEGER  8909
   int      iEndTime;         //结束时间      END_TIMEINTEGER  8910
   char     szData[256000+1]; //数据  DATA    VARCHAR(256000)  8901{日期|时间|开盘价|最高|最低|收盘价|成交量}{}...其中，日期YYYYMMDD时间HHMMSSmmm价格金额为实际价格10000倍
 };
 //tick快照
 struct MACLIAPI CReqMktDataTickSnapshotField
 {
   char     chStkex;          //交易所    STKEX     CHAR(1)     Y 8899
   char     szStkCodes[512+1];//代码(复数)STK_CODES VARCHAR(512)Y 8900 |分割多只代码,最多支持单次40只代码，超过40只，多余部分不再处理

 };

 struct MACLIAPI CRspMktDataTickSnapshotField
 {
   char     chStkex;       //交易所STKEX CHAR(1)         8899
   int      iAmount;       //数量  AMOUNTINTEGER         8901返回的数据条数
   char     szData[40960+1];//数据 DATA  VARCHAR(40960)  8902{..|..|..}{..|..|..}...其中，日期YYYYMMDD时间HHMMSSmmm价格金额为实际价格10000倍,整数；具体格式为ST_MKT_DATA结构体字段
 };

END_NAMESPACE_MACLI

#endif  //__MA_CLI_MKT_DATA_STRUCT_H__