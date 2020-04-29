#pragma once
#include "SmartSocketClient.h"

#include "SipInterface.h"
#include "MapStreamerSipID.h"


UINT WINAPI UACTCPProc(LPVOID lpParameter);

class CUACTCP:public CSipInterface
{
public:
	CUACTCP(void);
	virtual ~CUACTCP(void);
	int GetMsg();
	// 初始化对象
	virtual int Init(const char *pstrUACID, const char *pstrIP, const char *pstrPort, const char * pstrPassword = nullptr)override;

	int SendInvite(int nOperate, int nProType, const char *pstrDeviceID, 
		const char *pstrChannelID,  const char *pstrRecvIP, int nRecvPort[2],
		void *pStreamer, char *ptmStart, char *ptmEnd)override;
	int SendAnswered(int nStatus)override;
	int SendDeviceControl(const char *pstrDeviceID, const char *pstrChannelID, BYTE bCmdType, BYTE cData1, BYTE cData2, BYTE cData3, BYTE cZoom = DRAG_ZOOM_NONE, int nLength = 0, int nWidth = 0, int nMidPointX = 0, int nMidPointY = 0, int nLengthX = 0, int nLengthY = 0);
	int SendRemoteStartup(const char *pstrDeviceID)override;
	const char * SendDeviceCatalogInquiry(const char *pstrDeviceID)override;
	const char * SendDeviceInfoInquiry(const char *pstrDeviceID)override;
	const char * SendDeviceStatusInquiry(const char *pstrDeviceID)override;
	int SendEventSubscribe(const char *pstrDeviceID, int nStratPriority, int nEndPriority, 
							int nMethod, char* pstrStartTime, char* pstrEndTime)override;
	int SendCatalogSubscribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime)override;
	int SendRecord(const char *pstrDeviceID, const char *pstrChannelID, int nActive)override;
	int SendGuard(const char *pstrDeviceID, const char *pstrChannelID, int nActive)override;
	int SendBye(const char *pstrDeviceID, const char *pstrChannelID, void* pStreamer, int nID)override;
	int SendBye(const char *pstrDeviceID, const char *pstrChannelID, int nCid, int nDid, int nOption)override;
	int SendPlayBackCtrl(const char *pstrDeviceID, const char *pstrChannelID, void* pStreamer, int nOperate, float fValue)override;

	int SetPlayBackID(void* pStreamer, int nCid, int nDid)override;
	int GetPlayBackID(void* pStreamer, int &nCid, int &nDid)override;
	//bSecrecy 0：不涉密，1：涉密 
	//cy 2015.7.30 changed
	const char* SendRecordInfoInquiry(const char *pstrDeviceID, const char *pstrChannelID,  int nType,const char* pstrSTime,
		const char* strETime, const char* pstrFilePath = nullptr, const char* pstrAddress = nullptr, BYTE bSecrecy = 0, const char* pstrRecorderID = nullptr, BYTE IndistinctQuery = 0)override;

	int SendLogin()override;
	virtual int SendLogin(const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword)override;
	virtual int SendLogout()override;
	virtual int SendChangePassword(const char* pstrOldPassword, const char* pstrNewPassword)override;
	// PWolf: Add 2013.06.17
	//int SendDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOption);
	int SendDecorder(const char* pstrDecorderID, const char* pstrCameraID, int nOption)override;
	// PWolf: Add End
	int ReplyCallAck(int nDID)override;
	int SendResetAlarm(const char *pstrDeviceID, const char *pstrChannelID)override; //A.16.1
	int GetOwnerType()override;
	float GetSpeed(void* pStreamer)override;
	int GetPlayStatus(void* pStreamer);
  
	int SendDeviceConfig(const char *pstrDeviceID, deviceConfig_info_t* pDeviceConfig)override;
	DEVICE_PRESETQUERY_INFO_T* SendDevicePersetQuery(const char *pstrDeviceID) override;
	//DEVICECONFIG_AUDIOPARAMCONFIG DEVICECONFIG_AUDIOPARAMOPT 等
	DEVICE_CONFIG_INFO_T* SendConfigDownload(const char *pstrDeviceID,const char *strConfigType)override;
	const char * SendRealPlayUrl( const char *pstrChannelID, const int nWndNum, const char * pstrDeviceId )override;
	int SendStopPlayUrl(const char *pstrChannelID, const int nWndNum) override;
	int SendDecoderDivision(const char *pstrDeviceID, const char *pstrChannelID, int nDivision) override;
	const char * SendPlayBackUrl(const char *pstrDeviceID, const char *pstrChannelID, const char* strSTime, const char* strETime, const char* pstrLocation);//A.11
	const char * SendDecoderStatus(const char *pstrDeviceID, const char *pstrChannelID);//A.12
	int SendAlarmSubScribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime);//A.14
	const char * SendAlarmQuery(const char *pstrDeviceID, const char *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime,const char* pstrAlarmType);//A.17

	std::string GetLocalIP()override;

public:
	int OnMessage();
	int OnMessageResponse();
	int OnInviteRes();
	int OnRegister();
	int OnRegisterRes();
	int OnDeviceCatalogInquiryRes();
	int OnDeviceStatusInquiryRes();
	int OnDeviceInfoInquiryRes();
	int OnRecordInfoInquiryRes();
	int OnSetGuardRes();
	int OnSetRecordRes();
	int OnPlayBackFinished();
	int OnLogout();
	int OnOther();
	int OnNotify();


	int OnDevicePersetQueryRes();
	int OnConfigDownloadRes();
	int OnRealPlayUrlRes();
	int OnPlayBackUrlRes();
	int OnDecoderStatusRes();
	int OnAlarmQueryRes();
protected:
	//int ReadDeviceList();
	int OnHandleRegister(int nMsgType);
	int OnAlarm();
	int	ParseAlarmData(char *pData, char *strSrcID,  char *strAlarmPriority, char *strAlarmTime, char *strAlarmMethod, char * strAlarmType);
	
	//处理Notify消息
	int OnCatalogNotify();

	int GetMsgType(char *pMsgData);
	int GetMsgOperate(char *pMsgData);
	int GetMsgStatus(char *pMsgData);
	int SetSpeed(void* pStreamer, float fSpeed); //if speed >2, will play i frame only
	int SetPlayStatus(void* pStreamer, int nPlayStatus);
	int HandleTimer();

private:
	CExternalInterface externalInterface;
	list<string> m_listDowmPLDDevices;//下级平台的设备集合
	raw_data_t *m_pData;
	CSmartSocket *m_pSocket;
	HANDLE m_hMsgThread;
	CMapStreamerSipID m_mapStreamerID;
public:
	virtual int UnInit() override;
};

class CDataStream
{
public:
	CDataStream(BYTE *pBuf, int nBufSize)
	{
		m_pBuf = pBuf;
		m_pCurPos = pBuf;
		m_pLastPos = pBuf;
		m_nDataSize = 0;
		m_nBufSize = nBufSize;
	}

	CDataStream(BYTE *pBuf)
	{
		m_pBuf = pBuf;
		m_pCurPos = pBuf;
		m_pLastPos = pBuf;
		m_nDataSize = 0;
		m_nBufSize = 0;
	}

	~CDataStream(void)
	{}

	int InputData(int nPos, const void *pSrcBuf, int nCount)
	{

		//Input to the final position
		if(0 > nPos)
		{		
			memcpy_s(m_pCurPos, m_nBufSize-(m_pCurPos-m_pBuf), pSrcBuf, nCount);
			m_pCurPos += nCount;
			if(m_pCurPos > m_pLastPos)
				 m_pLastPos = m_pCurPos;

			auto pCharSrcBuf = const_cast<void*>(pSrcBuf);
			if(1 == nCount && ( reinterpret_cast<char*>(pCharSrcBuf))[0] == '#')
			{
				int nSize = GetLastPos()-4;
				memcpy_s(m_pBuf, 4, &nSize, 4);
				return 0;
			}
		}
		//Input to the specified position
		else
		{
			memcpy_s(m_pBuf+nPos, m_nBufSize-nPos, pSrcBuf, nCount);
			m_pCurPos = m_pBuf+nPos+nCount;
			if(m_pCurPos > m_pLastPos)
				m_pLastPos = m_pCurPos;
		}

		return m_nDataSize;
	}

	int OutputData(int nPos, void *pSrcBuf, int nCount)
	{
		if(0 > nPos)
		{	
			memcpy_s(pSrcBuf, nCount, m_pCurPos, nCount);
			m_pCurPos += nCount;
			if(m_pCurPos > m_pLastPos)
				 m_pLastPos = m_pCurPos;
		}
		else
		{
			memcpy_s(pSrcBuf, nCount, m_pBuf+nPos, nCount);
			m_pCurPos = m_pBuf+nPos+nCount;
			if(m_pCurPos > m_pLastPos)
				m_pLastPos = m_pCurPos;
		}

		return 0;
	}

	int GetLastPos() const
	{
		return static_cast<int>(m_pLastPos - m_pBuf);
	}
private:
	BYTE *m_pBuf;
	BYTE *m_pCurPos;
	BYTE *m_pLastPos;
	int m_nDataSize;
	int m_nBufSize;
};
