#include "StdAfx.h"
#include "RTSPTransSessionMgrRec.h"

CRTSPTransSessionRecMgr::CRTSPTransSessionRecMgr(void)
	: RECORD_TIME(15) //每15秒重新发送一次录像状态更新给VMS
{
}

CRTSPTransSessionRecMgr::~CRTSPTransSessionRecMgr(void)
{
	// 通知线程退出
	CString strDeviceID;
	ThreadInfo_t *pThreadInfo = nullptr;

	POSITION pos = m_oThreadQueueMap.GetStartPos();
	while (pos)
	{
		m_oThreadQueueMap.GetNext(pos, strDeviceID, pThreadInfo);
		pThreadInfo->oEvent.SetEvent();
	}
}

bool CRTSPTransSessionRecMgr::isCurrentSession(CModMessage * pMsg)
{
	CString strDeviceID = pMsg->GetDeviceID();
	ThreadInfo_t *pThreadInfo = nullptr;
	if (m_oThreadQueueMap.Lookup(strDeviceID, pThreadInfo))
	{
		return true;
	}

	return false;
}

void CRTSPTransSessionRecMgr::ProcessOperation(CModMessage * pMsg)
{
	CString strDeviceID = pMsg->GetDeviceID();
	auto eOperateType = pMsg->GetModAction();
	ThreadInfo_t *pThreadInfo = nullptr;

	switch (eOperateType.action_rtsp)
	{
	case mod_op_t::ot_rtsp::record_start:

		// 已经是录像状态
		if (!m_oThreadQueueMap.Lookup(strDeviceID, pThreadInfo))
		{
			// start new record.
			pThreadInfo = new ThreadInfo_t();
			pThreadInfo->strDeviceID = strDeviceID;
			pThreadInfo->oMsgQueue.Push(pMsg);
			m_oThreadQueueMap.SetAt(strDeviceID, pThreadInfo);
			ThreadParam_t thisparamInfo;
			thisparamInfo.pThis = this;
			thisparamInfo.pThreadInfo = pThreadInfo;

			std::thread(PlayThreadWorker, thisparamInfo).detach();
		}
		else
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "录像已经正在进行中,设备ID:%s", strDeviceID);
		}
		break;

	case mod_op_t::ot_rtsp::record_stop:
		// 发送应答指令
		if (m_oThreadQueueMap.Lookup(strDeviceID, pThreadInfo))
		{
			// 停止录像，通知线程退出
			pThreadInfo->oMsgQueue.Push(pMsg);
		}

		break;

	default:
		break;
	}
}

int CRTSPTransSessionRecMgr::Options(CRTSPSession *pSession)
{
	StartRecord(pSession);
	NoticeSDKCom(mod_op_t::ot_sdk::insert_video_record, pSession->pUnifiedMsg); //Notice SDKCom to Update the Site Video DB Record 
	return 0;
}


bool CRTSPTransSessionRecMgr::StartRecord(CRTSPSession *pSession)
{
	RECORD_INFO tRecordInfo;
	CString strCSeq;

	GenerateCSeq(&strCSeq);
	Utils::StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);

	tRecordInfo.url = tRecordInfo.url + pSession->szURL +
		RTSP_STRING_BACKSLASH +
		pSession->szGUID +
		RTSP_STRING_BACKSLASH "0";

	tRecordInfo.sequence = pSession->szCSeq;
	tRecordInfo.version = RTSP_STRING_VERSION;

	time_t tStartTime;
	time(&tStartTime);

	time_t tEndTime = tStartTime + 30;

	// 格式化开始时间
	tm tmStartGM;
	gmtime_s(&tmStartGM, &tStartTime);

	// 格式化结束时间
	tm tmEndGM;
	gmtime_s(&tmEndGM, &tEndTime);

	CString strStartTime;
	//strStartTime.Format("ntp=%lu.0-", tStartTime);
	strStartTime.Format("ntp=%lu.0-", mktime(&tmStartGM));

	CString strEndTime;
	//strEndTime.Format("%lu.0", tEndTime);
	strEndTime.Format("%lu.0", mktime(&tmEndGM));

	tRecordInfo.range = strStartTime + strEndTime;

	CString strRecord;
	RTSPParser().EncodeRecord(tRecordInfo, &strRecord);
	if (strRecord.IsEmpty())
	{
		return false;
	}
	// 更新状态
	pSession->eStatus = rs_record;

	auto b_ret = SendRTSP(pSession, strRecord);
	if (b_ret)
	{

		time_t tmCurrent;
		time(&tmCurrent);
		pSession->tmPoint_Keepalive = tmCurrent + KEEPAVLIE_TIME; //更新保活时间
		pSession->eStatus = rs_recording;
	}
	return b_ret;

}
void CRTSPTransSessionRecMgr::NoticeSDKCom(mod_op_t::u_op_type_t devOperateType, void * pMemoUnit)
{
	if (pMemoUnit)
	{
		auto pMsg = reinterpret_cast<CModMessage*>(pMemoUnit);
		pMsg->SetModAction(devOperateType);
		CRouter::PushMsg(SDKCOM, pMsg);
	}

}
int CRTSPTransSessionRecMgr::TimeOutProc(CRTSPSession *pSession)
{
	time_t tmCurrent;
	time(&tmCurrent);
	// 到达保活时间点
	if (0 < pSession->tmPoint_Keepalive && tmCurrent >= pSession->tmPoint_Keepalive)
	{

		//发送继续录像的通知消息给NVR	
		if (StartRecord(pSession))
		{
			NoticeSDKCom(mod_op_t::ot_sdk::update_video_record, pSession->pUnifiedMsg); //Notice SDKCom to Update the Site Video DB Record  
			pSession->eStatus = rs_recording;
			pSession->tmPoint_Keepalive = tmCurrent + KEEPAVLIE_TIME; //更新下次触发继续录像的时间点
		}

		pSession->tmPoint_Timeout = 0;
	}

	return 0;
}


UINT CRTSPTransSessionRecMgr::ThreadDestroy(CRTSPSession* pSession, ThreadInfo_t *pThreadInfo)
{
	if (pThreadInfo && m_oThreadQueueMap.GetSize() > 0)
	{
		m_oThreadQueueMap.RemoveKey(pThreadInfo->strDeviceID);
	}
	//调用父类
	CRTSPTransMgrBase::ThreadDestroy(pSession, pThreadInfo);
	return 0;
}



int CRTSPTransSessionRecMgr::ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT /* hNetEvent*/)
{

	auto eventType = pSession->GetRtspClient()->GetNetworkEvent();

	if (eventType == CRTSPClient::NETWORK_EVENT::RTSP_READ)   // 处理FD_READ通知消息
	{
		// 取得当前时间
		time(&(pSession->tmPoint_Timeout));
		// 计算下次Record指令的发送时间点
		pSession->tmPoint_Timeout += RECORD_TIME;
	}

	return 0;
}
//对于录像，第一次OPtion中的Record操作发送了开始录像，所以队列里只有一个操作，就是停止录像。
int CRTSPTransSessionRecMgr::ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t* pThreadInfo)
{
	CModMessage *pMsg = nullptr;
	if (pThreadInfo->oMsgQueue.Pop(pMsg, 1000))
	{
		SAFE_FREE_MOD_MSG(pMsg);	
		CLog::Log(RTSPCOM, LL_NORMAL, "停止录像 设备ID:%s", pSession->szToDeviceID);
		pThreadInfo->oEvent.SetEvent();
		return -1;
	}
	return 0;
}

