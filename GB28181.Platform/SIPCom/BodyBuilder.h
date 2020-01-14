#pragma once
#include "Common/common.h"
#include "tinyXML\tinyxml2.h"
typedef struct XmlParamStruct
{
	CString strParamVal1;
	CString strParamVal2;
	CString strParamVal3;
	CString strParamVal4;
	CString strParamVal5;
	CString strParamVal6;
	CString strParamVal7;
	CString strParamVal8;
}XmlParam_t;

typedef struct VideoParamStruct
{
	CString strStreamName;
	CString strVideoFormat;
	CString strResolution;
	CString strFrameRate;
	CString strBitRateType;
	CString strVideoBitRate;
}VideoParam_t;

typedef struct AudioParamStruct
{
	CString strStreamName;
	CString strAudioFormat;
	CString strAudioBitRate;
	CString strSamplingRate;
}AudioParam_t;

typedef struct ConfigParamStruct
{
	CString strName;
	CString strDeviceID;
	CString strServerID;
	CString strServerIP;
	CString strServerPort;
	CString strDomainName;
	CString strExpiry;
	CString strPassword;
	CString strHeartBeat;
	CString strMaxCount;
	std::vector<VideoParam_t> szVideoConfigs;
	std::vector<AudioParam_t> szAudioConfigs;
}ConfigParam_t;

typedef struct DragParamStruct
{
	CString strDragType;
	CString strLength;
	CString strWidth;
	CString strMidPointX;
	CString strMidPointY;
	CString strLengthX;
	CString strLengthY;
}DragParam_t;

#define  XML_HEAD_WITH_DECLARATION_ENCODING_UTF8     "<?xml version=\"1.0\" encoding = \"utf-8\"?>\r\n"
#define  XML_HEAD_WITH_DECLARATION_ENCODING_GB2312   "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
#define  XML_HEAD_WITH_DECLARATION_ENCODING_NONE	    "<?xml version=\"1.0\"?>\r\n"

class CBodyBuilder
{
public:
	using XMLDocument = tinyxml2::XMLDocument;
	using XMLElement = tinyxml2::XMLElement;
	using XMLText = tinyxml2::XMLText;
	using XMLPrinter = tinyxml2::XMLPrinter;
	CBodyBuilder(void) = default;
	virtual ~CBodyBuilder(void) = default;

	static int Init();

	// 创建Keepalive的数据包体
	static int CreateKeepaliveBody(char *pszKeepaliveBuf, int nBufLen, const char *pszSN, const char *pszDevID);

	// 创建报警上报的数据包体
	int CreateAlarmBody(char *pszAlarmBuf, int nBufLen, const char *pszSN, const char *pszDevID, const char *pszDescription, BYTE nLevel, const char *pszTime) const;
	// 创建状态上报的数据包体
	static int AddStatusReportBody(char *pszAlarmBuf, int &nBufLen, const char *pszDevID, const char* pszName, const char *pszParentID, const char* pszStatus);

	// 生成设备应答的SDP类型Body
	static int CreateInviteAnswerBody(char  *pszSDPBuf, int nBufLen,
		const char *pszIP, const char *pszPort,
		const char *pszOperateType, const char *pszActiveType,
		const char *pszStartTime, const char *pszEndTime,
		const char *pszID, const char *pszPassWord,
		const char *pszSSRC);

	int CreateInviteAnswerBody(char  *pszSDPBuf, int nBufLen,
		const char *pszIP, const char *pszPort,
		const char *pszOperateType, const char *pszActiveType,
		const char *pszStartTime, const char *pszEndTime,
		const char *pszID, const char *pszPassWord,
		const char *pszSSRC, int transType, int mediaType, const char *pszFileSize) const;

	// 开始生成录像文件信息xml文件头
	static int CreateRecordHead(char *pRecordBuf,					// xml文件的缓存
		int &nBufLen,						// IN-缓存长度，OUT-缓存已填充长度
		const char *pstrSN,					// SN号
		const char *pstrDeviceID,			// 设备ID
		const char *pstrDeviceName,			// 设备Name
		int nSumNum,						// 录像总数
		int nListNum);						// 当前xml中包含的录像个数
// 添加xml body
	static int AddRecordBody(char *pszRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		const char *pszDeviceID,							// 设备ID
		const char *pstrName,
		const char *pszStartTime,
		const char *pszEndTime,
		const char *pszAddress,
		const char *pszFilePath,
		int   nRecordType,
		int   nSecrecy);

	static int AddRecordBodyShengtong(char *pszRecordBuf,
		int &nBufLen,
		const char *pszDeviceID,
		const char *pstrName,
		const char *pszStartTime,
		const char *pszEndTime,
		const char *pszAddress,
		const char *pszFilePath,
		const char *FileSize,
		const char *RecorderID,
		int	  nRecordType,
		int   nSecrecy);

	// 完成xml
	static int CompleteRecord(char *pRecordBuf, int nBufLen);

	// 开始生成目录信息xml文件头
	static int CreateCatalogHead(char *pRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		const char *pstrSN,									// SN号
		const char *pstrDeviceID,							// 设备ID
		int nSumNum,										// 设备总数
		int nListNum);										// 当前xml中包含的设备个数
	static int CreateCatalogHead(char *pszRecordBuf,
		int &nBufLen,
		const char *pszDataType,
		const char *pstrSN,
		const char *pstrDeviceID,
		int nSumNum,
		int nListNum);
	// 添加Catalog xml body
	static int AddCatalogBody(char *pszRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		void *pszCatalog);
	static int AddCatalogBody(char* pszRecordBuf, int& nBufLen, const char* pszDeviceID, const char* pszEvent, const char* pstrName, const char* pszManufacturer, const char* pszModel, const char* pszOwner, const char* pszCivilCode, const char* pszAddress, const char* pszParental, const char* pszSafetyWay, const char* pszRegisterWay, const char* pszSecrecy, const char* pszStatus, const char* pszParentID, const char* pszPTZType);
	static int CompleteCatalog(char* pszRecordBuf, int nBufLen, const char* pszDataType);
	// 完成xml
	static int CompleteCatalog(char *pRecordBuf, int nBufLen);

	//// 生成status的xml
	//int CreateStatusBody(char *pRecordBuf, int &nBufLen,
	//					const char *pszSN,
	//					const char *pszDeviceID,
	//					const char *pszResult,
	//					const char *pszOnlien,
	//					const char *pszStatus,
	//					const char *pszEncode,
	//					const char *pszRecord,
	//					const char *pszTime,
	//					int nAlarmSum,
	//					bool bIsEncoder);
	   // 生成status的xml
	static int CreateStatusBody(char *pRecordBuf, int &nBufLen,
		const char *pszSN,
		const char *pszDeviceID,
		const char *pszResult,
		const char *pszOnlien,
		const char *pszStatus,
		const char *pszEncode,
		const char *pszRecord,
		const char *pszTime,
		int nAlarmSum,
		bool bIsEncoder);

	static int AddAlarmToStatusBody(char *pStatusBuf, int &nBufLen,
		const char *pszAlarmID, const char *pszDutyStatus);

	static int ComplateStatusBody(char *pStatusBuf, int nBufLen);

	static int CreatePropertyBody(char *pPropertyBuf, int nBufLen,
		const char *pszSN,
		const char *pszID,
		const char *pszResult,
		const char *pszDevType,
		const char *pszManufacturer,
		const char *pszModel,
		const char *pszFirmware,
		const char *pszMaxCamera,
		const char *pszMaxAlarm,
		bool bIsEncoder);

	static int	CreateMediaStatusBody(char *pMediaStatusBuf, int nBufLen,
		const char *pszSN,
		const char *pszID);
	static int CreateControlResponseBody(char *pszKeepaliveBuf, int nBufLen,
		const char *pszSN,
		const char *pszDevID,
		const char *pszResult);

	static int CreateDeviceConfigResponse(char *pszResponseBuf, int nBufLen,
		const char *pszSN,
		const char *pszDevID,
		const char *pszResult);

	static int CreateDecoderDivisonResponseBody(char *pszKeepaliveBuf, int nBufLen,
		const char *pszSN,
		const char *pszDevID,
		const char *pszResult);

	static int CreateStopPlayUrlResponseBody(char *pszKeepaliveBuf, int nBufLen,
		const char *pszSN,
		const char *pszDevID,
		const char *pszResult);

	static int CreateConfigSearchHead(char *pSearchBuf,
		int &nBufLen,
		const char *pszSN,
		const char *pszDeviceID,
		const char *pszResult);

	static int AddBasicParamConfig(char *pSearchBuf,
		int &nBufLen,
		const char *pName,
		const char *pDeviceID,
		const char *pServerID,
		const char *pServerIP,
		const char *pServerPort,
		const char *pDomainName,
		const char *pExpiration,
		const char *pPasswd,
		const char *pHeartBeat,
		const char *pHeartBeatCount);

	static int AddVideoParanOpt(char *pSearchBuf,
		int &nBufLen,
		const char *pVideoFormat,
		const char *pResolution,
		const char *pFramRate,
		const char *pBitRateType,
		const char *pVideoBitRate,
		const char *pDownloadSpeed);

	static int AddVideoParamConfigHead(char *pSearchBuf, int &nBufLen, int nNum);

	static int CompleteVideoParamConfig(char *pSearchBuf, int &nBufLen);

	static int AddVideoParamConfig(char *pSearchBuf,
		int &nBufLen,
		const char *pStreamName,
		const char *pVideoFormat,
		const char *pResolution,
		const char *pFrameRate,
		const char *pBitRateType,
		const char *pVideoBitRate);

	static int AddAudioParamOpt(char *pSearchBuf,
		int &nBufLen,
		const char *pAudioFormat,
		const char *pAudioBitRate,
		const char*pSamplingRate);

	static int AddAudioParamConfigHead(char *pSearchBuf, int &nBufLen, int nNum);

	static int AddAudioParamConfig(char *pSearchBuf,
		int &nBufLen,
		const char *pStreamName,
		const char *pAudioFormat,
		const char *pAudioBitRate,
		const char*pSamplingRate);

	static int CompleteAudioParamConfig(char *pSearchBuf, int &nBufLen);

	static int CompleteConfigSearch(char *pSearchBuf, int nBufLen);

	static int CreateRealPlayUrlResponse(char *pSearchBuf, int nBufLen,
		const char *pszSN,
		const char *pszID,
		const char *pszChannelID,
		const char *pszPlayUrl);

	static int CreateRealPlayUrlResponseByContrl(char *pSearchBuf, int nBufLen,
		const char *pszSN,
		const char *pszID,
		const char *pszChannelID,
		const char *pszPlayUrl,
		const char *pszResult);

	static int CreatePlayBackUrlResponse(char *pSearchBuf, int nBufLen,
		const char *pszSN,
		const char *pszID,
		const char *pszPlayUrl);

	static int CreatePlayBackUrlResponseByControl(char *pSearchBuf, int nBufLen,
		const char *pszSN,
		const char *pszID,
		const char *pszPlayUrl,
		const char *pszResult);

	static int parseBroadCastXml(char *xml, CString& sourceId, CString& targetId, CString& sn);

	int CreateBroadCastXml(char *pszBroadcastReponseBuf, CString& sourceId, CString& targetId, CString& sn, CString& result) const;

	// 开始生成录像文件信息xml文件头
	static int CreateAlarmRecordHead(char *pRecordBuf,					// xml文件的缓存
		int &nBufLen,						// IN-缓存长度，OUT-缓存已填充长度
		const char *pstrSN,					// SN号
		const char *pstrDeviceID,			// 设备ID
		const char *pstrDeviceName,			// 设备Name
		int nSumNum,						// 录像总数
		int nListNum);						// 当前xml中包含的录像个数
	// 添加xml body
	static int AddAlarmRecordBody(char *pszRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		const char *pszDeviceID,							// 设备ID
		const char *alarmPriority,
		const char *alarmMethod,
		const char *alarmTime,
		const char *alarmDescription,
		const char *longitude,
		const char *latitude,
		const char *alarmType,
		const char *alarmStatus);

	// 完成xml
	static int CompleteAlarmRecord(char *pRecordBuf, int nBufLen);

	// 创建报警上报的数据包体
	static int CreateAlarmBodyNew(char *pszAlarmBuf, int nBufLen, const char *pszSN, const char *pszDeviceID,							// 设备ID
		const char *alarmPriority,
		const char *alarmMethod,
		const char *alarmTime,
		const char *alarmDescription,
		const char *longitude,
		const char *latitude,
		const char *alarmType,
		const char *alarmStatus);

	static int createSubscribeResponse(char *pRecordBu, int nBufLen, const char *pszSn, const char *pszDeviceId, char *cmdTyp, char *result);

	static int CreateSubscribeResBody(char* pszAlarmBuf, int nBufLen, const char* pszSN, const char* pszDevID, const char* pszResult);

	static int CreateNotifyCatalogHead(char *pRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		const char *pstrSN,									// SN号
		const char *pstrDeviceID,							// 设备ID
		int nSumNum,										// 设备总数
		int nListNum);										// 当前xml中包含的设备个数
	// 添加Catalog xml body
	static int AddNotifyCatalogBody(char *pszRecordBuf,					// xml文件的缓存
		int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
		void *pszCatalog);

	// 完成xml
	static int CompleteNotifyCatalog(char *pRecordBuf, int nBufLen);

protected:
	//// SIP保活包体SN之前部分
	//CString m_strKeepaliveBody1;

	//// SIP保活包体SN之后部分
	//CString m_strKeepaliveBody2;
	// SDP数据包
	CString m_strInviteRespBody;

	// 录像文件xml头
	CString m_strRecordHead;

	// 录像文件xml尾
	CString m_strRecordEnd;
};
