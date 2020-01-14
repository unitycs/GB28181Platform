#include "StdAfx.h"
#include "ServerConsole.h"
#include "SIPCom.h"
#include "Main/MainThread.h"
#include "Memory/BigFile.h"

#define THIRD_CALL_SUPPORT_DECODER

CSIPCom::CSIPCom(void) :
	ALARM_TIMEOUT(15),
	m_pShareWriteQ(nullptr),
	m_pShareReadQ(nullptr),
	m_oModuleMsgMemMgr(sizeof(CModMessage), MEMPOOL_BLOCK_SUM),
	m_oBodyBufMemMgr(sizeof(CBigFile), MEMPOOL_BLOCK_SUM)
{
	m_strRemotePltID = appConf.m_UpperList[0].str_ID;
}

void  CSIPCom::Init(void)
{
	StartupSubProcess();

	// 读取配置文件
	if (0 != ReadConfig())
	{
		CLog::Log(SIPCOM, LL_NORMAL, "配置文件读取失败！");
		exit(0);
	}

	// 初始化SIP注册状态
	m_oSIPUnit.SetPlatformStatus(PF_STATUS_UNREGISTERED);

	// 注册Module消息处理函数
	RegisterProc(pfnQueueProc, this, 1);

	// 注册共享内存消息处理函数
	RegisterProc(pfnSharedQueueProc, this, 1);
}

void CSIPCom::Cleanup(void)
{
	if (m_pShareWriteQ)
		delete m_pShareWriteQ;
	if (m_pShareReadQ)
		delete m_pShareReadQ;
}

// 读取配置文件
int CSIPCom::ReadConfig()
{
	CLog::Log(SIPCOM, LL_NORMAL, "%s gateWayType = %d \r\n", __FUNCTION__, appConf.nSipComMode);
	//开始心保活
	m_oSIPUnit.Init(appConf.m_UpperList[0].str_ID,
		appConf.m_Current.KeepAliveInterval,
		appConf.m_Current.ExpiryTime - 10);

	CLog::Log(SIPCOM, LL_NORMAL, "Server2SIP Shared Memory Name:%s", appConf.m_SharedMemory.App_to_Sipcom.str_Name);
	m_pShareWriteQ = new CSharedVarQueue(appConf.m_SharedMemory.App_to_Sipcom.str_Name);
	m_pShareWriteQ->Init(MB2B(DEFAULT_SHARED_MEMORY_SIZE_MB), true);

	CLog::Log(SIPCOM, LL_NORMAL, "SIP2Server Shared Memory Name:%s", appConf.m_SharedMemory.Sipcom_to_App.str_Name);
	m_pShareReadQ = new CSharedVarQueue(appConf.m_SharedMemory.Sipcom_to_App.str_Name);
	m_pShareReadQ->Init(MB2B(DEFAULT_SHARED_MEMORY_SIZE_MB), true);

	return 0;
}

// 定时事件,包括心跳保活、重注册
int CSIPCom::RegisterTimerProc()
{
	if (appConf.nSipComMode == 1)
	{
		return 0;
	}
	// 未注册状态
	if (m_oSIPUnit.GetPlatformStatus() == PF_STATUS_UNREGISTERED)
	{
		if (m_oSIPUnit.CheckKeepalive())
		{
			ipc_sip_block_t	tRegisterPack;

			// 发送注册消息
			m_oSIPUnit.GetRegisterInfo(&tRegisterPack);
			tRegisterPack.nExpired = m_oSIPUnit.GetExpiry();
			SendShared(&tRegisterPack, nullptr);
			CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Register");
			// 设置注册状态
			m_oSIPUnit.SetPlatformStatus(PF_STATUS_REGISTERING);
			m_oSIPUnit.SetResponseTime();
		}
	}
	// 注册中
	else if (m_oSIPUnit.GetPlatformStatus() == PF_STATUS_REGISTERING)
	{
		int a = 0;
		// 注册中超时，SIPCom系统未启动，不再发注册消息
	}
	// 已注册
	else
	{
		// 检测Keepalive的应答是否超时
		if (m_oSIPUnit.CheckResponseTime())
		{
			CLog::Log(SIPCOM, LL_NORMAL, "保活应答超时, 当前状态设为PF_STATUS_UNREGISTERED");
			CModMessage *pMsg = nullptr;
			m_oSIPUnit.SetPlatformStatus(PF_STATUS_UNREGISTERED);

			// 通知停止报警
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pMsg));
			pMsg->SetModAction(mod_op_t::ot_sdk::stop_alarm);
			CRouter::PushMsg(SDKCOM, pMsg);

			pMsg = nullptr;

			// 通知停止所有播放
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pMsg));
			pMsg->SetModAction(mod_op_t::ot_rtsp::stop_all_play);
			CRouter::PushMsg(RTSPCOM, pMsg);
		}

		// 检测是否到达保活时间点
		if (m_oSIPUnit.CheckKeepalive())
		{
			Keepalive();
		}

		// 过期检测
		if (m_oSIPUnit.CheckExpiry())
		{
			ipc_sip_block_t tRegisterPack;		// 共享内存消息结构体

			// 取得注册需要的数据
			m_oSIPUnit.GetRegisterInfo(&tRegisterPack);
			tRegisterPack.nExpired = m_oSIPUnit.GetExpiry();

			// 发送重注册包
			SendShared(&tRegisterPack, nullptr);
			CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Re-Register");
		}
	}

	return 0;
}

// 接收其他模块发送的消息
bool CSIPCom::HandleMsg(CMemPoolUnit * pUnit)
{
	auto pUnifiedMsg = reinterpret_cast<CModMessage *>(pUnit);
	ipc_sip_block_t			tExportHeader;			// 共享内存消息结构
	CBigFile			*pSIPBody = nullptr;		// SIP消息体数据

	auto eOperateType = pUnifiedMsg->GetModAction();
	strcpy_s(tExportHeader.szFromDeviceID, pUnifiedMsg->GetRemoteID());
	tExportHeader.tHeader.ePackType = pack_kinds::tosend;
	CLog::Log(SIPCOM, LL_NORMAL, "%s remoteId = %s operateType = %d\r\n", __FUNCTION__, pUnifiedMsg->GetRemoteID(), eOperateType);

	switch (eOperateType.action_sipcom)
	{
		// 从RTSPCom发送过来的消息
		// 目前只有播放停止消息
	case mod_op_t::ot_sipcom::video_finished:
		tExportHeader.tHeader.eWorkType = work_kinds::video_call;
		tExportHeader.cmd_type = ST_CALL_MESSAGE;
		// 取得did
		tExportHeader.nDID = pUnifiedMsg->GetDID();
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetNotifyData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Call-Message, %d", tExportHeader.nDID);
		break;

	case mod_op_t::ot_sipcom::video_excepion_bye:
	{
		tExportHeader.tHeader.eWorkType = work_kinds::video_call;
		tExportHeader.cmd_type = ST_CALL_BYE;
		tExportHeader.nCID = pUnifiedMsg->nCID;
		tExportHeader.nDID = pUnifiedMsg->nDID;
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
	//	pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetNotifyData());
		CLog::Log(SIPCOM, LL_NORMAL, "%s SEND ST_CALL_EXCEPTION_BYE, %d", __FUNCTION__, tExportHeader.nDID);
		break;
	}

	case mod_op_t::ot_sipcom::invite_broadcast:
	{
		tExportHeader.tHeader.eWorkType = work_kinds::video_call;
		tExportHeader.cmd_type = ST_CALL_BROADCAST;
		tExportHeader.nSeq = pUnifiedMsg->nSeq;
		tExportHeader.nTID = pUnifiedMsg->GetTID();
		tExportHeader.nUniqueID = pUnifiedMsg->nUniqueID;
		tExportHeader.nOperation = pUnifiedMsg->nOperation;
		strcpy_s(tExportHeader.work_params.video_call.szRecvIP, pUnifiedMsg->work_params.video_call.szRecvIP);
		tExportHeader.work_params.video_call.nRecvPort[0] = pUnifiedMsg->work_params.video_call.nRecvPort[0];
		tExportHeader.work_params.video_call.nTransType = pUnifiedMsg->work_params.video_call.nTransType;
		tExportHeader.work_params.video_call.sender_parms.p_sender = pUnifiedMsg->GetCallSender();
		strcpy_s(tExportHeader.szToDeviceID, pUnifiedMsg->GetBroadcastSrcID());

		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Invitebroacast, pInviteSender = %08x TID:%d", tExportHeader.nTID);
		break;
	}

	case mod_op_t::ot_sipcom::broadcast_ack:
	{
		tExportHeader.tHeader.eWorkType = work_kinds::video_call;
		tExportHeader.cmd_type = ST_CALL_ACK_BROADCAST;
		tExportHeader.nDID = pUnifiedMsg->nDID;

		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP broadcast InviteResponse, TID:%d", tExportHeader.nTID);
		break;
	}
	// 从DevInfo或SDKCom发送过来的sdp消息
	case mod_op_t::ot_sipcom::invite_reponse:
		tExportHeader.tHeader.eWorkType = work_kinds::video_call;
		tExportHeader.cmd_type = ST_CALL_INVITE_RESPONSE;
		tExportHeader.nTID = pUnifiedMsg->GetTID();
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP InviteResponse, TID:%d", tExportHeader.nTID);
		break;
	case mod_op_t::ot_sipcom::video_ptz:
		tExportHeader.tHeader.eWorkType = work_kinds::default_message;
		tExportHeader.cmd_type = pUnifiedMsg->cmd_type;
		tExportHeader.nSeq = pUnifiedMsg->nSeq;
	//	tExportHeader.nTID = pUnifiedMsg->GetTID();
		tExportHeader.nOperation = pUnifiedMsg->nOperation;
		//pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
		//tExportHeader.szToDeviceID = pUnifiedMsg->szToDeviceID;
		strcpy_s(tExportHeader.szToDeviceID, pUnifiedMsg->szToDeviceID);
		tExportHeader.work_params.device_control.cCmdType = pUnifiedMsg->work_params.device_control.cCmdType;
		tExportHeader.work_params.device_control.cData1 = pUnifiedMsg->work_params.device_control.cData1;
		tExportHeader.work_params.device_control.cVerticalPace = pUnifiedMsg->work_params.device_control.cVerticalPace;
		tExportHeader.work_params.device_control.cZoomPace = pUnifiedMsg->work_params.device_control.cZoomPace;
		tExportHeader.work_params.device_control.cZoomAction = pUnifiedMsg->work_params.device_control.cZoomAction;
		//tExportHeader.work_params.device_control.nActive = 0 stop 1startup ,暂时没有
		tExportHeader.work_params.device_control.nLength = pUnifiedMsg->work_params.device_control.nLength;
		tExportHeader.work_params.device_control.nWidth = pUnifiedMsg->work_params.device_control.nWidth;
		tExportHeader.work_params.device_control.nMidPointX = pUnifiedMsg->work_params.device_control.nMidPointX;
		tExportHeader.work_params.device_control.nMidPointY = pUnifiedMsg->work_params.device_control.nMidPointY;
		tExportHeader.work_params.device_control.nLengthX = pUnifiedMsg->work_params.device_control.nLengthX;
		tExportHeader.work_params.device_control.nLengthY = pUnifiedMsg->work_params.device_control.nLengthY;
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP InviteResponse, TID:%d", tExportHeader.nTID);
		break;
		case mod_op_t::ot_sipcom::record_inqury: //录像查询
			tExportHeader.tHeader.eWorkType = work_kinds::default_message;
			tExportHeader.cmd_type = pUnifiedMsg->cmd_type;
			tExportHeader.nSeq = pUnifiedMsg->nSeq;
			tExportHeader.nOperation = pUnifiedMsg->nOperation;
			strcpy_s(tExportHeader.szToDeviceID, pUnifiedMsg->szToDeviceID);
			strcpy_s(tExportHeader.szToChannelID, pUnifiedMsg->szToChannelID);
			tExportHeader.work_params.query_record.nActive = pUnifiedMsg->work_params.query_record.nActive;
			strcpy_s(tExportHeader.work_params.query_record.szStartTime, pUnifiedMsg->work_params.query_record.szStartTime);
			strcpy_s(tExportHeader.work_params.query_record.szEndTime, pUnifiedMsg->work_params.query_record.szEndTime);
			if (pUnifiedMsg->work_params.query_record.szRecLocation != nullptr)
			{
				strcpy_s(tExportHeader.work_params.query_record.szRecLocation, pUnifiedMsg->work_params.query_record.szRecLocation);
			}
			if (pUnifiedMsg->work_params.query_record.szAddr != nullptr)
			{
				strcpy_s(tExportHeader.work_params.query_record.szAddr, pUnifiedMsg->work_params.query_record.szAddr);
			}
			strcpy_s(tExportHeader.work_params.query_record.cSecrecy, pUnifiedMsg->work_params.query_record.cSecrecy);
			if (pUnifiedMsg->work_params.query_record.szRecordID != nullptr)
			{
				strcpy_s(tExportHeader.work_params.query_record.szRecordID, pUnifiedMsg->work_params.query_record.szRecordID);
			}
			strcpy_s(tExportHeader.work_params.query_record.cIndistinctQuery, pUnifiedMsg->work_params.query_record.cIndistinctQuery);
		break;

		// 从SDKCom发送过来的控制应答消息或报警或
	case mod_op_t::ot_sipcom::notify_reponse:
		//	case ot_sip_alarm_request:
		tExportHeader.tHeader.eWorkType = work_kinds::event_notify;
		tExportHeader.cmd_type = ST_MESSAGE;
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetNotifyData());
		if (mod_op_t::ot_sipcom::notify_reponse == eOperateType.action_sipcom)
			CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP DeviceControl Response, DeviceID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		else
			CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Alarm, DeviceID:%s", pUnifiedMsg->GetDeviceID());
		break;

	case mod_op_t::ot_sipcom::alarm_request:
		tExportHeader.tHeader.eWorkType = work_kinds::event_notify;
		tExportHeader.cmd_type = ST_SUBSCRIBE_NOTIFY;
		tExportHeader.nDID = pUnifiedMsg->GetDID();
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetNotifyData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Alarm, DeviceID:%s", pUnifiedMsg->GetDeviceID());
		break;

		// 从DevInfo或SDKCom发送过来查询结果
	case mod_op_t::ot_sipcom::query_result:
		tExportHeader.tHeader.eWorkType = work_kinds::event_notify;
		tExportHeader.cmd_type = ST_MESSAGE;
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetSearchData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP QureyResponse, DeviceID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		break;

	case mod_op_t::ot_sipcom::decoder_status_notify_result:
	{
		tExportHeader.tHeader.eWorkType = work_kinds::event_notify;
		//tExportHeader.eSIPCmdType = ST_NORMAL_MESSAGE;
		tExportHeader.cmd_type = ST_MESSAGE_NOTIFY;
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetSearchData());
		CLog::Log(SIPCOM, LL_NORMAL, "%s ot_sip_decoder_status_notify_result, DeviceID:%s SN:%s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		break;
	}

	case mod_op_t::ot_sipcom::subscribe_notify:
		tExportHeader.tHeader.eWorkType = work_kinds::subscription;
		tExportHeader.cmd_type = ST_SUBSCRIBE_NOTIFY;
		tExportHeader.nDID = pUnifiedMsg->GetDID();
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetSearchData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP Subscribe Notify, DeviceID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		break;
	case mod_op_t::ot_sipcom::subscribe_timeout_notify:
	case mod_op_t::ot_sipcom::subscribe_noresrc_notify:
	{
		tExportHeader.tHeader.eWorkType = work_kinds::subscription;
		tExportHeader.cmd_type = ST_NOTIFY_TERMINATED;
		if (eOperateType.action_sipcom == mod_op_t::ot_sipcom::subscribe_noresrc_notify)
		{
			tExportHeader.work_params.subscription.n_ss_reason = SS_NORESOURCE;
		}
		else if (eOperateType.action_sipcom == mod_op_t::ot_sipcom::subscribe_timeout_notify)
		{
			tExportHeader.work_params.subscription.n_ss_reason = SS_TIMEOUT;
		}
		tExportHeader.cmd_type = ST_NOTIFY_TERMINATED;
		tExportHeader.nDID = pUnifiedMsg->GetDID();
		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetSearchData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP  Notify, DeviceID:%s SN:%s,Reason: %d", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN(), tExportHeader.work_params.subscription.n_ss_reason);
	}
	break;
	case mod_op_t::ot_sipcom::subscribe_reponse:
		tExportHeader.tHeader.eWorkType = work_kinds::subscription;
		tExportHeader.cmd_type = ST_SUBSCRIBE_RESPONSE;
		tExportHeader.nTID = pUnifiedMsg->GetTID();
		tExportHeader.nDID = pUnifiedMsg->GetDID();

		pSIPBody = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetSearchData());
		CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP QureyResponse, DeviceID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		break;
	default:
		goto error;
	}
#ifdef WIN_SERVICE
	// 未登录到平台，不处理设备消息
	if (!appConf.nSipComMode)
	{
		if (PF_STATUS_REGISTERED != m_oSIPUnit.GetPlatformStatus())
		{
			CLog::Log(SIPCOM, LL_NORMAL, "网关当前为未登录状态，丢弃此模块消息。消息操作类型: %d", eOperateType);
			goto error;
		}
	}
#endif
	Utils::StringCpy_s(tExportHeader.szDstDomainID, ID_BUF_LEN, m_oSIPUnit.GetPlatformID());

	strcpy_s(tExportHeader.szFromDeviceID, pUnifiedMsg->GetRemoteID());
	CLog::Log(SIPCOM, LL_NORMAL, "%s eOperateType %d szRemoted = %s", __FUNCTION__, eOperateType, tExportHeader.szFromDeviceID);
	if (pSIPBody)
		SendShared(&tExportHeader, pSIPBody->GetBuffer());
	else
		SendShared(&tExportHeader, nullptr);

error:
	if (pSIPBody)
	{
		// 释放xml、sdp等附加数据的缓存
		pSIPBody->Free();
		pSIPBody = nullptr;
	}

	// 释放该消息占用的内存
	pUnifiedMsg->Free();
	return 0;
}

UINT CSIPCom::pfnSharedQueueProc(LPVOID pParam)
{
	CSIPCom * pSIPCom = reinterpret_cast<CSIPCom *>(pParam);
	while (!pSIPCom->m_bIsExit)
	{
		pSIPCom->HandleShared();
	}

	return 0;
}

// 接受共享内存中的消息
int CSIPCom::HandleShared(void)
{
	char			     *p_ipc_data_block = nullptr;			    // 指向在两个进程间共享的内存数据块的指针
	CModMessage			 *p_mod_message = nullptr;	                // 指向单个进程内模块间消息缓存块的指针

	// 定时处理
	RegisterTimerProc();

	SubscribeTimerProc();

	// 等待共享内存消息
	WaitForSingleObject(m_pShareReadQ->GetEventHandle(), 1000);

	while (m_pShareReadQ->PopAlloc(reinterpret_cast<void**>(&p_ipc_data_block)))
	{
		auto p_sip_block = reinterpret_cast<ipc_sip_block_t *>(p_ipc_data_block);
		CBigFile *pDataBuf = nullptr;

		m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
		p_mod_message->FromSipDataBlock(p_sip_block);

		// 释放占用的共享内存
		m_pShareReadQ->PopDone(p_ipc_data_block);
#ifdef WIN_SERVICE
		if (!appConf.nSipComMode)
			// 未登录到平台，不处理注册消息之外的其它平台消息
		{
			if (PF_STATUS_REGISTERED != m_oSIPUnit.GetPlatformStatus() &&
				ST_REGISTER_SUCCESS != p_sip_block->cmd_type  &&
				ST_REGISTER_FAIL != p_sip_block->cmd_type
				)
			{
				CLog::Log(SIPCOM, LL_NORMAL, "网关当前为未登录状态，不处理注册之外的平台消息。\n消息类型: %d", p_sip_block->cmd_type);
				continue;
			}
		}
#endif
		switch (p_sip_block->cmd_type)
		{
			// 收到MESSAGE消息
		case    ST_MESSAGE:
			CLog::Log(SIPCOM, LL_NORMAL, "%s remoteId = %s ST_MESSAGE\r\n", __FUNCTION__, p_sip_block->szFromDeviceID);
			//ProcNormalMsg(sip_packet.tHeader.szCSeq, sip_packet.strBody.c_str(), p_mod_message);
			DispatchModMessage(p_mod_message);
			break;

			// 收到Message应答消息
		case ST_MESSAGE_RESPONSE:
			// 判断是否是保活包的应答
			if (TRUE == m_oSIPUnit.IsLastKeepaliveRes(p_sip_block->nSeq))
			{
				// 重新设置保活应答超时时间
				m_oSIPUnit.ResetResponseTime();
				CLog::Log(SIPCOM, LL_DEBUG, "RECV SIP KeepaliveMessage Response, Seq:%d", p_sip_block->nSeq);
			}
			else
				CLog::Log(SIPCOM, LL_DEBUG, "RECV SIP Message Response, Seq:%d", p_sip_block->nSeq);
			p_mod_message->Free();
			break;

			// 收到INVITE消息
		case ST_CALL_INVITE:
			CLog::Log(SIPCOM, LL_NORMAL, "%s", __FUNCTION__);
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			p_mod_message->FromSipDataBlock(p_sip_block);
			// Decoder请求
			if (p_sip_block->tHeader.nExtBodySize == 0)
			{
				p_mod_message->SetPlayData(nullptr);
				p_mod_message->SetDeviceType(HUSDevice_T::DECODER_CHANNEL);
#ifdef THIRD_CALL_SUPPORT_DECODER

				{
					p_mod_message->SetModAction(mod_op_t::ot_sdk::decoder_invite);
					// 发送到RTSPCOM模块
					CRouter::PushMsg(SDKCOM, p_mod_message);
					CLog::Log(SIPCOM, LL_NORMAL, "%s RECV SIP Invite, 设备ID:%s TID:%d CID-DID:%d nobody", __FUNCTION__, p_sip_block->szToDeviceID, p_sip_block->nTID, p_sip_block->nCallDialogID);
					break;
				}
#else
				{
					p_mod_message->SetModAction(ot_rtsp_play_no_sdp);
					// 发送到RTSPCom模块
					CRouter::PushMsg(RTSPCOM, p_mod_message);
					CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Invite third call no sdp, 设备ID:%s TID:%d CID-DID:%d", sip_packet.szToDeviceID, sip_packet.nTID, sip_packet.nCallDialogID);
					break;
				}
#endif
			}
			// 前端设备请求
			p_mod_message->SetModAction(mod_op_t::ot_rtsp::play_with_sdp);
			m_oBodyBufMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pDataBuf));
			pDataBuf->SetData(p_mod_message->strBody.c_str());
			p_mod_message->SetPlayData(pDataBuf);
			//p_mod_sip_packet->SetDeviceType(DT_DECODER_CHANNEL);
			// 发送到RTSPCom模块
			CRouter::PushMsg(RTSPCOM, p_mod_message);
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Invite, 设备ID:%s TID:%d CID-DID:%d", p_sip_block->szToDeviceID, p_sip_block->nTID, p_sip_block->nCallDialogID);
			break;

		case ST_CALL_ACK:
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			//p_mod_message->FromSipDataBlock(p_sip_block);
			// 普通ACK请求
			if (p_sip_block->tHeader.nExtBodySize == 0)
			{
				p_mod_message->SetModAction(mod_op_t::ot_rtsp::play_ctrl_start);

				// 发送到RTSPCom模块
				CRouter::PushMsg(RTSPCOM, p_mod_message);
			}
			// Decoderd的ACK请求
			else
			{
#ifdef THIRD_CALL_SUPPORT_DECODER
				p_mod_message->SetModAction(mod_op_t::ot_sdk::decoder_ack);
				// 发送到RTSPCom模块
				CRouter::PushMsg(SDKCOM, p_mod_message);
#else
				p_mod_message->SetModAction(mod_op_t::ot_rtsp::play_ctrl_start);
				// 发送到RTSPCom模块
				CRouter::PushMsg(RTSPCOM, p_mod_message);
#endif
			}

			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Ack, 设备ID:%s  CID-DID:%d", p_sip_block->szToDeviceID, p_sip_block->nCallDialogID);
			break;

		case ST_CALL_INVITE_RESPONSE:
		{
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			
			p_mod_message->FromSipDataBlock(p_sip_block);
			p_mod_message->nDID = p_sip_block->nDID;
			p_mod_message->nCID = p_sip_block->nCID;
			p_mod_message->nSeq = p_sip_block->nSeq;
			// invite reponse
			if (p_sip_block->tHeader.nExtBodySize != 0)
			{
				p_mod_message->SetModAction(mod_op_t::ot_gb_adaptor::call_invite_response);

				CRouter::PushMsg(GBADAPTORCOM, p_mod_message);

				// 发送到RTSPCom模块
			//	CRouter::PushMsg(RTSPCOM, p_mod_message);
			}
		}
		// 收到BYE消息
		case ST_CALL_BYE:
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			p_mod_message->FromSipDataBlock(p_sip_block);
			p_mod_message->SetModAction(mod_op_t::ot_rtsp::play_ctrl_stop);

			// 发送到RTSPCom模块
		//	CRouter::PushMsg(RTSPCOM, p_mod_message);
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Bye, 设备ID:%s Seq:%d CID&DID:%d", p_sip_block->szToDeviceID, p_sip_block->nSeq, p_sip_block->nCallDialogID);
			break;

			// 收到INFO消息
		case ST_CALL_INFO:
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			p_mod_message->FromSipDataBlock(p_sip_block);
			m_oBodyBufMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pDataBuf));
			p_mod_message->SetPlayData(pDataBuf);

			if (pDataBuf)
			{
				pDataBuf->SetData(p_mod_message->strBody.c_str());
			}
			p_mod_message->SetModAction(mod_op_t::ot_rtsp::play_ctrl);

			// 发送到RTSPCom模块
			CRouter::PushMsg(RTSPCOM, p_mod_message);
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Info, 设备ID:%s Seq:%d CID&DID:%d", p_sip_block->szToDeviceID, p_sip_block->nSeq, p_sip_block->nCallDialogID);
			break;

		case ST_SUBSCRIBE:
			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&p_mod_message));
			p_mod_message->FromSipDataBlock(p_sip_block);
			CLog::Log(SIPCOM, LL_NORMAL, "%s remoteId = %s ST_SUBSCRIBE\r\n", __FUNCTION__, p_sip_block->szToDeviceID);

			//ProcSubscribeMsg(sip_packet.tHeader.szCSeq, sip_packet.tHeader.szExpiresReason, sip_packet.strBody.c_str(), p_mod_message);
			DispatchModSSMessage(p_mod_message);
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Subscribe, 设备ID:%s Seq:%d CID&DID:%d", p_sip_block->szToDeviceID, p_sip_block->nSeq, p_sip_block->nCallDialogID);
			break;

			// 收到已注册到的平台信息
		case ST_REGISTER_SUCCESS:
			m_oSIPUnit.SetPlatformStatus(PF_STATUS_REGISTERED);
			m_oSIPUnit.ResetResponseTime();
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Register-Success");
			Keepalive();
			break;
		case ST_REGISTER_FAIL:
		{
			if (m_oSIPUnit.GetPlatformStatus() == PF_STATUS_REGISTERED)
			{
				// 通知停止所有播放
				CLog::Log(SIPCOM, LL_NORMAL, "%s RECV SIP Register-Fail 重注册\r\n", __FUNCTION__);
				CModMessage *pMsg = nullptr;
				m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pMsg));
				pMsg->SetModAction(mod_op_t::ot_rtsp::stop_all_play);
				CRouter::PushMsg(RTSPCOM, pMsg);
			}
		}
		m_oSIPUnit.SetPlatformStatus(PF_STATUS_UNREGISTERED);
		CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Register-Fail");
		break;
		default:
			CLog::Log(SIPCOM, LL_NORMAL, "RECV SIP Unknown");
			break;
		}
	}
	return 0;
}

//recv-->get(transfer)—->filter-->dispatch(to mod)

int CSIPCom::DispatchModMessage(CModMessage	 *p_mod_message)
{
	switch (p_mod_message->cmd_body.content_type)
	{
	case body_content_type_t::BT_QUERY:

		break;

	case body_content_type_t::BT_CONTROL:

		break;
	case body_content_type_t::BT_RESPONSE:
		break;

	default:
		break;
	}

	return 0;
}

//recv-->get(transfer)—->filter-->dispatch(to mod)
//ss == subscription
int CSIPCom::DispatchModSSMessage(CModMessage	 *p_mod_message)
{
	switch (p_mod_message->cmd_body.u_cases.e_subscribe)
	{
	case body_cmd_t::subscribe_t::subscribe_alarm:

		break;
	case  body_cmd_t::subscribe_t::subscribe_catalog:

		break;

	default:
		break;
	}

	return 0;
}

// 发送数据到共享内存
int CSIPCom::SendShared(ipc_sip_block_t * pExPortHeader, const char *pszData, const int nSeq)
{
	int nHeadSize = sizeof(ipc_sip_block_t);
	int nBodyDataSize = 0;
	if (pszData)
	{
		nBodyDataSize = strlen(pszData) + 1;
	}
	pExPortHeader->tHeader.ePackType = pack_kinds::tosend;
	pExPortHeader->tHeader.nPackSize = nHeadSize + nBodyDataSize;
	pExPortHeader->tHeader.nExtBodySize = nBodyDataSize;

	// 取得CSeq

	auto  nCSeq = nSeq > 0 ? nSeq : m_oSIPUnit.GenerateCSeq();

//	Utils::StringCpy_s(pExPortHeader->szCSeq, CSEQ_BUF_LEN, strCSeq.GetString());

	if (appConf.nSipComMode!=1)
	{
		pExPortHeader->nSeq = nCSeq;
	}

	char * pHeader = nullptr;

	if (m_pShareWriteQ->PushAlloc(reinterpret_cast<void**>(&pHeader), pExPortHeader->tHeader.nPackSize))
	{
		memcpy(pHeader, pExPortHeader, nHeadSize);

		if (0 < nBodyDataSize)
			memcpy(pHeader + nHeadSize, pszData, nBodyDataSize);
		m_pShareWriteQ->PushDone(pHeader);
		return nCSeq;
	}
	CLog::Log(SIPCOM, LL_NORMAL, "共享内存写入失败AvailData:%d AvailSpace:%d", m_pShareWriteQ->m_pHeader->nAvailData, m_pShareWriteQ->m_pHeader->nAvailSpace);

	return nCSeq;
}

void CSIPCom::Keepalive()
{
	ipc_sip_block_t	tKeepalivePack;						// 共享内存消息结构体
	char		szBodyBuffer[BIG_FILE_BUF_LEN];		// SIP消息体缓存


	m_oSIPUnit.GetKeepaliveInfo(&tKeepalivePack, szBodyBuffer, BIG_FILE_BUF_LEN, appConf.m_Current.str_ID);
	auto nSeq = m_oSIPUnit.GenerateCSeq(); // SIP消息序列号

	// 发送保活包
	SendShared(&tKeepalivePack, szBodyBuffer, nSeq);
	CLog::Log(SIPCOM, LL_NORMAL, "SEND SIP KeepaliveMessage, Seq:%d", nSeq);

	// 开始等待应答消息
	m_oSIPUnit.SetResponseTime();
	m_oSIPUnit.SetLastKeepaliveCSeq(nSeq);
}

// 注销
void CSIPCom::Unregister()
{
	ipc_sip_block_t	tUnregisterPack;

	m_oSIPUnit.GetRegisterInfo(&tUnregisterPack);
	tUnregisterPack.nExpired = 0;
	SendShared(&tUnregisterPack, nullptr);

	CLog::Log(SIPCOM, LL_NORMAL, "%s \r\n", __FUNCTION__);
}

// 终止SIP服务进程
void CSIPCom::StopSubprocess()
{
	/*unreg before exit*/
	Unregister();
	::Sleep(500);

	CLog::Log(SIPCOM, LL_NORMAL, "%s------ --- send unregister over\r\n", __FUNCTION__);

	ipc_sip_block_t	tTerminatedPack;
	tTerminatedPack.cmd_type = ST_EXIT_PROCESS;
	SendShared(&tTerminatedPack, nullptr);
}

int CSIPCom::SubscribeTimerProc()
{
	MExpiry_t nTimeOut;
	if (m_oSIPUnit.CheckSubExpiry(nTimeOut))
	{
		// 移除订阅
		for (auto it = nTimeOut.begin();
			it != nTimeOut.end();)
		{
			CModMessage	*pSIP_Packet = nullptr;

			m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pSIP_Packet));
			pSIP_Packet->SetModAction(mod_op_t::ot_devinfo::subscribe);
			pSIP_Packet->SetCallDialogID(it->first);
			pSIP_Packet->SetSubExpires("Timeout");

			// 发送到DEVINFO
			CRouter::PushMsg(DEVINFO, pSIP_Packet);
		}
	}

	return 0;
}