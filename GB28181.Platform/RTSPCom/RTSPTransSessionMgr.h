#pragma once
#include "RTSPTransMgrBase.h"

class CRTSPTransSessionMgr :
	public CRTSPTransMgrBase
{
public:
	CRTSPTransSessionMgr(void) = default;
	virtual  ~CRTSPTransSessionMgr(void) = default;


	void ProcessOperation(CModMessage * pMsg) override;

protected:

	// 发送Descri命令
	int Describe(CRTSPSession *pSession) override;

	// 发送Setup命令
	int Setup(CRTSPSession *pSession) override;

	// 发送SDP文件
	int SDPResponse(CRTSPSession *pSession) override;

	// 处理网络事件
	int ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT hNetEvent) override;

	// 处理队列事件
	int ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t * pThreadInfo)  override;

	int SendBye(CRTSPSession *pSession);


};
