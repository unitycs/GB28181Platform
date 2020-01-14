#pragma once
#include "RTSPTransMgrBase.h"

class CRTSPTransSessionRecMgr :
	public CRTSPTransMgrBase
{
public:
	CRTSPTransSessionRecMgr(void);
	virtual ~CRTSPTransSessionRecMgr(void);
	//判断是否在正在录像
	bool isCurrentSession(CModMessage * pMsg) override;
	void ProcessOperation(CModMessage * pMsg) override;

	int Options(CRTSPSession * pSession) override;
	// 发送RTSP录像指令
	bool StartRecord(CRTSPSession *pSession);


protected:
	// 线程通信队列
	CMapWithLock<CString, LPCSTR, ThreadInfo_t*, ThreadInfo_t*&> m_oThreadQueueMap;

	// 录像消息的重发时间
	const int RECORD_TIME;

	int TimeOutProc(CRTSPSession *pSession) override;

	UINT ThreadDestroy(CRTSPSession * pSession, ThreadInfo_t * pThreadInfo) override;

	int ProcessNetworkEvent(CRTSPSession * pSession, WSAEVENT hNetEvent) override;

	int ProcessMsgQueueEvent(CRTSPSession * pSession, ThreadInfo_t* pThreadInfo)  override;

private:

	void NoticeSDKCom(mod_op_t::u_op_type_t devOperateType, void * pMemoUnit);

};
