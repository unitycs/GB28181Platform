#include "StdAfx.h"
#include "RTSPTransSessionMgr.h"

void CRTSPTransSessionMgr::ProcessOperation(CModMessage * pMsg)
{
	INT64 nCallDialogID = 0;
	auto eOperateType = pMsg->GetModAction();
	ThreadInfo_t *pThreadInfo = nullptr;
	CLog::Log(RTSPCOM, LL_NORMAL, "处理会话内消息 code:%d", eOperateType);
	switch (eOperateType.action_rtsp)
	{
	case mod_op_t::ot_rtsp::play_with_sdp:
	{
		nCallDialogID = pMsg->GetCallDialogID();
		bool realPlayFlag = this->isRealPlay(pMsg);
		CLog::Log(RTSPCOM, LL_NORMAL, "%s subject= %s  ot_rtsp_play_with_sdp realPlayFlag = %d\r\n", __FUNCTION__, pMsg->GetSubject(), realPlayFlag);
		if (m_oThreadQueueMap.Lookup(nCallDialogID, pThreadInfo)) //表明当前已经在播放了。
		{
			// 通知播放控制的线程退出,重新播放
			pThreadInfo->oEvent.SetEvent();
			CLog::Log(RTSPCOM, LL_NORMAL, "%s subject= %s  play alredy exist! will close it and  re-play \r\n", __FUNCTION__, pMsg->GetSubject());
		}
		//开始新的播放线程
		pThreadInfo = new ThreadInfo_t();
		pThreadInfo->nCallDialogID = nCallDialogID;
		pThreadInfo->realPlayFlag = realPlayFlag;
		pThreadInfo->subject = pMsg->GetSubject();
		pThreadInfo->oMsgQueue.Push(pMsg);

		m_oThreadQueueMap.SetAt(nCallDialogID, pThreadInfo);
		ThreadParam_t thisparamInfo;
		thisparamInfo.pThis = this;
		thisparamInfo.nCallDialogID = nCallDialogID;
		thisparamInfo.pThreadInfo = pThreadInfo;

		std::thread(PlayThreadWorker, thisparamInfo).detach();
	}
	break;
	// 其它处理
	case mod_op_t::ot_rtsp::stop_all_play:
	{
		CLog::Log(RTSPCOM, LL_NORMAL, "%s ot_rtsp_stop_all_play getlock\r\n", __FUNCTION__);
		m_oQueueMapLock.Lock();
		POSITION pos = m_oThreadQueueMap.GetStartPos();
		INT64 key;
		while (pos)
		{
			pThreadInfo = nullptr;
			m_oThreadQueueMap.GetNext(pos, key, pThreadInfo);
			if (pThreadInfo)
			{
				CModMessage *pNewMsg = nullptr;
				m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pNewMsg));
				pNewMsg->SetModAction(mod_op_t::ot_rtsp::stop_all_play);

				pThreadInfo->oMsgQueue.Push(pNewMsg);
			}
		}

		CLog::Log(RTSPCOM, LL_NORMAL, "%s ot_rtsp_stop_all_play getlock\r\n", __FUNCTION__);
		m_oQueueMapLock.Unlock();
	}

	case mod_op_t::ot_rtsp::play_ctrl_start:
	case mod_op_t::ot_rtsp::play_ctrl_stop:
	case mod_op_t::ot_rtsp::play_ctrl:
		nCallDialogID = pMsg->GetCallDialogID();
		//添加的时候不能删除，保证同步
		m_oQueueMapLock.Lock();
		if (m_oThreadQueueMap.Lookup(nCallDialogID, pThreadInfo))
		{
			pThreadInfo->oMsgQueue.Push(pMsg);
		}
		// 没有此会话
		else
		{
			//旧逻辑，用来处理解码器,目前解码器业务由SDKCOM模块出来。
			if (mod_op_t::ot_rtsp::play_ctrl_stop == eOperateType.action_rtsp)
			{
				pMsg->SetModAction(mod_op_t::ot_sdk::decoder_bye);
				CRouter::PushMsg(SDKCOM, pMsg);
			}
			else
			{
				SAFE_FREE_MOD_MSG(pMsg);
			}
		}
		m_oQueueMapLock.Unlock();
		break;

	default:
		SAFE_FREE_MOD_MSG(pMsg);
		break;
	}
}

#pragma region RTSPSession Operation With NVR.

int CRTSPTransSessionMgr::Describe(CRTSPSession *pSession)
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
		pSession->szGUID +
		RTSP_STRING_AND
		RTSP_STRING_SSRC
		RTSP_STRING_EQUALSSIGN +
		pSession->szSSRC
		;

	tDescribeInfo.user_agent = RTSP_STRING_USERAGENT;
	// 添加录像回放扩展头
//	if(mt_replay == pSession->eMediaType)
	if (mt_replay == pSession->eMediaActionT || mt_download == pSession->eMediaActionT)
	{
		// 绝对时间，1970年开始的秒数
		CString strRang;
		tm tmInfo;
		//    tmInfo = *localtime(&pSession->tmStart);
		_gmtime64_s(&tmInfo, &(pSession->tmStart));
		strRang.Format("%d%02d%02dT%02d%02d%02dZ",
			tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
			tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

		tDescribeInfo.url = tDescribeInfo.url +
			RTSP_STRING_AND
			RTSP_STRING_PLAYTIME
			RTSP_STRING_EQUALSSIGN +
			strRang;

		tm tmEndInfo;
		//    tmEndInfo = *localtime(&pSession->tmEnd);
		_gmtime64_s(&tmEndInfo, &(pSession->tmEnd));
		CString strEndTime;
		strEndTime.Format("%d%02d%02dT%02d%02d%02dZ",
			tmEndInfo.tm_year + 1900, tmEndInfo.tm_mon + 1, tmEndInfo.tm_mday,
			tmEndInfo.tm_hour, tmEndInfo.tm_min, tmEndInfo.tm_sec);

		tDescribeInfo.url = tDescribeInfo.url +
			RTSP_STRING_AND
			RTSP_STRING_ENDTIME
			RTSP_STRING_EQUALSSIGN +
			strEndTime;

		CLog::Log(RTSPCOM, LL_NORMAL, "[HON] RTSP URL:%s", tDescribeInfo.url);

		//      tDescribeInfo.require = RTSP_STRING_REQUIRE;

		tDescribeInfo.require = RTSP_STRING_REQUIRE;
	}

	CString strDescribe;
	RTSPParser().EncodeDescribe(tDescribeInfo, &strDescribe);
	SendRTSP(pSession, strDescribe);

	// 更新状态
	pSession->eStatus = rs_describe;

	return 0;
}

int CRTSPTransSessionMgr::Setup(CRTSPSession *pSession)
{
	SETUP_INFO tSetupInfo;
	CString strCSeq;
	CString strTrackID;
	CString strFileSize;
	CString strMediaPort;

	GetVideoTrack(*pSession->pRTSPMsg, &strTrackID);
	Getfilesize(*pSession->pRTSPMsg, &strFileSize);
	GetTCPMediaPort(*pSession->pRTSPMsg, &strMediaPort);
	if (strMediaPort.GetLength() > 3)
	{
		Utils::StringCpy_s(pSession->m_szTCPMediaPort, 10, strMediaPort);
		CLog::Log(RTSPCOM, LL_NORMAL, "%s  Get MediaPort in NVR SDP: %s", __FUNCTION__, strMediaPort);
	}
	memset(pSession->szFileSize, 0, sizeof(pSession->szFileSize));
	if (strFileSize.GetLength() > 0)
	{
		strncpy_s(pSession->szFileSize, strFileSize, sizeof(pSession->szFileSize) - 1);
	}
	else
	{
		//    strcpy(pSession->szFileSize, "10000");
	}

	pSession->pRTSPMsg->GetFieldValue(RTSP_TYPE_CONTENT_BASE, pSession->szURL, URL_BUF_LEN);
	// 生成URL
	tSetupInfo.url = pSession->szURL;
	tSetupInfo.url = tSetupInfo.url +
		strTrackID;

	// 生成Transport
	CString strTransport = RTSP_STRING_UDPTRANSPORT
		RTSP_STRING_SEMICOLON
		RTSP_STRING_UNICAST
		RTSP_STRING_SEMICOLON
		RTSP_STRING_DESTINATION
		RTSP_STRING_EQUALSSIGN;

	if (pSession->m_transProto == tt_tcp)
	{
		strTransport = RTSP_STRING_TCPTRANSPORT
			RTSP_STRING_SEMICOLON
			RTSP_STRING_UNICAST
			RTSP_STRING_SEMICOLON
			RTSP_STRING_DESTINATION
			RTSP_STRING_EQUALSSIGN;
	}

	strTransport += pSession->szClientIP;

	strTransport += RTSP_STRING_SEMICOLON
		RTSP_STRING_CLIENTPORT
		RTSP_STRING_EQUALSSIGN;

	tSetupInfo.transport = strTransport + pSession->szClientPort;

	int nRTCPPort = atoi(pSession->szClientPort);
	nRTCPPort++;

	CString strRTCPPort;
	strRTCPPort.Format("%d", nRTCPPort);
	tSetupInfo.transport = tSetupInfo.transport + RTSP_STRING_BARS + strRTCPPort;

	if (strlen(pSession->m_szDownLoadSpeed) > 0)
	{
		char downLoadSpeedBuf[MAX_PATH] = { 0 };
		sprintf_s(downLoadSpeedBuf, ";downloadspeed=%s", pSession->m_szDownLoadSpeed);
		tSetupInfo.transport += downLoadSpeedBuf;
	}

	// 生成CSeq
	GenerateCSeq(&strCSeq);
	//StringCpy_s(pSession->szCSeq, CSEQ_BUF_LEN, strCSeq);
	time(&(pSession->tmPoint_Timeout));
	pSession->tmPoint_Timeout += RES_TIMEOUT;

	tSetupInfo.sequence = strCSeq;

	// 发送RTSP消息
	CString strSetup;
	RTSPParser().EncodeSetup(tSetupInfo, &strSetup);
	SendRTSP(pSession, strSetup);

	// 更新当前状态
	pSession->eStatus = rs_setup;
	CLog::Log(RTSPCOM, LL_NORMAL, "%s url = %s", __FUNCTION__, strSetup);
	return 0;
}

int CRTSPTransSessionMgr::SDPResponse(CRTSPSession *pSession)
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
		else
		{
			strTransport = pSession->m_szTCPMediaPort;
		}

		// CString sdp_string;
		//	auto ssrc=	pSession->describeSDP->ssrc_info;

			//获取ssrc 做为y
		nPos = orginStrSSrc.Find("ssrc=");
		//nPos = sdp_string.Find("y=");
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
		oSDP.SetSSRC(pSession->szSSRC);
		oSDP.SetTransType(pSession->m_transProto);
		oSDP.SetMediaType(0);
		if (pSession->m_mediaDataType == md_audio)
		{
			oSDP.SetMediaType(1);
		}

		if (pSession->eMediaActionT == mt_download)
		{
			oSDP.SetFileSize(pSession->szFileSize);
			//  oSDP.SetFileSize((char*)(LPCTSTR)strFileSize);
		}
		oSDP.SetDeviceID(pSession->szToDeviceID);
		if (!strFirewall_MediaIP.IsEmpty())
		{
			strNVRIP = strFirewall_MediaIP;
		}
		oSDP.SetMediaIP(strNVRIP.GetString());
		oSDP.SetMediaPort(strTransport);
		//	oSDP.SetSessionName("HUS NVR");
		oSDP.SetSessionName("Play");
		if (pSession->eMediaActionT == mt_download)
		{
			oSDP.SetSessionName("Download");
		}
		if (pSession->eMediaActionT == mt_replay)
		{
			oSDP.SetSessionName("Playback");
		}
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

int CRTSPTransSessionMgr::SendBye(CRTSPSession *pSession)
{
	//  static int nSN = 0;
	CString strData;
	//    CBodyBuilder fileFinished;

	CModMessage * pUnifiedMsg = nullptr;
	CBigFile		* pRes = nullptr;
	m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pUnifiedMsg));
	m_oBodyBufMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pRes));

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::video_excepion_bye);
	pUnifiedMsg->SetCallDialogID(pSession->nCallDialogID);

	CLog::Log(RTSPCOM, LL_NORMAL, "%s\r\n", __FUNCTION__);
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	return 0;
}

int CRTSPTransSessionMgr::ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT /* hNetEvent */)
{
	ASSERT(pSession != nullptr);
	int nResult = 0;
	char szBuf[BIG_FILE_BUF_LEN] = { 0 };
	int receivelenth = 0;
	auto eRtspEvent = pSession->GetRtspClient()->GetNetworkEvent(szBuf, BIG_FILE_BUF_LEN, receivelenth);

	if (eRtspEvent != CRTSPClient::NETWORK_EVENT::RTSP_READ) return 1;

	CLog::Log(RTSPCOM, LL_NORMAL, "%s RECV RTSP Message nLen = %d szBuf = \r\n %s\r\n", __FUNCTION__, receivelenth, szBuf);

	RTSP_Unknown_Msg rtspUnknownMsg;
	// 解析未知类型的RTSP消息
	auto eMsgType = RTSPParser().DecodeUnknowMsg(szBuf, rtspUnknownMsg);

	if (rmt_unknown != eMsgType)
	{
		if (0 != strncmp("200", rtspUnknownMsg.GetStatus(), 3))
		{
			if (eMsgType != rmt_setparameter)
				// 添加错误处理
				// 删除对应Session
			{
				m_oThreadQueueMap.RemoveKey(pSession->nCallDialogID);
				return 0;
			}
		}

		if (eMsgType != rmt_setparameter)// 取得序列号
		{
			CString strCSeq;
			rtspUnknownMsg.GetFieldValue("CSeq", strCSeq);

			// 根据序列号判断是否是保活应答数据包
			if (strCSeq == pSession->szCSeq)
			{
				// 停止应答超时计时
				pSession->tmPoint_Timeout = 0;

				// 开始下次保活计时
				time(&(pSession->tmPoint_Keepalive));
				pSession->tmPoint_Keepalive += KEEPAVLIE_TIME;
			}
		}

		// 取得SessionID
		rtspUnknownMsg.GetFieldValue(RTSP_TYPE_SESSION, pSession->szSessionID, SESSION_BUF_LEN);
		pSession->pRTSPMsg = &rtspUnknownMsg;
		if (this->m_pfnRTSPProc[eMsgType][pSession->eStatus])
			nResult = (this->*(m_pfnRTSPProc[eMsgType][pSession->eStatus]))(pSession);
	}
	return nResult;
}

int CRTSPTransSessionMgr::ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t* pThreadInfo)
{
	CModMessage *pMsg = nullptr;
	DWORD dwWaitTm = MAKEWORD(1000, 0);
	if (pThreadInfo->oMsgQueue.Pop(pMsg, dwWaitTm))
	{
		switch (pMsg->GetModAction().action_rtsp)
		{
		case mod_op_t::ot_rtsp::play_ctrl_start:

			if (rs_setup == pSession->eStatus)
			{
				//StringCpy_s(pSession->szGUID, GUID_BUF_LEN, pUnifiedMsg->GetDeviceID());
				// 开始视频播放的信令交互
				Play(pSession);
				CLog::Log(RTSPCOM, LL_NORMAL, "开始播放 CID-DID:%d", pSession->nCallDialogID);
			}
			break;

			// 停止播放
		case mod_op_t::ot_rtsp::play_ctrl_stop:

			CLog::Log(RTSPCOM, LL_NORMAL, "开始停止视频会话 ot_rtsp_play_stop CID-DID:%d pSession->eStatus = %d\r\n", pSession->nCallDialogID, pSession->eStatus);
			// 没有SDP文件直接发送SetParameter
			if (pSession->eStatus != rs_null)
			{
				CLog::Log(RTSPCOM, LL_NORMAL, "send teardown停止视频会话 CID-DID:%d", pSession->nCallDialogID);
				Teardown(pSession);
				pSession->eStatus = rs_null;
				//发送线程退出通知
				pThreadInfo->oEvent.SetEvent();
			}

			CLog::Log(RTSPCOM, LL_NORMAL, "停止视频会话 CID-DID:%d", pSession->nCallDialogID);
			break;

		case mod_op_t::ot_rtsp::stop_all_play:
		{
			if (pSession->eStatus == rs_null)
			{
				CLog::Log(RTSPCOM, LL_NORMAL, "%s pSession->eStatus == rs_null  ot_rtsp_stop_all_play 停止视频会话CID-DID:%d\r\n", __FUNCTION__, pSession->nCallDialogID);
			}
			else
			{
				SendBye(pSession);
				Teardown(pSession);
				pSession->eStatus = rs_null;
				pThreadInfo->oEvent.SetEvent();
			}

			CLog::Log(RTSPCOM, LL_NORMAL, "%s   停止视频会话 ot_rtsp_stop_all_play CID-DID:%d", __FUNCTION__, pSession->nCallDialogID);
			break;
		}

		// 播放控制
		case mod_op_t::ot_rtsp::play_ctrl:
		{
			// 解析RTSP文件
			RTSP_Unknown_Msg rtspUnknownMsg;
			CBigFile *pSDP = reinterpret_cast<CBigFile*>(pMsg->GetPlayData());
			CString buff = CString(pSDP->GetBuffer());
			RTSPParser().DecodeUnknowMsg(CString(pSDP->GetBuffer()), rtspUnknownMsg);

			// 暂停
			if (rtspUnknownMsg.GetMethod() == RTSP_METHOD_PAUSE)
			{
				Pause(pSession);
				CLog::Log(RTSPCOM, LL_NORMAL, "暂停 CID-DID:%d", pSession->nCallDialogID);
			}
			// 播放
			else if (rtspUnknownMsg.GetMethod() == RTSP_METHOD_PLAY)
			{
				// 设置播放速度
				rtspUnknownMsg.GetFieldValue(RTSP_TYPE_SCALE, pSession->szScale, SCALE_BUF_LEN);

				// 设置播放的前期时间
				rtspUnknownMsg.GetFieldValue(RTSP_TYPE_RANGE, pSession->szCurTime, TIME_BUF_LEN);

				PlayCtrl(pSession);

				int len = strlen(pSession->szCurTime);
				if (len > 0)
				{
					//::Sleep(500);
					pSession->szCurTime[0] = '\0';
					PlayCtrl(pSession);
				}
				//   pSession->eStatus = rs_play;
				CLog::Log(RTSPCOM, LL_NORMAL, "发送第二次player");

				CLog::Log(RTSPCOM, LL_NORMAL, "播放 Scale:%s Range:%s CID-DID:%d", pSession->szScale, pSession->szCurTime, pSession->nCallDialogID);
			}
		}
		break;
		default: break;
		}
		// 删除控制信息占用的缓存
		SAFE_FREE_MOD_MSG(pMsg);

		return 0;
	}
	return -1;
}

#pragma endregion