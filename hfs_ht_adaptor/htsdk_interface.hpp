#ifndef __HT_HT_SDK_INTERFACE_H__
#define __HT_HT_SDK_INTERFACE_H__

#ifdef _WIN32
#   ifdef HT_API_EXPORT
#       define HT_API __declspec(dllexport)
#   else
#       define HT_API __declspec(dllimport)
#   endif
#else
#   define HT_API
#endif

/*
* c header
*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <float.h>

#ifdef _WIN32
#   include <WS2tcpip.h>    // before Windows.h
#   include <Windows.h>
#   include <process.h>
#   include <Psapi.h>
#   include <io.h>
#   include <sys/locking.h>
#   include <direct.h>
#else
#   include <dirent.h>
#   include <sys/time.h>
#   include <arpa/inet.h>
#   include <dlfcn.h>
#   include <execinfo.h>
#   include <unistd.h>
#   include <sys/ipc.h>
#   include <sys/shm.h>
#   include <sys/mman.h>
#   include <sys/sem.h>
#   include <semaphore.h>
#   include <sys/syscall.h>
#endif


/*
* c++ header
*/
#ifdef __cplusplus
#   include <string>
#   include <map>
#   include <vector>
#   include <iostream>
#   include <set>
#   include <list>
#   include <exception>
#   include <typeinfo>
#endif


/*
* about OS difference
*/


#ifdef _WIN32
typedef void HT_void;
#else
#   define HT_void void     // make g++ comfortably
#endif

typedef bool HT_bool;

// for string
typedef char HT_char;
typedef unsigned char HT_uchar;

// for binary
typedef char HT_int8;
typedef unsigned char HT_uint8;

typedef short HT_int16;
typedef unsigned short HT_uint16;

typedef int HT_int32;
typedef unsigned int HT_uint32;

#ifdef _WIN32
typedef __int64 HT_int64;
typedef unsigned __int64 HT_uint64;
#else
typedef long long HT_int64;
typedef unsigned long long HT_uint64;
#endif

typedef long HT_long;               // DO NOT use this individually
typedef unsigned long HT_ulong;     // DO NOT use this individually

typedef float HT_float;
typedef double HT_double;


class CHtMessage;

/**
*消息打包类
*/
class HT_API CPackMessageCommon
{
public:
    CPackMessageCommon();
    ~CPackMessageCommon();

    /**
    *根据AddValue等函数设置的信息，打包得到消息体指针，此处的指针可以直接用同步或者异步的方式发送，无需调用者回收内存
    * @param  
    * @return  CHtMessage * pMsg  消息体指针
    */
    CHtMessage* GetPackedMsg();

    /**
    *通过AddValue函数设置当前行指定的key对应的char型数据
    * @param  const HT_uint32 nKey    需要设置的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_char cValue    指定字段定义的char型值
    * @return  等于0设置成功，否则失败
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_char cValue);

    /**
    *通过AddValue函数设置当前行指定的key对应的HT_uint32型数据
    * @param  const HT_uint32 nKey    需要设置的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int32 cValue    指定字段定义的HT_uint32型值
    * @return  等于0设置成功，否则失败
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_int32 nValue);

    /**
    *通过AddValue函数设置当前行指定的key对应的HT_uint64型数据
    * @param  const HT_uint32 nKey    需要设置的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int64 lValue    指定字段定义的HT_uint64型值
    * @return  等于0设置成功，否则失败
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_int64 lValue);

    /**
    *通过AddValue函数设置当前行指定的key对应的HT_double型数据
    * @param  const HT_uint32 nKey    需要设置的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_double cValue    指定字段定义的HT_double型值
    * @return  等于0设置成功，否则失败
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_double dValue);

    /**
    *通过AddValue函数设置当前行指定的key对应的字符串型数据
    * @param  const HT_uint32 nKey    需要设置的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_char * pstValue    指定字段定义的字符串型值
    * @return  等于0设置成功，否则失败
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_char * pstValue);

    /**
    *通过key删除当前行对应的键值的字段值
    * @param  const HT_uint32 nKey    需要删除的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @return  
    */
    HT_int32 DelValue(const HT_uint32 nKey);

    /**
    *获取消息体总的行数
    * @return  const HT_int32 消息体行数
    */
    const HT_int32 GetRowCount();

    /**
    *设定当前消息体游标的行号，打包消息时，行号最大只能设置到之前设置过最大行号的下一行, 行号从0开始
    * @param  const HT_int32 指定消息体的行号
    * @return  true成功，false失败
    */
    HT_bool SetCurrentRow(HT_int32 iRow);

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_char型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_char&  cValue    指定字段对应的HT_char型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_char& cValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_uint32型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int32&  cValue    指定字段对应的HT_uint32型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int32& nValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_uint64型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int64&  lValue    指定字段对应的HT_uint64型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int64& lValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_double型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_double&  cValue    指定字段对应的HT_double型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_double& dValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的字符串型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param const HT_char *&  返回值为0时，指针返回指定字段对应的字符串
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, const HT_char* &pstValue) const;
    
    /**
    *利用加密函数来对密码进行加密
    * @param const char* pIn 传入的待加密的字符串
    * @param const char* pOut 输出参数，加密之后的字符串，传入长度建议长一点，比如256，由外部申请
    * @param HT_int32 outBufferLengh 输出参数的长度
    * @return HT_int32 =0 成功  等于-1，传入参数有空指针， 等于-10023，传入参数的长度过短
    */
    static HT_int32 Encode(const HT_char *pIn, HT_char *pOut, HT_int32 outBufferLengh);
private:
    HT_uint32 m_nCurrentIndex;
    CHtMessage *m_CHtMessage;
};

class HT_API CUnPackMessageCommon
{

public:
    CUnPackMessageCommon();
    ~CUnPackMessageCommon();

    /**
    *设定当前消息体
    * @param  const CHtMessage * cMessage指定消息体指针
    * @return  0成功，-1失败
    */
    HT_int32 SetPackedMsg(const CHtMessage *cMessage);

    /**
    *获取消息体总的行数
    * @return  const HT_int32 消息体行数
    */
    const HT_int32 GetRowCount();

    /**
    *设定当前消息体游标的行号，行号最大只能设置到GetRowCount() - 1, 行号从0开始
    * @param  const HT_int32 指定消息体的行号
    * @return  true成功，false失败
    */
    HT_bool SetCurrentRow(HT_int32 iRow);

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_char型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_char&  cValue    指定字段对应的HT_char型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_char& cValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_uint32型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int32&  cValue    指定字段对应的HT_uint32型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int32& nValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_uint64型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_int64&  lValue    指定字段对应的HT_uint64型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int64& lValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的HT_double型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param  const HT_double&  cValue    指定字段对应的HT_double型值
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_double& dValue)const;

    /**
    *通过GetValue函数获取当前行指定的key对应的字符串型数据
    * @param  const HT_uint32 nKey    需要获取的字段定义，参见HtClientFiledDefine.h文件中HtFiled域中的定义
    * @param const HT_char *&  返回值为0时，指针返回指定字段对应的字符串
    * @return  等于0获取成功，否则失败
    */
    HT_int32 GetValue(const HT_uint32 nKey, const HT_char* &pstValue) const;

private:
    HT_uint32 m_nCurrentIndex;
    const CHtMessage *m_CHtMessage;
};


class CCallbackInterface
{
public:
/**
    *异步发送消息的回复处理函数
    * @param  HT_int32 iRet              异步请求成功标识
    * @param  const CHtMessage* pstMsg   异步请求回复的消息体指针
    * @return  
    */
    //异步请求回调，需使用者释放pstMsg资源
    virtual HT_void ProcAsyncCallBack(HT_int32 iRet, const CHtMessage* pstMsg) = 0; 

    /**
    *各类事件通知，用户可以在这里处理事件
    * @param  HT_int32 eventType              事件类型
    * @param  const CHtMessage* pstMsg       事件的消息体指针
    * @return  
    */
    virtual HT_void ProcEvent(HT_int32 eventType, const CHtMessage* pstMsg) = 0;

    /**
    *网络断线重连后的通知，业务可以在这里处理网络断线重连后的事情，比如重新订阅推送
    * @return  
    */
    virtual HT_void OnConnect() = 0;

    /**
    *网络断开后的通知，此时业务发送任何消息都是失败的
    * @return  
    */
    virtual HT_void OnClose() = 0;

     /**
    *用户登录状态已失效，用户可以在这里处理失效之后的逻辑，比如根据pstMsg中的用户id，对该用户进行重新登录处理, 需使用者释放pstMsg资源
    * @return  
    */
    virtual HT_void OnTokenLoseEfficacy(const CHtMessage* pstMsg) = 0;

    /**
    *网络发送失败
    * @return  
    */
    virtual HT_void OnError() = 0;
};


class CSendMsgInterface
{
protected:
    CSendMsgInterface(HT_void);
    virtual ~CSendMsgInterface(HT_void);
public:
     /**
    *异步发送消息
    * @param  const CHtMessage* pstMsg   需要发送的消息体指针
    * @return  等于0发送成功，否则失败
    */
    virtual HT_int32 Send(const CHtMessage* pstMsg) = 0;

    /**
    *同步发送消息
    * @param  const CHtMessage* pstMsg       需要发送的消息体指针
    * @param  const CHtMessage* pstRsponse   回复的消息体指针
    * @param  HT_int32 timeout               同步请求的超时时间
    * @return  等于0同步请求成功并收到回复，否则失败
    */
    //注意，对于同步调用的方式，如果返回pstRsponse不为空的话，
    // pstRsponse需要调用者主动调用HtDecreaseMsgPtr(pstRsponse)来释放消息的资源，否则会造成内存泄露
    virtual HT_int32 Send_Sync(const CHtMessage* pstMsg, CHtMessage* &pstRsponse, HT_int32 timeout = 10000) = 0;
};

extern "C"
{
/**
*初始化SDK
* @param  const HT_char * filePath  license以及网络配置等文件的目录路径
* @param  const HT_char * channelID    SDK通道标识
* @param  HT_int32 timeout  SDK初始化的超时时间
* @return HT_int32   等于0，初始化成功， 否则返回的是错误原因对应的错误码
*/
HT_API HT_int32 HtClientManagerInit(const HT_char * filePath, HT_int32 timeout = 10000, const HT_char * channelID = NULL);

/**
*销毁SDK实例
* @param  const HT_char * channelID    SDK通道标识
* @return HT_int32   等于0，初始化成功， 否则返回的是错误原因对应的错误码
*/
HT_API HT_int32 HtClientManagerDestroy(const HT_char * channelID = NULL);

/**
* 初始化实现CSendMsgInterface接口的类的指针
* @param CCallbackInterface * pCallbackInterface 传入业务的回调类指针
* @param  const HT_char * channelID    SDK通道标识
* @return 实现CSendMsgInterface接口的类的指针  返回为空则为失败
*/
HT_API CSendMsgInterface* HtSendMsgInterfaceInit(CCallbackInterface * pCallbackInterface, const HT_char * channelID = NULL);

/**
* 获取实现CSendMsgInterface接口的类的指针
* @param  const HT_char * channelID    SDK通道标识
* @return实现CSendMsgInterface接口的类的指针 返回为空则为失败
*/
HT_API CSendMsgInterface* HtGetSendMsgInterface(const HT_char * channelID = NULL);

/**
*将消息体指针的引用计数减去1，由SDK完成消息体指针的内存回收
* @param  const CHtMessage * pMsg  需要计数器减一的消息体指针
* @return 
*/
HT_API HT_void HtDecreaseMsgPtr(const CHtMessage * pMsg);

/**
*根据消息体指针指向的消息体内容，转换成json格式的字符串，pOut由外部自行申请并完成内存回收
* @param  const CHtMessage * pMsg  需要转成json格式字符串的消息体指针
* @param  HT_char * pOut  用于存放转换后的json格式字符串的buffer指针
* @param  HT_int32 bufferLengh  buffer的大小
* @return HT_int32   大于等于0则转换成功，此时返回值即字符串的有效长度 -10002 入参有空指针 -10023 传入用于接收的字符串内存大小不够
*/
HT_API HT_int32 HtMsgPtrToString(const CHtMessage * pMsg, HT_char * pOut, HT_int32 bufferLengh);


/**
*获取消息体的唯一消息号
* @param const CHtMessage* pstMsg 传入的待取消息号的消息体指针
* @return const HT_char *   等于NULL获取失败， 否则返回的是指向消息号字符串的指针
*/
HT_API HT_int32 HtGetClientMsgID(const CHtMessage* pstMsg, const HT_char * & pClientMsgID);


    
}



//错误码描述信息

// 0         成功     
// -1        失败     
// -10001    网络错误 
// -10002    参数错误 
// -10003    buffer大小不够
// -10004    消息体为空
// -10005    协议报文json中缺少协议头
// -10006    协议报文json中无字段数据
// -10007    协议报文json中无正文数据
// -10008    报文头缺失
// -10009    消息体类型转换失败
// -10010    消息写入收发队列失败
// -10011    不存在对应字段
// -10012    字段对应的值类型转换失败
// -10013    内存new失败
// -10014    网络收发模块未初始化完成
// -10015    SDK管理器未完成初始化
// -10016    业务callback为空
// -10017    业务Process类未完成注册
// -10018    SDK未能找到clientmsgid
// -10019    process已经注册过
// -10020    消息发送超时
// -10021    SDK未认证成功
// -10022    SDK的license文件未找到
// -10023    SDK初始化超时
// -10024    SDK初始化文件路径为空
// -10025    未初始化消息发送接口的类

#endif //__HT_HT_SDK_INTERFACE_H__

