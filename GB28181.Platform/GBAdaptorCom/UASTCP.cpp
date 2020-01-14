#include "StdAfx.h"
#include "UASTCP.h"
#include "SIPConsole.h"
#include "Socket/SocketDataMgr.hpp"
#include "Socket/SmartSocket.h"
#include "GBAdaptorCom/GBAdaptorCom.h"
/*
参数列表
int select(int maxfdp, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
int maxfdp是一个整数值，是指集合中所有文件描述符的范围，即所有文件描述符的最大值加1，不能错！在Windows中这个参数的值无所谓，可以设置不正确。　
fd_set *readfds是指向fd_set结构的指针，这个集合中应该包括文件描述符，我们是要监视这些文件描述符的读变化的，即我们关心是否可以从这些文件中读取数据了，如果这个集合中有一个文件可读，select就会返回一个大于0的值，表示有文件可读，如果没有可读的文件，则根据timeout参数再判断是否超时，若超出timeout的时间，select返回0，若发生错误返回负值。可以传入NULL值，表示不关心任何文件的读变化。
fd_set *writefds是指向fd_set结构的指针，这个集合中应该包括文件描述符，我们是要监视这些文件描述符的写变化的，即我们关心是否可以向这些文件中写入数据了，如果这个集合中有一个文件可写，select就会返回一个大于0的值，表示有文件可写，如果没有可写的文件，则根据timeout参数再判断是否超时，若超出timeout的时间，select返回0，若发生错误返回负值。可以传入NULL值，表示不关心任何文件的写变化。	　　
fd_set *errorfds同上面两个参数的意图，用来监视文件错误异常。	  　　
struct timeval* timeout是select的超时时间，这个参数至关重要，它可以使select处于三种状态：
第一，若将NULL以形参传入，即不传入时间结构，就是将select置于阻塞状态，一定等到监视文件描述符集合中某个文件描述符发生变化为止；
第二，若将时间值设为0秒0毫秒，就变成一个纯粹的非阻塞函数，不管文件描述符是否有变化，都立刻返回继续执行，文件无变化返回0，有变化返回一个正值；
第三，timeout的值大于0，这就是等待的超时时间，即 select在timeout时间内阻塞，超时时间之内有事件到来就返回了，否则在超时后不管怎样一定返回，返回值同上述。
*/

CUASTCP::CUASTCP(void) :m_SocketServer(nullptr)
{
	m_nPort = appConf.m_Current.nPort2;
	m_strIP = appConf.m_Current.str_IP;

	m_SocketServer = new CSmartSocket(m_nPort, m_strIP);
	m_pSocketSession = std::make_shared<SocketSession>();


}


void  CUASTCP::SetContext(GBAdaptorCom* pGBAdaptorCom)
{
	m_pGBAdaptorCom = pGBAdaptorCom;

	m_SocketServer->SetContext(this);

}

int CUASTCP::RunSocketServerProc()
{
	auto sock = m_SocketServer->Startup();
	if (sock == INVALID_SOCKET)
	{
		CLog::Log(GBADAPTORCOM, LL_DEBUG, "%s UASTCP 服务端监听,初始化失败! \n", __FUNCTION__);
	}

	return sock;
}

int CUASTCP::MsgTimeOutCheck()
{
	std::list<std::string> m_vecMsgList;
	static int nCount = 0;
	time_t timCreateTime;
	nCount++;
	if (TIMEOUT_MESSAGE < TIMEOUT_EVENT_WAIT * nCount)
	{
		nCount = 0;
		time(&timCreateTime);
		for (auto itr = m_vecMsgList.begin(); itr != m_vecMsgList.end(); ++itr)
		{
			//if (TIMEOUT_MESSAGE < (timCreateTime - (*itr)->timCreateTime) * 1000)
			//{
			//	//delete (*itr);
			//	m_vecMsgList.erase(itr);
			//	return 0;
			//}
		}

	}
	return NOTFIND_TCPMSG_ERROR;
}


int CUASTCP::GetUniqueSeq()
{

	if (nCSeq < 0)
	{
		nCSeq = 0;
	}
	return nCSeq++;  //发送出去的序号

}


int CUASTCP::RawDataHandleProc(raw_data_t *pData, void* pParam)
{

	auto pCUASTcp = static_cast<CUASTCP*>(pParam);

	MsgTimeOutCheck();

	auto pUnifiedMsg = m_pGBAdaptorCom->m_MemAllocator.AllocModMessage();
	//原始的二进制协议数据包
	//解析数据包，将请求转发到进程内的SipCom模块
	ClassifyAndParserRawData(pData, pUnifiedMsg);

	//	pUnifiedMsg->nSeq = pCUASTcp->GetUniqueSeq();  //发送出去的序号

	CLog::Log(GBADAPTORCOM, LL_NORMAL, "收到HUS的客户端请求，原始类型:%0x\n", __FUNCTION__, pData->nCmdType);

	//发送到第三方
	//auto nRet = (m_pGBAdaptorCom->*(m_pGBAdaptorCom->m_pFwdSipCallFun[pUnifiedMsg->cmd_type]))(pUnifiedMsg);

	nUniqueID++;
	pUnifiedMsg->nUniqueID = nUniqueID;
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	return	m_pSocketSession->SaveSession(pData, pUnifiedMsg->nUniqueID);


}

int CUASTCP::ClassifyAndParserRawData(raw_data_t *pRawData, CModMessage *  pUnifiedMsg)
{

	ASSERT(pRawData != nullptr && pUnifiedMsg != nullptr);


	pUnifiedMsg->tHeader.ePackType = pack_kinds::tosend;  //要发送给第三放平台的sip包

	switch (pRawData->nCmdType)
	{
	case CMD_LOGIN:
	{
		//当前收到的包的表明的工作类型
		pUnifiedMsg->tHeader.eWorkType = work_kinds::register_info;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_REGISTER;
		RawDataPkgParser::ParseRegisterData(pRawData, pUnifiedMsg);
	}
	break;
	case CMD_USEREXIT:
	{
		//当前收到的包的表明的工作类型
		pUnifiedMsg->tHeader.eWorkType = work_kinds::register_info;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_REGISTER;
		pUnifiedMsg->nExpired = 0;  //用户注销
		RawDataPkgParser::ParseRegisterData(pRawData, pUnifiedMsg);
	}
	break;
	case CMD_INVITE:
	{
		//业务类型
		pUnifiedMsg->tHeader.eWorkType = work_kinds::video_call;
		//业务中具体的操作
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_CALL_INVITE;
		//操作中传递的数据格式
		pUnifiedMsg->cmd_body.content_type = body_content_type_t::BT_CALL_SDP;
		//具体数据格式中包含的操作参数
		pUnifiedMsg->cmd_body.u_cases.e_call_sdp = body_cmd_t::call_sdp_t::streaming_realplay_url;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_INVITE;
		RawDataPkgParser::ParseInviteData(pRawData, pUnifiedMsg);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::invite_broadcast);

	}
	break;
	case CMD_CALL_ACK:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::video_call;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_CALL_ACK;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_ACK;
		RawDataPkgParser::ParseAckData(pRawData, pUnifiedMsg);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::broadcast_ack);
	}
	break;
	case CMD_INFO:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::video_call;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_CALL_INFO;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_INFO;

		RawDataPkgParser::ParsePlayBackCtrlData(pRawData, pUnifiedMsg);
	}
	break;
	case CMD_BYE:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::video_call;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_CALL_BYE;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_BYE;
		RawDataPkgParser::ParseByeData(pRawData, pUnifiedMsg);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::video_excepion_bye);
	}
	break;

	case CMD_DECODER_PLAY:
		pUnifiedMsg->tHeader.eWorkType = work_kinds::video_call;
		pUnifiedMsg->cmd_type = pack_cmd_t::ST_CALL_BYE;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_INVITE;
		RawDataPkgParser::ParseDecoderPlayData(pRawData, pUnifiedMsg);
		break;

	case CMD_MESSAGE:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::default_message;
		//pUnifiedMsg->cmd_type = pack_cmd_t::ST_VIDEO_PTZ;
		pUnifiedMsg->work_params.video_call.sender_parms.n_call_type = ST_CALL_MESSAGE;
		//message 分为:BT_QUERY,	BT_CONTROL,BT_CONTROL_ALARM,
		RawDataPkgParser::OnMessageParseProc(pRawData, pUnifiedMsg);
	}
	case CMD_MESSAGERES:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::default_message;
		pUnifiedMsg->cmd_body.content_type = body_content_type_t::BT_RESPONSE;
	}
	break;
	case CMD_RESET_ALARM:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::default_message;
		pUnifiedMsg->cmd_body.content_type = body_content_type_t::BT_CONTROL_ALARM;
		pUnifiedMsg->cmd_body.u_cases.e_alarm = body_cmd_t::alarm_t::alarm_reset_alarm;
	}
	break;
	case CMD_SUBSCRIBE:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::subscription;
		RawDataPkgParser::OnSubscribeParseProc(pRawData, pUnifiedMsg);
	}
	break;
	case CMD_NOTIFY:
	{
		pUnifiedMsg->tHeader.eWorkType = work_kinds::event_notify;
		pUnifiedMsg->cmd_body.content_type = body_content_type_t::BT_NOTIFY;
	}
	break;
	case CMD_PRIVATE:
		RawDataPkgParser::OnPrivateMessageProc(pRawData, pUnifiedMsg);
		break;
	default:
		break;
	}

	return 0;

}


//用来找出回复到哪个socket上
SOCKET  CUASTCP::MatchRequest(int nCseq)
{
	if (m_pSocketSession)
	{
		CLog::Log(GBADAPTORCOM, LL_NORMAL, "未找到，对应的客户端请求.\n", __FUNCTION__);

		return	m_pSocketSession->GetSessionSocket(nCseq);
	}
	return INVALID_SOCKET;
}


int CUASTCP::SendSocketMessage(RAW_DATA_PKG_PTR pData)
{

	ASSERT(pData != nullptr);

	return	m_SocketServer->SendBytestoSocket(pData->socketClient, pData->pDataBuf, pData->nLastPos);

}


#pragma region 收到来自下级sip域的回复,将回复转发给HUS

//当HUS作为GB客户端登陆时候的逻辑
int CUASTCP::OnRegisterRes(CModMessage* pUnifiedMsg)
{

	if (pUnifiedMsg->cmd_type == ST_REGISTER && pUnifiedMsg->nExpired == 0)
	{
		ReplyHusRegister(pUnifiedMsg->nSeq, pUnifiedMsg->tHeader.nStatus, 200);
	}

	ReplyHusCatalogQuery(pUnifiedMsg->nSeq, pUnifiedMsg->tHeader.nStatus, nullptr, nullptr);

	pUnifiedMsg->Free();
	return 0;
}

int CUASTCP::OnMessageRes(CModMessage* pUnifiedMsg)
{

	//auto p_raw_data = m_SeqRawData[pUnifiedMsg->nSeq];

	auto msg = RawDataPkgmaker::BuildMsgResWithExAnswer(pUnifiedMsg->nSeq, pUnifiedMsg->tHeader.nStatus, 0, nullptr);
	UINT unSeq;
	int nStatus = 0;



	//Sendto();

	return nStatus;
}

//收到下级的200OK及其携带的SDP.
int CUASTCP::OnHusInviteRes(CModMessage * pUnifiedMsg)
{

	ASSERT(pUnifiedMsg != nullptr);

	auto pExtendData = pUnifiedMsg->strBody.c_str();
	//	auto pExtendData = pUnifiedMsg->work_params.video_call.pPlayInfo;
	auto status = pUnifiedMsg->tHeader.nStatus;

	ReplyHusInvite(pUnifiedMsg->nUniqueID, pUnifiedMsg->nSeq, pUnifiedMsg->nCID, pUnifiedMsg->nDID, status, pExtendData);

	pUnifiedMsg->Free();

	return 0;
}

int CUASTCP::OnHusCallAckRes(CModMessage* pUnifiedMsg)
{
	auto pData = MatchRequest(pUnifiedMsg->nSeq);
	if (pData)
	{
		int nDID = 0;
		ReplyHusCallAck(nDID);
		CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s 发送ACK, did:%d\n", __FUNCTION__, nDID);
	}
	return 0;
}

//视频会话中的消息
int CUASTCP::OnCallMessage(CModMessage* pUnifiedMsg)
{
	auto pData = MatchRequest(pUnifiedMsg->nSeq);
	if (pData)
	{
		int nDID = 0;
		ReplyHusCallAck(nDID);
		CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s 发送ACK, did:%d\n", __FUNCTION__, nDID);
	}
	return 0;
}



int CUASTCP::OnsByeRes(CModMessage * pUnifiedMsg)
{
	throw std::logic_error("The method or operation is not implemented.");


}

//比拖动回放等，下级回复了200OK.
int CUASTCP::OnPlayBackCtrlRes(CModMessage*  pUnifiedMsg)
{
	auto pData = MatchRequest(pUnifiedMsg->nSeq);

	//auto pParamInfo = RawDataPkgParser::ParsePlayBackCtrlData(nullptr,pData);

	CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s nCid:%d, nDid:%d\n", __FUNCTION__, pUnifiedMsg->nCID, pUnifiedMsg->nDID);

	return 0;
}


int CUASTCP::OnSubscribeRes(CModMessage* pUnifiedMsg)
{

	switch (pUnifiedMsg->cmd_body.u_cases.e_subscribe)
	{
	case body_cmd_t::subscribe_t::subscribe_catalog:
		//发送目录消息给上级
		ReplyHusCatalogQuery(pUnifiedMsg->nSeq, pUnifiedMsg->tHeader.nStatus, nullptr, nullptr);

		break;
	case  body_cmd_t::subscribe_t::subscribe_alarm:
	{

	}
	break;
	default:
		break;
	}



	return 0;

}

#pragma endregion 收到来自下级sip域的回复

#pragma region 回复消息给到GBAdaptor

int CUASTCP::SendHusMsgResWithEx(int nSeq, int nStatus, int nOperate, const char* pExtendData)
{

	int nExterndSize = strlen(pExtendData) + 1;
	int nBufSize = nExterndSize + 128;
	char szDeviceCatalogBuf[LEN_SEND_RESPONCE * 2];
	//memset(szDeviceCatalogBuf,0,nBufSize);
	int nCmdID = CMD_MESSAGERES;

	if (pExtendData)
	{
		//Sendto(pParam->socUser, szDeviceCatalogBuf, dataStream.GetLastPos());
	}

	return 0;
}

int CUASTCP::SendHusAlarmNotify(int nSeq, int nStatus, int nOperate, const char* strDstID, const char* pAlarmContent)
{
	CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s 发送报警信息\n", __FUNCTION__);



	usertcp_t user;
	user.nType = EC_DEVICE_TYPE;					//找到EC的socket
	user.socUser = INVALID_SOCKET;
	//m_pUsers->GetUserInfo(user);


	int nRet = 0;
	if (user.socUser)
		//nRet = Sendto(user.socUser, szAlarmBuf, dataStream.GetLastPos());
		CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s 发送报警信息,EC未登陆\n", __FUNCTION__);
	//delete message info
	//DeleteMsgFormList(pAlarmData);
	//delete szAlarmBuf;
	return nRet;
	//AddMsgToList(pAlarmData);

}



int CUASTCP::ReplyHusCatalogQuery(int nSeq, int nStatus, const char* szFromDeviceID, void* pParams, int * pCount)
{

	auto socket = MatchRequest(nSeq);

	if (socket == INVALID_SOCKET) return -1;

	//发送目录消息给上级

	auto pRawData = RawDataPkgmaker::BuildCatalogInfo(nSeq, nStatus, szFromDeviceID, nullptr, nullptr);
	pRawData->socketClient = socket;

	CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s 发送%d个已登记设备ID\n", __FUNCTION__, *pCount);
	return SendSocketMessage(pRawData);
}

int CUASTCP::OnSubscribeNotify(CModMessage * pUnifiedMsg)
{
	throw std::logic_error("The method or operation is not implemented.");
}

int CUASTCP::SendHusCatalogNotify(int nSeq, int nStatus, int nOperate, const char* pExtendData)
{


	//if (NULL == pNotifyData->pExtendData)
	//{
	//	return 0;
	//}
	//message_package_t * pParam = pNotifyData;
	//if (pParam)
	//	pNotifyData->unSeq = pParam->unSeq;
	//else
	//{
	//	if (pNotifyData)
	//		delete pNotifyData;
	//	return 0;
	//}

	//char szDeviceCatalogBuf[LEN_SEND_RESPONCE];
	//int nCmdID = CMD_NOTIFY;
	//int nOperate = pNotifyData->nOperate;
	//int nExterndSize = strlen(pNotifyData->pExtendData) + 1;


	//CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s nOperate:%d, nExterndSize:%d, pExtendDataIP:%s\n", __FUNCTION__, nOperate, nExterndSize, pNotifyData->pExtendData);

	//if (pParam)
	//{
	//	//循环发送给登陆客户
	//	if (pParam->socUser == 0)
	//	{
	//		vector<usertcp_t*>* ret = new vector<usertcp_t *>();// = m_pUsers->GetUserInfos();
	//		for (vector<usertcp_t*>::iterator it = ret->begin(); it != ret->end(); ++it)
	//		{
	//			usertcp_t* info = *it;
	//			if (info->nType == NVR_DEVICE_TYPE)
	//			{
	//				//Sendto(info->socUser, szDeviceCatalogBuf, dataStream.GetLastPos());
	//			}
	//			else if (info->nType == EC_DEVICE_TYPE)
	//			{
	//				//Sendto(info->socUser, szDeviceCatalogBuf, dataStream.GetLastPos());
	//			}
	//		}
	//	}
	//	else if (pParam->socUser > 0)
	//		//Sendto(pParam->socUser, szDeviceCatalogBuf, dataStream.GetLastPos());
	//	//else
	//		return 0;
	//}
	return 1;
}


int CUASTCP::SendHusMediaStatusNotify(int nSeq, int nStatus, const char*strDstID, const char*strDstSubID, int nCid, int nDid)
{

	char strPlayBackInfo[LEN_SEND_RESPONCE];
	int nCmdID = CMD_MESSAGE;
	int nOperate = CALL_MSG_FILE_TOEND;


	int nCount = 0; // = m_pUsers->GetUserCount();
	for (int i = 0; i < nCount; i++)
	{
		SOCKET socUser = 100;
		//Sendto(socUser, strPlayBackInfo, dataStream.GetLastPos());
	}
	//CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s dstID:%s, subDstID:%s, cid:%d, did:%d\n", __FUNCTION__, pFileData->strDstID, pFileData->strDstSubID, pFileData->nCid, pFileData->nDid);


	return 0;
}

#pragma endregion 回复消息给到GBAdaptor



//send response of inviteMsg 
int CUASTCP::ReplyHusInvite(int nUniqueID, int nSeq, int nCid, int nDid, int nMsgStatus, const void* pExtendData)
{
	auto socket = MatchRequest(nUniqueID);

	auto pExtendDataMore = reinterpret_cast<const char*>(pExtendData);



	auto pData = RawDataPkgmaker::BuildInviteAnswer(nSeq, nCid, nDid, nMsgStatus, pExtendDataMore);

	pData->socketClient = socket;

	SendSocketMessage(pData);

	//BuildAnswerInvite
	//CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s CallID %s, DataSize:%d, Data:%s\n", __FUNCTION__, pInvitData->strCallID.c_str(), pDataSize, pInvitData->pExtendData);

	return 0;
}

int CUASTCP::ReplyHusCallAck(void)
{
	return 0;
}

int CUASTCP::ReplyHusCallAck(int nDID)
{
	return 0;
}

int CUASTCP::ReplyHusClientLogin(int nSeq, int nStatus, int nLoginSum, const char* strSrcID, int nCmdID)
{
	return 0;
}



int CUASTCP::ReplyHusRegister(int nSeq, int nStatus, int nRegStatus)
{
	auto socket = MatchRequest(nSeq);

	if (socket == INVALID_SOCKET) return -1;

	auto pRawData2Send = RawDataPkgmaker::BuildRegisterAnswer(nSeq, nStatus, nRegStatus);
	pRawData2Send->socketClient = socket;
	return SendSocketMessage(pRawData2Send);

}

int CUASTCP::ReplyHusRemoteStartup(int nSeq, int nStatus, int nOperate)
{
	char strRemoteStartup[LEN_SEND_RESPONCE] = { 0 };
	int nCmdID = CMD_MESSAGERES;



	//Sendto(pRemoteStartup->socUser, strRemoteStartup, dataStream.GetLastPos());

	return 0;
}

int CUASTCP::ReplyHusDeviceCatalogQuery(int nSeq, int nStatus, const char* pExterndData, const char* szFromDeviceID)
{
	auto  rawdata = RawDataPkgmaker::BuildCatalogInfo(nSeq, 200, szFromDeviceID, nullptr, nullptr);
	//	int nRet = SendMsgResWithEx(pCatalogData, MSG_CATALOG_INQUIRE_RES);
	//CLog::Log(GBADAPTORCOM, LL_NORMAL, "%s Catalog XML:%s\n\n", __FUNCTION__, pCatalogData->pExtendData);

	return 0;
}


int CUASTCP::ReplyHusRecordInfoQuery(int nSeq, int nStatus, const char* pExterndData)
{
	int nRet = SendHusMsgResWithEx(nSeq, nStatus, MSG_RECORD_INQUIRE_RES, pExterndData);

	return nRet;
}


int CUASTCP::ReplyHusSetGuard(int nSeq, int nStatus, const char* pExterndData)
{
	/*int nRet = SendMsgResWithEx(pSetData, MSG_SET_GUARD_RES);
	if (pSetData && pSetData->pExtendData)
	delete pSetData;*/
	return 0;
}

int CUASTCP::ReplyHusDeviceInfoQuery(int nSeq, int nStatus, const char* pExterndData)
{
	/*int nRet = SendMsgResWithEx(pDeviceInfoData, MSG_INFO_INQUIRE_RES);
	if (pDeviceInfoData && pDeviceInfoData->pExtendData)
	delete pDeviceInfoData;

	return nRet;*/
	return 0;
}

int CUASTCP::ReplyDeviceControl(int nSeq, int nStatus, int nOperate)
{
	return 0;
}



int CUASTCP::ReplyHusAlarmQuery(int nSeq, int nStatus, const char* pExtendData)
{
	return 0;
}

int CUASTCP::ReplyHusDeviceStatusQuery(int nSeq, int nStatus, const char* pExterndData)
{
	/*int nRet = SendMsgResWithEx(pDeviceStutsData, MSG_STATUS_INQUIRE_RES);
	if (pDeviceStutsData && pDeviceStutsData->pExtendData)
	delete pDeviceStutsData;
	*/
	return 0;
}

int CUASTCP::ReplyHusPlayBackCtrl(int nSeq, int nStatus)
{
	char strRemoteStartup[LEN_SEND_RESPONCE];
	int nCmdID = CMD_MESSAGERES;



	//Sendto(pPlayBackCtrl->socUser, strRemoteStartup, dataStream.GetLastPos());

	return 0;
}


int CUASTCP::ReplyHusSetRecord(int nSeq, int nStatus, const char * pExterndData)
{
	return 0;
}

int CUASTCP::ReplyHusRealPlayUrlQuery(int nSeq, const char *strPlayUrl)
{
	char strRemoteStartup[LEN_SEND_RESPONCE];
	int nCmdID = CMD_MESSAGERES;
	//pRealPlayUrlData->nPlayUrlSize = strlen(pRealPlayUrlData->strPlayUrl);


	//Sendto(pRealPlayUrlData->socUser, strRemoteStartup, dataStream.GetLastPos());

	return 0;
}


