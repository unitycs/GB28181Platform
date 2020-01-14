#include "StdAfx.h"
#include "ServerConsole.h"
#include "RTSPSessionBuilder.h"
#include "RTSPTransMgrBase.h"


DWORD CRTSPSessionBuilder::m_dwSSRCBase = 0;

CRTSPSessionBuilder::CRTSPSessionBuilder(void)
	:pRelatedMsg(nullptr), m_oSessionMemMgr(sizeof(CRTSPSession), MEMPOOL_BLOCK_SUM)
{
	strGateWayId = appConf.m_Current.str_ID;
}

// 创建会话对象
CRTSPSession* CRTSPSessionBuilder::CreateLocalCall(CModMessage * pUnifiedMsg)
{
	CRTSPSession *pSession = nullptr;
	const char	szMediaType[][20] = { RTSP_STRING_REALSTREAM, RTSP_STRING_RECORDSTREAM, RTSP_STRING_RECORDSTREAM };
	CreateRealRTSPSession(pUnifiedMsg, &pSession);

	// 创建Invite会话时的处理
	if (mod_op_t::ot_rtsp::record_start > pUnifiedMsg->GetModAction().action_rtsp)
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
			return nullptr;
		}
		auto op_type_str = sdpStack.GetSDPStack().session_name.name.MakeLower();
		// 判断媒体类型
		if (op_type_str.CompareNoCase("play") == 0)
		{
			pSession->eMediaActionT = mt_real;
		}
		else if (op_type_str.CompareNoCase("playback") == 0)
		{
			pSession->eMediaActionT = mt_replay;
		}
		else if (op_type_str.CompareNoCase("Download") == 0)
		{
			pSession->eMediaActionT = mt_download;

			//   int count = sdpStack.GetSDPStack().attribute_list.size();
			list<SESSION_ATTRIBUTE>::iterator iter = sdpStack.GetSDPStack().attribute_list.begin();
			for (; iter != sdpStack.GetSDPStack().attribute_list.end(); iter++)
			{
				SESSION_ATTRIBUTE attr = *iter;
				CString downloadspeed = "downloadspeed";
				if (attr.attribute == downloadspeed)
				{
					strcpy_s(pSession->m_szDownLoadSpeed, attr.value);
				}
			}
		}
		else if (op_type_str.CompareNoCase("talk") == 0)
		{
			pSession->eMediaActionT = mt_talk;
		}

		Utils::StringCpy_s(pSession->szStartTime, TIME_BUF_LEN, sdpStack.GetSDPStack().active_time.start);
		Utils::StringCpy_s(pSession->szEndTime, TIME_BUF_LEN, sdpStack.GetSDPStack().active_time.stop);
		Utils::StringCpy_s(pSession->szClientIP, IP_BUF_LEN, sdpStack.GetSDPStack().connect_info.address);
		Utils::StringCpy_s(pSession->szClientPort, PORT_BUF_LEN, sdpStack.GetSDPStack().media_list.front().port);

		pSession->m_transProto = tt_udp;
		CString transTypeStr = sdpStack.GetSDPStack().media_list.front().transport;

		if (transTypeStr.Find(_T("TCP")) >= 0 || transTypeStr.Find(_T("tcp")) >= 0)
		{
			pSession->m_transProto = tt_tcp;
		}

		if ((transTypeStr.Find("audio") > 0))
		{
			pSession->m_mediaDataType = md_audio;
		}

		pSession->tmStart = _atoi64(pSession->szStartTime);
		pSession->tmEnd = _atoi64(pSession->szEndTime);

		Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, szMediaType[pSession->eMediaActionT]);

	}

	return pSession;
}

CRTSPSession * CRTSPSessionBuilder::Create(CModMessage * pUnifiedMsg)
{
	auto  calltype = pUnifiedMsg->GetModAction();
	if (calltype.action_rtsp == mod_op_t::ot_rtsp::play_no_sdp)
		return CreateThirdCall(pUnifiedMsg);
	else if (calltype.action_rtsp == mod_op_t::ot_rtsp::play_broadcast)
		return CreateBroadcast(pUnifiedMsg);
	return CreateLocalCall(pUnifiedMsg);

}

CRTSPSession* CRTSPSessionBuilder::CreateThirdCall(CModMessage * pUnifiedMsg)
{
	CRTSPSession *pSession = nullptr;
	CLog::Log(RTSPCOM, LL_NORMAL, "%s type = %d", __FUNCTION__, pSession->nCallDialogID, pUnifiedMsg->GetModAction().action_rtsp);

	CreateRealRTSPSession(pUnifiedMsg, &pSession);
	// 生成URL
	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_THIRD_CALL_RELA_STREAM);

	return pSession;
}

CRTSPSession *CRTSPSessionBuilder::CreateBroadcast(CModMessage * pUnifiedMsg)
{
	CRTSPSession *pSession = nullptr;
	CLog::Log(RTSPCOM, LL_NORMAL, "%s type = %d", __FUNCTION__, pSession->nCallDialogID, pUnifiedMsg->GetModAction().action_rtsp);
	CreateRealRTSPSession(pUnifiedMsg, &pSession);

	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_BROADCAST_STREAM);

	return pSession;
}

// 销毁会话对象
void CRTSPSessionBuilder::Destroy(CRTSPSession *&pSession)
{
	if (pSession != nullptr)
	{
		pSession->DestroyRtspClient();
		pSession->Free();
	}
}

CModMessage * CRTSPSessionBuilder::GetUnifiedMsg()
{
	return pRelatedMsg;
}

void CRTSPSessionBuilder::GenerateSSRC(CRTSPSession *pSession) {
	if (nullptr == pSession) {
		return;
	}

	DWORD dwSSRC = InterlockedIncrement(&m_dwSSRCBase);

	CString strSSRC;
	strSSRC.Format(_T("%01d%s%04d"),
		(mt_real == pSession->eMediaActionT) ? 0 : 1,
		strGateWayId.Mid(3, 5),
		dwSSRC % 10000
	);

	Utils::StringCpy_s(pSession->szSSRC, SSRC_BUF_LEN, strSSRC);
}

void CRTSPSessionBuilder::CreateRealRTSPSession(CModMessage * pUnifiedMsg, CRTSPSession **ppSession)
{

	// 分配Session内存
	m_oSessionMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(*ppSession));

	auto pSession = *ppSession;

	if (!pSession) return;
	pSession->pUnifiedMsg = pUnifiedMsg;
	pSession->eStatus = rs_null;
	pSession->m_szDownLoadSpeed[0] = NULL;
	pSession->m_mediaDataType = md_video;
	pSession->m_transProto = tt_udp;

	pSession->nTID = pUnifiedMsg->GetTID();
	pSession->nCallDialogID = pUnifiedMsg->GetCallDialogID();
	pSession->pInviteSender = pUnifiedMsg->GetCallSender();

	// 给会话对象赋值
	Utils::StringCpy_s(pSession->szToDeviceID, ID_BUF_LEN, pUnifiedMsg->GetDeviceID());


	Utils::StringCpy_s(pSession->szGUID, GUID_BUF_LEN, pUnifiedMsg->GetPlayGUID());

	// 去除GUID字符串两端的"{" "}"
	if ('{' == pSession->szGUID[0])
	{
		int nLen = strlen(pSession->szGUID);
		memcpy(pSession->szGUID, pSession->szGUID + 1, nLen - 2);
		pSession->szGUID[nLen - 2] = 0;
	}

	Utils::StringCpy_s(pSession->szServerIP, IP_BUF_LEN, pUnifiedMsg->GetNVRIP());


	// 生成URL
	Utils::StringCpy_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_PROTOCOL);
	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_COLON);
	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_DOUBLEBACKSLASH);

	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, pUnifiedMsg->GetNVRIP());

	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_COLON);
	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, "554");
	Utils::StringCat_s(pSession->szURL, URL_BUF_LEN, RTSP_STRING_BACKSLASH);

	GenerateSSRC(pSession);
	//pSession->pRtspClient = new CRTSPClient(pSession->szServerIP);

}
