#pragma once
#ifdef USESIPCLIENT

#include "sipinterface.h"
#include "..\..\SipUAS\export\UASGB28181.h"
class CSipClient :
    public CSipInterface, public UASGB28181CallBack
{
public:
    CSipClient(void);
    ~CSipClient(void);
    virtual int Init(const char *pstrSipId, const char *pstrSipIp, const char * pstrSipPort, const char * pstrPassword = NULL);
    virtual int SendInvite(int nOperate, int nProType, const char *pstrDeviceID, const char *pstrChannelID,  const char *pstrRecvIP, int nRecvPort[2], void *pStreamer, ClientRealData pFunction, void **ppRTPTrans, char *ptmStart, char *ptmEnd);
    virtual int SendAnswered(int nStatus);
    virtual int SendDeviceControl(const char *pstrDeviceID, const char *pstrChannelID, BYTE bCmdType, BYTE cData1, BYTE cData2, BYTE cData3, BYTE cZoom = DRAG_ZOOM_NONE, int nLength = 0, int nWidth = 0, int nMidPointX = 0, int nMidPointY = 0, int nLengthX = 0, int nLengthY = 0);
    virtual int SendRemoteStartup(const char *pstrDeviceID);
    virtual const char * SendDeviceCatalogInquiry(const char *pstrDeviceID);
    virtual const char * SendDeviceInfoInquiry(const char *pstrDeviceID);
    virtual const char * SendDeviceStatusInquiry(const char *pstrDeviceID);
    virtual int SendEventSubscribe(const char *pstrDeviceID, int nStratPriority, int nEndPriority, int nMethod, char* pstrStartTime, char* pstrEndTime);
    virtual int SendCatalogSubscribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime);
    virtual int SendRecord(const char *pstrDeviceID, const char *pstrChannelID, int nActive);
    virtual int SendGuard(const char *pstrDeviceID, const char *pstrChannelID, int nActive);
    virtual int SendBye(const char *pstrDeviceID, const char *pstrChannelID, void **ppRTPTrans, int nOption);
    virtual int SendBye(const char *pstrDeviceID, const char *pstrChannelID, int nCid, int nDid, int nOption);
    virtual int SendPlayBackCtrl(const char *pstrDeviceID, const char *pstrChannelID, void **ppRTPTrans, int nOperate, float fValue);

    virtual int ProcessAudio(void * pRTPTrans, BYTE * pBuffer, int nBufSize);
    virtual int CreateMediaStreamer(int nProtocolType, int nStreamerType, _tstring strRemoteIP, int nRemotePort, lpInviteParam_t pInviteInfo);
    virtual int StopStreamer(void **ppRTPTrans, int nOption);
    virtual int SetPlayBackID(void **ppRTPTrans, int nCid, int nDid);
    virtual int GetPlayBackID(void **ppRTPTrans, int &nCid, int &nDid);
    //bSecrecy 0£∫≤ª…Ê√‹£¨1£∫…Ê√‹ 
    //cy 2015.7.30 changed
    virtual const char* SendRecordInfoInquiry(const char *pstrDeviceID, const char *pstrChannelID,  int nType,const char* pstrSTime, const char* strETime, const char* pstrFilePath = NULL, const char* pstrAddress = NULL, BYTE bSecrecy = 0, const char* pstrRecorderID = NULL, BYTE IndistinctQuery = 0);

    virtual int SendLogin(const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword);
    virtual int SendLogout();
    virtual int SendChangePassword(const char* pstrOldPassword, const char* pstrNewPassword) { return 0; }

    //int SendDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOption);
    virtual int SendDecorder(const char* pstrDecorderID, const char* pstrCameraID, int nOption);
    // PWolf: Add End
    virtual int SendCallAck(int nDID);
    virtual int SendResetAlarm(const char *pstrDeviceID, const char *pstrChannelID); //A.16.1
    virtual int GetOwnerType();

    virtual int SendDeviceConfig(const char *pstrDeviceID, deviceConfig_info_t* pDeviceConfig);
    virtual DEVICE_PRESETQUERY_INFO_T* SendDevicePersetQuery(const char *pstrDeviceID);
    //DEVICECONFIG_AUDIOPARAMCONFIG DEVICECONFIG_AUDIOPARAMOPT µ»
    virtual DEVICE_CONFIG_INFO_T* SendConfigDownload(const char *pstrDeviceID,const char *strConfigType);
    virtual const char * SendRealPlayUrl(const char *pstrDeviceID, const char *pstrChannelID);
    virtual const char * SendPlayBackUrl(const char *pstrDeviceID, const char *pstrChannelID, const char* strSTime, const char* strETime, const char* pstrLocation);//A.11
    virtual const char * SendDecoderStatus(const char *pstrDeviceID, const char *pstrChannelID);//A.12
    virtual int SendAlarmSubScribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime);//A.14
    virtual const char * SendAlarmQuery(const char *pstrDeviceID, const char *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime,const char* pstrAlarmType);//A.17
protected:
    virtual int OnUASGB28181Data(lpMsgParam_t pMsgParam);
    int OnInviteRes(lpMsgParam_t m_pMsgParam);
    int OnRecordInfoInquiryRes(lpMsgParam_t m_pMsgParam);
    int OnDeviceCatalogInquiryRes(lpMsgParam_t m_pMsgParam);
    int OnCatalogSubscribeRes(lpMsgParam_t m_pMsgParam);
    int OnAlarmSubscribeRes(lpMsgParam_t m_pMsgParam);
    int OnPlayBackUrlRes(lpMsgParam_t m_pMsgParam);
    int OnDeviceStatusInquiryRes(lpMsgParam_t m_pMsgParam);
    int OnDeviceInfoInquiryRes(lpMsgParam_t m_pMsgParam);

    int SetSpeed(void **ppRTPTrans, float fSpeed);
    int GetPlayStatus(void **ppRTPTrans);
    int SetPlayStatus(void **ppRTPTrans, int nPlayStatus);
    float GetSpeed(void **ppRTPTrans);
private:
    CUASGB28181 m_uas;
};

#endif