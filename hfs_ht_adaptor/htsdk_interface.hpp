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
*��Ϣ�����
*/
class HT_API CPackMessageCommon
{
public:
    CPackMessageCommon();
    ~CPackMessageCommon();

    /**
    *����AddValue�Ⱥ������õ���Ϣ������õ���Ϣ��ָ�룬�˴���ָ�����ֱ����ͬ�������첽�ķ�ʽ���ͣ���������߻����ڴ�
    * @param  
    * @return  CHtMessage * pMsg  ��Ϣ��ָ��
    */
    CHtMessage* GetPackedMsg();

    /**
    *ͨ��AddValue�������õ�ǰ��ָ����key��Ӧ��char������
    * @param  const HT_uint32 nKey    ��Ҫ���õ��ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_char cValue    ָ���ֶζ����char��ֵ
    * @return  ����0���óɹ�������ʧ��
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_char cValue);

    /**
    *ͨ��AddValue�������õ�ǰ��ָ����key��Ӧ��HT_uint32������
    * @param  const HT_uint32 nKey    ��Ҫ���õ��ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int32 cValue    ָ���ֶζ����HT_uint32��ֵ
    * @return  ����0���óɹ�������ʧ��
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_int32 nValue);

    /**
    *ͨ��AddValue�������õ�ǰ��ָ����key��Ӧ��HT_uint64������
    * @param  const HT_uint32 nKey    ��Ҫ���õ��ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int64 lValue    ָ���ֶζ����HT_uint64��ֵ
    * @return  ����0���óɹ�������ʧ��
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_int64 lValue);

    /**
    *ͨ��AddValue�������õ�ǰ��ָ����key��Ӧ��HT_double������
    * @param  const HT_uint32 nKey    ��Ҫ���õ��ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_double cValue    ָ���ֶζ����HT_double��ֵ
    * @return  ����0���óɹ�������ʧ��
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_double dValue);

    /**
    *ͨ��AddValue�������õ�ǰ��ָ����key��Ӧ���ַ���������
    * @param  const HT_uint32 nKey    ��Ҫ���õ��ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_char * pstValue    ָ���ֶζ�����ַ�����ֵ
    * @return  ����0���óɹ�������ʧ��
    */
    HT_int32 AddValue(const HT_uint32 nKey, const HT_char * pstValue);

    /**
    *ͨ��keyɾ����ǰ�ж�Ӧ�ļ�ֵ���ֶ�ֵ
    * @param  const HT_uint32 nKey    ��Ҫɾ�����ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @return  
    */
    HT_int32 DelValue(const HT_uint32 nKey);

    /**
    *��ȡ��Ϣ���ܵ�����
    * @return  const HT_int32 ��Ϣ������
    */
    const HT_int32 GetRowCount();

    /**
    *�趨��ǰ��Ϣ���α���кţ������Ϣʱ���к����ֻ�����õ�֮ǰ���ù�����кŵ���һ��, �кŴ�0��ʼ
    * @param  const HT_int32 ָ����Ϣ����к�
    * @return  true�ɹ���falseʧ��
    */
    HT_bool SetCurrentRow(HT_int32 iRow);

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_char������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_char&  cValue    ָ���ֶζ�Ӧ��HT_char��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_char& cValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_uint32������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int32&  cValue    ָ���ֶζ�Ӧ��HT_uint32��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int32& nValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_uint64������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int64&  lValue    ָ���ֶζ�Ӧ��HT_uint64��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int64& lValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_double������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_double&  cValue    ָ���ֶζ�Ӧ��HT_double��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_double& dValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ���ַ���������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param const HT_char *&  ����ֵΪ0ʱ��ָ�뷵��ָ���ֶζ�Ӧ���ַ���
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, const HT_char* &pstValue) const;
    
    /**
    *���ü��ܺ�������������м���
    * @param const char* pIn ����Ĵ����ܵ��ַ���
    * @param const char* pOut �������������֮����ַ��������볤�Ƚ��鳤һ�㣬����256�����ⲿ����
    * @param HT_int32 outBufferLengh ��������ĳ���
    * @return HT_int32 =0 �ɹ�  ����-1����������п�ָ�룬 ����-10023����������ĳ��ȹ���
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
    *�趨��ǰ��Ϣ��
    * @param  const CHtMessage * cMessageָ����Ϣ��ָ��
    * @return  0�ɹ���-1ʧ��
    */
    HT_int32 SetPackedMsg(const CHtMessage *cMessage);

    /**
    *��ȡ��Ϣ���ܵ�����
    * @return  const HT_int32 ��Ϣ������
    */
    const HT_int32 GetRowCount();

    /**
    *�趨��ǰ��Ϣ���α���кţ��к����ֻ�����õ�GetRowCount() - 1, �кŴ�0��ʼ
    * @param  const HT_int32 ָ����Ϣ����к�
    * @return  true�ɹ���falseʧ��
    */
    HT_bool SetCurrentRow(HT_int32 iRow);

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_char������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_char&  cValue    ָ���ֶζ�Ӧ��HT_char��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_char& cValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_uint32������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int32&  cValue    ָ���ֶζ�Ӧ��HT_uint32��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int32& nValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_uint64������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_int64&  lValue    ָ���ֶζ�Ӧ��HT_uint64��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_int64& lValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ��HT_double������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param  const HT_double&  cValue    ָ���ֶζ�Ӧ��HT_double��ֵ
    * @return  ����0��ȡ�ɹ�������ʧ��
    */
    HT_int32 GetValue(const HT_uint32 nKey, HT_double& dValue)const;

    /**
    *ͨ��GetValue������ȡ��ǰ��ָ����key��Ӧ���ַ���������
    * @param  const HT_uint32 nKey    ��Ҫ��ȡ���ֶζ��壬�μ�HtClientFiledDefine.h�ļ���HtFiled���еĶ���
    * @param const HT_char *&  ����ֵΪ0ʱ��ָ�뷵��ָ���ֶζ�Ӧ���ַ���
    * @return  ����0��ȡ�ɹ�������ʧ��
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
    *�첽������Ϣ�Ļظ�������
    * @param  HT_int32 iRet              �첽����ɹ���ʶ
    * @param  const CHtMessage* pstMsg   �첽����ظ�����Ϣ��ָ��
    * @return  
    */
    //�첽����ص�����ʹ�����ͷ�pstMsg��Դ
    virtual HT_void ProcAsyncCallBack(HT_int32 iRet, const CHtMessage* pstMsg) = 0; 

    /**
    *�����¼�֪ͨ���û����������ﴦ���¼�
    * @param  HT_int32 eventType              �¼�����
    * @param  const CHtMessage* pstMsg       �¼�����Ϣ��ָ��
    * @return  
    */
    virtual HT_void ProcEvent(HT_int32 eventType, const CHtMessage* pstMsg) = 0;

    /**
    *��������������֪ͨ��ҵ����������ﴦ�������������������飬�������¶�������
    * @return  
    */
    virtual HT_void OnConnect() = 0;

    /**
    *����Ͽ����֪ͨ����ʱҵ�����κ���Ϣ����ʧ�ܵ�
    * @return  
    */
    virtual HT_void OnClose() = 0;

     /**
    *�û���¼״̬��ʧЧ���û����������ﴦ��ʧЧ֮����߼����������pstMsg�е��û�id���Ը��û��������µ�¼����, ��ʹ�����ͷ�pstMsg��Դ
    * @return  
    */
    virtual HT_void OnTokenLoseEfficacy(const CHtMessage* pstMsg) = 0;

    /**
    *���緢��ʧ��
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
    *�첽������Ϣ
    * @param  const CHtMessage* pstMsg   ��Ҫ���͵���Ϣ��ָ��
    * @return  ����0���ͳɹ�������ʧ��
    */
    virtual HT_int32 Send(const CHtMessage* pstMsg) = 0;

    /**
    *ͬ��������Ϣ
    * @param  const CHtMessage* pstMsg       ��Ҫ���͵���Ϣ��ָ��
    * @param  const CHtMessage* pstRsponse   �ظ�����Ϣ��ָ��
    * @param  HT_int32 timeout               ͬ������ĳ�ʱʱ��
    * @return  ����0ͬ������ɹ����յ��ظ�������ʧ��
    */
    //ע�⣬����ͬ�����õķ�ʽ���������pstRsponse��Ϊ�յĻ���
    // pstRsponse��Ҫ��������������HtDecreaseMsgPtr(pstRsponse)���ͷ���Ϣ����Դ�����������ڴ�й¶
    virtual HT_int32 Send_Sync(const CHtMessage* pstMsg, CHtMessage* &pstRsponse, HT_int32 timeout = 10000) = 0;
};

extern "C"
{
/**
*��ʼ��SDK
* @param  const HT_char * filePath  license�Լ��������õ��ļ���Ŀ¼·��
* @param  const HT_char * channelID    SDKͨ����ʶ
* @param  HT_int32 timeout  SDK��ʼ���ĳ�ʱʱ��
* @return HT_int32   ����0����ʼ���ɹ��� ���򷵻ص��Ǵ���ԭ���Ӧ�Ĵ�����
*/
HT_API HT_int32 HtClientManagerInit(const HT_char * filePath, HT_int32 timeout = 10000, const HT_char * channelID = NULL);

/**
*����SDKʵ��
* @param  const HT_char * channelID    SDKͨ����ʶ
* @return HT_int32   ����0����ʼ���ɹ��� ���򷵻ص��Ǵ���ԭ���Ӧ�Ĵ�����
*/
HT_API HT_int32 HtClientManagerDestroy(const HT_char * channelID = NULL);

/**
* ��ʼ��ʵ��CSendMsgInterface�ӿڵ����ָ��
* @param CCallbackInterface * pCallbackInterface ����ҵ��Ļص���ָ��
* @param  const HT_char * channelID    SDKͨ����ʶ
* @return ʵ��CSendMsgInterface�ӿڵ����ָ��  ����Ϊ����Ϊʧ��
*/
HT_API CSendMsgInterface* HtSendMsgInterfaceInit(CCallbackInterface * pCallbackInterface, const HT_char * channelID = NULL);

/**
* ��ȡʵ��CSendMsgInterface�ӿڵ����ָ��
* @param  const HT_char * channelID    SDKͨ����ʶ
* @returnʵ��CSendMsgInterface�ӿڵ����ָ�� ����Ϊ����Ϊʧ��
*/
HT_API CSendMsgInterface* HtGetSendMsgInterface(const HT_char * channelID = NULL);

/**
*����Ϣ��ָ������ü�����ȥ1����SDK�����Ϣ��ָ����ڴ����
* @param  const CHtMessage * pMsg  ��Ҫ��������һ����Ϣ��ָ��
* @return 
*/
HT_API HT_void HtDecreaseMsgPtr(const CHtMessage * pMsg);

/**
*������Ϣ��ָ��ָ�����Ϣ�����ݣ�ת����json��ʽ���ַ�����pOut���ⲿ�������벢����ڴ����
* @param  const CHtMessage * pMsg  ��Ҫת��json��ʽ�ַ�������Ϣ��ָ��
* @param  HT_char * pOut  ���ڴ��ת�����json��ʽ�ַ�����bufferָ��
* @param  HT_int32 bufferLengh  buffer�Ĵ�С
* @return HT_int32   ���ڵ���0��ת���ɹ�����ʱ����ֵ���ַ�������Ч���� -10002 ����п�ָ�� -10023 �������ڽ��յ��ַ����ڴ��С����
*/
HT_API HT_int32 HtMsgPtrToString(const CHtMessage * pMsg, HT_char * pOut, HT_int32 bufferLengh);


/**
*��ȡ��Ϣ���Ψһ��Ϣ��
* @param const CHtMessage* pstMsg ����Ĵ�ȡ��Ϣ�ŵ���Ϣ��ָ��
* @return const HT_char *   ����NULL��ȡʧ�ܣ� ���򷵻ص���ָ����Ϣ���ַ�����ָ��
*/
HT_API HT_int32 HtGetClientMsgID(const CHtMessage* pstMsg, const HT_char * & pClientMsgID);


    
}



//������������Ϣ

// 0         �ɹ�     
// -1        ʧ��     
// -10001    ������� 
// -10002    �������� 
// -10003    buffer��С����
// -10004    ��Ϣ��Ϊ��
// -10005    Э�鱨��json��ȱ��Э��ͷ
// -10006    Э�鱨��json�����ֶ�����
// -10007    Э�鱨��json������������
// -10008    ����ͷȱʧ
// -10009    ��Ϣ������ת��ʧ��
// -10010    ��Ϣд���շ�����ʧ��
// -10011    �����ڶ�Ӧ�ֶ�
// -10012    �ֶζ�Ӧ��ֵ����ת��ʧ��
// -10013    �ڴ�newʧ��
// -10014    �����շ�ģ��δ��ʼ�����
// -10015    SDK������δ��ɳ�ʼ��
// -10016    ҵ��callbackΪ��
// -10017    ҵ��Process��δ���ע��
// -10018    SDKδ���ҵ�clientmsgid
// -10019    process�Ѿ�ע���
// -10020    ��Ϣ���ͳ�ʱ
// -10021    SDKδ��֤�ɹ�
// -10022    SDK��license�ļ�δ�ҵ�
// -10023    SDK��ʼ����ʱ
// -10024    SDK��ʼ���ļ�·��Ϊ��
// -10025    δ��ʼ����Ϣ���ͽӿڵ���

#endif //__HT_HT_SDK_INTERFACE_H__

