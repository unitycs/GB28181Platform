#pragma once
#include <vector>
#include "Memory/MapWithLock.h"
#include "Memory/ObjectAllocator.hpp"
const int DECODER_TIMEOUT = 10;

enum SectionID
{
	SI_PTZ_COMMAND = 1,
	SI_DECODER_PLAY = 221,
	SI_DECODER_LAYOUT = 222,
	SI_DECODER_PLAY2 = 333,
	SI_DECODER_STOP,
};
enum PTZItemID
{
	PTZ_LEFT = 1,
	PTZ_RIGHT,
	PTZ_UP,
	PTZ_DOWN,
	PTZ_ZOOMOUT,
	PTZ_ZOOMIN,
	PTZ_STOP = 153
};
enum DecoderItemID
{
	DII_DECODER_ID = 1,     // 解码器编号,文本
	DII_ENCODER_ID,			// 编码器编号,文本
	DII_OPERATE_MODE,       // 模式,{选择窗口\/0}{实时PTZ\/1}{实时数字缩放\/2}{回放\/3}{回放数字缩放\/4}
	DII_ERROR_CODE,         // 错误码,数字（最小值0）
	DII_ERROR_MSG,			// 错误消息
	DII_DEVICE_TREE,		// 设备树信息,文本
	DII_PLAY_MODE,			// 播放模式,实时\/0}{回放\/1}
	DII_USER_NAME = 11,     // 用户名,文本
	DII_HUSSIE_ADDRESS		// HUS站点地址,文本
};

typedef struct SubAlarmInfoStruct
{
	INT64   nCallID;
	CString szToDeviceID;
	CString szSN;
	CString  alarmMethod;
	CString remoteId;
	int      deviceIdType;
	time_t     startTime;
	time_t     endTime;
	time_t     expireTime;
	GUID     guid;
	int      startProperty;
	int      endProperty;
}SubAlarmInfo_t;

struct screenInfoStruc
{
	int screenNum;
	std::string screenGbId;
	std::string status;
};

struct decoderChnInfoStruc
{
	int  chnNum;
	int  curScreenCount;
	std::string chnGbId;
	std::vector<screenInfoStruc> screenInfo;
};

struct decoderInfoStruc
{
	std::string gbId;
	std::string guid;
	int    chnIdStart;
	int    screenIdStart;
	int    maxScreenCount;
	int    validFlag;
	std::vector<decoderChnInfoStruc> decoderChnInfo;
};


// 报警信息对象类型
typedef struct _DeviceAlarmInfo
{
	//布防/撤防状态
	enum DutyStatus
	{
		ONDUTY = 0,
		OFFDUTY = 1,
		ALARM = 2
	};

	_DeviceAlarmInfo(const TCHAR *pszID, DutyStatus eStatus_in) : m_nAlarmMethord(0), m_nAlarmType(0)
	{
		szAlarmID = pszID;
		eStatus = eStatus_in;
	}

	_DeviceAlarmInfo(const char *pszGBID, int nAlarmMethord, int nAlarmType, const char * pszAlarmStatus, const char *pszDescribe, const char *pszLevel, const char *pszTime) : m_nAlarmMethord(0), m_nAlarmType(0)
	{
		m_nAlarmMethord = nAlarmMethord;
		m_nAlarmType = nAlarmType;
		strDeviceID = pszGBID;
		m_strAlarmStatus = pszAlarmStatus;
		m_Description = pszDescribe;
		m_strPriority = pszLevel;
		m_strTime = pszTime;
		szAlarmID = pszGBID;
		eStatus = ONDUTY;
	}
	_DeviceAlarmInfo() : m_nAlarmMethord(0), m_nAlarmType(0)
	{

		eStatus = OFFDUTY;
	}
	// 布、撤防状态
	DutyStatus eStatus;

	CString szAlarmID;

	int m_nAlarmMethord;

	int m_nAlarmType;

	CString  m_strAlarmStatus;

	CString strDeviceID;
	// 报警描述
	CString m_Description;

	// 报警级别
	CString m_strPriority;

	// 报警时间
	CString m_strTime;

	CString  m_StrGUID;

	CString m_strDeviceName;

	void  Clear()
	{
		m_nAlarmType = 0;
		m_nAlarmMethord = 0;
		m_Description.Empty();
		m_strPriority.Empty();
		m_strTime.Empty();
	}

}DeviceAlarmInfo;


typedef struct _DecoderPairInfo
{
	_DecoderPairInfo() {}
	_DecoderPairInfo(const GUID &guidDecoderRef, const GUID &guidEncoderRef, const GUID &guidECRef, const char *pszNumber)
	{
		guidDecoder = guidDecoderRef;
		guidEncoder = guidEncoderRef;
		guidEC = guidECRef;
		strchannelIndex = pszNumber;
	}
	// 解码器GUID
	GUID guidDecoder;
	// 编码器GUID
	GUID guidEncoder;
	// 解码器对应的EC GUID
	GUID guidEC;
	// 解码器通道编号
	CString strchannelIndex;
}DecoderPairInfo_t;

// 设备关联的服务器，解码器等信息
typedef struct _HUSDeviceLinkInfo_T
{
	GUID		guidDevice;
	GUID		guidParent;
	GUID		guidEC;
	GUID		guidVirSteamer; //for ec cascade control.
	GUID		guidVirChannel; //for ec cascade control.
	GUID		guidNVR;
	GUID		guidStreamer;
	CString		strNVRIP;
	CString		strStreamerGUID;

	CString		strDevIP;
	CString		strDevPort;
	CString		strChannelNum;
	// 录像查询时使用
	CString		strDeviceName;
}HUSDeviceConnect_T;

typedef CMapWithLock<CString, LPCSTR, GUID, GUID&>						ECInfoMap;
typedef CMapWithLock<CString, LPCSTR, INT, INT&>						DeviceChangedMap;

static _ATL_FUNC_INFO OnEventInfo = { CC_STDCALL, VT_I4, 1, { VT_BSTR } };
static _ATL_FUNC_INFO OnFactoryEventInfo = { CC_STDCALL, VT_I4, 2, { VT_UNKNOWN, VT_BYREF | VT_USERDEFINED } };
