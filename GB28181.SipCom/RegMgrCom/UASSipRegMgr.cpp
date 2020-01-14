#include "StdAfx.h"
#include "UASSipRegMgr.h"
#include "Log/Log.h"
#include "Main/MainThread.h"
#include "GBUtils.h"
#include "tinyXML/tinyxml2.h"
#include <sys/timeb.h>
#include "SipServiceCom/SIPService.h"
#include <osipparser2/osip_md5.h>


SipRegMgr::SipRegMgr(CSIPService *pSipServiceWorker) :
	m_pContext_eXosip(nullptr),
	m_pSipWorker(pSipServiceWorker)
{

}



void SipRegMgr::InitSipContext(eXosip_t* pSipContext)
{
	m_pContext_eXosip = pSipContext;

	//同时初始化远程域信息
	InitAllRemoteDomain();
}

void SipRegMgr::SetDevInfoConfigPath(std::string path)
{
	this->m_strConfigpath = path;
}

CSIPService * SipRegMgr::GetSipServiceWorker()
{
	return this->m_pSipWorker;
}

bool SipRegMgr::GetRegisterObjectInfo(std::string& id, SIP_REGISTER_OBJECT_T** ppRegisterObjInfo)
{
	if (m_RegisterObjects[id].strDeviceID.empty()) return false;
	*ppRegisterObjInfo = &m_RegisterObjects[id];
	return true;
}

bool SipRegMgr::ModifyDomainPasswdById(std::string& id, std::string& newPassWord)
{
	if (m_RegisterObjects[id].strDeviceID.empty()) return false;
	m_RegisterObjects[id].strPassWord = newPassWord;
	return true;
}

bool SipRegMgr::CheckRegMsgHeader(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	if (!CheckMsgHeader(m_pEvent, registeringObject))
	{
		SendAnsweredByCode(m_pEvent->tid, 403);
		return false;
	}
	osip_header_t * dest = nullptr;
	int i = 0;
	osip_message_get_header(m_pEvent->request, i, &dest);

	if (nullptr == dest)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "无效的osip_header_t头字段 %s", __FUNCTION__);
		return false;
	}
	auto b_find_expires = false;
	while (dest)
	{
		i++;
		if (0 == strcmp(dest->hname, "expires"))
		{
			b_find_expires = true;
			break;
		}
		osip_message_get_header(m_pEvent->request, i, &dest);
	}

	if (!b_find_expires)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "未找到expires 字段 %s", __FUNCTION__);
		return false;
	}

	registeringObject->nExpires = std::stoi(dest->hvalue);;

	if (registeringObject->nExpires == 0)
	{
		registeringObject->nStatus = STATUS_REGISTER_EXIT;
	}

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s ip = %s port = %d expire = %d \r\n", __FUNCTION__, registeringObject->strDeviceID.c_str(), registeringObject->strIP.c_str(), registeringObject->nPort, registeringObject->nExpires);
	return true;
}

bool SipRegMgr::CheckMsgHeader_FromTo(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{

	auto pFrom = osip_message_get_from(m_pEvent->request);
	if (nullptr == pFrom)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "未找到From头字段 %s", __FUNCTION__);
		return false;
	}
	auto pFromUrl = osip_from_get_url(pFrom);
	if (nullptr == pFromUrl || nullptr == pFromUrl->username)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "无效的from_url %s", __FUNCTION__);
		return false;
	}
	registeringObject->strDeviceID = pFromUrl->username;
	return true;

}
bool SipRegMgr::CheckMsgHeader_Contact(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{

	osip_contact_t * pDest = nullptr;
	osip_message_get_contact(m_pEvent->request, 0, &pDest);
	if (nullptr == pDest)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "未找到contact头字段 %s", __FUNCTION__);
		return false;
	}
	auto pContactUrl = osip_contact_get_url(pDest);
	if (nullptr == pContactUrl || nullptr == pContactUrl->username || nullptr == pContactUrl->host || nullptr == pContactUrl->port)
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "contact_url中,无效的uri %s", __FUNCTION__);
		return false;
	}
	registeringObject->strDeviceID = pContactUrl->username;
	registeringObject->strIP = pContactUrl->host;
	registeringObject->nPort = std::stoi(pContactUrl->port);
	return true;
}

bool SipRegMgr::CheckMsgHeader(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject, bool with_contact)
{

	if (!CheckMsgHeader_FromTo(m_pEvent, registeringObject))
	{
		return false;
	}
	if (with_contact)
	{
		return  CheckMsgHeader_Contact(m_pEvent, registeringObject);
	}


	return true;
}

bool SipRegMgr::CheckRegAuthentication(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	osip_authorization_t *pAuthInfo = nullptr;
	osip_message_get_authorization(m_pEvent->request, 0, &pAuthInfo);
	if (!pAuthInfo)
	{
		SendRegAnsweredByCode(m_pEvent->tid, 401, registeringObject->nExpires);
		CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s ip = %s port = %d expire = %d pAuthInfo = %08x resend 401\r\n", __FUNCTION__, registeringObject->strDeviceID.c_str(), registeringObject->strIP.c_str(), registeringObject->nPort, registeringObject->nExpires, pAuthInfo);
		return false;
	}
	auto strQop = GetAuthorizationMessage_qop(pAuthInfo);

	auto strUser = GetAuthorizationUsername(pAuthInfo);
	auto strRealm = GetAuthorizationRealm(pAuthInfo);
	auto strCNonce = GetAuthorizationCnonce(pAuthInfo);
	auto strDigestUri = GetAuthorizationUri(pAuthInfo);
	auto strNonce = GetAuthorizationNonce(pAuthInfo);

	HASHHEX HA1;
	HASHHEX HA2 = "";
	HASHHEX Response;
	DigestCalcHA1("MD5", strUser.c_str(), strRealm.c_str(), registeringObject->strPassWord.c_str(), strNonce.c_str(), strCNonce.c_str(), HA1);
	DigestCalcResponse(static_cast<char *>(HA1), strNonce.c_str(), "00000001", strCNonce.c_str(), strQop.c_str(), 0, "REGISTER", strDigestUri.c_str(), HA2, Response);

	std::string strResponse = Response;

	return 0 == strcmp(strResponse.c_str(), Response);

}

int SipRegMgr::DealStatus_Send401(void *p_eXoSipEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	auto m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);

	registeringObject->nStatus = STATUS_SENT_401_UNAUTHORIZED;
	SendRegAnsweredByCode(m_pEvent->tid, 401, registeringObject->nExpires);
	//SendRegAnswer401(m_pEvent->tid);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s ip = %s port = %d send 401\r\n", __FUNCTION__, registeringObject->strDeviceID.c_str(), registeringObject->strIP.c_str(), registeringObject->nPort);

	return true;
}

void SipRegMgr::DealStatus_Sent401(void *p_eXoSipEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	auto m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);

	if (CheckRegAuthentication(m_pEvent, registeringObject))
	{
		this->m_RegisterObjects[registeringObject->strDeviceID].nStatus = STATUS_REGISTER_SUCCESS;
		m_CurPlatform = *registeringObject;
		SendRegAnsweredByCode(m_pEvent->tid, 200, registeringObject->nExpires);
	}
	else
	{
		this->m_RegisterObjects[registeringObject->strDeviceID].nStatus = STATUS_SENDT_403_FORBIDDEN;
		SendRegAnsweredByCode(m_pEvent->tid, 403, registeringObject->nExpires);

	}
	//有授权信息
}

void SipRegMgr::DealStatus_DefaultRegister(void *p_eXoSipEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	eXosip_event_t *m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);

	auto final_status = STATUS_REGISTER_SUCCESS;
	if (registeringObject->nStatus == STATUS_REGISTER_EXIT)
	{
		final_status = STATUS_REGISTER_EXIT;
	}

	this->m_RegisterObjects[registeringObject->strDeviceID].nStatus = final_status;
	m_CurPlatform = *registeringObject;
	SendRegAnsweredByCode(m_pEvent->tid, 200, registeringObject->nExpires);

	return;
}

void SipRegMgr::DealStatus_Send403(void *p_eXoSipEvent, SIP_REGISTER_OBJECT_T *registeringObject)
{
	auto m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);

	registeringObject->nStatus = STATUS_NOT_REGISTER;
	//this->m_RegisterObjects[registeringObject->strDeviceID] = *registeringObject;
	SendRegAnsweredByCode(m_pEvent->tid, 403, registeringObject->nExpires);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s ip = %s port = %d send 401\r\n", __FUNCTION__, registeringObject->strDeviceID.c_str(), registeringObject->strIP.c_str(), registeringObject->nPort);


}

void SipRegMgr::AddRegisterObjectInfo(const char * str_DeviceID, const char * str_IP, const unsigned n_Port, unsigned int ntype, const char * str_Password, const char * str_username)
{

	//FOR TEST
	SIP_PLT_INFO_T m_DomainInfo;
	m_DomainInfo.strDeviceID = str_DeviceID;
	m_DomainInfo.nType = ntype;
	m_DomainInfo.strIP = str_IP;
	m_DomainInfo.nPort = n_Port;
	m_DomainInfo.strPassWord = "12345678"; //默认值
	if (str_Password)
	{
		m_DomainInfo.strPassWord = str_Password;
	}
	m_DomainInfo.userName = str_DeviceID;
	if (str_username)
	{
		m_DomainInfo.userName = str_username;

	}
	AddRegisterObjectInfo(m_DomainInfo);

}

void SipRegMgr::AddRegisterObjectInfo(SIP_PLT_INFO_T domain_info)
{
	m_RegisterObjects[domain_info.strDeviceID] = domain_info;

}

void SipRegMgr::InitAllRemoteDomain()
{

	if (m_RegisterObjects.size() <= 0) return;

	std::list<std::string> domain2remove;
	for (auto & item : m_RegisterObjects)
	{
		bool b_remove = false;
		switch (item.second.nType)
		{
		case  SIP_OBJECT_CURRENT_DOMAIN:
		case  SIP_OBJECT_LOWER_DOMAIN:
			m_RemoteDomains.emplace(item.first, std::move(item.second));
			b_remove = true;
			break;

		case  SIP_OBJECT_UPPER_DOMAIN:
			m_RemoteDomains.emplace(item.first, std::move(item.second));
			m_RemoteDomains[item.first].SetNeedConected(true);
			b_remove = true;
		default:
			break;
		}
		if (b_remove)
		{
			domain2remove.emplace_back(item.first);
		}

	}
	if (domain2remove.size() == 0) return;

	//for (auto & item : domain2remove)
	//{
	//	m_RegisterObjects.erase(item);
	//}

}

void SipRegMgr::AddRelayRouting(CSipDomain * domianNode1, CSipDomain * domianNode2)
{

	if (domianNode1 == nullptr || domianNode2 == nullptr) return;

	this->m_RelayRoutingTable.emplace(domianNode1->GetID(), domianNode2);
	this->m_RelayRoutingTable.emplace(domianNode2->GetID(), domianNode1);

	m_oCurrentPLTForm.AddRouteTo(domianNode1);
	m_oCurrentPLTForm.AddRouteTo(domianNode2);
}

CSipDomain& SipRegMgr::GetDomain(const char * str_DeviceID)
{
	return m_RemoteDomains[str_DeviceID];
	// TODO: insert return statement here
}

const char* SipRegMgr::GetID()
{
	return m_oCurrentPLTForm.GetID().c_str();
}

const char* SipRegMgr::GetIP()
{
	return m_oCurrentPLTForm.GetIP().c_str();
}

int SipRegMgr::OnRegister(void *p_eXoSipEvent)
{
	auto m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);
	if (nullptr == m_pEvent)
		return 0;

	SIP_REGISTER_OBJECT_T registeringObject;

	if (!CheckRegMsgHeader(m_pEvent, &registeringObject))
	{
		SendRegAnsweredByCode(m_pEvent->tid, 403, registeringObject.nExpires);
		CLog::Log(SIPSERVICE, LL_NORMAL, "%s Registering DomainID = %s \r\n", __FUNCTION__, registeringObject.strDeviceID.c_str());
		return 0;
	}

	auto registering_status = GetRegisteringStatus(registeringObject);

	switch (registering_status)
	{
	case STATUS_SEND_401_UNAUTHORIZED:
	{
		DealStatus_Send401(p_eXoSipEvent, &registeringObject);
		break;
	}
	case STATUS_SENT_401_UNAUTHORIZED:
	{
		DealStatus_Sent401(p_eXoSipEvent, &registeringObject);
		break;
	}

	case STATUS_SEND_403_FORBIDDEN:
	{
		DealStatus_Send403(p_eXoSipEvent, &registeringObject);
		break;
	}

	case STATUS_REGISTER_SUCCESS:
	case STATUS_REGISTER_DEFAULT:
	{
		DealStatus_DefaultRegister(p_eXoSipEvent, &registeringObject);
		break;
	}
	default:
		break;

	}

	return 0;
}

int SipRegMgr::GetRegisteringStatus(SIP_REGISTER_OBJECT_T &registeringObject)
{
	auto registering_status_to_handle = STATUS_REGISTER_DEFAULT;

	SIP_REGISTER_OBJECT_T *localCheckObject = nullptr;

	auto b_find = this->GetRegisterObjectInfo(registeringObject.strDeviceID, &localCheckObject);

	if (!b_find)
	{
		registering_status_to_handle = STATUS_SEND_403_FORBIDDEN;
	}
	else
	{
		CLog::Log(SIPSERVICE, LL_NORMAL, "%s Registering DomainID = %s GetDomainInfoById ret = %d\r\n", __FUNCTION__, registeringObject.strDeviceID.c_str(), b_find);

		switch (localCheckObject->nStatus)
		{
		case  STATUS_NOT_REGISTER:
			if (localCheckObject->nNeedAuth)
			{
				registering_status_to_handle = STATUS_SEND_401_UNAUTHORIZED;
			}
			if (registeringObject.nStatus == STATUS_REGISTER_EXIT)
			{
				registering_status_to_handle = STATUS_SEND_403_FORBIDDEN;
			}

			break;

		case  STATUS_REGISTER_SUCCESS:

			if (localCheckObject->nNeedAuth)
			{
				registering_status_to_handle = STATUS_SEND_401_UNAUTHORIZED;
			}

			break;

		default:
			registering_status_to_handle = localCheckObject->nStatus;
			break;
		}

	}
	return registering_status_to_handle;
}

std::string SipRegMgr::GetAuthorizationMessage_qop(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrResponse = osip_authorization_get_message_qop(pAuth);
	if (nullptr == pstrResponse)
		return strTmp;

	int nStrLen = strlen(pstrResponse);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrResponse[0])
		strncpy_s(pTmp, nStrLen, pstrResponse + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrResponse);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

std::string SipRegMgr::GetAuthorizationResponse(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrResponse = osip_authorization_get_response(pAuth);
	if (nullptr == pstrResponse)
		return strTmp;

	int nStrLen = strlen(pstrResponse);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrResponse[0])
		strncpy_s(pTmp, nStrLen, pstrResponse + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrResponse);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}
std::string SipRegMgr::GetAuthorizationUsername(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrUser = osip_authorization_get_username(pAuth);
	if (nullptr == pstrUser)
		return strTmp;

	int nStrLen = strlen(pstrUser);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrUser[0])
		strncpy_s(pTmp, nStrLen, pstrUser + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrUser);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

std::string SipRegMgr::GetAuthorizationRealm(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrRealm = osip_authorization_get_realm(pAuth);
	if (nullptr == pstrRealm)
		return strTmp;

	int nStrLen = strlen(pstrRealm);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrRealm[0])
		strncpy_s(pTmp, nStrLen, pstrRealm + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrRealm);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

std::string SipRegMgr::GetAuthorizationCnonce(osip_authorization_t *pAuth) const
{
	std::string strTmp = "0a4f113b";
	if (nullptr == pAuth)
		return strTmp;

	char *pstrCNonce = osip_authorization_get_cnonce(pAuth);
	if (nullptr == pstrCNonce)
		return strTmp;

	int nStrLen = strlen(pstrCNonce);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrCNonce[0])
		strncpy_s(pTmp, nStrLen, pstrCNonce + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrCNonce);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

std::string SipRegMgr::GetAuthorizationUri(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrDigestUri = osip_authorization_get_uri(pAuth);
	if (nullptr == pstrDigestUri)
		return strTmp;

	int nStrLen = strlen(pstrDigestUri);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrDigestUri[0])
		strncpy_s(pTmp, nStrLen, pstrDigestUri + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrDigestUri);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

std::string SipRegMgr::GetAuthorizationNonce(osip_authorization_t *pAuth) const
{
	std::string strTmp;
	if (nullptr == pAuth)
		return strTmp;

	char *pstrNonce = osip_authorization_get_nonce(pAuth);
	if (nullptr == pstrNonce)
		return strTmp;

	int nStrLen = strlen(pstrNonce);
	char * pTmp = new char[nStrLen + 1];
	memset(pTmp, 0, nStrLen + 1);

	if ('\"' == pstrNonce[0])
		strncpy_s(pTmp, nStrLen, pstrNonce + 1, nStrLen - 2);
	else
		strcpy_s(pTmp, nStrLen + 1, pstrNonce);

	strTmp = pTmp;
	delete[]pTmp;
	return strTmp;
}

int SipRegMgr::OnKeepAlive(eXosip_event_t * m_pEvent)
{
	if (nullptr == m_pEvent)
		return EVENT_NOTINIT_ERROR;

	SIP_REGISTER_OBJECT_T registeringObject;
	if (!CheckMsgHeader(m_pEvent, &registeringObject, false))
	{
		SendAnsweredByCode(m_pEvent->tid, 403);
		return USERMANAGER_INIT_ERROR;
	}

	SIP_REGISTER_OBJECT_T *localCacheObject = nullptr;
	bool ret = this->GetRegisterObjectInfo(registeringObject.strDeviceID, &localCacheObject);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s getUserInfoById ret = %d\r\n", __FUNCTION__, localCacheObject->strDeviceID.c_str(), ret);
	if (!ret)
	{
		SendAnsweredByCode(m_pEvent->tid, 403);
		return 0;
	}
	time_t timCurrent = 0;
	time(&timCurrent);
	localCacheObject->timLastKeepAlive = timCurrent;
	SendAnsweredByCode(m_pEvent->tid, 200);
	return 0;
}

void SipRegMgr::CirclingCheckingTask()
{
	for (auto &item : m_RemoteDomains)
	{
		if (item.second.NeedSendRegister())
		{
			RegisterTo(item.second);
			CLog::Log(SIPSERVICE, LL_NORMAL, "SIP 检查注册状态和注册超时，向上级注册。");
		}
		if (item.second.NeedSendkeepAlive())
		{
			KeepAliveTo(item.second);
			CLog::Log(SIPSERVICE, LL_NORMAL, "SIP 检查心跳超时，重新向上级发送keepalive。");
		}
		if (item.second.NeedSendSubscribe())
		{
			//TODO add Checking Subscribe timeout
			CLog::Log(SIPSERVICE, LL_NORMAL, "SIP 检查订阅超时，重新向下级订阅目录。");
		}
	}
}


void SipRegMgr::ConnectToRemoteDomains()
{
	auto callbinding = std::bind(&SipRegMgr::CirclingCheckingTask, this);
	m_CircleTaskWorker.setparameter(std::move(callbinding), 10);
	m_CircleTaskWorker.start();
}

int SipRegMgr::RegisterTo(CSipDomain & pDomain)
{
	sip_packet_t SipPacket;
	pDomain.Fill_SipPacket(SipPacket);
	return  m_pSipWorker->Register(&SipPacket);
}

int SipRegMgr::OnRegisterToSucess(eXosip_event_t * m_pEvent)
{
	auto pRequstUri = osip_message_get_uri(m_pEvent->request);

	m_RemoteDomains[pRequstUri->username].UpdateStatus(STATUS_REGISTER_SUCCESS);
	return 0;
}



int SipRegMgr::UnRegisterFrom(CSipDomain & pDomain)
{
	sip_packet_t SipPacket;
	SipPacket.SetRemoteID(pDomain.GetID().c_str());

	SipPacket.pPlatform = &m_oCurrentPLTForm;
	return  m_pSipWorker->Unregister(&SipPacket);
}

int SipRegMgr::KeepAliveTo(CSipDomain & pDomain)
{
	sip_packet_t SipPacket;
	pDomain.Fill_SipPacket(SipPacket);
	std::string strMsgBody =
		"<?xml version=\"1.0\"?>\r\n"
		"<Notify>\r\n"
		"<CmdType>Keepalive</CmdType>\r\n";
	strMsgBody += "<SN>";
	strMsgBody += pDomain.GetStrCSeq();
	strMsgBody += "</SN>\r\n";
	strMsgBody += "<DeviceID>";
	strMsgBody += m_oCurrentPLTForm.GetID();
	strMsgBody += "</DeviceID>\r\n"
		"<Status>OK</Status>\r\n"
		"</Notify>\r\n";
	SipPacket.strBody = std::move(strMsgBody);
	SipPacket.tHeader.ePackType = pack_kinds::tosend;
	SipPacket.tHeader.eWorkType = work_kinds::keepalive;
	return m_pSipWorker->KeepAlive(&SipPacket);
}

int SipRegMgr::SendRegAnsweredByCode(int tid, int nCode, int nExpires) const
{
	if (nullptr == m_pContext_eXosip)
		return CONTEXT_NOTINIT_ERROR;

	eXosip_lock(m_pContext_eXosip);
	osip_message_t *answer = nullptr;

	if (eXosip_message_build_answer(m_pContext_eXosip, tid, nCode, &answer))
	{
		eXosip_unlock(m_pContext_eXosip);
		return BUILD_ANSWER_ERROR;
	}

	//create contact
	auto pFrom = osip_message_get_from(answer);
	std::string strContact = "<sip:";
	strContact += pFrom->url->username;
	strContact += "@";
	strContact += pFrom->url->host;
	strContact += ">";
	osip_message_set_header(answer, "Contact", strContact.c_str());

	char strTime[MAX_PATH] = { 0 };
	_itoa_s(nExpires, strTime, 10);
	osip_message_set_header(answer, "Expires", strTime);

	struct timeb tb;
	ftime(&tb);
	tm pTm;
	localtime_s(&pTm, &tb.time);

	sprintf_s(strTime, "%d-%02d-%02dT%02d:%02d:%02d.%03d", pTm.tm_year + 1900, pTm.tm_mon + 1, pTm.tm_mday, pTm.tm_hour, pTm.tm_min, pTm.tm_sec, tb.millitm);
	osip_message_set_header(answer, "Date", strTime);
	auto nRet = eXosip_message_send_answer(m_pContext_eXosip, tid, nCode, answer);

	eXosip_unlock(m_pContext_eXosip);


	return nRet;
}

int SipRegMgr::SendAnsweredByCode(int tid, int nCode, char * strContentType, const char* strbody, size_t lenth) const
{
	return m_pSipWorker->SendAnsweredByCode(tid, nCode, strContentType, strbody, lenth);
}
const std::string  & SipRegMgr::GetSIPFrom()
{
	return m_oCurrentPLTForm.GetSIPFrom();

}


const std::string SipRegMgr::GetSIPTo(std::string target_id, std::string str_ip, int n_port)
{
	std::string str_sip_to;
	return str_sip_to
		.append("sip:")
		.append(target_id)
		.append("@")
		.append(str_ip)
		.append(":")
		.append(std::to_string(n_port));

}

DOMAINS_ROUTING_T * SipRegMgr::GetRelayRoutings()
{
	return &m_RelayRoutingTable;
}

void ParseDataByDelimit(std::string strDelimit, std::string fromInfo, vector<std::string >& toInfo)
{
	std::string str1 = "";

	for (unsigned int i = 0; i < fromInfo.size() && strDelimit.size() == 1; i++)
	{
		if (fromInfo[i] == strDelimit[0])
		{
			toInfo.push_back(str1);
			str1 = "";
		}
		else
		{
			str1 += fromInfo[i];
		}
	}

	toInfo.push_back(str1);
}

int SipRegMgr::OnModifyPassword(void *p_eXoSipEvent, std::string xml)
{
	eXosip_event_t *m_pEvent = static_cast<eXosip_event_t *>(p_eXoSipEvent);
	if (nullptr == m_pEvent)
		return EVENT_NOTINIT_ERROR;

	SIP_REGISTER_OBJECT_T localCacheObject;
	if (!CheckMsgHeader(m_pEvent, &localCacheObject))
	{
		return NOTFIND_CONTACT_ERROR;
	}

	SIP_REGISTER_OBJECT_T* pLocalCacheObject = nullptr;
	bool ret = this->GetRegisterObjectInfo(localCacheObject.strDeviceID, &pLocalCacheObject);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s userId = %s getUserInfoById ret = %d\r\n", __FUNCTION__, localCacheObject.strDeviceID.c_str(), ret);
	if (!ret)
	{
		return 0;
	}

	SIP_OBJECT_PASSWD_T passWordInfo;

	ParseModifyuserPassword(xml, passWordInfo);
	passWordInfo.result = "OK";

	std::string md5Str;
	std::string inputStr = passWordInfo.nonce + ":" + localCacheObject.strPassWord;

	GetMd5String(md5Str, inputStr.c_str(), inputStr.size());
	//char decoderBase64Buf[1024] = { 0 };
	//Base64Decode(decoderBase64Buf, passWordInfo.nonce1.c_str(), passWordInfo.nonce1.size());
	//std::string nonce1 = decoderBase64Buf;

	auto nonce1 = BASE64Cryptor().Decode(passWordInfo.nonce1.c_str(), passWordInfo.nonce1.size());


	CLog::Log(SIPSERVICE, LL_NORMAL, "%s xml = %s myMd5Str = %s nonceDeocdeBase64 = %s\r\n", __FUNCTION__, xml.c_str(), md5Str.c_str(), nonce1.c_str());

	if (md5Str != nonce1)
	{
		passWordInfo.result = "NGN";

		xml = PackModifyPasswordRet(passWordInfo);
		return 0;
	}

	//Base64Decode(decoderBase64Buf, passWordInfo.nonce.c_str(), passWordInfo.nonce.size());
	//std::string from = decoderBase64Buf;
	
	auto from = BASE64Cryptor().Decode(passWordInfo.nonce.c_str(), passWordInfo.nonce.size());
	vector<std::string> toInfo;
	ParseDataByDelimit(":", from, toInfo);
	int count = toInfo.size() - 1;

	ModifyDomainPasswdById(localCacheObject.strDeviceID, toInfo[count]);
	ModifyPassword(toInfo[count]);

	std::string oldXml = xml;
	xml = PackModifyPasswordRet(passWordInfo);
	CLog::Log(SIPSERVICE, LL_NORMAL, "%s xml = %s myMd5Str = %s nonceDeocdeBase64 = %s password = %s xml = %s\r\n", __FUNCTION__, oldXml.c_str(), md5Str.c_str(), nonce1.c_str(), toInfo[count].c_str(), xml.c_str());

	return 1;
}

int SipRegMgr::ModifyPassword(std::string password)
{
	WritePrivateProfileString(_T("GATEWAY_INFO"), _T("Password"), password.c_str(), m_strConfigpath.c_str());
	return true;
}

bool SipRegMgr::ParseModifyuserPassword(std::string& xml, SIP_OBJECT_PASSWD_T& passWordInfo)
{
	std::string cmdType;
	std::string sn;
	std::string deviceId;
	using XMLDocument = tinyxml2::XMLDocument;
	XMLDocument bodyDocument;
	bodyDocument.Parse(xml.c_str());

	auto pRootElement = bodyDocument.RootElement();
	if (nullptr == pRootElement)
		return false;

	std::string strTypeName = pRootElement->Value();
	auto pElement = pRootElement->FirstChildElement();
	if (nullptr == pElement)
		return false;

	for (int i = 0; i < 3; i++)
	{
		std::string strElementName = pElement->Value();
		if (strElementName == "CmdType")
		{
			cmdType = pElement->GetText();
			passWordInfo.cmdType = cmdType;
		}
		else if (strElementName == "SN")
		{
			// SN号
			sn = pElement->GetText();
			passWordInfo.sn = sn;
		}
		else if (strElementName == "DeviceID")
		{
			deviceId = pElement->GetText();
			passWordInfo.deviceId = deviceId;
		}

		pElement = pElement->NextSiblingElement();
		if (nullptr == pElement)
			break;
	}

	auto node = pRootElement->FirstChildElement("Info");
	auto  child = node->FirstChildElement("nonce");
	if (child)
	{
		passWordInfo.nonce = child->GetText();
	}

	child = node->FirstChildElement("nonce1");
	if (child)
	{
		passWordInfo.nonce1 = child->GetText();
	}

	child = node->FirstChildElement("algorithm");
	if (child)
	{
		passWordInfo.algorithm = child->GetText();
	}
	return true;
}

std::string  SipRegMgr::PackModifyPasswordRet(SIP_OBJECT_PASSWD_T& passWordInfo)
{
	char tmpBuf[512] = { 0 };
	std::string  xml = "<?xml version=\"1.0\" ?><Response><CmdType>DeviceControl</CmdType><SN>%s</SN><DeviceID>%s</DeviceID><Result>%s</Result></Response>";
	sprintf_s(tmpBuf, xml.c_str(), passWordInfo.sn.c_str(), passWordInfo.deviceId.c_str(), passWordInfo.result.c_str());
	std::string  retXml = tmpBuf;

	CLog::Log(SIPSERVICE, LL_NORMAL, "%s retXml = %s\r\n", __FUNCTION__, retXml.c_str());

	return retXml;
}

void SipRegMgr::GetMd5String(std::string  &strMD5, const char *input, int inputlen)
{
	unsigned char digest[16];
	osip_MD5_CTX Md5Ctx;
	osip_MD5Init(&Md5Ctx);
	auto tmpinput = const_cast<char *>(input);
	osip_MD5Update(&Md5Ctx, reinterpret_cast<unsigned char*>(tmpinput), static_cast<unsigned int>(inputlen));
	osip_MD5Final(static_cast<unsigned char*>(digest), &Md5Ctx);

	char strTemp[10] = { 0 };
	for (int i = 0; i < 16; i++)
	{
		sprintf_s(strTemp, "%02x", digest[i]);
		strMD5 += strTemp;
	}
}


