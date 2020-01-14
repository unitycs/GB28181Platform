#include "StdAfx.h"
#include "RTSPCom.h"
#include "DataManager/DataStore.h"

void CRTSPCom::Init(void)
{
	// 注册Module消息处理函数
	RegisterProc(pfnQueueProc, this, 1);
}

void CRTSPCom::Cleanup(void)
{
}
void CRTSPCom::ProcessVideoSession(CModMessage * pMsg)
{
	auto calldialogid = pMsg->GetCallDialogID();

	if (m_oMapCidSessionMgr.count(calldialogid) > 0)
	{
		m_oMapCidSessionMgr[calldialogid]->ProcessOperation(pMsg);
	}
}

void CRTSPCom::ProcessRecordSession(CModMessage * pMsg)
{
	m_oRecordMgr.ProcessOperation(pMsg);
}

void CRTSPCom::StartPlaySession(CModMessage * pUnifiedMsg, mod_op_t::ot_rtsp eOperateType)
{
	auto calldialogid = pUnifiedMsg->GetCallDialogID();
	if (eOperateType == mod_op_t::ot_rtsp::play_with_sdp)
	{
		m_oMapCidSessionMgr[calldialogid] = &m_oSessionMgrLocal;
		this->m_oSessionMgrLocal.ProcessOperation(pUnifiedMsg);
	}
	else if (eOperateType == mod_op_t::ot_rtsp::play_no_sdp)
	{
		m_oMapCidSessionMgr[calldialogid] = &m_oThirdCallSessionMgr;
		this->m_oThirdCallSessionMgr.ProcessOperation(pUnifiedMsg);
	}

	else if (eOperateType == mod_op_t::ot_rtsp::play_broadcast)
	{
		m_oMapCidSessionMgr[calldialogid] = &m_oBroadCastCallSessionMgr;
		m_oBroadCastCallSessionMgr.ProcessOperation(pUnifiedMsg);
	}
}

bool CRTSPCom::HandleMsg(CMemPoolUnit * pUnit)
{
	CModMessage * pUnifiedMsg = reinterpret_cast<CModMessage *>(pUnit);
	// 取得消息类型
	auto eOperateType = pUnifiedMsg->GetModAction();

	switch (eOperateType.action_rtsp)
	{
	case mod_op_t::ot_rtsp::play_no_sdp:
	case mod_op_t::ot_rtsp::play_broadcast:
	case mod_op_t::ot_rtsp::play_with_sdp:
	{
		//先去查询必要的GUID如果查询成功则播放。
		auto retSearch = SearchGUIDForPlay(pUnifiedMsg, eOperateType.action_rtsp);
		if (retSearch)
		{
			this->StartPlaySession(pUnifiedMsg, eOperateType.action_rtsp);
		}
	}
	break;

	case mod_op_t::ot_rtsp::stop_all_play:
	{
		CLog::Log(RTSPCOM, LL_NORMAL, "%s ot_rtsp_stop_all_play\r\n", __FUNCTION__);
		this->m_oSessionMgrLocal.ProcessOperation(pUnifiedMsg);
		break;
	}

	// 播放控制
	case mod_op_t::ot_rtsp::play_ctrl_start:
	case mod_op_t::ot_rtsp::play_ctrl_stop:
	case mod_op_t::ot_rtsp::play_ctrl:
		//	m_oSessionMgr.ProcessSIP(pUnifiedMsg);
		this->ProcessVideoSession(pUnifiedMsg);
		break;
		// 录像控制
	case mod_op_t::ot_rtsp::record_start:
	case mod_op_t::ot_rtsp::record_stop:
		this->ProcessRecordSession(pUnifiedMsg);
		break;
	default:
		break;
	}

	return true;
}

// 处理设备GUID查询
bool CRTSPCom::SearchGUIDForPlay(CModMessage * pUnifiedMsg, mod_op_t::ot_rtsp eOperateType)
{
	CString strDecoderID;
	CDevSDP oDevSDP;
	CString strDeviceID = pUnifiedMsg->GetDeviceID();
	DeviceObject deviceInfo;
	//非编码器查询
	if (!ChannelLookup(strDeviceID, deviceInfo))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "GUID查询失败，未知的GB28181设备ID：%s", strDeviceID);
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return false;
	}
	if (deviceInfo.linked.strNVRIP.IsEmpty() ||
		deviceInfo.linked.strStreamerGUID.IsEmpty()
		)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "设备IP或者码流GUID不存，播放失败,请检查站点配置，设备GBID：%s", strDeviceID);
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return false;
	}
	pUnifiedMsg->SetNVRIP(deviceInfo.linked.strNVRIP);
	pUnifiedMsg->SetPlayGUID(deviceInfo.linked.strStreamerGUID);

	if (eOperateType == mod_op_t::ot_rtsp::play_broadcast)
	{
		CBodyBuilder oSIPBody;
		CBigFile *pResponse = nullptr;
		// 创建Response文件
		m_oResponseMemMgr.alloc(reinterpret_cast<CMemPoolUnit **>(&pResponse));
		CString sn = pUnifiedMsg->GetQuerySN();
		CString broadcastSrcId = pUnifiedMsg->GetBroadcastSrcID();
		CString broadcastTargetId = pUnifiedMsg->GetBroadcastTargetID();
		CString status = "OK";
		oSIPBody.CreateBroadCastXml(pResponse->GetBuffer(), broadcastSrcId, broadcastTargetId, sn, status);
		pUnifiedMsg->SetNotifyData(reinterpret_cast<void *>(pResponse), pUnifiedMsg->GetQuerySN());

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::notify_reponse);
		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg); //消息会被删除，如果再使用则会有问题,暂时无相关业务，所以暂时不处理
		pUnifiedMsg->SetModAction(mod_op_t::ot_rtsp::play_broadcast);
	}
	return true;
}

BOOL CRTSPCom::ChannelLookup(const char *pszKey, DeviceObject & husdeviceInfo)
{
	DeviceObject husDevceInfo;
	if (CDataStore::LookupDevice(pszKey, husDevceInfo, FALSE))
	{
		husdeviceInfo = husDevceInfo;
		return TRUE;
	}
	return FALSE;
	//return m_oChannelGBIDMap.Lookup(pszKey, pInfo, TRUE);
}