#pragma once
#include "time.h"
#include <vector>
#include <list>
using namespace std;
#include "Log.h"

//////////////////////////////////////////////////////////////////////////
#define MAX_RECORDLIST_SIZE	10
#define UPDATE_STATUS_TIME	20
#define THREAD_CREATE_ERROR	102
//////////////////////////////////////////////////////////////////////////
//command id
#define CMD_REGISTER		0x00000001	
#define CMD_REGISTERRES		0x80000001	
#define CMD_INVITE			0x00000002	
#define CMD_INVITERES		0x80000002	
#define CMD_MESSAGE			0x00000003	
#define CMD_MESSAGERES		0x80000003	
#define SEARCH_CMD			0x00000004	
#define SEARCHRES_CMD		0x80000004	
#define CMD_SUBSCRIBE		0x00000005	
#define CMD_SUBSCRIBERES	0x80000005	
#define CMD_NOTIFY			0x00000006	
#define CMD_NOTIFYRES		0x80000006	
#define CMD_INFO			0x00000007	
#define CMD_INFORES			0x80000007	
#define BAY_CMD				0x00000008	
#define BAYRES_CMD			0x80000008	

//message buffer size
#define EXTERND_DATA_SIZE	20000
#define LOGIN_MSG_SIZE		128
#define ACK_MSG_SIZE		128
#define INVITE_MSG_SIZE		512
#define SEARCH_MSG_SIZE		512
#define CTRL_MSG_SIZE		129
#define RES_MSG_SIZE		32
#define ID_LEN				20
#define	ID_BUFFER_SIZE		(ID_LEN+1)
#define IP_LEN				15
#define	IP_BUFFER_SIZE		(IP_LEN+1)
#define PORT_LEN			5
#define TYPE_LEN	2
#define LEN_PASSWD 32
//////////////////////////////////////////////////////////////////////////
#define REALPLAY	0
#define PLAYBACK	1
#define DOWNLOAD	2

typedef	void (CALLBACK *DeviceStaustAlarm)(LPCTSTR , LPCTSTR, int , int);
typedef	void (CALLBACK *ClientRealData)(void *, BYTE *, INT64, DWORD, INT64);
typedef	void (CALLBACK *PlayBackFinished)(LPCTSTR, LPCTSTR, int, int);
typedef void (CALLBACK *InviteResponsed)(const char *sdp, void *pBindObj);
typedef struct MEDIA_INFO_T{
	explicit MEDIA_INFO_T(int nSize):tmStart(0),tmEnd(0)
	{
		//if(0 < nSize)
		//	pName = new char[nSize];
		//else
		pName[0] = 0;
	}
	~MEDIA_INFO_T()
	{
		//if(pName)
		//	delete []pName;
	}
	char pName[10];
	DATE tmStart;
	DATE tmEnd;
}media_info_t;

typedef struct INVITE_PARAME_T{
	INVITE_PARAME_T():ppRTPTrans(nullptr)
	{
		strcpy_s(strSrcID, "");
		strcpy_s(strDstID, "");

		//socUser = INVALID_SOCKET;
		nCmdID = -1;
		unSeq = 0;
		pStreamer = nullptr;
		nOperate = 0;
		fpClientHandleRealData = nullptr;
		time(&timCreateTime);
	}

	char strSrcID[ID_LEN+1];
	char strDstID[ID_LEN+1];

	int nCmdID;
	UINT unSeq;
	int nRecvPort[2];
	int nOperate;
	void *pStreamer;
	ClientRealData fpClientHandleRealData;
	void *ppRTPTrans;
	time_t timCreateTime;

}invite_param_t, *lpInviteParam_t;

typedef struct DEVICE_CONFIG_INFO_T{
public:
	typedef struct VIDEO_PARAM_OPT_T
	{
		string strVideoFormatOpt;
		string strResolutionOpt;
		int nFrameRateOpt;
		int nBitRateTypeOpt;
		int nVideoBitRateOpt;
		string strDwonloadSpeedOpt;
	}videoParamOpt_t,*lpVideoParamOpt_t;

	typedef struct AUDIO_PARAM_OPT_T
	{
		string strAudioParamOpt;
		int nAudioBitRateOpt;
		string strSamplingRateOpt;
	}audioParamOpt_t,*lpAudioParamOpt_t;
	typedef struct VIDEO_PARAM_ATTRIBUTE_T
	{
		string strStreamName;		//SteamName
		string strVideoFormat;		//VideoFormat
		string strResolution;		//Resolution
		string strFrameRate;			//FrameRate
		string strBitRateType;		//BitRateType
		string strVideoBitRate;		//VideoBitRate
	}videoParamAttribute_t,*lpVideoParamAttribute_t;

	typedef struct AUDIO_PARAM_ATTRIBUTE_T
	{
		string strStreamName;		//StreamName
		string strAudioFormat;		//AudioFormat
		string strAudioBitRate;		//AudioBitRate
		string strSamplingRate;		//SamplingRate
	}audioParamAttribute_t,*lpAudioParamAttribute_t;

	typedef struct ROI_PARAM_ATTRIBUTE_T
	{
		ROI_PARAM_ATTRIBUTE_T()
		{
			nROISeq = 0;
			nTopLeft = 0;
			nBottomRight = 0;
			nROIQP = 0;
		}
		~ROI_PARAM_ATTRIBUTE_T()
		{}
		int nROISeq;
		int nTopLeft;
		int nBottomRight;
		int nROIQP;
	}ROIParamAttribute_t,*lpRIOParamAttribute_t;

	DEVICE_CONFIG_INFO_T()
	{
		cBasicParam = 0;
		lpBasicParam = nullptr;
		nVideoParamConfigNum = 0;
		lpVideoParamAttribute  =nullptr;
		nAudioParamConfigNum = 0;
		lpAudioParamAttribute = nullptr;
		lpVideoParamOpt = nullptr;
		lpAudioParamOpt = nullptr;
		memset(&SVACEncodeConfig, 0, sizeof(SVACEncodeConfig));
		memset(&SVACDecodeConfig, 0, sizeof(SVACDecodeConfig));
	}
	~DEVICE_CONFIG_INFO_T()
	{
		if (lpBasicParam) delete lpBasicParam;
		if (lpVideoParamAttribute) delete []lpVideoParamAttribute;
		if (lpAudioParamAttribute) delete []lpAudioParamAttribute;
		if(lpVideoParamOpt) delete []lpVideoParamOpt;
		if(lpAudioParamOpt) delete []lpAudioParamOpt;
	}

	typedef struct BASICPARAM
	{
		char strDeviceID[ID_LEN+1];    //deviceId
		string strName;					//Name

		char strSIPServerId[ID_LEN+1];	//SIPServerId
		char strSIPServerIp[IP_LEN+1];	//SIPServerIp
		int nSIPServerPort;				//SIPServerPort
		string strDomainName;			//DomainName
		int nExpiration;				//Expirarion
		int nPasswordLen;				//length of Password
		string strPassword;				//Password
		int nHeartBeatInternal;			//HeartBeatInterval
		int nHeartBeatCount;			//HeartBeatCount
	}basicParam_t,*lpBasicParam_t;

	BYTE cBasicParam;				//flag of BasicParam
	struct BASICPARAM * lpBasicParam;


	//Video
	struct VIDEO_PARAM_OPT_T* lpVideoParamOpt;
	int nVideoParamConfigNum;		//Number of VideoParamConfig
	struct VIDEO_PARAM_ATTRIBUTE_T* lpVideoParamAttribute;
	//Audio
	struct AUDIO_PARAM_OPT_T* lpAudioParamOpt;
	int nAudioParamConfigNum;
	struct AUDIO_PARAM_ATTRIBUTE_T* lpAudioParamAttribute;
	struct  
	{
		BYTE cFlag;						//flag
		struct
		{
			BYTE cFlag;					//flag
			int nROIFlag;				//ROIFlag
			int nROINumber;				//ROINumber
			struct ROI_PARAM_ATTRIBUTE_T* lpROIParamAttribute;
			int nBackGroundQP;			//BackGroundQP
			int nBackGroundSkipFlag;	//BackGroundSkipFlag
		}ROIParam;
		struct
		{
			BYTE cFlag;					//flag
			int nSVCFlag;				//SVCFlag
			int nSVCSTMMode;			//SVCSTMMode
			int nSVCSpaceDomainMode;	//SVCSpaceDomainMode
			int nSVCTimeDomainMode;	//SVCTimeDomainMode
		}SVCParam;
		struct  
		{
			BYTE cFlag;					//flag
			int nTimeFlag;				//TimeFlag
			int nEventFlag;			//EventFlag
			int nAlertFlag;			//AlterFlag
		}SurveillanceParam;
		struct 
		{
			BYTE cFlag;					//flag
			int nEncryptionFlag;		//EncryptionFlag
			int nAuthenticationFlag;	//AuthenticationFlag
		}EncryptParam;
		struct
		{
			BYTE cFlag;					//flag
			int nAudioRecognitionFlag;	//AudioRecognitionFlag
		}AudioParam;
	}SVACEncodeConfig;
	struct								//SVACDecodeConfig
	{
		BYTE cFlag;
		struct
		{
			BYTE cFlag;					//flag
			int nSVCSTMMode;			//SVCSTMMode
		}SVCParam;
		struct
		{
			BYTE cFlag;					//flag
			int nTimeShowFlag;			//TimeShowFlag
			int nEventShowFlag;			//EventShowFlag
			int nAlertShowFlag;			//AlerShowtFlag
		}SurveillanceParam;
	}SVACDecodeConfig;
}deviceConfig_info_t,*lpDeviceConfig_info_t;

typedef struct DEVICE_PRESETQUERY_INFO_T
{
public:
	DEVICE_PRESETQUERY_INFO_T()
	{
		nSN =1;
	}
	typedef struct DEVICE_PRESET_INFO_T
	{
	public:
		string strPresetId;
		string strPresetName;
	}Device_Preset_Info_t,*lpDevice_Preset_Info_t;
	string strCmdType;
	int nSN;
	string strDeviceID;
	vector<DEVICE_PRESET_INFO_T> v_PresetInfo;
}devicePresetQuery_info_t,*lpDevicePresetQuery_info_t;

///////////////////////////////////////////////////////////////
//                       Alarm Data
///////////////////////////////////////////////////////////////
enum enmAlarmType {
	AT_EXTERN_ALARM = 500,
	AT_DEVICE_STATUS = 1000,

	//AT_EXTERN_ALARM  = 1,
	//AT_VIDEO_LOST    = 2,
	//AT_MOTION_DETECT = 3
};

// playback control operate type defined by adaptor 
enum enmAdaptorPlaybackCtrlParam
{
	ADAPTOR_PLAYBACK_SEEK  = 0x01,
	ADAPTOR_PLAYBACK_SPEED = 0x02,
	//ADAPTOR_PLAYBACK_SOUND = 0x03,
	//   ADAPTOR_PLAYBACK_FRAME = 0x04
};

// Device Status Content
enum enmDeviceStatusContent {
	AC_DEVICE_STATUS_NONE = 0,
	AC_DEVICE_STATUS_ONLINE = 1,
	AC_DEVICE_STATUS_OFFLINE = 2
};

//message command sub type
#define MSG_DEVICE_CTRL			1
#define MSG_DEVICE_CTRL_RES		2
#define MSG_INFO_INQUIRE			3
#define MSG_INFO_INQUIRE_RES		4
#define MSG_STATUS_INQUIRE			5
#define MSG_STATUS_INQUIRE_RES		6
#define MSG_RECORD_INQUIRE			7
#define MSG_RECORD_INQUIRE_RES		8
#define MSG_CATALOG_INQUIRE		9
#define MSG_CATALOG_INQUIRE_RES	10
#define MSG_SET_RECORD				11
#define MSG_SET_RECORD_RES			12
#define MSG_SET_GUARD				13
#define MSG_SET_GUARD_RES			14
#define MSG_ALARM_NOTIFY			15
#define MSG_ALARM_NOTIFY_RES		16
#define MSG_REMOTE_STARTUP			17
#define MSG_REMOTE_STARTUP_RES		18
#define MSG_FILE_TOEND				19
#define MSG_FILE_TOEND_RES			20

#define MSG_DEVICE_CONFIG          21
#define MSG_PRESET_QUERY            22
#define MSG_CONFIG_DOWNLOAD         23
#define MSG_CATALOG_SUBSCRIBE       24
#define MSG_REAL_PLAY_URL           25
#define MSG_PLAY_BACK_URL           26
#define MSG_DECODER_STATUS          27
#define MSG_SUBSCRIBE_ALARM        28
#define MSG_ALARM_QUERY            29
#define MSG_ALARM_QUERY_RES	    30
#define MSG_SEND_EVENT_SUBSCRIBE        31
#define MSG_STOP_PLAY_URL          32
#define MSG_DECODER_DIVISION       33

//自定义类型
#define SIP_CLIENT_LOGIN_TYPE       1
#define SIP_CLIENT_LOGIN_RES_TYPE   2
#define SIP_CLIENT_LOGOUT_TYPE      3
#define SIP_CLIENT_LOGOUT_RES_TYPE  4
#define SIP_CLIENT_CHANGEPASSWORD_TYPE       5
#define SIP_CLIENT_CHANGEPASSWORD_RES_TYPE   6

//info command sub type
#define PLAY_TYPE	0
#define PAUSE_TYPE	1
#define SPEED_TYPE	2
#define PLAY_RESUME_TYPE 3//继续播放

//pre command
#define PRESET_CTRL		0x81
#define PRECALL_CTRL	0x82
#define PREDEL_CTRL		0x83

//cruise command
#define CRUADD_CTRL		0x84 
#define CRUDEL_CTRL		0x85
#define CRUSPEED_CTRL	0x86
#define CRUTIME_CTRL	0x87
#define CRUSTART_CTRL	0x88

//scan command
#define SCANRANGE_CTRL	0x89
#define SCANSTART_FLG	0x00
#define SCANLEFT_FLG	0x01
#define SCANRIGHT_FLG	0x02
#define SCANSPEED_CTRL	0x8a

//PTZ command
#define UP_DIR			0x08
#define UPPERRIGHT_DIR	0x09
#define RIGHT_DIR		0x01
#define LOWERRIGHT_DIR	0x05
#define DOWN_DIR		0x04
#define LOWERLEFT_DIR	0x06
#define LEFT_DIR		0x02
#define UPPERLEFT_DIR	0x0a
#define FORWARD_DIR		0x10
#define BACK_DIR		0x20

//FI command
#define ZOOMOUT_DIR			0x48  //光圈缩小
#define ZOOMOUT_FAR_DIR		0x4a  //光圈缩小同时聚焦近
#define ZOOMIN_DIR			0x44  //光圈放大
#define ZOOMIN_FAR_DIR		0x45  //光圈放大同时焦距远
#define NEAR_DIR			0x42  //焦距近
#define FAR_DIR				0x41  //焦距远
#define ZOOMIN_NEAR_DIR		0x46  //光圈放大焦距近	
#define ZOOMOUT_NEAR_DIR	0x49  //光圈缩小焦距远	

//trans type
#define CMD_UNDEFINED		0x00000000
#define CMD_REGISTER		0x00000001	
#define CMD_REGISTERRES		0x80000001	
#define CMD_INVITE			0x00000002	
#define CMD_INVITERES		0x80000002	
#define CMD_MESSAGE			0x00000003	
#define CMD_MESSAGERES		0x80000003	
#define CMD_LOGIN			0x00000004	
#define CMD_LOGINRES		0x80000004	
#define CMD_SUBSCRIBE		0x00000005	
#define CMD_SUBSCRIBERES	0x80000005	
#define CMD_NOTIFY			0x00000006	
#define CMD_NOTIFYRES		0x80000006	
#define CMD_INFO			0x00000007	
#define CMD_INFORES			0x80000007	
#define CMD_BYE				0x00000008	
#define CMD_BYERES			0x80000008	
#define CMD_LOGOUT			0x00000009	
#define CMD_LOGOUTRES		0x80000009


// PWolf: Add 2013.06.17
#define CMD_DECODER_PLAY		0x0000000A	
#define CMD_DECODER_PLAYRES	0x8000000A	
// PWolf: Add End
#define CMD_CALLACK			0x00000010	
#define CMD_CALLACKRES		0x80000010	
#define CMD_RESET_ALARM		0x00000011	
#define CMD_RESET_ALARMRES	0x80000011	
#define DEVICECONFIG_CMD    0x00000012
#define DEVICECONFIGRES_CMD 0x80000012
#define CMD_USEREXIT		0x00001000

//自定义类型
#define CMD_PRIVATE         0x000000FF


//error code
#define DATA_EMPTY_ERROR			-100
#define CREATE_MEDIASTREAMER_ERROR	-101
#define TIME_FORMAT_ERROR			-102
#define RECORD_INFO_ERROR			-103

//constant define
#define TIME_LEN	32
#define XML_LEN		65535
#define LEN_PRIORITY	1
#define METHOD_LEN		7
//typedef struct DeviceInfo
//{
//	char strIP[IP_LEN+1];
//	char strPort[PORT_LEN+1];
//	char strDeviceID[ID_LEN+1];
//
//}DeviceInfo_t;

//playback control
#define PALY_CTRL	0
#define PAUSE_CTRL	1
#define SPEED_CTRL	2

#define DRAG_ZOOM_NONE	0
#define DRAG_ZOOM_IN	1
#define DRAG_ZOOM_OUT	2
#define DRAG_ZOOM_NULL	3

//device config type

#define DEVICECONFIG_BASICPARAM "BasicParam"
#define DEVICECONFIG_VIDEOPARAMOPT "VideoParamOpt"
#define DEVICECONFIG_VIDEOPARAMCONFIG "VideoParamConfig"
#define DEVICECONFIG_AUDIOPARAMOPT "AudioParamOpt"
#define DEVICECONFIG_AUDIOPARAMCONFIG "AudioParamConfig"
#define DEVICECONFIG_SVACENCODECONFIG "SVACEncodeConfig"
#define DEVICECONFIG_SVACDecodeConfig "SVACDecodeConfig"

template<class T>
class WaitList
{
public:
	WaitList()
	{
		InitializeCriticalSection(&m_stCriSec);
		nTime = 0;
	}

	~WaitList()
	{
		EnterCriticalSection(&m_stCriSec);

		for (auto  itr = m_vecMsgList.begin(); itr != m_vecMsgList.end();)
		{
			delete (*itr);
			itr = m_vecMsgList.erase(itr);
		}
		LeaveCriticalSection(&m_stCriSec);
		DeleteCriticalSection(&m_stCriSec);
	}

	T* NewWaitMsg()
	{
		T* pWaitMsg = new T();
		EnterCriticalSection(&m_stCriSec);
		m_vecMsgList.push_back(pWaitMsg);
		LeaveCriticalSection(&m_stCriSec);
		return pWaitMsg;
	}

	T* GetWaitMsg(int mCmdID, UINT unSeq)
	{
		EnterCriticalSection(&m_stCriSec);

		for (auto  itr = m_vecMsgList.begin(); itr != m_vecMsgList.end(); itr++)
		{
			if(mCmdID == (*itr)->nCmdID && unSeq == (*itr)->unSeq)
			{
				T* pTmp = (*itr);
				m_vecMsgList.erase(itr);
				LeaveCriticalSection(&m_stCriSec);
				return pTmp;
			}
		}
		LeaveCriticalSection(&m_stCriSec);
		return nullptr;
	}

	int TimeOutCheck()
	{
		//		static int nTime = 0;
		if(100 < nTime)
		{
			time_t timCurrentTime;
			time(&timCurrentTime);
			EnterCriticalSection(&m_stCriSec);

			for (auto itr = m_vecMsgList.begin(); itr != m_vecMsgList.end();)
			{
				//message5秒超时
				if(5 < timCurrentTime - (*itr)->timCreateTime)
				{
					delete (*itr);
					itr == m_vecMsgList.erase(itr);
				}
				else
					++itr;
			}
			LeaveCriticalSection(&m_stCriSec);
			nTime = 0;
		}
		else
			nTime++;

		return 0;
	}	

private:
	list<T*> m_vecMsgList;
	CRITICAL_SECTION m_stCriSec;
	int nTime;
};

#include "ExternalInterface.h"
class CExternalInterface;
class CSipInterface
{
public:
	CSipInterface(void);
	virtual ~CSipInterface(void) =default;
	virtual int Init(const char *pstrUACID, const char *pstrIP, const char *pstrPort, const char * pstrPassword = NULL){ return 0;}
	virtual int UnInit(){ return 0; }
	void SetExternalInterfaceCallback(CExternalInterface* cb) { m_externalInterface = cb; }
	virtual int SendInvite(int nOperate, int nProType, const char *pstrDeviceID, const char *pstrChannelID,  const char *pstrRecvIP, int nRecvPort[2], void *pStreamer, char *ptmStart, char *ptmEnd){ return 0;}
	virtual int SendAnswered(int nStatus){ return 0;}
	virtual int SendDeviceControl(const char *pstrDeviceID, const char *pstrChannelID, BYTE bCmdType, BYTE cData1, BYTE cData2, BYTE cData3, BYTE cZoom = DRAG_ZOOM_NONE, int nLength = 0, int nWidth = 0, int nMidPointX = 0, int nMidPointY = 0, int nLengthX = 0, int nLengthY = 0){ return 0;}
	virtual int SendRemoteStartup(const char *pstrDeviceID){ return 0;}
	virtual const char * SendDeviceCatalogInquiry(const char *pstrDeviceID){ return 0;}
	virtual const char * SendDeviceInfoInquiry(const char *pstrDeviceID){ return 0;}
	virtual const char * SendDeviceStatusInquiry(const char *pstrDeviceID){ return 0;}
	virtual int SendEventSubscribe(const char *pstrDeviceID, int nStratPriority, int nEndPriority, int nMethod, char* pstrStartTime, char* pstrEndTime){ return 0;}
	virtual int SendCatalogSubscribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime){ return 0;}
	virtual int SendRecord(const char *pstrDeviceID, const char *pstrChannelID, int nActive){ return 0;}
	virtual int SendGuard(const char *pstrDeviceID, const char *pstrChannelID, int nActive){ return 0;}
	virtual int SendBye(const char *pstrDeviceID, const char *pstrChannelID, void *pStreamer, int nID){ return 0;}
	virtual int SendBye(const char *pstrDeviceID, const char *pstrChannelID, int nCid, int nDid, int nOption){ return 0;}
	virtual int SendPlayBackCtrl(const char *pstrDeviceID, const char *pstrChannelID, void* pStreamer, int nOperate, float fValue){ return 0;}
	
	virtual int ProcessAudio(void * pRTPTrans, BYTE * pBuffer, int nBufSize){ return 0;}
	virtual int CreateMediaStreamer(int nProtocolType, int nStreamerType, _tstring strRemoteIP, int nRemotePort, lpInviteParam_t pInviteInfo){ return 0;}
	virtual int StopStreamer(void **ppRTPTrans, int nOption){ return 0;}
	virtual int SetDeviceStatusCallBack(DeviceStaustAlarm);
	virtual int SetPlayBackFinishedCallBack(PlayBackFinished fpPlayBackFinished);
	virtual int SetInviteResponseInfoCallback(InviteResponsed fpInviteResponsed);
	virtual int SetPlayBackID(void* pStreamer, int nCid, int nDid){ return 0;}
	virtual int GetPlayBackID(void* pStreamer, int &nCid, int &nDid){ return 0;}
	//bSecrecy 0：不涉密，1：涉密 
	//cy 2015.7.30 changed
	virtual const char* SendRecordInfoInquiry(const char *pstrDeviceID, const char *pstrChannelID,  int nType,const char* pstrSTime, const char* strETime, const char* pstrFilePath = NULL, const char* pstrAddress = NULL, BYTE bSecrecy = 0, const char* pstrRecorderID = NULL, BYTE IndistinctQuery = 0){ return 0;}

	virtual int SendLogin(){ return 0;}
	virtual int SendLogin(const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword) { return 0;}
	virtual int SendLogout() { return 0; }
	virtual int SendChangePassword(const char* pstrOldPassword, const char* pstrNewPassword) { return 0; }

	// PWolf: Add 2013.06.17
	//int SendDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOption);
	virtual int SendDecorder(const char* pstrDecorderID, const char* pstrCameraID, int nOption){ return 0;}
	// PWolf: Add End
	virtual int ReplyCallAck(int nDID){ return 0;}
	virtual int SendResetAlarm(const char *pstrDeviceID, const char *pstrChannelID){ return 0;} //A.16.1
	virtual int GetOwnerType(){ return 0;}
	virtual float GetSpeed(void* pStreamer) {return 0;}
	int GetPlayStatus(void* pStreamer){return 0;}

	virtual int SendDeviceConfig(const char *pstrDeviceID, deviceConfig_info_t* pDeviceConfig){ return 0;}
	virtual DEVICE_PRESETQUERY_INFO_T* SendDevicePersetQuery(const char *pstrDeviceID){ return 0;}
	//DEVICECONFIG_AUDIOPARAMCONFIG DEVICECONFIG_AUDIOPARAMOPT 等
	virtual DEVICE_CONFIG_INFO_T* SendConfigDownload(const char *pstrDeviceID,const char *strConfigType){ return 0;}
   virtual const char * SendRealPlayUrl( const char *pstrChannelID, const int nWndNum, const char * pstrDeviceId ){ return 0;}
	virtual int SendStopPlayUrl(const char *pstrChannelID, const int nWndNum){ return 0;}
	virtual int SendDecoderDivision(const char *pstrDeviceID, const char *pstrChannelID, int nDivision){ return 0;}
	virtual const char * SendPlayBackUrl(const char *pstrDeviceID, const char *pstrChannelID, const char* pstrSTime, const char* strETime, const char* pstrLocation){ return 0;}
	virtual const char * SendDecoderStatus(const char *pstrDeviceID, const char *pstrChannelID){ return 0;}//A.12
	virtual int SendAlarmSubScribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime){ return 0;}//A.14
	virtual const char * SendAlarmQuery(const char *pstrDeviceID, const char *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime,const char* pstrAlarmType){ return 0;}//A.17

	virtual std::string GetLocalIP() {return "";}
public:
	BOOL IsExit();
	virtual void ExitThread();
protected:
	BOOL m_bIsExit;
protected:
	UINT m_unSeq;
	char m_strOwnID[ID_BUFFER_SIZE];
	//vector<DeviceInfo_t*> m_vecDevice;

	void *m_pStreamer;
	//CMeidaDecoder *m_pDecoder;

	string m_strRecordListXml;
	string m_strDeviceStatusListXml;
	string m_strDeviceInfoListXml;
	string m_strCatalogInfoListXml;
	string m_strDevicePersetQueryXml;
	string m_strDeviceConfigDownloadXml;
	string m_strRealPlayUrlXml;
	string m_strPlayBackUrlXml;
	string m_strDecoderStatusXml;
	string m_strAlarmQueryXml;
	int m_nRecordSum;

	INT64 nTargetTime;

	WaitList<invite_param_t> m_objInviteList;
	DeviceStaustAlarm m_fpDeviceStaustAlarm;
	PlayBackFinished m_fpPlayBackFinished;
	InviteResponsed m_fpInviteResponsed;
	ClientRealData m_fpClientHandleRealData;
	DEVICE_PRESETQUERY_INFO_T* ParseDevicePresetQuery(const char * pXml);
	DEVICE_CONFIG_INFO_T* ParseDeviceConfigDownload(const char * pXml);
	int ParseRealPlayUrl(const char * pXml,char* url);
	int ParseDecoderStatusVideoDeviceID(const char * pXml,char* strVideoDeviceID);

	string m_strSipClientID;
	CExternalInterface* m_externalInterface;
};

