#pragma once
#include "Socket/RawDataPkgMaker.hpp" 
#include <atomic>
class CModMessage;
class GBAdaptorCom;
class CSmartSocket;
class SocketSession;
class CUASTCP :public IUAS, public IRawDataHandle
{

public:
	CUASTCP(void);
	~CUASTCP(void) = default;

	void SetContext(GBAdaptorCom * pGBAdaptorCom = nullptr);

	//用来的监听的socket服务端主逻辑过程

	int RunSocketServerProc();

	//回掉socket消息，发送到SipCom模块
	int RawDataHandleProc(raw_data_t *pData = nullptr, void* pParam = nullptr) override;

	//recv ModMessage from Sipcom Module

	int OnRegisterRes(CModMessage * pUnifiedMsg = nullptr);

	int OnMessageRes(CModMessage* pUnifiedMsg = nullptr);

	int OnSubscribeRes(CModMessage * pUnifiedMsg = nullptr);

	int OnHusInviteRes(CModMessage * pUnifiedMsg = nullptr);

	int OnPlayBackCtrlRes(CModMessage * pUnifiedMsg = nullptr);

	int OnHusCallAckRes(CModMessage * pUnifiedMsg = nullptr);

	int OnsByeRes(CModMessage * pUnifiedMsg = nullptr);

	int OnCallMessage(CModMessage * pUnifiedMsg = nullptr);

	int OnSubscribeNotify(CModMessage * pUnifiedMsg);

	
	
	int ReplyHusDeviceStatusQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) override;

	int ReplyHusRegister(int nSeq, int nStatus, int nRegStatus);

	int ReplyHusPlayBackCtrl(int nSeq, int nStatus) override;

	int ReplyHusInvite(int nUniqueID,int nSeq, int nCid, int nDid, int nMsgStatus, const void* pExtendData) override;

	int ReplyHusCallAck(void)override;

	int ReplyHusCallAck(int nDID)override;

	int ReplyHusDeviceCatalogQuery(int nSeq, int nStatus, const char* pExterndData, const char* szFromDeviceID = nullptr) override;

	int ReplyHusClientLogin(int nSeq, int nStatus, int nLoginSum, const char* strSrcID, int nCmdID = CMD_LOGINRES) override;


	int ReplyHusRemoteStartup(int nSeq, int nStatus, int nOperate) override;

	int ReplyHusDeviceInfoQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) override;

	int  ReplyDeviceControl(int nSeq, int nStatus, int nOperate) override;
	
	int ReplyHusAlarmQuery(int nSeq, int nStatus, const char* pExtendData = nullptr) override;

	int ReplyHusRecordInfoQuery(int nSeq, int nStatus, const char* pExterndData = nullptr) override;


	//int ReplyHusDeviceConfig(raw_data_t *pData = nullptr) override;
	//int ReplyHusConfigDownload(raw_data_t *pData = nullptr) override;

	int SendHusMsgResWithEx(int nSeq, int nStatus, int nOperate, const char* pExtendData = nullptr);

	//布防/撤防
	int ReplyHusSetGuard(int nSeq, int nStatus, const char* pExterndData = nullptr) override;
	
	
	int ReplyHusSetRecord(int nSeq, int nStatus, const char* pExterndData = nullptr) override;
	
	int ReplyHusRealPlayUrlQuery(int nSeq, const char *strPlayUrl = nullptr) override;
	
	//int ReplyHusPresetQuery(raw_data_t *pData = nullptr) override;

	//解码器操作


	//Sip客户端登陆/退出/修改密码

	//Send消息类别是由下级域主动发起的事务内会话(要基上级的某项业务需求，比如上级订阅了目录消息/报警等)

	int SendHusCatalogNotify(int nSeq, int nStatus, int nOperate, const char* pExtendData = nullptr) override;

	int SendHusAlarmNotify(int nSeq, int nStatus, int nOperate, const char* strDstID = nullptr, const char* pAlarmContent = nullptr) override;

	int SendHusMediaStatusNotify(int nSeq, int nStatus, const char*strDstID, const char*strDstSubID, int nCid, int nDid) override;

	int ReplyHusCatalogQuery(int nSeq, int nStatus, const char* szFromDeviceID, void* pParams, int * pCount = nullptr) override;



private:

	//int PlayStart(char* pstrCameraID, char* pstrDecoderID) ;

	//int PlayStop(char* pstrCameraID, char* pstrDecoderID) ;

	int ClassifyAndParserRawData(raw_data_t * pRawData, CModMessage * pUnifiedMsg = nullptr);


	int SendSocketMessage(RAW_DATA_PKG_PTR pData = nullptr);


	int MsgTimeOutCheck();

	int GetUniqueSeq();

	SOCKET MatchRequest(int nCseq = 0);


	CSmartSocket *m_SocketServer;

	std::shared_ptr<SocketSession> m_pSocketSession = nullptr;

	u_short m_nPort;

	std::string m_strIP;

	GBAdaptorCom * m_pGBAdaptorCom;

	std::atomic_int nCSeq = 0;

	int nUniqueID = 0;

	//平台设备列表(可以支持到多平台)
	std::unordered_map<std::string, std::vector<std::string>>  m_PlatformDeviceList;


};

