#pragma once
#include "RTSPTransMgrBase.h"

class CRTSPTransSessionMgrBroadcast :
	public CRTSPTransMgrBase
{
public:
	CRTSPTransSessionMgrBroadcast(void) = default;
	virtual ~CRTSPTransSessionMgrBroadcast(void) = default;

	void ProcessOperation(CModMessage * pMsg) override;

protected:


	// 处理网络事件
	int ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT hNetEvent) override;

	// 处理队列事件
	int ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t* pThreadInfo) override;

	// 发送Play命令
	int Play(CRTSPSession *pSession) override;


	int  SetupBroadcast(CRTSPSession *pSession, CModMessage *pMsg);

private:
	void parseBroadcastSdp(CRTSPSession *pSession, CModMessage *pMsg) const;


};
