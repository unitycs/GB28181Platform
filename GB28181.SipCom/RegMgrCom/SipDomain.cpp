#include "stdafx.h"
#include "SipDomain.h"
#include "Common/Utils.h"
//#include "Common/AlgorithmTool.h"
#include "GBUtils.h"
#include <thread>

CSipDomain::CSipDomain(const SIP_REGISTER_OBJECT_T& register_obejct) :SIP_REGISTER_OBJECT_T(register_obejct)
{


}

// 设置注册的CallID
void CSipDomain::SetCallID(const char *pszCallID)
{
	m_strCallID = pszCallID;
}

const std::string CSipDomain::GetStrCSeq()
{
	if (0 > m_nCSeq)
		m_nCSeq = 0;
	return to_string(m_nCSeq++);
}

const int CSipDomain::GetnSeq()
{
	if (0 > m_nCSeq)
		m_nCSeq = 0;
	return	m_nCSeq++;
}


void CSipDomain::RemoveRouteTo(string domainID)
{

	if (domainID.length() < 18) return;

	m_RoutingTable.erase(domainID);


}

void CSipDomain::RemoveRouteTo(CSipDomain* pDomain)
{

	if (pDomain == nullptr) return;

	m_RoutingTable.erase(pDomain->GetID());


}

void CSipDomain::AddRouteTo(CSipDomain* pDomain)
{
	if (pDomain == nullptr) return;

	m_RoutingTable.emplace(pDomain->GetID(), pDomain);
}

void CSipDomain::Initialize(const char * str_DeviceID, const char * str_IP, const unsigned n_Port, const char * str_Password, const char * str_username)
{
	//FOR TEST
	this->strDeviceID = str_DeviceID;
	this->nType = 4;
	this->strIP = str_IP;
	this->nPort = n_Port;
	this->strPassWord = str_Password;

	if (!str_username)
	{
		this->userName = str_DeviceID;
	}
	else
	{
		this->userName = str_username;
	}

}

const std::string & CSipDomain::GetSIPFrom()
{
	if (!m_strSipFrom.empty()) return m_strSipFrom;

	m_strSipFrom = "sip:" + strDeviceID + "@" + strIP + ":" + to_string(nPort);

	return m_strSipFrom;
}

const std::string CSipDomain::GetAuthHeader()  const
{
	std::string strNonce;
	//add www_auth info to response
	CreateNonce(strNonce, NONCE_LEN);

	return "Digest realm=" + GetRealm() + "  nonce=" + strNonce;

}



//sipto domian
//str_DeviceID在当前域的设备ID,目前不校验
const std::string  CSipDomain::GetSIPTo(std::string str_DeviceID)
{
	return "sip:" + str_DeviceID + "@" + strIP + ":" + std::to_string(nPort);
}

int CSipDomain::NeedSendkeepAlive()
{
	if (nStatus == STATUS_REGISTER_SUCCESS)
	{
		time_t tmCur = 0;
		time(&tmCur);

		if (timLastKeepAlive == 0)
		{
			timLastKeepAlive = tmCur;
			return 1;
		}
		else if (tmCur - timLastKeepAlive >= nKeepaliveInterval)
		{
			timLastKeepAlive = tmCur;
			return 2;
		}
	}
	return 0;
}

int CSipDomain::NeedSendRegister()
{
	time_t tmCur = 0;
	time(&tmCur);
	if (nStatus == STATUS_REGISTER_SUCCESS)
	{
		if (tmCur - timLastRegister < nExpires)
		{
			timLastRegister = tmCur;
			return 0;
		}
	}
	if (m_bNeedConneted)
	{
		timLastRegister = tmCur;
		return 1;
	}
	return 0;
}

int CSipDomain::NeedSendSubscribe()
{
	return 0;
}

void CSipDomain::UpdateStatus(int NewStatus)
{
	this->nStatus = NewStatus;
}


void CSipDomain::Fill_SipPacket(sip_packet_t& SipPacket)
{
	SipPacket.SetRemoteID(GetID().c_str());
	SipPacket.pPlatform = this;
	SipPacket.nSeq = GetnSeq();

}
