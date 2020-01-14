#include "StdAfx.h"
#include "RTSPTransSessionMgrThird.h"

void CRTSPTransSessionMgrThird::ProcessOperation(CModMessage * pMsg)
{
	INT64 nCallDialogID;
	auto eOperateType = pMsg->GetModAction();
	ThreadInfo_t *pThreadInfo = nullptr;
	CLog::Log(RTSPCOM, LL_NORMAL, "%s 处理会话内消息 operateType = %d", __FUNCTION__, eOperateType);
	switch (eOperateType.action_rtsp)
	{
	case  mod_op_t::ot_rtsp::play_no_sdp:
	{
		nCallDialogID = pMsg->GetCallDialogID();

		if (m_oThreadQueueMap.Lookup(nCallDialogID, pThreadInfo))
		{
			// 通知线程退出
			pThreadInfo->oEvent.SetEvent();
		}

		pThreadInfo = new ThreadInfo_t();
		pThreadInfo->nCallDialogID = nCallDialogID;
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
	case mod_op_t::ot_rtsp::play_ctrl_start:
	case mod_op_t::ot_rtsp::play_ctrl_stop:
	case mod_op_t::ot_rtsp::play_ctrl:
		nCallDialogID = pMsg->GetCallDialogID();
		if (m_oThreadQueueMap.Lookup(nCallDialogID, pThreadInfo))
		{
			pThreadInfo->oMsgQueue.Push(pMsg);
		}
		// 没有此会话
		else
		{
			// 没有对应的会话对象，可能是解码器的停止指令
			if (mod_op_t::ot_rtsp::play_ctrl_stop == eOperateType.action_rtsp)
			{
				pMsg->SetModAction(mod_op_t::ot_sdk::decoder_bye);
				CRouter::PushMsg(SDKCOM, pMsg);
			}
			// 解码器没有操作指令
			else
			{
				SAFE_FREE_MOD_MSG(pMsg);
			}
		}
		break;

	default:
		SAFE_FREE_MOD_MSG(pMsg);
		break;
	}
}

void CRTSPTransSessionMgrThird::parseThirdSdp(CRTSPSession *pSession, CModMessage *pMsg) const
{
	//  INT64 nCallDialogID				=	pMsg->GetCallDialogID();
	const char	szMediaType[][20] = { RTSP_STRING_REALSTREAM, RTSP_STRING_RECORDSTREAM, RTSP_STRING_RECORDSTREAM };
	CModMessage *pUnifiedMsg = pMsg;

	{
		// SDP协议栈
		SDPParser sdpStack;
		if (nullptr != pUnifiedMsg->GetPlayData())
		{
			CBigFile *pSDPFile = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
			// 解析SDP文件
			sdpStack.decode_sdp(pSDPFile->GetBuffer());
		}
		else
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "未找到DSP文件", pUnifiedMsg->GetQuerySN());
			return;
		}

		// 判断媒体类型
		if ("Play" == sdpStack.GetSDPStack().session_name.name)
		{
			pSession->eMediaActionT = mt_real;
		}
		else if ("Playback" == sdpStack.GetSDPStack().session_name.name)
		{
			pSession->eMediaActionT = mt_replay;
		}
		else if ("Download" == sdpStack.GetSDPStack().session_name.name)
		{
			pSession->eMediaActionT = mt_download;

			//   int count = sdpStack.GetSDPStack().attribute_list.size();
			list<SESSION_ATTRIBUTE>::iterator iter = sdpStack.GetSDPStack().attribute_list.begin();
			for (; iter != sdpStack.GetSDPStack().attribute_list.end(); ++iter)
			{
				SESSION_ATTRIBUTE attr = *iter;
				CString downloadspeed = "downloadspeed";
				if (attr.attribute == downloadspeed)
				{
					strcpy_s(pSession->m_szDownLoadSpeed, attr.value);
				}
			}
		}
		else if ("Talk" == sdpStack.GetSDPStack().session_name.name)
		{
			pSession->eMediaActionT = mt_talk;
		}

		Utils::StringCpy_s(pSession->szStartTime, TIME_BUF_LEN, sdpStack.GetSDPStack().active_time.start);
		Utils::StringCpy_s(pSession->szEndTime, TIME_BUF_LEN, sdpStack.GetSDPStack().active_time.stop);
		Utils::StringCpy_s(pSession->szSSRC, SSRC_BUF_LEN, sdpStack.GetSDPStack().ssrc_info.ssrc);
		Utils::StringCpy_s(pSession->szClientIP, IP_BUF_LEN, sdpStack.GetSDPStack().connect_info.address);
		Utils::StringCpy_s(pSession->szClientPort, PORT_BUF_LEN, sdpStack.GetSDPStack().media_list.front().port);

		pSession->m_transProto = tt_udp;
		CString transTypeStr = sdpStack.GetSDPStack().media_list.front().transport;
		if ((transTypeStr.Find("TCP") > 0) || (transTypeStr.Find("tcp") > 0))
		{
			pSession->m_transProto = tt_tcp;
		}

		if ((transTypeStr.Find("audio") > 0))
		{
			pSession->m_mediaDataType = md_audio;
		}

		pSession->tmStart = _atoi64(pSession->szStartTime);

		Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, szMediaType[pSession->eMediaActionT]);
	}
}

int  CRTSPTransSessionMgrThird::SetupThird(CRTSPSession *pSession, CModMessage *pMsg)
{
	SETUP_INFO tSetupInfo;
	CString strCSeq;
	CString strTrackID;

	parseThirdSdp(pSession, pMsg);

	GetVideoTrack(*(pSession->pRTSPMsg), &strTrackID);
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
		char downLoadSpeedBuf[128] = { 0 };
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

	return 0;
}

int CRTSPTransSessionMgrThird::ProcessNetworkEvent(CRTSPSession *pSession, WSAEVENT hNetEvent)
{
	RTSP_Unknown_Msg rtspUnknownMsg;
	int nResult = 0;
	char szBuf[BIG_FILE_BUF_LEN];

	int nLen = 0;
	WSANETWORKEVENTS tEvent;
	// 判断网络事件
	::WSAEnumNetworkEvents(pSession->GetRtspClient()->GetScoket(), hNetEvent, &tEvent);
	if (tEvent.lNetworkEvents & FD_READ)   // 处理FD_READ通知消息
	{
		if (tEvent.iErrorCode[FD_READ_BIT] == 0)
		{
			nLen = pSession->GetRtspClient()->RecvData(szBuf, BIG_FILE_BUF_LEN);
			szBuf[nLen] = 0;
			if (1 > nLen)
			{
				return -1;
			}
			CLog::Log(RTSPCOM, LL_DEBUG, "RECV RTSP Message\r\n%s", szBuf);
		}
	}
	else if (tEvent.lNetworkEvents & FD_CLOSE)
	{
		return -1;
	}
	else
		return 0;

	szBuf[nLen - 1] = 0;

	// 解析未知类型的RTSP消息
	RTSPMsgType eMsgType = RTSPParser().DecodeUnknowMsg(szBuf, rtspUnknownMsg);
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

		if (eMsgType != rmt_setparameter)
			// 取得序列号
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
			nResult = (this->*m_pfnRTSPProc[eMsgType][pSession->eStatus])(pSession);
	}

	return nResult;
}

int CRTSPTransSessionMgrThird::ProcessMsgQueueEvent(CRTSPSession *pSession, ThreadInfo_t* pThreadInfo)
{
	CModMessage *pMsg = nullptr;
	DWORD dwWaitTm = MAKEWORD(1000, 0);
	if (pThreadInfo->oMsgQueue.Pop(pMsg, dwWaitTm))
	{
		auto eActionType = pMsg->GetModAction();
		switch (eActionType.action_rtsp)
		{
		case mod_op_t::ot_rtsp::play_ctrl_start:

			//	if(rs_setup == pSession->eStatus)
			if (rs_third_200_ret == pSession->eStatus)
			{
				//StringCpy_s(pSession->szGUID, GUID_BUF_LEN, pUnifiedMsg->GetDeviceID());
				// 开始视频播放的信令交互
				this->SetupThird(pSession, pMsg);
				//	Play(pSession);
				CLog::Log(RTSPCOM, LL_NORMAL, "开始播放 CID-DID:%d", pSession->nCallDialogID);
			}
			break;

			// 停止播放
		case mod_op_t::ot_rtsp::play_ctrl_stop:

			// 没有SDP文件直接发送SetParameter
			Teardown(pSession);
			pSession->eStatus = rs_null;
			CLog::Log(RTSPCOM, LL_NORMAL, "停止视频会话 CID-DID:%d", pSession->nCallDialogID);
			pThreadInfo->oEvent.SetEvent();
			break;

			// 播放控制
		case mod_op_t::ot_rtsp::play_ctrl:
		{
			// 解析RTSP文件
			RTSP_Unknown_Msg rtspUnknownMsg;
			CBigFile *pSDP = reinterpret_cast<CBigFile*>(pMsg->GetPlayData());
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