#include "StdAfx.h"
#include "Log/Log.h"
#include "SIPConsole.h"
#include "SIPService.h"
#include "Main/MainThread.h"
#include "Common/common.h"


//INVITE cmd type
#define INVITE_PLAY		0
#define INVITE_PLAYBACK 1
#define INVITE_DOWNLOAD 2

#define PTZ_CMD			0
#define DRAG_ZOOM_NONE	0
#define DRAG_ZOOM_IN	1
#define DRAG_ZOOM_OUT	2
#define DRAG_ZOOM_NULL	3

CSIPService::CSIPService(void) :
	m_pContext_eXosip(nullptr), m_pEvent(nullptr),
	m_nSharedMemSize(0),
	m_pSharedWriteQ(nullptr), m_pSharedReadQ(nullptr),
	m_nFirewallPort(0), m_nAppWorkMode(0),
	m_RegManager(this)
{
	m_pSendFun[ST_MESSAGE] = &CSIPService::SendRequestMessage;
	m_pSendFun[ST_CALL_MESSAGE] = &CSIPService::SendCallRequest;
	m_pSendFun[ST_REGISTER] = &CSIPService::Register;
	m_pSendFun[ST_CALL_INVITE_RESPONSE] = &CSIPService::SendInviteCallAnswer;
	m_pSendFun[ST_CALL_BROADCAST] = &CSIPService::SendInviteBroadCast;
	m_pSendFun[ST_CALL_ACK_BROADCAST] = &CSIPService::SendAckBroadCast;
	m_pSendFun[ST_SUBSCRIBE_RESPONSE] = &CSIPService::SendSubscribeAnswer;
	m_pSendFun[ST_SUBSCRIBE_NOTIFY] = &CSIPService::SendSubScribeNotify;
	m_pSendFun[ST_NOTIFY_TERMINATED] = &CSIPService::SendSubScribeTerminateNotify;
	m_pSendFun[ST_MESSAGE_NOTIFY] = &CSIPService::SendRequestNotifyMessage;
	m_pSendFun[ST_CALL_BYE] = &CSIPService::SendCallBye;
	m_pSendFun[ST_VIDEO_PTZ] = &CSIPService::SendDeviceControl;
}


void CSIPService::Init()
{
	//Sleep(15000);
	InitgGlobalConfigInfo();

	m_pContext_eXosip = eXosip_malloc();
	if (0 != eXosip_init(m_pContext_eXosip))
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "SIP初始化失败");
		return;
	}

	if (m_nAppWorkMode > 0) //作为上级平台模式和转发模式都需要，注册管理
	{
		m_RegManager.InitSipContext(m_pContext_eXosip);
		//同时创建转发工作者对象
		if (m_nAppWorkMode == 2)
		{
			m_pSipRelayWorker = std::make_unique<CSipServiceRealy>(&m_RegManager);
			m_pSipRelayWorker->Init_relay_context(m_pContext_eXosip);
		}
	}

	//网络事件监听
	auto strIP = m_RegManager.CurrentDomain().GetIP().c_str();
	auto nPort = m_RegManager.CurrentDomain().GetPort();
	if (0 != eXosip_listen_addr(m_pContext_eXosip, IPPROTO_UDP, strIP, nPort, AF_INET, 0))
	{
		eXosip_quit(m_pContext_eXosip);
		CLog::Log(SIPSERVICE, LL_NORMAL, "SIP(UDP):IP绑定失败 IP:%s Port:%d", strIP, nPort);
		return;
	}
	CLog::Log(SIPSERVICE, LL_NORMAL, "SIP(UDP):IP绑定成功 IP:%s Port:%d", strIP, nPort);

	if (!m_strFirewallIP.IsEmpty())
	{
		eXosip_masquerade_contact(m_pContext_eXosip, m_strFirewallIP, m_nFirewallPort);
		CLog::Log(SIPSERVICE, LL_NORMAL, "SIP Firewall IP:%s Port:%d", m_strFirewallIP, m_nFirewallPort);
	}
	// 添加平台参数，其中EXOSIP_OPT_SET_IPV4_FOR_GATEWAY是为了解决多网卡显示不正确问题
	eXosip_set_user_agent(m_pContext_eXosip, _T("HON-Platform"));
	eXosip_set_option(m_pContext_eXosip, EXOSIP_OPT_SET_IPV4_FOR_GATEWAY, strIP);

	//注册SIP业务处理线程
	RegisterProc(pfnSIPProc, this, 1);

	if (m_nAppWorkMode == 0)
	{
		// 作为下级,注册与HUS业务进程通信的处理线程
		RegisterProc(pfnSharedProc, this, 1);
	}
	else if (m_nAppWorkMode == 2)
	{
		//转发模式，不需要处理HUS业务，所以直接连接到远程的上下级SIP域
		m_RegManager.ConnectToRemoteDomains();
	}
	else if (m_nAppWorkMode == 1)
	{
		//作为上级,需要注册模块消息处理线程,处理HUSMgmtCom模块的Adapter消息
		RegisterProc(pfnSharedProc, this, 1);
	//	RegisterProc(pfnQueueProc, this, 1);
	}

}

void CSIPService::Cleanup()
{
	m_bIsExit = true;
	Sleep(300);
	eXosip_quit(m_pContext_eXosip);
	osip_free(m_pContext_eXosip);

	if (m_pSharedWriteQ)
		delete m_pSharedWriteQ;
	if (m_pSharedReadQ)
		delete m_pSharedReadQ;

}

// CSIPService::读取配置文件
void CSIPService::InitgGlobalConfigInfo(void)
{
	// 本地信息
	auto m_strID = appConf.m_Current.str_ID;
	auto m_strIP = appConf.m_Current.str_IP;
	auto nPort = appConf.m_Current.nPort;

	auto usrName = appConf.m_Current.str_Username;
	auto passWord = appConf.m_Current.str_Password;

	m_RegManager.SetDevInfoConfigPath(appConf.strDevInfoConfPath.GetString());
	//初始化当前域
	m_RegManager.CurrentDomain().Initialize(
		appConf.m_Current.str_ID,
		appConf.m_Current.str_IP,
		nPort,
		passWord.GetString(),
		usrName.GetString());
	CLog::Log(SIPSERVICE, LL_NORMAL, "LocalPLT Info# ID:%s IP:%s Port:%d Username:%s Password:%s", m_strID, m_strIP, nPort, usrName, passWord);

	// 上级平台信息
	auto  strPlatformID = appConf.m_UpperList[0].str_ID;
	auto  strPlatformIP = appConf.m_UpperList[0].str_IP;
	auto  nPlatformPort = appConf.m_UpperList[0].nPort;
	CLog::Log(SIPSERVICE, LL_NORMAL, "UpperPLT Info# ID:%s IP:%s Port:%d", strPlatformID, strPlatformIP, nPlatformPort);

	//添加注册对象,初始化上级平台域
	m_RegManager.AddRegisterObjectInfo(strPlatformID, strPlatformIP, nPlatformPort, SIP_OBJECT_UPPER_DOMAIN);
	m_RegManager.UpperDomain().Initialize(strPlatformID, strPlatformIP, nPlatformPort);

	m_nAppWorkMode = appConf.nSipComMode;

	CString curGateWayType = "GATEWAY_DOWN";
	if (2 == m_nAppWorkMode)
	{
		// 下级平台信息
		strPlatformID = appConf.m_LowerList[0].str_ID;
		strPlatformIP = appConf.m_LowerList[0].str_IP;
		nPlatformPort = appConf.m_LowerList[0].nPort;

		//添加注册对象,初始化下级域
		m_RegManager.AddRegisterObjectInfo(strPlatformID, strPlatformIP, nPlatformPort);
		m_RegManager.LowerDomain().Initialize(strPlatformID, strPlatformIP, nPlatformPort);

		//临时添加转发路由,以后上面的域信息，都会从配置文件读取
		m_RegManager.AddRelayRouting(&m_RegManager.LowerDomain(), &m_RegManager.UpperDomain());
		curGateWayType = "GATEWAY_ROUTE";
	}
	else if (1 == m_nAppWorkMode)
	{
		curGateWayType = "GATEWAY_UP";
	
		if (appConf.m_LowerList.size()>0)
		{
			strPlatformID = appConf.m_LowerList[0].str_ID;
			strPlatformIP = appConf.m_LowerList[0].str_IP;
			nPlatformPort = appConf.m_LowerList[0].nPort;
			m_RegManager.LowerDomain().Initialize(strPlatformID, strPlatformIP, nPlatformPort);

		}
	}
	CLog::Log(SIPSERVICE, LL_NORMAL, "配置文件读取：本地域平台类型 = %d SIP域模式 = %s", appConf.nSipComMode, curGateWayType);

	m_nSharedMemSize = MB2B(appConf.m_SharedMemory.Sipcom_to_App.uint_Size); // 10M

	CLog::Log(SIPSERVICE, LL_NORMAL, "SIP2Server Shared Memory Name:%s", appConf.m_SharedMemory.Sipcom_to_App.str_Name);
	m_pSharedWriteQ = new CSharedVarQueue(appConf.m_SharedMemory.Sipcom_to_App.str_Name);
	m_pSharedWriteQ->Init(m_nSharedMemSize, false);

	m_nSharedMemSize = MB2B(appConf.m_SharedMemory.App_to_Sipcom.uint_Size); // 10M
	CLog::Log(SIPSERVICE, LL_NORMAL, "Server2SIP Shared Memory Name:%s", appConf.m_SharedMemory.App_to_Sipcom.str_Name);
	m_pSharedReadQ = new CSharedVarQueue(appConf.m_SharedMemory.App_to_Sipcom.str_Name);
	m_pSharedReadQ->Init(m_nSharedMemSize, false);

	m_strFirewallIP = appConf.m_Firewall.str_IP;
	m_nFirewallPort = appConf.m_Firewall.nPort;

}

///模块间的通信消息处理，目前暂未使用，正在coding
bool CSIPService::HandleMsg(CMemPoolUnit *   pUnit)
{
	auto pSipPacket = reinterpret_cast<sip_packet_t *>(pUnit);

	auto nRet = (this->*m_pSendFun[pSipPacket->cmd_type])(pSipPacket);

	return nRet > 0 ? true : false;
}


#pragma region send message to 3rd-domain

// 把消息推入缓存队列
UINT CSIPService::pfnSharedProc(LPVOID pParam)
{
	CSIPService * pSIPService = reinterpret_cast<CSIPService*>(pParam);
	int nFlg = 0;
	while (!pSIPService->m_bIsExit && 0 == nFlg)
	{
		nFlg = pSIPService->HandleShared();
	}

	return 0;
}

// 接收共享内存消息
int CSIPService::HandleShared()
{
	char * pData = nullptr;
	sip_packet_t sip_packet;
	if (WAIT_TIMEOUT != WaitForSingleObject(m_pSharedReadQ->GetEventHandle(), 1000))
	{
		while (FALSE == m_bIsExit && m_pSharedReadQ->PopAlloc(reinterpret_cast<void **>(&pData)))
		{
			// 读取共享内存中的内容到sip_packet_t中
			sip_packet = reinterpret_cast<ipc_sip_block_t *>(pData);
			sip_packet.pPlatform = &m_RegManager.UpperDomain();
			sip_packet.pPlatform->SetExpiry(sip_packet.nExpired);

		//	CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SharedMemory Message, Type: %s, CSeq:%d", sip_packet.tHeader.ePackType, sip_packet.nSeq);
			m_pSharedReadQ->PopDone(pData);

			// 发送至三方平台
			SendSIP(&sip_packet);
		}
	}

	return 0;
}

// 发送消息
int CSIPService::SendSIP(sip_packet_t * pSipPacket)
{
	return (this->*m_pSendFun[pSipPacket->cmd_type])(pSipPacket);
}



// 发送会话内请求(INFO)
int CSIPService::SendCallInfoRequest(sip_packet_t * pSipPacket)
{
	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager).
		build_request_message_info_send(
			pSipPacket->nDID,
			pSipPacket->strBody.c_str(),
			pSipPacket->strBody.length());

	return nRet;
}

int CSIPService::SendCallRequest(sip_packet_t * pSipPacket)
{

	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager).
		build_call_request_message_send(
			pSipPacket->nDID,
			pSipPacket->strBody.c_str(),
			pSipPacket->strBody.length());

	if (nRet == -1)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "%s ret = %d failure\r\n", __FUNCTION__);
	}

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s send strBody = %s ret = %d\r\n", __FUNCTION__, pSipPacket->strBody.c_str(), nRet);
	return nRet;
}

// 发送会话内请求(INFO)
int CSIPService::SendCallBye(sip_packet_t * pSipPacket)
{
	if (nullptr == m_pContext_eXosip)
		return CONTEXT_NOTINIT_ERROR;

	eXosip_lock(m_pContext_eXosip);
	eXosip_call_terminate(m_pContext_eXosip, pSipPacket->nCID, pSipPacket->nDID);
	eXosip_unlock(m_pContext_eXosip);
	/*auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager).
		build_call_closed_send(pSipPacket->nCID, pSipPacket->nDID);*/
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s nCid = %d nDid = %d\r\n", __FUNCTION__, pSipPacket->nCID, pSipPacket->nDID);

	return 0;
}


int CSIPService::SendDeviceControl(sip_packet_t * pSipPacket)
{
	if (nullptr == m_pContext_eXosip)
		return CONTEXT_NOTINIT_ERROR;

	if (nullptr == pSipPacket)
		return PTZINFO_EMPTY_ERROR;

	string strFrom = m_RegManager.CurrentDomain().GetSIPFrom();
	string strTo = m_RegManager.LowerDomain().GetSIPTo(pSipPacket->szFromDeviceID);

	char strSN[32];
	_itoa_s(m_unSN, strSN, 10);
	BYTE bPTZCmd[8] = { 0xa5, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
	string strPTZCmd;

	bPTZCmd[3] = pSipPacket->cmd_type;
	bPTZCmd[4] = pSipPacket->work_params.device_control.cData1;
	bPTZCmd[5] = pSipPacket->work_params.device_control.cVerticalPace;
	//高四位有意义，低四位为地址高四位。地址默认为“0x01”
	bPTZCmd[6] = pSipPacket->work_params.device_control.cZoomPace << 4;


	bPTZCmd[7] = (bPTZCmd[0] + bPTZCmd[1] + bPTZCmd[2] + bPTZCmd[3] + bPTZCmd[4] + bPTZCmd[5] + bPTZCmd[6]) % 0xff;
	for (int i = 0; i < 8; i++)
	{
		char strTmp[10];
		sprintf_s(strTmp, "%02X", bPTZCmd[i]);
		strPTZCmd += strTmp;
	}

	std::string strMsg = "<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>";
	strMsg += strSN;
	strMsg += "</SN>\r\n"
		"<DeviceID>";
	strMsg += pSipPacket->szToDeviceID;
	strMsg += "</DeviceID>\r\n";
	if (PTZ_CMD == pSipPacket->work_params.device_control.cZoomAction)
	{
		strMsg += "<PTZCmd>";
		strMsg += strPTZCmd;
		strMsg += "</PTZCmd>\r\n";
	}
	//Add by Rongqian Soong 20150723 -S
	else if (DRAG_ZOOM_NONE < pSipPacket->work_params.device_control.cZoomAction&& pSipPacket->work_params.device_control.cZoomAction < DRAG_ZOOM_NULL)
	{
		std::stringstream ssZoomPart;
		ssZoomPart << "<" << drag_zoom_string(pSipPacket->work_params.device_control.cZoomAction) << ">\r\n";
		ssZoomPart << "<Length>";
		ssZoomPart << pSipPacket->work_params.device_control.nLength;
		ssZoomPart << "</Length>\r\n";
		ssZoomPart << "<Width>";
		ssZoomPart << pSipPacket->work_params.device_control.nWidth;
		ssZoomPart << "</Width>\r\n";
		ssZoomPart << "<MidPointX>";
		ssZoomPart << pSipPacket->work_params.device_control.nMidPointX;
		ssZoomPart << "</MidPointX>\r\n";
		ssZoomPart << "<MidPointY>";
		ssZoomPart << pSipPacket->work_params.device_control.nMidPointY;
		ssZoomPart << "</MidPointY>\r\n";
		ssZoomPart << "<LengthX>";
		ssZoomPart << pSipPacket->work_params.device_control.nLengthX;
		ssZoomPart << "</LengthX>\r\n";
		ssZoomPart << "<LengthY>";
		ssZoomPart << pSipPacket->work_params.device_control.nLengthY;
		ssZoomPart << "<LengthY>\r\n";
		ssZoomPart << "</" << drag_zoom_string(pSipPacket->work_params.device_control.cZoomAction) << ">\r\n";

		strMsg += ssZoomPart.str();
	}
	//Add by Rongqian Soong 20150723 -E
	strMsg += "</Control>\r\n\r\n";

	strcpy_s(pSipPacket->szSN, strSN);
	string callid = "33";

	int  nRet = SendMessages("MESSAGE", strTo.c_str(), strFrom.c_str(), strMsg.c_str(),
		strMsg.length(), "Application/MANSCDP+xml", callid);

	return nRet;
}



// 发送broadcast invite
int CSIPService::SendInviteBroadCast(sip_packet_t * pSipPacket)
{
	const char *strDeviceID = pSipPacket->szToDeviceID;
	char strSubjectBuf[256];
	sprintf_s(strSubjectBuf, "%s:%s,%s:%s", pSipPacket->szFromDeviceID, "0-4-0", m_RegManager.CurrentDomain().GetID().c_str(), "1");

	char strSDPBuf[1024];
	char strOperateType[][10] = { "Play", "Playback", "Download" };
	char strF[][20] = { "0100000001", "1000000001", "1000000001" };
	char strPlayTime[][2][32] = { { "0", "0" },{ "", "" },{ "", "" } };
	char *pstrUserIP = pSipPacket->work_params.video_call.szRecvIP;

	int  nUserPort = pSipPacket->work_params.video_call.nRecvPort[0];
	int startTime = atoi(pSipPacket->work_params.video_call.szStartTime) - 8 * 3600;
	int endTime = atoi(pSipPacket->work_params.video_call.szEndTime) - 8 * 3600;
	char pstrStartTime[100];
	//_itoa(startTime, pstrStartTime, 10);
	char pstrEndTime[100];
	//_itoa(endTime, pstrEndTime, 10);

	if (INVITE_PLAY != pSipPacket->nOperation)
	{
		strncpy_s(strPlayTime[pSipPacket->nOperation][0], pstrStartTime, strlen(pstrStartTime));
		strncpy_s(strPlayTime[pSipPacket->nOperation][1], pstrEndTime, strlen(pstrEndTime));
	}	

	static char strMediaTransMode[][10] = { "RTP/AVP", "tcp" };
	int nMediaTransMode = (0 == pSipPacket->work_params.video_call.nTransType) ? 0 : 1;
	//const  char *pstrSubject = pSipPacket->work_params.video_call.szSubjectID;

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s start invite broadcast peerDeviceId = %s strSubjectBuf = %s", __FUNCTION__, strDeviceID, strSubjectBuf);

	if (nullptr == m_pContext_eXosip)
		return -1;

	//const char *pstrSDP = pSipPacket->strBody.c_str();

	osip_message_t *pInvite = nullptr;
	osip_call_id_t *pCallID = nullptr;

	string strFrom = m_RegManager.CurrentDomain().GetSIPFrom();
	string strTo = m_RegManager.LowerDomain().GetSIPTo(pSipPacket->szFromDeviceID);

	eXosip_lock(m_pContext_eXosip);
	int  nRet = eXosip_call_build_initial_invite(m_pContext_eXosip,
		&pInvite,
		strTo.c_str(),
		strFrom.c_str(),
	//	pSipPacket->pPlatform->GetSIPTo(pSipPacket->szFromDeviceID).c_str(),		//"sip:34020000001320000010@10.10.124.174:12330",	//to
		//m_RegManager.GetSIPFrom().c_str(),		//"sip:scgw@10.10.124.174:5060",				//from
		nullptr,															//route
		strSubjectBuf			//"This is a call for a conversation"			//subject
	);
	if (nRet != 0)
	{
		eXosip_unlock(m_pContext_eXosip);
		return -1;
	}

	pCallID = osip_message_get_call_id(pInvite);
	M_UniqueIDToCallID.emplace(atoi(pCallID->number), pSipPacket->nUniqueID);
	M_SeqIDToCallID.emplace(atoi(pCallID->number), pSipPacket->nSeq);
	sprintf_s(strSDPBuf, "v=0\r\n"
		"o=%s 0 0 IN IP4 %s\r\n"    //reciverID and reciverIP
		"s=%s\r\n"					//mediaType Play, Playback, Download
		"u=%s:1\r\n"
		"c=IN IP4 %s\r\n"			//reciverIP
		"t=%s %s\r\n"				//startTime and endTime of media
		"m=video %d %s 96 98\r\n"//"m=video %d RTP/AVP 96\r\n"	//reciverPort, PS stream payload type is 96
		"a=recvonly\r\n"
		"a=rtpmap:96 PS/90000\r\n"
		"a=rtpmap:98 H264/90000\r\n"
		"y=%s\r\n"
		"f=\r\n\r\n",
		m_RegManager.CurrentDomain().GetID().c_str(),
		pstrUserIP,
		strOperateType[pSipPacket->nOperation],
		strDeviceID,
		pstrUserIP,
		strPlayTime[pSipPacket->nOperation][0],
		strPlayTime[pSipPacket->nOperation][1],
		nUserPort,
		strMediaTransMode[nMediaTransMode],
		strF[pSipPacket->nOperation]);

	osip_message_set_body(pInvite, strSDPBuf, strlen(strSDPBuf));
	osip_message_set_content_type(pInvite, "APPLICATION/SDP");


	nRet = eXosip_call_send_initial_invite(m_pContext_eXosip, pInvite);
	if (nRet < 0)
	{
		eXosip_unlock(m_pContext_eXosip);
		return SEND_INVITE_ERROR;
	}

	eXosip_unlock(m_pContext_eXosip);

	call_sender_t callsender;
	callsender.p_sender = pSipPacket->work_params.video_call.sender_parms.p_sender;
	callsender.n_call_type = ST_CALL_BROADCAST;
	this->m_callIdSenderMap[pCallID->number] = std::move(callsender);

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s start invite broadcast peerDeviceId = %s strSubjectBuf = %s pCallId->number = %s pInviteSender = %08x", __FUNCTION__, strDeviceID, strSubjectBuf, pCallID->number, pSipPacket->work_params.video_call.sender_parms.p_sender);

	return nRet;
}

// 发送broadcast ack
int CSIPService::SendAckBroadCast(sip_packet_t * pSipPacket)
{

	osip_message_t *ack = nullptr;
//	const char *pstrSDP = pSipPacket->strBody.c_str();

	eXosip_lock(m_pContext_eXosip);
	if (0 != eXosip_call_build_ack(m_pContext_eXosip, pSipPacket->nDID, &ack))
	{
		eXosip_unlock(m_pContext_eXosip);
		return BUILD_ACK_ERROR;
	}

	//if (pstrSDP) {
	//	osip_message_set_body(ack, pstrSDP, strlen(pstrSDP));
	//	osip_message_set_content_type(ack, "APPLICATION/SDP");
	//}
	if (eXosip_call_send_ack(m_pContext_eXosip, pSipPacket->nDID, ack))
	{
		eXosip_unlock(m_pContext_eXosip);
	//	g_objLog.LogoutInfo(k_LOG_EXE, "%s向前端发送Ack消息:SEND_ACK_ERROR\n", __FUNCTION__);
		return SEND_ACK_ERROR;
	}
	//auto ret = eXosip_call_send_ack(m_pContext_eXosip, pSipPacket->nTID, ack);
	eXosip_unlock(m_pContext_eXosip);

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s send ack OK", __FUNCTION__);

	return 0;
}

int CSIPService::SendMessages(const char *pstrMsgType, const char *pstrTo, const char *pstrFrom,
	const char *pstrBody, int nBodyLen, const char *pstrBodyType,
	string &strCallID)
{
	if (nullptr == pstrMsgType || nullptr == pstrTo || nullptr == pstrFrom)
		return BODY_INFO_ERROR;

	osip_message_t *message = nullptr;
	eXosip_lock(m_pContext_eXosip);
	eXosip_message_build_request(m_pContext_eXosip,
		&message,
		pstrMsgType,
		pstrTo,
		pstrFrom,
		nullptr);

	int  m_unSeq;

	if (nullptr == message)
	{
		eXosip_unlock(m_pContext_eXosip);
		return BUILD_MESSAGE_ERROR;
	}

	char strSeq[10];
	//itoa(m_unSeq, strSeq, 10);
	osip_message_set_cseq(message, strSeq);

	osip_call_id_t *pCallID = osip_message_get_call_id(message);
	if (pCallID)
		strCallID = pCallID->number;


	if (pstrBody && pstrBodyType && 1 < nBodyLen)
	{
		osip_message_set_body(message, pstrBody, nBodyLen);
		osip_message_set_content_type(message, pstrBodyType);
	}

	eXosip_message_send_request(m_pContext_eXosip, message);
	eXosip_unlock(m_pContext_eXosip);

	return 0;
}


int CSIPService::SendRequestMessage(sip_packet_t * pSipPacket)
{
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s targetid = %s localPLTid = %s\r\n", __FUNCTION__, pSipPacket->szFromDeviceID, m_RegManager.m_CurPlatform.strDeviceID.c_str());

	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager)
		.init_from_to(pSipPacket->szFromDeviceID, pSipPacket->pPlatform)
		.build_new_request_message_send(
			pSipPacket->nSeq,
			pSipPacket->strBody.c_str(),
			pSipPacket->strBody.length());

	return nRet;
}

int CSIPService::SendRequestNotifyMessage(sip_packet_t * pSipPacket)
{

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s g_userInfo.strDeviceId = %s\r\n", __FUNCTION__, pSipPacket->szFromDeviceID, m_RegManager.m_CurPlatform.strDeviceID.c_str());

	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager)
		.init_from_to(pSipPacket->szFromDeviceID, pSipPacket->pPlatform)
		.build_message_notify_send(
			pSipPacket->nSeq,
			pSipPacket->strBody.c_str(),
			pSipPacket->strBody.length());
	return nRet;
}


// 发送Invite的应答指令
int CSIPService::SendInviteCallAnswer(sip_packet_t * pSipPacket)
{
	return SendAnsweredByCode(pSipPacket->nTID, pSipPacket->tHeader.nStatus, "application/sdp", pSipPacket->strBody.c_str());

}


int CSIPService::SendAnsweredByCode(int tid, int nCode, char * strContentType, const char* strbody, size_t lenth) const
{

	auto nRet = SipMessageMaker(m_pContext_eXosip)
		.build_message_answer_send(tid, nCode, strContentType, strbody, lenth);

	return nRet;
}

int CSIPService::SendSubscribeAnswer(sip_packet_t * pSipPacket)
{
	return SendAnsweredByCode(pSipPacket->nTID, pSipPacket->tHeader.nStatus, "Application/MANSCDP+xml", pSipPacket->strBody.c_str());
}



int CSIPService::SendSubScribeTerminateNotify(sip_packet_t * pSipPacket)
{
	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager)
		.init_from_to(pSipPacket->szFromDeviceID, pSipPacket->pPlatform)
		.build_insubscription_notify_send(
			pSipPacket->nTID,
			pSipPacket->nDID,
			pSipPacket->nSeq,
			nullptr,
			pSipPacket->strBody.length(),
			EXOSIP_SUBCRSTATE_TERMINATED, pSipPacket->work_params.subscription.n_ss_reason);

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s  nDid = %d reason = %d\r\n", __FUNCTION__, pSipPacket->nDID, pSipPacket->work_params.subscription.n_ss_reason);
	return nRet;
}

int CSIPService::SendSubScribeNotify(sip_packet_t * pSipPacket)
{
	auto nRet = SipMessageMaker(m_pContext_eXosip, &m_RegManager)
		.init_from_to(pSipPacket->szFromDeviceID, pSipPacket->pPlatform)
		.build_new_request_message_send(
			pSipPacket->nSeq,
			pSipPacket->strBody.c_str(),
			pSipPacket->strBody.length(),
			"NOTIFY");

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s  nDid = %d body = %x\r\n", __FUNCTION__, pSipPacket->nDID, pSipPacket->strBody);
	return nRet;
}


// 向平台注册
int CSIPService::Register(sip_packet_t * pSipPacket)
{

	if (nullptr != pSipPacket->pPlatform)
	{
		auto pPlatformInfo = pSipPacket->pPlatform;

		eXosip_lock(m_pContext_eXosip);

		int nReigsterID = pPlatformInfo->GetRegisgerID();
		if (-1 < nReigsterID)
			eXosip_register_remove(m_pContext_eXosip, nReigsterID);
		osip_message_t *reg = nullptr;

		auto str_sip_from = m_RegManager.GetSIPFrom().c_str();
		auto str_sip_to = pSipPacket->pPlatform->GetSIPTo(pSipPacket->szFromDeviceID);
		nReigsterID = eXosip_register_build_initial_register(
			m_pContext_eXosip,
			str_sip_from,//"sip:34010000002000000001@127.0.0.1:7777",
			str_sip_to.c_str(),//"sip:34020000002000000001@127.0.0.1:5060",
			str_sip_from,//"sip:34010000002000000001@127.0.0.1:7777",
			pPlatformInfo->GetExpiry(), &reg);
		if (nReigsterID < 1) {
			eXosip_unlock(m_pContext_eXosip);
			return -1;
		}
		pPlatformInfo->SetRegisgerID(nReigsterID);

		auto strRegCallID = pPlatformInfo->GetCallID().c_str();
		// 替换CallID
		if ('\0' == strRegCallID[0])
		{
			strRegCallID = osip_call_id_get_number(reg->call_id);
		}
		osip_free(reg->call_id->number);
		osip_call_id_set_number(reg->call_id, osip_strdup(strRegCallID));
		pPlatformInfo->SetCallID(strRegCallID);
		// 替换注册用的CSeq
		SetCSeq(reg, pPlatformInfo->GetStrCSeq().c_str());
		auto register_ret = eXosip_register_send_register(m_pContext_eXosip, nReigsterID, reg);

		eXosip_unlock(m_pContext_eXosip);

		return register_ret;
	}

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s register send failed: no platform info %s ", __FUNCTION__);

	return -1;
}

// 向平台发送心跳
int CSIPService::KeepAlive(sip_packet_t* pSipPacket)
{
	return SendRequestMessage(pSipPacket);
}

// 从平台注销
int CSIPService::Unregister(sip_packet_t * pSipPacket)
{
	if (nullptr == pSipPacket->pPlatform)
	{
		return -1;
	}
	pSipPacket->pPlatform->SetExpiry(0);
	Register(pSipPacket);
	CLog::Log(SIPSERVICE, LL_NORMAL, "SEND Unregister message");
	return 0;
}

// 设置CSeq
int CSIPService::SetCSeq(osip_message_t * pMessage, const char *pszCSeq)
{
	osip_free(pMessage->cseq->number);
	osip_cseq_set_number(pMessage->cseq, osip_strdup(pszCSeq));

	return 0;
}


#pragma endregion


#pragma region handle message from 3rd sip-domain

// 接收SIP消息
UINT CSIPService::pfnSIPProc(LPVOID pParam)
{
	auto pSIPService = reinterpret_cast<CSIPService*>(pParam);

	while (!pSIPService->m_bIsExit)
	{
		pSIPService->HandleSIP();
	}

	return 0;
}


// 处理SIP消息
int CSIPService::HandleSIP()
{
	// 取得线程状态
	// 取得监听到的事件
	m_pEvent = eXosip_event_wait(m_pContext_eXosip, 0, TIMEOUT_EVENT_WAIT);

	if (m_pEvent)
	{
		if (m_pEvent->request)
		{
			eXosip_lock(m_pContext_eXosip);
			eXosip_default_action(m_pContext_eXosip, m_pEvent);
			eXosip_unlock(m_pContext_eXosip);


			//解析当前会话的请求消息
			auto sip_msg_reqst = SipMessage().parser_message(m_pEvent->request);

			// 判断事件类型
			switch (m_pEvent->type)
			{

				// 收到上级域的订阅消息
			case EXOSIP_IN_SUBSCRIPTION_NEW:
			{
				SendAnsweredByCode(m_pEvent->tid, 200);
				if (MSG_IS_SUBSCRIBE(m_pEvent->request))
				{
					if (m_nAppWorkMode == 2)
					{
						m_pSipRelayWorker->relay_subscription(m_pEvent);
						break;
					}

					ipc_sip_block_t sip_block;
					sip_block.tHeader.eWorkType = work_kinds::subscription;
					sip_block.cmd_type = ST_SUBSCRIBE;
					ParseMessageBody(sip_msg_reqst, &sip_block);
					HandleSubscribe(sip_msg_reqst, sip_block);
					CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_IN_SUBSCRIPTION_NEW CSeq:%s TID:%d", sip_msg_reqst.psz_cseq, m_pEvent->tid);
				}
			}
			break;

			//收到下级的notify.
			case EXOSIP_SUBSCRIPTION_NOTIFY:
			{
				if (MSG_IS_NOTIFY(m_pEvent->request))
					SendAnsweredByCode(m_pEvent->tid, 200);
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_SUBSCRIPTION_NOTIFY CSeq:%s TID:%d", sip_msg_reqst.psz_cseq, m_pEvent->tid);

				ipc_sip_block_t sip_block;
				sip_block.tHeader.eWorkType = work_kinds::event_notify;
				sip_block.cmd_type = ST_SUBSCRIBE_NOTIFY;
				ParseMessageBody(sip_msg_reqst, &sip_block);

				//组装模块消息，转发到HUS模块处理


			}
			break;
			// 收到其他域Message请求,需要解析其body xml 才能知道其cmdtype
			case EXOSIP_MESSAGE_NEW:
			{
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_MESSAGE_NEW_MESSAGE CSeq:%s TID:%d", sip_msg_reqst.psz_cseq, m_pEvent->tid);
				//如果是下级域的注册请求
				if (MSG_IS_REGISTER(m_pEvent->request))
				{
					m_RegManager.OnRegister(m_pEvent);
					break;
				}
				if (sip_msg_reqst.is_keep_alive())
				{
					//收到下级域的保活数据包
					return	m_RegManager.OnKeepAlive(m_pEvent);
				}
				if (m_nAppWorkMode == 2)
				{
					m_pSipRelayWorker->relay_message_new(m_pEvent);

					break;
				}
				//其他消息，可能
				ipc_sip_block_t sip_block;
				sip_block.tHeader.eWorkType = work_kinds::default_message;
				sip_block.cmd_type = ST_MESSAGE;
				ParseMessageBody(sip_msg_reqst, &sip_block);
				HandleMessage(sip_msg_reqst, sip_block);

			}
			break;

			// 收到上级域Message的应答
			case EXOSIP_MESSAGE_ANSWERED:
			{
				if (m_nAppWorkMode == 2)
				{
					if (sip_msg_reqst.is_keep_alive())
						break;
					m_pSipRelayWorker->relay_message_answered(m_pEvent);
					break;
				}
				ipc_sip_block_t sip_block;
				sip_block.tHeader.eWorkType = work_kinds::default_message;
				sip_block.cmd_type = ST_MESSAGE_RESPONSE;
				SendDataToShared(sip_msg_reqst, sip_block);
			}
			break;

			// 收到上级域的Invite请求
			case EXOSIP_CALL_INVITE:
				if (m_nAppWorkMode == 2)
				{
					m_pSipRelayWorker->relay_call_invite(m_pEvent);
					break;
				}
				// 返回应答消息
				HandleInvite(sip_msg_reqst);
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_CALL_INVITE CSeq:%s TID:%d DID:%d", sip_msg_reqst.psz_cseq, m_pEvent->tid, m_pEvent->did);

				break;

			case EXOSIP_CALL_ANSWERED:
			
				HandleInviteResponse(sip_msg_reqst);

				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_CALL_ANSWERED CSeq:%s CID:%d DID:%d", sip_msg_reqst.psz_cseq, m_pEvent->cid, m_pEvent->did);

				break;
				// 收到上级域的ACK确认
			case EXOSIP_CALL_ACK:
				if (m_nAppWorkMode == 2)
				{
					m_pSipRelayWorker->relay_call_ack(m_pEvent);
					break;
				}
				// 使用SDK通知EC去请求前端视频
				HandleACK(sip_msg_reqst);
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_CALL_ACK CSeq:%s CID:%d DID:%d", sip_msg_reqst.psz_cseq, m_pEvent->cid, m_pEvent->did);

				break;

				// 收到上级域的会话内Message请求
			case EXOSIP_CALL_MESSAGE_NEW:
			{
				SendAnsweredByCode(m_pEvent->tid, 200);
				// 收到上级域的INFO请求
				if (MSG_IS_INFO(m_pEvent->request))
				{
					if (m_nAppWorkMode == 2)
					{
						m_pSipRelayWorker->relay_message_new(m_pEvent);
						break;
					}
					// 使用SDK通知EC向前端发送控制指令
					HandleInfo(sip_msg_reqst);
					CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_CALL_MESSAGE_NEW_INFO CSeq:%s CID:%d DID:%d", sip_msg_reqst.psz_cseq, m_pEvent->cid, m_pEvent->did);
				}
			}
			break;
			//收到上级域的BYE信令
			case EXOSIP_CALL_CLOSED:
			{
				if (MSG_IS_BYE(m_pEvent->request))
				{
					if (m_nAppWorkMode == 2)
					{
						m_pSipRelayWorker->relay_call_closed(m_pEvent);
						break;
					}
					HandleBye(sip_msg_reqst);
					CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_CALL_CLOSED_BYE CSeq:%s CID:%d DID:%d", sip_msg_reqst.psz_cseq, m_pEvent->cid, m_pEvent->did);
				}

			}
			break;
			// 向上级域注册成功
			case EXOSIP_REGISTRATION_SUCCESS:
				// 校对时间
				// 发送心跳保活信息
				m_RegManager.OnRegisterToSucess(m_pEvent);
				if (m_nAppWorkMode == 2) return 0; //转发模式
				HandleRegSuccess(sip_msg_reqst);
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_REGISTRATION_SUCCESS CSeq:%s", sip_msg_reqst.psz_cseq);
				break;

				// 向上级域注册失败
			case EXOSIP_REGISTRATION_FAILURE:
				// 401错误，再次发送包含鉴权的信息
				HandleRegFail(sip_msg_reqst);
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_REGISTRATION_FAILURE CSeq:%s", sip_msg_reqst.psz_cseq);
				break;

			default:
				CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Unknown Message, CSeq:%s code:%d", sip_msg_reqst.psz_cseq, m_pEvent->type);
				break;
			}
		}
		eXosip_event_free(m_pEvent);
	}
	//else
	//{
	//	CLog::Log(SIPSERVICE, LL_NORMAL, "Sip Domain Initialize Failed in IP :%s  Port:%d", m_RegManager.CurrentDomain().GetIP().c_str(), m_RegManager.CurrentDomain().GetPort());
	//}
	
	return 0;
}


int CSIPService::SendShared(ipc_sip_block_t * pSipPacket, const char * pszBody, const unsigned nBodySize) const
{
	// 计算数据包尺寸
	int nBlockSize = sizeof(ipc_sip_block_t);
	int nExtBodyDataSize = 0;
	if (pszBody)
	{
		nExtBodyDataSize = nBodySize == 0 ? strlen(pszBody) + 1 : nBodySize + 1;
	}
	// 保存数据包尺寸
	pSipPacket->tHeader.ePackType = pack_kinds::received;
	pSipPacket->tHeader.nExtBodySize = nExtBodyDataSize;
	auto nPacketTotalSize = nBlockSize + nExtBodyDataSize;
	pSipPacket->tHeader.nPackSize = nPacketTotalSize;

	char *pBufHeader = nullptr;
	// 向共享内存写入数据
	if (m_pSharedWriteQ->PushAlloc(reinterpret_cast<void**>(&pBufHeader), nBlockSize))
	{
		memcpy_s(pBufHeader, nPacketTotalSize, pSipPacket, nBlockSize);
		// 写入Body数据
		if (nExtBodyDataSize > 0)
			memcpy_s(pBufHeader + nBlockSize, nExtBodyDataSize, pszBody, nExtBodyDataSize);
		m_pSharedWriteQ->PushDone(pBufHeader);

		return 0;
	}
	CLog::Log(SIPSERVICE, LL_NORMAL, "共享内存写入失败 AvailData:%d AvailSpace:%d", m_pSharedWriteQ->m_pHeader->nAvailData, m_pSharedWriteQ->m_pHeader->nAvailSpace);
	return 0;
}


int CSIPService::SendDataToShared(SipMessage &sip_msg, ipc_sip_block_t & transfer_block)
{

	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);
	auto psz_remote_id = sip_msg.psz_from_id;
	if (transfer_block.cmd_type == ST_REGISTER_FAIL ||
		transfer_block.cmd_type == ST_REGISTER_SUCCESS)
	{
		psz_remote_id = sip_msg.psz_req_uri_id;
		transfer_block.tHeader.eWorkType = work_kinds::register_info;
	}

	CLog::Log(SIPSERVICE, LL_NORMAL, "RECV SIP Message, Type: EXOSIP_MESSAGE_ANSWERED CSeq:%s", sip_msg.psz_cseq);

	return SendShared(&transfer_block, sip_msg.psz_body, sip_msg.p_body->length);
}

body_cmd_t CSIPService::ParseMessageBody(SipMessage &sip_msg, ipc_sip_block_t* p_sip_block)
{

	body_cmd_t result_cmd_type;
	assert(p_sip_block != nullptr);
	if (sip_msg.psz_body)
	{

		switch (p_sip_block->tHeader.eWorkType)
		{
		case work_kinds::subscription:

			result_cmd_type.u_cases.e_subscribe = sip_msg.parser_subscription_body(p_sip_block);
			break;
		case work_kinds::event_notify:
		{
			result_cmd_type.u_cases.e_subscribe = sip_msg.parser_subscription_body(p_sip_block);
		}
		break;
		case work_kinds::default_message:
		{
			//里面有response 和request 两类消息
			result_cmd_type = sip_msg.parser_message_body(p_sip_block);
		}
		break;
		default:
			CLog::Log(SIPSERVICE, LL_NORMAL, "%s  no body message in \r\n", __FUNCTION__);
			break;
		}

	}

	return result_cmd_type;
}


// 处理Message请求
int CSIPService::HandleMessage(SipMessage &sip_msg, ipc_sip_block_t& sip_block)
{
	SendAnsweredByCode(m_pEvent->tid, 200);
	if (sip_block.cmd_body.u_cases.e_config == body_cmd_t::config_t::modify_password)
	{
		m_RegManager.OnModifyPassword(m_pEvent, sip_msg.psz_body);
		sip_packet_t sipPacket;
		sipPacket.strBody = sip_msg.psz_body;
		return	this->SendRequestMessage(&sipPacket);
	}
	if (m_nAppWorkMode == 2) return 0;//转发模式直接返回，不需要发送注册消息
	//向共享内存发送消息
	return SendDataToShared(sip_msg, sip_block);
}

// 处理Invite消息
int CSIPService::HandleInvite(SipMessage &sip_msg)
{
	ipc_sip_block_t transfer_block;
	transfer_block.tHeader.eWorkType = work_kinds::video_call;
	transfer_block.cmd_type = ST_CALL_INVITE;

	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);

	auto sdp = sip_msg.parser_sdp_body();
	auto cip = sdp.connect_info.address;
	memcpy(transfer_block.work_params.video_call.szRecvIP, cip.GetString(), IP_LEN);
	auto subjectid = sip_msg.getid_from_subject();
	memcpy(transfer_block.work_params.video_call.szSubjectID, subjectid.c_str(), ID_LEN);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s Get subjectID is : %s", __FUNCTION__, subjectid);


	return SendShared(&transfer_block, sip_msg.psz_body, sip_msg.p_body->length);

}

// 处理ACK消息
int CSIPService::HandleInviteResponse(SipMessage &sip_msg)
{
	ipc_sip_block_t transfer_block;
	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);
	transfer_block.tHeader.eWorkType = work_kinds::video_call;
	transfer_block.cmd_type = ST_CALL_INVITE_RESPONSE;
	// 取得cid和did，用于dialog内操作的对象的匹配
	transfer_block.nCID = m_pEvent->cid;
	transfer_block.nDID = m_pEvent->did;
	transfer_block.nSeq = M_SeqIDToCallID[atoi(sip_msg.psz_call_id)];
	transfer_block.nUniqueID = M_UniqueIDToCallID[atoi(sip_msg.psz_call_id)];
	auto iter = this->m_callIdSenderMap.find(sip_msg.psz_call_id);
	void *pInviteSender = nullptr;
	if (iter != this->m_callIdSenderMap.end())
	{
		pInviteSender = iter->second.p_sender;
		transfer_block.work_params.video_call.sender_parms = iter->second;
	}
	//if (!pInviteSender)
	//{
	//	CLog::Log(SIPSERVICE, LL_NORMAL, "%s  no pInviteSender exception", __FUNCTION__);
	//	return -1;
	//}


	//通过共享内存发送消息
	return	SendShared(&transfer_block, sip_msg.psz_body, sip_msg.p_body->length);

}

// 处理ACK消息
int CSIPService::HandleACK(SipMessage &sip_msg)
{
	ipc_sip_block_t transfer_block;
	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);
	transfer_block.tHeader.eWorkType = work_kinds::video_call;
	transfer_block.cmd_type = ST_CALL_ACK;


	//通过共享内存发送消息

	return	SendShared(&transfer_block, sip_msg.psz_body, sip_msg.p_body->length);

}

// 处理注册成功消息
int CSIPService::HandleRegSuccess(SipMessage &sip_msg)
{
	ipc_sip_block_t sip_block;
	sip_block.tHeader.eWorkType = work_kinds::register_info;
	sip_block.cmd_type = ST_REGISTER_SUCCESS;
	return SendDataToShared(sip_msg, sip_block);
}

// 处理注册失败消息
int CSIPService::HandleRegFail(SipMessage &sip_msg)
{
	sip_packet_t	transfer_block;
	auto	pPlatformInfo = &m_RegManager.UpperDomain();
	auto    strUserName = m_RegManager.CurrentDomain().GetUsername().c_str();
	auto    strID = m_RegManager.CurrentDomain().GetID().c_str();
	osip_message_t	*reg = nullptr;

	if (401 == m_pEvent->response->status_code)
	{
		eXosip_lock(m_pContext_eXosip);
		eXosip_add_authentication_info(m_pContext_eXosip, strUserName, strID, pPlatformInfo->GetPassword().c_str(), "MD5", nullptr);
		if (0 != eXosip_register_build_register(m_pContext_eXosip, m_pEvent->rid, pPlatformInfo->GetExpiry(), &reg))
		{
			eXosip_unlock(m_pContext_eXosip);
			return -1;
		}
		// 二次注册seq累加
		SetCSeq(reg, pPlatformInfo->GetStrCSeq().c_str());

		auto ret = eXosip_register_send_register(m_pContext_eXosip, pPlatformInfo->GetRegisgerID(), reg);

		eXosip_unlock(m_pContext_eXosip);

		return ret;
	}


	if (m_nAppWorkMode == 2) return 0;

	// 未回应答或返回失败应答，
	// 都返回注册失败信息
	ipc_sip_block_t sip_block;
	sip_block.tHeader.eWorkType = work_kinds::register_info;
	sip_block.cmd_type = ST_REGISTER_FAIL;
	SendDataToShared(sip_msg, sip_block);
	return 0;
}

// 处理INFO消息
int CSIPService::HandleInfo(SipMessage &sip_msg)
{
	ipc_sip_block_t transfer_block;
	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);

	transfer_block.tHeader.eWorkType = work_kinds::video_call;
	transfer_block.cmd_type = ST_CALL_INFO;
	//通过共享内存发送消息
	return	SendShared(&transfer_block, sip_msg.psz_body);

}

// 处理播放停止消息
int CSIPService::HandleBye(SipMessage &sip_msg)
{
	ipc_sip_block_t transfer_block;
	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);

	transfer_block.tHeader.eWorkType = work_kinds::video_call;
	transfer_block.cmd_type = ST_CALL_BYE;
	//通过共享内存发送消息

	return	SendShared(&transfer_block, nullptr);

}

int CSIPService::HandleSubscribe(SipMessage &sip_msg, ipc_sip_block_t & transfer_block)
{

	sip_msg.fill_sip_block_base_info(transfer_block);
	fill_sip_block_from_event(transfer_block, m_pEvent);
	transfer_block.tHeader.eWorkType = work_kinds::subscription;
	transfer_block.cmd_type = ST_SUBSCRIBE;
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s  nDid = %d tid = %d\r\n", __FUNCTION__, m_pEvent->did, m_pEvent->tid);
	//向共享内存发送消息
	return	SendShared(&transfer_block, sip_msg.psz_body, sip_msg.p_body->length);

}

void CSIPService::fill_sip_block_from_event(ipc_sip_block_t & transfer_block, eXosip_event_t * pEvent)
{
	transfer_block.nCallDialogID = GetCallDialogID(pEvent);
	transfer_block.nTID = pEvent->tid;
	transfer_block.tHeader.nStatus = pEvent->response->status_code;
}

INT64 CSIPService::GetCallDialogID(eXosip_event_t * pEvent)
{

	INT64 nCallDialogID = 0;
	nCallDialogID = pEvent->cid;
	nCallDialogID = nCallDialogID << 32;
	nCallDialogID = pEvent->did;

	return nCallDialogID;
}

#pragma endregion
