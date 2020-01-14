#include "StdAfx.h"
#include "GBAdaptorCom.h"
#include "UASTCP.h"
#include "ServerConsole.h"
#include "Main/UnifiedMessage.h"
GBAdaptorCom::GBAdaptorCom() :pUASTCPServer(nullptr)
{
}


void GBAdaptorCom::InitgGlobalConfigInfo(void)
{
	m_EndPoint.str_ID = appConf.m_Current.str_ID;
	m_EndPoint.str_IP = appConf.m_Current.str_IP;
	m_EndPoint.str_Password = appConf.m_UpperList[0].str_Password;
	m_EndPoint.nPort2 = appConf.m_Current.nPort2;
}


void GBAdaptorCom::Init()
{
	if (appConf.nSipComMode != 0)
	{
		InitgGlobalConfigInfo();
	
		pUASTCPServer = new CUASTCP();	  // com with HUS GBAdaptor.
		pUASTCPServer->SetContext(this);

		// 注册Module消息处理函数
		RegisterProc(pfnQueueProc, this, 1);

		//处理来自GBAdaptor端的Socket消息处理
		RegisterProc(pfnUASPProc, this, 1);

	}
}

void GBAdaptorCom::Cleanup()
{
	if (pUASTCPServer != nullptr)
	{
		delete pUASTCPServer;
	}

}


UINT GBAdaptorCom::pfnUASPProc(LPVOID pParam)
{
	///这里解析RawData基础分类
	auto  pGBAdaptorCom = reinterpret_cast<GBAdaptorCom*>(pParam);
	auto  pUASTCPSrvEnd = pGBAdaptorCom->pUASTCPServer;
	//while (!pGBAdaptorCom->m_bIsExit)
	//{
		pUASTCPSrvEnd->RunSocketServerProc();
	//}
	return 0;
}




//处理来自其他SipCom回复，转发回复消息给GBAdaptor
bool GBAdaptorCom::HandleMsg(CMemPoolUnit *  pUnit)
{
	auto pUnifiedMsg = reinterpret_cast<CModMessage *>(pUnit);

	auto eOperateType = pUnifiedMsg->GetModAction();

	//tExportHeader.tHeader.ePackType = pack_kinds::tosend;
	CLog::Log(SIPCOM, LL_NORMAL, "%s remoteId = %s operateType = %d\r\n", __FUNCTION__, pUnifiedMsg->GetRemoteID(), eOperateType);

	switch (eOperateType.action_gb_adaptor)
	{

	case mod_op_t::ot_gb_adaptor::register_response:


		break;
	case mod_op_t::ot_gb_adaptor::message_response:
		ProcMessageResponse(pUnifiedMsg);
		break;
	case 	mod_op_t::ot_gb_adaptor::call_invite_response:
		ProcInviteResponse(pUnifiedMsg);
		break;			// Answer of Invite
	case 	mod_op_t::ot_gb_adaptor::call_message:
		ProcCallMessage(pUnifiedMsg);
		break;		// such as MediaStatus
	case 	mod_op_t::ot_gb_adaptor::call_bye_response:
		ProcCallByeResponse(pUnifiedMsg);
		break;
		/*subscribe*/
	case 	mod_op_t::ot_gb_adaptor::subscribe_response:
		ProcSubscribeResponse(pUnifiedMsg);
		break;              // Answer of Subscribe
	case 	mod_op_t::ot_gb_adaptor::subscribe_notify:
		ProcSubscribeNotify(pUnifiedMsg);
		break;                  //message notify
	default:
		break;
	}

	CLog::Log(SIPCOM, LL_NORMAL, "%s remoteId = %s CmdType = %d\r\n", __FUNCTION__, pUnifiedMsg->szFromDeviceID, pUnifiedMsg->cmd_type);

	return false;
}

int GBAdaptorCom::ProcRegisterResponse(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnRegisterRes(pUnifiedMsg);
}


int GBAdaptorCom::ProcMessageResponse(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnMessageRes(pUnifiedMsg);
}

int GBAdaptorCom::ProcInviteResponse(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnHusInviteRes(pUnifiedMsg);

}

int GBAdaptorCom::ProcCallMessage(CModMessage * pUnifiedMsg)
{
	return 	pUASTCPServer->OnCallMessage(pUnifiedMsg);
}

int GBAdaptorCom::ProcCallByeResponse(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnsByeRes(pUnifiedMsg);
}

int GBAdaptorCom::ProcSubscribeResponse(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnSubscribeRes(pUnifiedMsg);
}

int GBAdaptorCom::ProcSubscribeNotify(CModMessage * pUnifiedMsg)
{
	return pUASTCPServer->OnSubscribeNotify(pUnifiedMsg);
}
