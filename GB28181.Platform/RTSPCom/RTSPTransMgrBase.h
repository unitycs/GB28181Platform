#pragma once
#include "RTSPSessionBuilder.h"
#include "RTSPParser.h"
#include "Memory/SimpleQueue.h"
#include "TCPClient.h"

const int RES_TIMEOUT = 10;
const int KEEPAVLIE_TIME = 15;

class CRTSPTransMgrBase :
	public CObject
{
public:
	CRTSPTransMgrBase(void);
	virtual ~CRTSPTransMgrBase(void);


	virtual bool isCurrentSession(CModMessage * pMsg);
	virtual void ProcessOperation(CModMessage * pMsg) = 0;

	typedef struct _ThreadInfo_
	{
		//当前线程所处会话的CID和DID
		INT64    nCallDialogID;
		// 和父线程之间的通信队列
		CSimpleQueue<CModMessage*> oMsgQueue;
		// 退出事件
		CEvent   oEvent;
		CString  subject;
		CString	 strDeviceID;
		int      realPlayFlag;
		void ClearMsgQueue()
		{
			CModMessage *pMsg = nullptr;
			while (oMsgQueue.Pop(pMsg, 1000))
				SAFE_FREE_MOD_MSG(pMsg);
				pMsg = nullptr;
		}

		

	}ThreadInfo_t;

	typedef struct ThreadParam
	{
		CRTSPTransMgrBase	*pThis;
		INT64			nCallDialogID;
		CString			strDeviceID;
		void            *pThreadInfo;
	}ThreadParam_t;

	typedef std::function<UINT(ThreadParam_t)> PlaySessionThreadDelegate;

	static bool isRealPlay(CModMessage *pUnifiedMsg)
	{
		SDPParser sdpStack;
		if (NULL != pUnifiedMsg->GetPlayData())
		{
			auto pSDPFile = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
			// 解析SDP文件
			sdpStack.decode_sdp(pSDPFile->GetBuffer());
		}
		else
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "未找到DSP文件", pUnifiedMsg->GetQuerySN());
			return false;
		}

		// 判断媒体类型,是play而不是playback
		if ("play" == sdpStack.GetSDPStack().session_name.name.Trim().MakeLower())
		{
			return true;
		}
		return false;
	}
	
protected:

	using  MSG_PTR_QUEUE_T = CSimpleQueue<CModMessage*>;

	// 生成RTSP序列号
	void GenerateCSeq(CString *pstrCSeq);
	// 发送RTSP消息
	static bool SendRTSP(CRTSPSession *pSession, const char *pszData);
	// 发送Options命令
	virtual int Options(CRTSPSession *pSession);

	virtual int Describe(CRTSPSession * pSession);

	virtual int SDPResponse(CRTSPSession * pSession);

	virtual int Setup(CRTSPSession * pSession);

	virtual int Teardown(CRTSPSession * pSession);

	virtual int Pause(CRTSPSession * pSession);

	virtual int PlayCtrl(CRTSPSession * pSession);

	virtual int Play(CRTSPSession * pSession);

	virtual int GetParameter(CRTSPSession * pSession);

	virtual int SetParameter(CRTSPSession * pSession);

	virtual int ProcessNetworkEvent(CRTSPSession * pSession, WSAEVENT hNetEvent);

	virtual int ProcessMsgQueueEvent(CRTSPSession* pSession, ThreadInfo_t* pThreadInfo);

	virtual UINT ThreadProc(ThreadParam_t  paramInfo);

	virtual UINT ThreadEventLoop(CRTSPSession * pSession, ThreadInfo_t * pThreadInfo);

	virtual UINT ThreadDestroy(CRTSPSession * pSession, ThreadInfo_t * pThreadInfo);

	virtual int TimeOutProc(CRTSPSession * pSession);

	int StartKeepalive(CRTSPSession * pSession);

	int GetVideoTrack(RTSP_Unknown_Msg & tRTSPMsg, CString * pstrTrackID);

	int GetTCPMediaPort(RTSP_Unknown_Msg & tRTSPMsg, CString * pstrMediaPort);

	int Getfilesize(RTSP_Unknown_Msg & tRTSPMsg, CString * pstrFileSize);

	void ParserMediaServerIPList();

	// 会话对象管理器
	CRTSPSessionBuilder m_oRTSPSessionBuilder;

	// CSeq的互斥变量
	CCriticalSection	m_oCS;

	// RTSP 序列号
	int m_nCSeq;

	// 内部模块message对象内存管理器
	CMemPool m_oModuleMsgMemMgr;

	// 要回复的消息体数据缓存管理器
	CMemPool m_oBodyBufMemMgr;

	// 线程通信列表
	CMapWithLock<INT64, INT64&, ThreadInfo_t*, ThreadInfo_t*&> m_oThreadQueueMap;

	CCriticalSection m_oQueueMapLock;

	PlaySessionThreadDelegate PlayThreadWorker;


	CString m_strPassword;

	CString strFirewall_MediaIP;

	CMap<CString, LPCTSTR, CString, LPCTSTR> m_MediaMapIpList;

	// 下载倍率:默认是1,比如当上级为4倍速，下级默认也为4倍速。
	//但是有些上级平台要求的实际麻溜倍率是放大的,比如，当金鹏的上级GB平台，下发8倍速下载的时候，实际要求是以32倍速发送码流。
	UINT m_nDownLoadRate = 1;

	using  FUNC_TYPE_PTR = int (CRTSPTransMgrBase::*)(CRTSPSession *pSession);

	FUNC_TYPE_PTR m_pfnRTSPProc[rmt_maxtype][rs_maxstatus] = { { nullptr } };



private:
	void setGlobalMediaParameter();

};

class CRTSPSession : public CMemPoolUnit
{
public:
	CRTSPSession() :
		pRtspClient(nullptr),
		pInviteSender(nullptr),
		pUnifiedMsg(nullptr)
	{
		memset(szToDeviceID, 0, ID_BUF_LEN);
		memset(szClientPort, 0, PORT_BUF_LEN);
		memset(szServerIP, 0, IP_BUF_LEN);
		memset(szURL, 0, URL_BUF_LEN);
	}
	char szToDeviceID[ID_BUF_LEN];			// 前端视频设备ID
	char szClientIP[IP_BUF_LEN];			// 码流接收端的IP
	char szClientPort[PORT_BUF_LEN];		// 码流接收端的端口
	char szServerIP[IP_BUF_LEN];			// 前端视频IP
	char szGUID[GUID_BUF_LEN];				// 设备GUID
	char szScale[SCALE_BUF_LEN];			// 播放速率
	char szCurTime[TIME_BUF_LEN];			// 播放开始时间
	char szStartTime[TIME_BUF_LEN];			// 录像开始时间
	char szEndTime[TIME_BUF_LEN];			// 录像结束时间
	char szSessionID[SESSION_BUF_LEN];		// RTSP会话Session-ID
	char szCSeq[CSEQ_BUF_LEN];				// 上一次RTSP消息的序列号
	char szURL[URL_BUF_LEN];				// 设备对应的URL
	char szSSRC[SSRC_BUF_LEN];				//域内码流唯一性标识的10位ID值
	char szFileSize[CSEQ_BUF_LEN];			// 录像文件大小
	char szBroadcastSrcId[ID_BUF_LEN];     //broadcast源
	void *pInviteSender;                    //标识invite 发送者是一个threadinfo结构体
	void * pUnifiedMsg;                    //标识要处理的UnifiedMsg
	INT64			nCallDialogID;			// 当前会话对应的SIP的cid和did
	int				nTID;					// 当前会话对应的SIP的tid
	RTSPStatus		eStatus;				// 会话当前状态
	MediaAction_T	eMediaActionT;				// 会话的媒体类型
	RTSP_Unknown_Msg *pRTSPMsg;				// 当前接收的RTSP消息
	time_t		     tmStart;				// 播放开始时间
	time_t           tmEnd;
	time_t			tmPoint_Keepalive;		// 下次保活的时间点
	time_t			tmPoint_Timeout;		// 应答超时的时间点

	TransProto_T    m_transProto;            //视频或音频数据是udp 还是tcp
	char            m_szDownLoadSpeed[20];   //下载速度
	MediaData_T     m_mediaDataType;            //0 视频 1音频
	char			m_szTCPMediaPort[PORT_BUF_LEN];		//for guang 6 metro //tcp stream port


	//如果不存在则创建Rtsp客户端端对象
	CRTSPClient* GetRtspClient()
	{
		if (!pRtspClient) //判断是否存在tcpclient链接
			pRtspClient = new CRTSPClient(szServerIP);
		return pRtspClient;
	}

	void DestroyRtspClient()
	{
		if (pRtspClient)
		{
			delete pRtspClient;
			pRtspClient = nullptr;
		}
	}




private:
	CRTSPClient		*pRtspClient;			// RTSP链路的客户端

};