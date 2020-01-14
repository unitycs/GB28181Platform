#include "StdAfx.h"
#include "SIPUnit.h"
//#include "SIPCom.h"

CSIPUnit::CSIPUnit(void) :
	RESPONSE_TIME(10),
	REGISTER_TIME(10),
	m_unSN(0),
	m_tmExpiry(3600),
	m_tmKeepalive(60),
	m_nKeepaliveInterval(60),
	m_nExpiryInterval(3600),
	m_ePlatformStatus(PF_STATUS_UNREGISTERED),
	m_tmResponse(0),
	m_unSeq(0)
{
}


CSIPUnit::~CSIPUnit(void)
{
}

int CSIPUnit::Init(const char *pszPlatformID, int nKeepaliveInterval, int nExpiryInterval)
{
	// 初始化平台信息
	m_strPFID = pszPlatformID;
	m_nKeepaliveInterval = nKeepaliveInterval;
	m_nExpiryInterval = nExpiryInterval;

	return 0;
}

// 注册到平台
int CSIPUnit::GetRegisterInfo(ipc_sip_block_t * pRegisterData)
{
	// 向SIPCom模块发送注册信息
	// 包括To信息，登陆密码，CSeq
	pRegisterData->cmd_type = ST_REGISTER;
	pRegisterData->tHeader.nExtBodySize = 0;
	memcpy(pRegisterData->szDstDomainID, m_strPFID, ID_BUF_LEN);
	memcpy(pRegisterData->szFromDeviceID, m_strPFID, ID_BUF_LEN);

	return 0;
}

// 发送保活信息
int CSIPUnit::GetKeepaliveInfo(ipc_sip_block_t * pRegisterData, char *pszDataBuf, int nBufSize, const char *pszGWID)
{
	CBodyBuilder oSIPBody;

	// 向SIPCom模块发送保活信息
	// 包括To信息，保活包的Body
	pRegisterData->cmd_type = ST_MESSAGE;
	memcpy(pRegisterData->szDstDomainID, m_strPFID, ID_BUF_LEN);

	// 生成SN号
	CString strSN;
	GenerateSN(strSN);

	// 创建保活数据包
	oSIPBody.CreateKeepaliveBody(pszDataBuf, nBufSize, strSN.GetString(), pszGWID);

	return 0;
}

CString &CSIPUnit::GetPlatformID()
{
	return m_strPFID;
}

void CSIPUnit::SetPlatformStatus(PlatformStatus ePlatformStatus)
{
	m_ePlatformStatus = ePlatformStatus;
	time_t tmCur;
	time(&tmCur);

	// 设置下一个保活时间点
	m_tmKeepalive = tmCur + m_nKeepaliveInterval;

	// 设置过期时间点
	m_tmExpiry = tmCur + m_nExpiryInterval;
}

// 心跳保活间隔检查
// 判断是否应该发送保活消息
// 返回值：0 时间未到；1 时间已到
int CSIPUnit::CheckKeepalive(void)
{
	time_t tmCur;
	time(&tmCur);

	// 到达保活时间点
	if (m_tmKeepalive <= tmCur)
	{
		// 设置下一个保活时间点
		m_tmKeepalive = tmCur + m_nKeepaliveInterval;
		return 1;
	}

	return 0;
}

int CSIPUnit::CheckExpiry(void)
{
	time_t tmCur;
	time(&tmCur);

	// 到达保活时间点
	if (m_tmExpiry - (m_nExpiryInterval * 0.1) <= tmCur)
	{
		// 设置下一个保活时间点
		m_tmExpiry = tmCur + m_nExpiryInterval;
		return 1;
	}

	return 0;
}

// 生成新的SN号
int CSIPUnit::GenerateSN(CString &strSN)
{
	int nSN;
	m_oLock.Lock();

	m_unSN++;
	nSN = m_unSN;
	strSN.Format(_T("%d"), m_unSN);
	m_oLock.Unlock();
	return nSN;
}

PlatformStatus CSIPUnit::GetPlatformStatus()
{
	return m_ePlatformStatus;
}

void CSIPUnit::SetResponseTime()
{
	time_t timeCurrent;
	time(&timeCurrent);
	m_tmResponse = timeCurrent + RESPONSE_TIME;
}

void CSIPUnit::ResetResponseTime()
{
	m_tmResponse = 0;
}

int CSIPUnit::CheckResponseTime()
{
	time_t timeCurrent;
	time(&timeCurrent);
	if (0 < m_tmResponse && m_tmResponse < timeCurrent)
	{
		SetResponseTime();
		return 1;
	}

	return 0;
}

// 判断是否是保活包的应答消息
BOOL CSIPUnit::IsLastKeepaliveRes(const int nSeq)
{
	if (nSeq == m_nLastKeepaliveSeq)
		return TRUE;

	return FALSE;
}

void CSIPUnit::SetLastKeepaliveCSeq(const int nSeq)
{
	m_nLastKeepaliveSeq = nSeq;
}

int CSIPUnit::GetExpiry()
{
	return m_nExpiryInterval;
}

// 生成CSeq号
int CSIPUnit::GenerateCSeq()
{
	UINT nSeq;
	m_oLock.Lock();
	m_unSeq++;

	if (1 > m_unSeq)
		m_unSeq = 1;

	nSeq = m_unSeq;
	m_oLock.Unlock();

	return nSeq;
}

int CSIPUnit::AddSubscribeExpires(INT64 nCallID, int nSubExpiryInterval)
{
	time_t tmCur;
	time(&tmCur);

	m_oLock.Lock();
	MExpiry_t::iterator ifind = m_tmSubExpiry.find(nCallID);
	if (ifind != m_tmSubExpiry.end())
	{
		m_tmSubExpiry.erase(ifind);
	}

	// 取消订阅将不在Check范围内
	if (0 != nSubExpiryInterval)
		m_tmSubExpiry[nCallID] = tmCur + nSubExpiryInterval;

	m_oLock.Unlock();

	return 0;
}



int CSIPUnit::CheckSubExpiry(MExpiry_t& szTimeOut)
{
	time_t tmCur;
	time(&tmCur);

	m_oLock.Lock();
	//  int ret;
	for (MExpiry_t::iterator it = m_tmSubExpiry.begin();
		it != m_tmSubExpiry.end(); )
	{
		if (it->second <= tmCur)
		{
			szTimeOut[it->first] = it->second;
			it = m_tmSubExpiry.erase(it);
			continue;
		}
		++it;
	}
	m_oLock.Unlock();

	return 0;
}
