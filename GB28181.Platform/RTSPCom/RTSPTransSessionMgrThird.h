#pragma once
#include "RTSPTransMgrBase.h"

class CRTSPTransSessionMgrThird :
	public CRTSPTransMgrBase
{
public:
	CRTSPTransSessionMgrThird(void) = default;
	virtual ~CRTSPTransSessionMgrThird(void) = default;

	void ProcessOperation(CModMessage * pMsg) override;


protected:


	// 处理网络事件
	int ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT hNetEvent)  override;

	// 处理队列事件
	int ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t* pThreadInfo)  override;


	int  SetupThird(CRTSPSession *pSession, CModMessage *pMsg);


protected:

	//int (CRTSPTransSessionMgr::*m_pfnRTSPProc[rmt_maxtype][rs_maxstatus])(CRTSPSession *pSession);
	// RTSP服务端事件的处理函数列表

private:
	void parseThirdSdp(CRTSPSession *pSession, CModMessage *pMsg) const;


};
