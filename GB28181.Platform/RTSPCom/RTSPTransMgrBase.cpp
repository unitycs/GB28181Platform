#include "StdAfx.h"
#include "RTSPTransMgrBase.h"
#include "ServerConsole.h"

CRTSPTransMgrBase::CRTSPTransMgrBase(void) :
	m_nCSeq(0), m_oModuleMsgMemMgr(sizeof(CModMessage),
		MEMPOOL_BLOCK_SUM), m_oBodyBufMemMgr(sizeof(CBigFile), MEMPOOL_BLOCK_SUM)
{
	// rmt_response事件发生时，不同状态下的处理
	m_pfnRTSPProc[rmt_response][rs_options] = &CRTSPTransMgrBase::Describe;
	m_pfnRTSPProc[rmt_response][rs_describe] = &CRTSPTransMgrBase::Setup;
	m_pfnRTSPProc[rmt_response][rs_setup] = &CRTSPTransMgrBase::SDPResponse;
	m_pfnRTSPProc[rmt_response][rs_play] = &CRTSPTransMgrBase::StartKeepalive;

	//rmt_setparameter事件发生时，不同状态下的处理
	m_pfnRTSPProc[rmt_setparameter][rs_playing] = &CRTSPTransMgrBase::SetParameter;
	m_pfnRTSPProc[rmt_setparameter][rs_pause] = &CRTSPTransMgrBase::SetParameter;

	PlayThreadWorker = std::bind(&CRTSPTransMgrBase::ThreadProc, this, std::placeholders::_1);
	setGlobalMediaParameter();
	ParserMediaServerIPList();
}

CRTSPTransMgrBase::~CRTSPTransMgrBase(void)
{
	// 通知线程退出
	INT64 calldialogid;
	ThreadInfo_t *pThreadInfo = nullptr;

	POSITION pos = m_oThreadQueueMap.GetStartPos();
	while (pos)
	{
		m_oThreadQueueMap.GetNext(pos, calldialogid, pThreadInfo);
		pThreadInfo->oEvent.SetEvent();
	}
}
void CRTSPTransMgrBase::ParserMediaServerIPList()
{
	int Pos = 0;
	CString strMediaServerMapList = appConf.m_MediaSeverMapList.IPList;
	CString strTmp = strMediaServerMapList.Tokenize(";", Pos);
	while (strTmp.Trim() != _T(""))
	{
		int rank = 0;
		CString strServerIP = strTmp.Tokenize(":", rank);
		CString strMapIP = strTmp.Tokenize(":", rank);
		m_MediaMapIpList.SetAt(strServerIP, strMapIP);
		strTmp = strMediaServerMapList.Tokenize(";", Pos);
	}
}
//for control such as :pause / stop / continue.
bool CRTSPTransMgrBase::isCurrentSession(CModMessage * pMsg)
{
	INT64 nCallDialogID = pMsg->GetCallDialogID();
	ThreadInfo_t *pThreadInfo = nullptr;
	if (m_oThreadQueueMap.Lookup(nCallDialogID, pThreadInfo))
	{
		return true;
	}

	return false;
}

void CRTSPTransMgrBase::GenerateCSeq(CString *pstrCSeq)
{
	m_oCS.Lock();
	m_nCSeq++;
	if (0 > m_nCSeq)
		m_nCSeq = 0;
	m_oCS.Unlock();

	pstrCSeq->Format("%d", m_nCSeq);
}

bool CRTSPTransMgrBase::SendRTSP(CRTSPSession *pSession, const char *pszData)
{
	CLog::Log(RTSPCOM, LL_DEBUG, "SEND RTSP Message\r\n%s", pszData);

	//如果rtps不需要重连，表明新建的链接依然存在
	if (pSession->GetRtspClient()->CanSend())
		return pSession->GetRtspClient()->SendData(pszData);

	//重新创建tcp客户端，连接NVR
	auto b_ret = pSession->GetRtspClient()->Reconnect();
	if (b_ret)
	{
		return pSession->GetRtspClient()->SendData(pszData);
	}

	CLog::Log(RTSPCOM, LL_DEBUG, "SEND RTSP Message Failed : Connect NVR Failed! \r\n%s", pszData);
	return false;
}

int CRTSPTransMgrBase::ProcessNetworkEvent(CRTSPSession* /* pSession */, WSAEVENT /* hNetEvent */)
{
	return 0;
}

int CRTSPTransMgrBase::ProcessMsgQueueEvent(CRTSPSession * /* pSession */, ThreadInfo_t * /* pThreadInfo*/)
{
	return 0;
}

UINT CRTSPTransMgrBase::ThreadProc(ThreadParam_t  paramInfo)
{
	auto pThreadInfo = static_cast<ThreadInfo_t*>(paramInfo.pThreadInfo);
	CRTSPSession *pSession = nullptr;
	CModMessage *pMsg = nullptr;

	if (pThreadInfo->oMsgQueue.Pop(pMsg, 1000))
	{
		// 删除此pMsg指向的内存
		// 应答消息需要的内存，重新分配
		//SAFE_FREE_UNI_MSG(pMsg);
		pSession = m_oRTSPSessionBuilder.Create(pMsg);
		if (pSession)
		{
			// 发送Options指令
			Options(pSession);
			CLog::Log(RTSPCOM, LL_NORMAL, "开始视频会话 CID-DID:%d", pSession->nCallDialogID);
			ThreadEventLoop(pSession, pThreadInfo);
		}
	}

	ThreadDestroy(pSession, pThreadInfo);
	return 0;
}

UINT CRTSPTransMgrBase::ThreadEventLoop(CRTSPSession* pSession, ThreadInfo_t *pThreadInfo)
{
	HANDLE hEvents[3];
	hEvents[0] = pSession->GetRtspClient()->GetNetworkEventHandle();  // 可能获取不到，获取不到会得到nullptr;
	hEvents[1] = pThreadInfo->oMsgQueue.m_hObject;
	hEvents[2] = pThreadInfo->oEvent.m_hObject;

	while (true)
	{
		if (0 != TimeOutProc(pSession))
			break;

		// 监听全部模块
		DWORD dwWaitObject = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, 1000);

		auto exitThread = false;
		switch (dwWaitObject)
		{
		case	WAIT_OBJECT_0:
			// 处理RTSP的网络事件,解析并处理RTSP过程事件option/setup等
			if (0 > ProcessNetworkEvent(pSession, hEvents[0]))
				exitThread = true;
			break;
		case	WAIT_OBJECT_0 + 1:
			//来自SIP的消队列,解析码流的play/stop等控制消息
			ProcessMsgQueueEvent(pSession, pThreadInfo);
			break;
		case  WAIT_FAILED:
		case WAIT_OBJECT_0 + 2:
			exitThread = true;
			break;
		default: //		such as WAIT_TIMEOUT we need to continue:
			break;
		}
		if (exitThread) break;
	}

	return 0;
}

UINT CRTSPTransMgrBase::ThreadDestroy(CRTSPSession* pSession, ThreadInfo_t *pThreadInfo)
{

	if (pThreadInfo)
	{
		if (m_oThreadQueueMap.GetSize() > 0)
		{
			m_oThreadQueueMap.RemoveKey(pThreadInfo->nCallDialogID);
		}
		m_oQueueMapLock.Lock();
		pThreadInfo->ClearMsgQueue();
		CLog::Log(RTSPCOM, LL_NORMAL, "%s 退出视频会话线程 CID-DID:%d delete pThreadInfo getlock\r\n", __FUNCTION__, pSession->nCallDialogID);
		delete pThreadInfo;
		m_oQueueMapLock.Unlock();
	}
	CLog::Log(RTSPCOM, LL_NORMAL, " %s 退出视频会话线程 CID-DID:%d delete pThreadInfo getlock", __FUNCTION__, pSession->nCallDialogID);
	m_oRTSPSessionBuilder.Destroy(pSession);
	return 0;
}

int CRTSPTransMgrBase::StartKeepalive(CRTSPSession *pSession)
{

	// 发送保活包
	GetParameter(pSession);

	// 计时下次保活的时间点
	time_t tmCurrent;
	time(&tmCurrent);

	pSession->tmPoint_Timeout = tmCurrent + RES_TIMEOUT;
	pSession->tmPoint_Keepalive = 0;
	pSession->eStatus = rs_playing;
	return 0;
}

int CRTSPTransMgrBase::TimeOutProc(CRTSPSession *pSession)
{
	time_t tmCurrent;
	time(&tmCurrent);

	//如果当前流程，还未到播放状态，则不需要处理保活事件
	if (pSession->eStatus < rs_playing)
	{
		return 0;
	}

	// 保活应答已超时
	if (0 < pSession->tmPoint_Timeout && tmCurrent > pSession->tmPoint_Timeout)
	{
		CLog::Log(RTSPCOM, LL_NORMAL, "%s Seq %s 应答超时，删除Seeesion", __FUNCTION__, pSession->szCSeq);
		return 1;
	}

	// 到达保活时间点
	if (0 < pSession->tmPoint_Keepalive && tmCurrent >= pSession->tmPoint_Keepalive)
	{
		// 发送保活包
		StartKeepalive(pSession);
	}

	return 0;
}

int CRTSPTransMgrBase::GetParameter(CRTSPSession *pSession)
{
	GET_PARAMETER_INFO tGetParamInfo;
	CString strCSeq;
	tGetParamInfo.url = pSession->szURL;

	GenerateCSeq(&strCSeq);
	Utils::StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);
	tGetParamInfo.sequence = pSession->szCSeq;

	tGetParamInfo.session = pSession->szSessionID;

	CString strGetParameter;

	RTSPParser().EncodeGetParameter(tGetParamInfo, &strGetParameter);

	// 发送RTSP消息
	SendRTSP(pSession, strGetParameter);
	time(&(pSession->tmPoint_Timeout));
	pSession->tmPoint_Timeout += RES_TIMEOUT;

	return 0;
}

int CRTSPTransMgrBase::SetParameter(CRTSPSession *pSession)
{
	static int nSN = 0;
	CString strData;
	CBodyBuilder fileFinished;

	// 返回RTSP应答
	SendRTSP(pSession, strData);
	CModMessage * pUnifiedMsg = nullptr;
	CBigFile		* pRes = nullptr;
	m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pUnifiedMsg));
	m_oBodyBufMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pRes));

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::video_finished);
	pUnifiedMsg->SetCallDialogID(pSession->nCallDialogID);

	CString strSN;
	strSN.Format("%d", nSN++);
	fileFinished.CreateMediaStatusBody(pRes->GetBuffer(), pRes->GetBufferLen(), strSN, pSession->szToDeviceID);

	pUnifiedMsg->SetNotifyData(pRes);
	// 通知SIPCom播放已停止
	CLog::Log(RTSPCOM, LL_NORMAL, "%s ot_sip_video_finished xml = %s\r\n", __FUNCTION__, pRes->GetBuffer());
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	return 0;
}

void CRTSPTransMgrBase::setGlobalMediaParameter()
{
	m_strPassword = appConf.m_Current.str_Password;
	m_nDownLoadRate = appConf.m_DownLoadConfig.n_Rate;

	if (!appConf.m_Firewall.str_IP.IsEmpty())
	{
		strFirewall_MediaIP = appConf.m_Firewall.str_IP;
	}
}

int CRTSPTransMgrBase::Options(CRTSPSession *pSession)
{
	OPTION_INFO tOptionInfo;
	CString strCSeq;

	GenerateCSeq(&strCSeq);
	//StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);
	time(&(pSession->tmPoint_Timeout));
	pSession->tmPoint_Timeout += RES_TIMEOUT;

	tOptionInfo.url = pSession->szURL;
	tOptionInfo.url = tOptionInfo.url + RTSP_STRING_QUESTIONMARK
		RTSP_STRING_STREAMID
		RTSP_STRING_EQUALSSIGN +
		pSession->szGUID;

	tOptionInfo.sequence = strCSeq;
	tOptionInfo.version = RTSP_STRING_VERSION;
	CString strOptions;
	RTSPParser().EncodeOption(tOptionInfo, &strOptions);
	SendRTSP(pSession, strOptions);
	// 更新状态
	pSession->eStatus = rs_options;
	CLog::Log(RTSPCOM, LL_NORMAL, "%s url = %s", __FUNCTION__, tOptionInfo.url);
	return 0;
}

int CRTSPTransMgrBase::Describe(CRTSPSession *pSession)
{
	DESCRIBE_INFO tDescribeInfo;
	CString strCSeq;

	GenerateCSeq(&strCSeq);
	//StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);
	time(&(pSession->tmPoint_Timeout));
	pSession->tmPoint_Timeout += RES_TIMEOUT;

	tDescribeInfo.sequence = strCSeq;
	tDescribeInfo.url = pSession->szURL;
	tDescribeInfo.url = tDescribeInfo.url + RTSP_STRING_QUESTIONMARK
		RTSP_STRING_STREAMID
		RTSP_STRING_EQUALSSIGN +
		pSession->szGUID;

	tDescribeInfo.user_agent = RTSP_STRING_USERAGENT;

	CString strDescribe;
	RTSPParser().EncodeDescribe(tDescribeInfo, &strDescribe);
	SendRTSP(pSession, strDescribe);

	// 更新状态
	pSession->eStatus = rs_describe;

	return 0;
}

int CRTSPTransMgrBase::SDPResponse(CRTSPSession *pSession)
{
	// 协商成功已经分配了Sessio ID
	if (0 != pSession->szSessionID[0])
	{
		CBigFile		*pSDPFile = nullptr;
		CModMessage *pUnifiedMsg = nullptr;
		CString           strFileSize;
		// 生成消息
		m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pUnifiedMsg));
		m_oBodyBufMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pSDPFile));

		pUnifiedMsg->SetTID(pSession->nTID);

		CString strTransport;
		CString orginStrSSrc;
		pSession->pRTSPMsg->GetFieldValue(RTSP_TYPE_TRANSPORT, strTransport);
		orginStrSSrc = strTransport;
		// 提前媒体发送端的端口
		int nPos = strTransport.Find("server_port=");
		if (0 < nPos)
		{
			strTransport = strTransport.Right(strTransport.GetLength() - nPos - 12); \
				nPos = strTransport.Find("-");
			if (0 < nPos)
			{
				strTransport = strTransport.Left(nPos);
			}
		}

		//获取ssrc 做为y
		nPos = orginStrSSrc.Find("ssrc=");
		CString strSsrc;
		if (nPos > 0)
		{
			strSsrc = orginStrSSrc.Right(orginStrSSrc.GetLength() - nPos - 5);
			int endPos = strSsrc.Find(";");
			if (endPos > 0)
			{
				strSsrc = strSsrc.Left(endPos);
			}
		}

		nPos = orginStrSSrc.Find("filesize=");

		if (nPos > 0)
		{
			strFileSize = orginStrSSrc.Right(orginStrSSrc.GetLength() - nPos - 9);
			int endPos = strFileSize.Find(";");
			if (endPos > 0)
			{
				strFileSize = strFileSize.Left(endPos);
			}
		}

		// 取得NVRIP
		CString strNVRIP = pSession->GetRtspClient()->GetClientIP();
		if (m_MediaMapIpList[strNVRIP] != "")
		{
			strNVRIP = m_MediaMapIpList[strNVRIP];
		}
		CDevSDP oSDP;
		oSDP.SetSSRC(strSsrc);
		oSDP.SetTransType(pSession->m_transProto);
		oSDP.SetMediaType(0);
		if (pSession->m_mediaDataType == md_audio)
		{
			oSDP.SetMediaType(1);
		}

		if (pSession->eMediaActionT == mt_download)
		{
			oSDP.SetFileSize(const_cast<char*>(static_cast<LPCTSTR>(strFileSize)));
		}

		oSDP.SetDeviceID(pSession->szToDeviceID);
		oSDP.SetMediaIP(strNVRIP.GetString());
		oSDP.SetMediaPort(strTransport);
		oSDP.SetSessionName("HUS NVR");
		oSDP.SetMediaTime(pSession->szStartTime, pSession->szEndTime);
		oSDP.SetPassword(m_strPassword);
		oSDP.GetCatalogBodyContent(pSDPFile->GetBuffer(), pSDPFile->GetBufferLen(), nullptr, nullptr);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::invite_reponse);
		pUnifiedMsg->SetPlayData(reinterpret_cast<void*>(pSDPFile));

		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}

	return 0;
}

int CRTSPTransMgrBase::Setup(CRTSPSession *pSession)
{
	if (0 != pSession->szSessionID[0])
	{
		return Play(pSession);
	}
	return 0;
}

int CRTSPTransMgrBase::Teardown(CRTSPSession *pSession)
{
	pSession->eStatus = rs_null;

	TEARDOWN_INFO tTeardownInfo;
	CString strCSeq;

	GenerateCSeq(&strCSeq);
	//StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);

	tTeardownInfo.url = pSession->szURL;
	tTeardownInfo.sequence = strCSeq;
	tTeardownInfo.session = pSession->szSessionID;

	CString strTeardown;

	RTSPParser().EncodeTeardown(tTeardownInfo, &strTeardown);

	// 发送RTSP消息
	SendRTSP(pSession, strTeardown);

	CLog::Log(RTSPCOM, LL_NORMAL, "%s strTearDown = %s\r\n", __FUNCTION__, strTeardown);
	pSession->Free();

	return 0;
}

int CRTSPTransMgrBase::Pause(CRTSPSession *pSession)
{
	PAUSE_INFO tPauseInfo;
	CString strCSeq;
	GenerateCSeq(&strCSeq);
	// 不更新序列号
	//StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);
	tPauseInfo.sequence = strCSeq;
	tPauseInfo.session = pSession->szSessionID;
	tPauseInfo.url = pSession->szURL;

	CString strPause;
	RTSPParser().EncodePause(tPauseInfo, &strPause);

	// 发送RTSP消息
	SendRTSP(pSession, strPause);

	// 更新状态
	pSession->eStatus = rs_pause;

	return 0;
}

int CRTSPTransMgrBase::PlayCtrl(CRTSPSession *pSession)
{
	CString strPlay;
	// 生成RTSP数据
	PLAY_INFO tPlayInfo;

	// 设置序列号
	GenerateCSeq(&tPlayInfo.sequence);
	Utils::StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, tPlayInfo.sequence);
	tPlayInfo.url = pSession->szURL;

	// 时间不为空
	if ('\0' != pSession->szCurTime[0])
	{
		// 结束Range中的开始时间
		char * pEquals = strstr(pSession->szCurTime, "=");
		if (pEquals)
		{
			char *pBars = strstr(pEquals, "-");
			if (pBars)
			{
				pBars[0] = '\0';
			}
			int nNPTStart = atoi(pEquals + 1);
			if (_stricmp(pEquals + 1, "now") == 0)
			{
				tPlayInfo.range = "";
			}
			else if (0 <= nNPTStart)
			{
				__time64_t tmCurrent = pSession->tmStart + nNPTStart;

				// 绝对时间，1970年开始的秒数
				CString strRang;
				tm tmInfo;
				_gmtime64_s(&tmInfo, &tmCurrent);
				strRang.Format("clock=%d%02d%02dT%02d%02d%02dZ-",
					tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
					tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

				tPlayInfo.range = strRang;
			}
		}
	}

	tPlayInfo.scale = pSession->szScale;
	tPlayInfo.user_agent = "HUS-GB-Agent";
	tPlayInfo.session = pSession->szSessionID;
	//   tPlayInfo.speed = pSession->m_szDownLoadSpeed;

	RTSPParser().EncodePlay(tPlayInfo, &strPlay);

	// 发送RTSP消息
	SendRTSP(pSession, strPlay);

	CLog::Log(RTSPCOM, LL_NORMAL, "%s strPlay = %s\r\n", __FUNCTION__, strPlay);

	return 0;
}

int CRTSPTransMgrBase::Play(CRTSPSession *pSession)
{
	Utils::StringCpy_s(pSession->szScale, SCALE_BUF_LEN, "1.0");
	pSession->szCurTime[0] = '\0';

	time(&(pSession->tmPoint_Timeout));
	pSession->tmPoint_Timeout += RES_TIMEOUT;

	// 下载操作
	if (mt_download == pSession->eMediaActionT)
	{
		auto nDLSpeed = atof(pSession->m_szDownLoadSpeed);
		if (nDLSpeed > 0 && (8 - nDLSpeed) >= 0)
		{
			// 普通下载速度为播放速度的4倍(业界规定)
			// 即：8倍下载速度，相对于普通播放速度为 8×4 = 32倍
			auto nScaleSpeed = nDLSpeed * m_nDownLoadRate;
			sprintf_s(pSession->szScale, "%.2f", nScaleSpeed);
		}
		else
			Utils::StringCpy_s(pSession->szScale, SCALE_BUF_LEN, _T("4.0"));
	}

	int nRet = PlayCtrl(pSession);

	// 更新状态
	pSession->eStatus = rs_play;

	//清空Seq，只有在保活后的Seq才是有效的
	memset(pSession->szCSeq, 0, CSEQ_BUF_LEN);
	return nRet;
}

// 取得Video的TrackID
int CRTSPTransMgrBase::GetVideoTrack(RTSP_Unknown_Msg &tRTSPMsg, CString *pstrTrackID)
{
	*pstrTrackID = "trackID=0";
	int i = -1;
	auto itrMedia = tRTSPMsg.GetSDPStack().media_list.begin();
	for (itrMedia; itrMedia != tRTSPMsg.GetSDPStack().media_list.end(); ++itrMedia)
	{
		i++;
		if ("video" == (*itrMedia).media)
		{
			break;
		}
	}
	auto itrAttribute = tRTSPMsg.GetSDPStack().attribute_list.begin();
	for (itrAttribute; itrAttribute != tRTSPMsg.GetSDPStack().attribute_list.end(); ++itrAttribute)
	{
		if ("control" == (*itrAttribute).attribute)
		{
			if (0 == i)
			{
				*pstrTrackID = (*itrAttribute).value;
				return 0;
			}
			i--;
		}
	}

	return -1;
}

int CRTSPTransMgrBase::GetTCPMediaPort(RTSP_Unknown_Msg &tRTSPMsg, CString *pstrMediaPort)
{
	auto& media_ItemList = tRTSPMsg.GetSDPStack().media_list;
	for (auto& mediaItem : media_ItemList)
	{
		if (mediaItem.media.MakeLower().Find(_T("video")) >= 0)
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "find MediaPort in NVR SDP : %s \r\n", mediaItem.port);
			*pstrMediaPort = mediaItem.port;
			return 0;
		}
	}

	return -1;
}

int CRTSPTransMgrBase::Getfilesize(RTSP_Unknown_Msg &tRTSPMsg, CString *pstrFileSize)
{
	auto itrAttribute = tRTSPMsg.GetSDPStack().attribute_list.begin();
	for (itrAttribute; itrAttribute != tRTSPMsg.GetSDPStack().attribute_list.end(); ++itrAttribute)
	{
		if ("filesize" == (*itrAttribute).attribute)
		{
			*pstrFileSize = (*itrAttribute).value;
			return 0;
		}
	}

	return -1;
}