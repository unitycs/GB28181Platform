#pragma once
class IUAS
{
public:

	IUAS(void) = default;

	virtual ~IUAS(void) = default;

	virtual int KeepAlive();

	virtual int SendGetwayAlive(void);

	virtual int ReplyHusRegister(int nSeq, int nStatus, int nRegStatus) = 0;

	//设备控制
	virtual int ReplyDeviceControl(int nSeq, int nStatus, int nOperate) = 0;

	virtual int ReplyHusDeviceCatalogQuery(int nSeq, int nStatus, const char* pExterndData = nullptr, const char* szFromDeviceID = nullptr) = 0;

	virtual int ReplyHusRemoteStartup(int nSeq, int nStatus, int nOperate) = 0;

	virtual int ReplyHusAlarmQuery(int nSeq, int nStatus, const char* pExtendData = nullptr) = 0;


	//媒体播放控制

	virtual int ReplyHusPlayBackCtrl(int nSeq, int nStatus) = 0;

	virtual int ReplyHusCallAck(void) = 0;

	virtual int ReplyHusCallAck(int nDID) = 0;

	virtual int ReplyHusInvite(int nUniqueID, int nSeq, int nCid, int nDid, int nStatus, const void* pExtendData = nullptr) = 0;

	virtual int ReplyHusCatalogQuery(int nSeq, int nStatus, const char* szFromDeviceID, void* pParams, int * pCount) = 0;

	virtual int ReplyHusClientLogin(int nSeq, int nStatus, int nLoginSum, const char* strSrcID, int nCmdID) = 0;

	virtual int ReplyHusSetGuard(int nSeq, int nStatus, const char* pExterndData = nullptr) = 0;

	virtual int ReplyHusSetRecord(int nSeq, int nStatus, const char* pExterndData = nullptr) = 0;

	//virtual int ReplyHusDeviceConfig(raw_data_t *pData = nullptr) = 0;

	//virtual int ReplyHusConfigDownload(raw_data_t *pData = nullptr) = 0;

	//virtual int ReplyHusPresetQuery(raw_data_t *pData = nullptr) = 0;


	virtual int ReplyHusDeviceInfoQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) = 0;

	virtual int ReplyHusDeviceStatusQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) = 0;

	virtual int ReplyHusRecordInfoQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) = 0;

	//解码器相关
	virtual int ReplyHusRealPlayUrlQuery(int nSeq, const char *strPlayUrl = nullptr) = 0;


	virtual int SendHusMediaStatusNotify(int nSeq, int nStatus, const char* szDstID, const char*szDstSubID, int nCid, int nDid) = 0;

	virtual int SendHusCatalogNotify(int nSeq, int nStatus, int nOperate, const char* pExtendData = nullptr) = 0;

	virtual int SendHusAlarmNotify(int nSeq, int nStatus, int nOperate, const char* strDstID = nullptr, const char* pAlarmContent = nullptr) = 0;


};