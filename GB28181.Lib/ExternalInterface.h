#pragma once
#include "SipInterface.h"

#define TYPE_LEN	2

#define INIT_FAILD_ERROR	100
#define GET_FREEPORT_ERROR	101
#define THREAD_CREATE_ERROR	102

#define UDP_TRANS	0
#define TCP_TRANS	1

class CSipInterface;
class GB28181DLL_API CExternalInterface
{
public:
	CExternalInterface(void);
	~CExternalInterface(void);
	int Init(const char *pstrUACID, const char *pstrIP, const char *pstrPort);

public:
	int SetDeviceStatusCallBack(DeviceStaustAlarm);
	int SetPlayBackFinishedCallBack(PlayBackFinished);
	int SetInviteResponseInfoCallback(InviteResponsed);
	int Play(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, const TCHAR *pstrMediaID, int nStreamerType, 
			void *pStreamer, time_t startTime, time_t endTime, const char* pLocalIP, int nRecvPort[2], int nMediaTransferMode = UDP_TRANS);
	int Pause(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, int nPos);
	int Resume(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, int nPos);
	int Speed(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, float fValue);
	int Seek(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, UINT unValue);
	int Stop(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, int nOption);
	int GetStreamerID(void* pStreamer, int &nCid, int &nDid);
	int SetGuard(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, BYTE bCmdType);
	int SetPTZControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, BYTE bCmdType,int nValue);
	int SetScanControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nSectionID, int nValue1, int nValue2);
	int SetCruiseControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nSectionID, int nValue1, int nValue2);
	const char * SearchRecord(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nType, DATE tmStart, DATE tmEnd, const char* pstrFilePath = NULL, const char* pstrAddress = NULL, BYTE bSecrecy = 0, const char* pstrRecorderID = NULL, BYTE IndistinctQuery = 0);
	int SearchDeviceInfo(const TCHAR *pstrDeviceID);
	int SearchDeviceStatus(const TCHAR *pstrDeviceID);
	const char *  GetDeviceStatus(const TCHAR *pstrDeviceID);
	int RemoteStartup(const TCHAR *pstrDeviceID);
	int SetRecord(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nActive);
	int SearchCatalog(const TCHAR *pstrDeviceID);
	const char * GetCatalog(const TCHAR *pstrDeviceID);
	int SetDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOperate);
	int ExtractXML(const char * pXml, char *pKey, char *pElement1L, char *Element1R, char *Element2L, char *Element2R, DATE &tmStart, DATE &tmEnd);
	int ExtractXML(const char * pXml, char *pKey, char *pElementL, char *ElementR, TCHAR *strValue, int nStrBufLen);
	int ExtractXML(const char * pXml, char *pKey, TCHAR *strValue, int nStrBufLen);
	int ExtractDeviceID(const char * pXml, TCHAR *strValue, int nStrBufLen);
	int ShowResult(int nShowDataType);
	int ShowDeviceInfo(HWND hWndInfo, const char * pXML);
	int ShowDeviceStatus(HWND hWndInfo, HWND hWndAlarm, const char * pXML);
	// PWolf: Add 2013.06.18
	int ShowDeviceCatalog(HWND hWndInfo, const char * pXML);
	int ShowAlaramQuery(HWND hWndInfo, const char * pXML);
	const char* SearchPlatformCatalog(void);
	// PWolf: Add End
	int ResetAlarm(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID);
	int GetOwnerType();
	float GetSpeed(void* pStreamer);
	int GetPlayStatus(void* pStreamer);

	int SetPlayStatus(void* pStreamer, int nPlayStatus);

	//cy added 2015.7.28
	int SetDeviceConfig(const TCHAR *pstrDeviceID, deviceConfig_info_t* pDeviceConfig);
	int DevicePersetQuery(const TCHAR *pstrDeviceID);
	//DEVICECONFIG_AUDIOPARAMCONFIG DEVICECONFIG_AUDIOPARAMOPT 等
	int ConfigDownload(const TCHAR *pstrDeviceID, const TCHAR *strConfigType);
	const char * RealPlayUrl(const TCHAR *pstrChannelID, const int nWndNum, const TCHAR * pstrDeviceId);
	int StopPlayUrl(const TCHAR *pstrChannelID, const int nWndNum);
	int DecoderDivision(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nDivision);
	const char * PlayBackUrl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, DATE tmStart, DATE tmEnd,const TCHAR *pstrLocation);//A.11
	const char * DecoderStatus(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID);//A.12

	int SipClientLogin(const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword);
	int SipClientLogout();
	virtual int SendChangePassword(const char* pstrOldPassword, const char* pstrNewPassword);
	virtual int SendCatalogSubscribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime);
	virtual int SendAlarmSubScribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime);//A.14
	virtual int SendAlarmQuery(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const TCHAR* pstrSTime,const TCHAR* pstrETime,const TCHAR* pstrAlarmType);//A.17

	bool GetLocalIP(char* pReceiveIP, int nLen);
protected:
	CSipInterface *m_pCUATcp;
	
	std::string strStatus;
	std::string strCatalog;
	std::string strPresetQuery;
	std::string strDecoderStatus;
};

