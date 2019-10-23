//----------------------------------------------------------------------------
// 版权声明：本程序模块属于金证微内核架构平台(KMAP)的一部分
//           金证科技股份有限公司  版权所有
//
// 文件名称：maMktData.h
// 模块名称：微架构客户端(Client)的行情结构定义
// 模块描述：
// 开发作者：何万刚
// 创建日期：2015-07-17
// 模块版本：1.0
//----------------------------------------------------------------------------
// 修改日期      版本          作者            备注
//----------------------------------------------------------------------------
// 2015-07-17    1.0          何万刚          初创
// 2016-03-18    1.0          高剑            新增ST_MKT_DATA2，ST_MKT_DATA3成交量，成交金额字段，
//                                            出现空字符串时，无法反序列化，重写反序列化实现
// 2016-08-16    1.0          高剑            ST_MKT_DATA2增加昨日收盘价，以及4个预留字段
// 2016-12-16    1.1          高剑            增加ST_MKT_DATA字段的描述
// 2018-03-08    1.2          高剑            增加ST_MKT_MATCH逐笔、ST_MKT_ORDER委托结构体,ST_MKT_INDEX指标结构体
//                                            行情订阅启用DATA_SET入参,2(原有行情订阅),8(成交订阅),16(委托订阅),32(指标订阅),入参为空向前兼容,订阅多种取值用或
//----------------------------------------------------------------------------
#if !defined(__MA_MKT_DATA_H__)
#define __MA_MKT_DATA_H__

#include <stdio.h>

#if defined(WIN32) || defined(WIN64) || defined(OS_IS_WINDOWS)
  #if defined(MA_CLIAPI_EXPORTS)
    #define MACLIAPI __declspec(dllexport)
  #else
    #define MACLIAPI __declspec(dllimport)
  #endif
  #define MACLI_STDCALL __stdcall
  #define MACLI_EXPORTS __declspec(dllexport)
#else
  #define MACLIAPI
  #define MACLI_STDCALL
  #define MACLI_EXPORTS
#endif

#if defined (_MSC_VER) && (_MSC_VER == 1200)
  #define FORMAT_LONGLONG "%I64d"
  #if defined(WIN32) || defined(WIN64)
  typedef __int64 LONGLONG;
  #endif
#else
  #define FORMAT_LONGLONG "%lld"
  typedef long long LONGLONG;
#endif

#pragma pack(1)
//////////////////////////////////////////////////////////////////////////
// llMktDataTime:行情服务器的接收时间,格式YYYYMMDDHHMMSSsss
// chExchange   :交易所，0上交所、1深交所、A上期所、B大商所、C郑商所、G中金所
// iTrdDate     :行情数据本身的日期,格式YYYYMMDD
// iDataTime    :行情数据本身的时间,格式HHMMSSsss
// uiMatchedCnt :笔数,无小数部分
// szStatus     :品种状态
//               '0' 未设置
//               'Y' 新产品
//               'R' 交易间，禁止任何交易活动
//               'P' 休市，例如，午餐休市。无撮合市场内部信息披露。
//               'B' 停牌
//               'C' 收市
//               'D' 停牌
//               'Z' 产品待删除
//               'V' 结束交易
//               'T' 固定价格集合竞价
//               'W' 暂停，除了自有订单和交易的查询之外，任何交易活动都被禁止。
//               'X' 停牌,( 'X'和'W'的区别在于'X'时可以撤单)
//               'I' 盘中集合竞价。
//               'N' 盘中集合竞价订单溥平衡
//               'L' 盘中集合竞价PreOBB
//               'I' 开市集合竞价
//               'M' 开市集合况竞价OBB
//               'K' 开市集合竞价订单溥平衡(OBB)前期时段
//               'S' 非交易服务支持
//               'U' 盘后处理
//               'F' 盘前处理
//               'E' 启动
//               'O' 连续撮合
//               'Q' 连续交易和集合竞价的波动性中断
//
// 价格         :元,4位小数,实际价格10000倍
// 量、成交量   :股票为股数,期权期货为手数,均无小数部分
// 金额         :元,无小数部分
// 市盈率等其它字段,格式待定
//////////////////////////////////////////////////////////////////////////
//行情数据(快照)
struct MACLIAPI ST_MKT_DATA
{
  bool      bInitialized;
  LONGLONG  llMktDataTime;                    // 接收时间(YYYYMMDDHHMMSSmmm)
  char      chExchange;                       // 交易所
  char      szTrdCode[16 + 1];                // 品种代码
  int       iTrdDate;                         // 行情日期
  int       iDataTime;                        // 行情时间(HHMMSSmmm)
  char      szStatus[1+1];                    // 品种状态

  unsigned int    uiPreClosePrice;            // 昨收盘价
  unsigned int    uiOpenPrice;                // 开盘价
  unsigned int    uiHighPrice;                // 最高价
  unsigned int    uiLowPrice;                 // 最低价
  unsigned int    uiCurPrice;                 // 最新价
  unsigned int    uiAnsPrice[10];             // 申卖价
  LONGLONG        llAnsVol[10];               // 申卖量
  unsigned int    uiBidPrice[10];             // 申买价
  LONGLONG        llBidVol[10];               // 申买量
  unsigned int    uiMatchedCnt;               // 成交笔数
  LONGLONG        llMatchedVol;               // 成交总量
  LONGLONG        llMatchedAmtTot;            // 成交总金额
  LONGLONG        llBidVolTot;                // 买入总量
  LONGLONG        llAnsVolTot;                // 卖出总量
  unsigned int    uiBidPriceAvg;              // 平均委买价格
  unsigned int    uiAnsPriceAvg;              // 平均委卖价格
  int             iIopv;                      // IOPV
  int             iYield;                     // 到期收益率
  unsigned int    uiHighLmtPrice;             // 涨停价
  unsigned int    uiLowLmtPrice;              // 跌停价
  char            szStkPrefix[16 + 1];        // 证券前缀
  LONGLONG        llPreOpenVol;               // 昨持仓量
  unsigned int    uiSettPrice;                // 今结算价
  int             iPreDelta;                  // 昨虚实度
  int             iCurrDelta;                 // 今虚实度
  int             iNsyl1;                     // 市盈率1
  int             iNsyl2;                     // 市盈率2
  int             iNsd2;                      // 升跌2
  LONGLONG        llOpenVol;                  // 持仓总量
  unsigned int    uiPreSettPrice;             // 昨结算价
  unsigned int    uiClosePrice;               // 今收盘
  char            szContractID[32 +1];        // 期权合约代码
  char            szUnderlyingSecurityID[32]; // 标的证券代码
  char            szCallOrPut[1+1];           // 认购认沽C1
  int             iExerciseDate;              // 期权行权日，YYYYMMDD
  char            szCallOrPutStatus[1+1];     //

  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);
};

//行情数据(推送)
struct MACLIAPI ST_MKT_DATA2
{
  bool            bInitialized;
  LONGLONG        llMktDataTime;              // 接收时间(YYYYMMDDHHMMSSmmm)
  char            chExchange;                 // 交易所
  char            szTrdCode[16 + 1];          // 品种代码
  int             iTrdDate;                   // 行情日期
  int             iDataTime;                  // 行情时间(HHMMSSmmm)
  char            szStatus[1+1];              // 品种状态

  unsigned int    uiCurPrice;                 // 最新价（价格*10000)
  unsigned int    uiAnsPrice[10];             // 申卖价（价格*10000)
  LONGLONG        llAnsVol[10];               // 申卖量
  unsigned int    uiBidPrice[10];             // 申买价（价格*10000)
  LONGLONG        llBidVol[10];               // 申买量
  unsigned int    uiMatchedCnt;               // 成交笔数
  LONGLONG        llMatchedVol;               // 成交量
  LONGLONG        llMatchedValue;             // 成交金额

  unsigned int    uiPreClosePrice;            // 昨收盘价 2016/8/16新增
  unsigned int    uiReserved1;                // 预留1    2016/8/16新增
  unsigned int    uiReserved2;                // 预留2    2016/8/16新增
  unsigned int    uiReserved3;                // 预留3    2016/8/16新增
  unsigned int    uiReserved4;                // 预留4    2016/8/16新增

  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);
};

//行情数据(预留)
struct MACLIAPI ST_MKT_DATA3
{
  bool            bInitialized;
  LONGLONG        llMktDataTime;              // 接收时间(YYYYMMDDHHMMSSmmm)
  char            chExchange;                 // 交易所
  char            szTrdCode[16 + 1];          // 品种代码
  int             iTrdDate;                   // 行情日期
  int             iDataTime;                  // 行情时间(HHMMSSmmm)
  char            szStatus[1+1];              // 品种状态

  unsigned int    uiCurPrice;                 // 最新价（价格*10000)
  unsigned int    uiAnsPrice[5];              // 申卖价（价格*10000)
  LONGLONG        llAnsVol[5];                // 申卖量
  unsigned int    uiBidPrice[5];              // 申买价（价格*10000)
  LONGLONG        llBidVol[5];                // 申买量
  unsigned int    uiMatchedCnt;               // 成交笔数
  LONGLONG        llMatchedVol;               // 成交量
  LONGLONG        llMatchedValue;             // 成交金额
  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);
};

//成交数据
struct MACLIAPI ST_MKT_MATCH
{
  bool           bInitialized;
  char           chExchange;             // 交易所
  char           szTrdCode[16 + 1];      // 品种代码
  int            iTrdDate;               // 行情日期
  char           chType;                 // 类型:0多换1空换2双换;3多开4空开5双开;6多平7空平8双平
  char           chBSSide;               // 买卖方向:0:买 1:卖 其它:
  int            iDataDate;              // 数据日期
  int            iDataTime;              // 行情时间  
  int            iIopv;                  // (Indicative Optimized Portfolio Value)ETF的基金份额参考净值,由交易所每15秒计算并公告一次,实际价格10000倍
  unsigned int   uiPrice;                // 成交价	
  int            iOpenVol;               // 持仓增量
  unsigned int   uiMatchedCnt;           // 成交笔数
  unsigned int	 uiMatchedVol;           // 成交量
  LONGLONG       llMatchedAmtTot;        // 成交金额
  int            iTimediff;              // 计算此笔成交的两笔快照时间差 越小越准确
  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);
};

//委托数据
struct MACLIAPI ST_MKT_ORDER
{
  bool           bInitialized;
  char           chExchange;             // 交易所
  char           szTrdCode[16 + 1];      // 品种代码
  int            iTrdDate;               // 行情日期
  char           chOrderType;            // 委托类别 1=买  2=卖    G=借入    F=出借
  int            iDataDate;              // 委托日期
  int            iDataTime;	             // 委托时间
  int 	         iOrderNo;               // 委托号
  unsigned int	 uiOrderPrice;           // 委托价格
  int 	         iOrderVol;              // 委托数量
  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);  
};

// 指标数据
struct MACLIAPI ST_MKT_INDEX
{
  bool                bInitialized;
  char                chExchange;         // 交易所
  char                szTrdCode[16 + 1];  // 品种代码
  int                 iTrdDate;           // 行情日期
  unsigned short      uiIndexType;        // 指标类别 指标类别定义指标数据的具体含义,具体类型后续扩展
  int                 iDataDate;          // 指标日期
  int                 iDataTime;          // 指标时间
  unsigned long long  ullIndexMask;       // 数据掩码 i位bit决定uiIndexData[i]有无
  unsigned int	      uiIndexData[64];    // 数据    
  int Serialize(char *p_szFmtMktDataStr, int p_iFmtMktDataStrLen);
  int DeSerialize(char * p_szFmtMktDataStr);  
};

struct ST_EXCHANGE_CVT
{
  char chExchange;
  char szHongHuiType[3+1];
  char szTsType[2+1];
};

#pragma pack()

const ST_EXCHANGE_CVT g_stExchangeCvt[] =
{
  {'0', "SZ",  "SZ"},   // 深圳证券交易所
  {'1', "SH",  "SH"},   // 上海证券交易所
  {'A', "SHF", "SQ"},   // 上海期货交易所
  {'B', "DCE", "DL"},   // 大连期货交易所
  {'C', "CZC", "ZZ"},   // 郑州期货交易所
  {'G', "CF",  "ZJ"},   // 中国金融期货交易所
  {'7', "HK",  "HK"},   // 香港交易所
  {'2', "BJ",  "BJ"}    // 北京证券交易所
};

#endif  // __MA_MKT_DATA_H__
