#include "stdafx.h"
#include "SubscribeList.h"
#include "Main/MainThread.h"
#include "Common/common.h"
#include "Main/UnifiedMessage.h"
#include "Log/Log.h"


CSubscribeList::CSubscribeList()
{
	m_strSysSubInfo.did = 0;
	m_strSysSubInfo.exp = 0;
}

CSubscribeList::~CSubscribeList()
{}

void CSubscribeList::Init(const char *pszGWID)
{
	m_strSysSubscribe = pszGWID;
}

// 添加订阅范围（行政区划或系统）
void CSubscribeList::Add(const char *pszDeviceID, int nExp, int nDID)
{
	time_t tmCur;
	time(&tmCur);

	// 订阅整个系统目录
	if (0 == m_strSysSubscribe.Compare(pszDeviceID))
	{
		m_strSysSubscribe = pszDeviceID;
		m_strSysSubInfo.did = nDID;
		m_strSysSubInfo.exp = nExp + tmCur;
		return;
	}

	SubInfo_t  subInfo;
	subInfo.exp = nExp + tmCur;
	subInfo.did = nDID;
	m_oSubscribeMap.SetAt(pszDeviceID, subInfo);
}
// 查询当前设备是否属于被订阅范围
// OUT pzsSubscribeID：订阅范围，系统ID或行政区划
int CSubscribeList::Find(const char *pszDeviceID, char *pzsSubscribeID, int &nDID)
{
	char tmp[ID_BUF_LEN];
	time_t tmCur;
	time(&tmCur);

	// 系统订阅，所有设备都有效
	if (0 < m_strSysSubInfo.exp)
	{
		Utils::StringCpy_s(pzsSubscribeID, ID_BUF_LEN, m_strSysSubscribe.GetString());
		nDID = m_strSysSubInfo.did;
		// 判断系统订阅是否过期
		if (tmCur < m_strSysSubInfo.exp)
		{
			return 0;
		}
		else
		{
			Del(pzsSubscribeID);
			return 1;
		}
	}

	// 省、市、区/县、基层单位共4层目录, 最后一次是设备ID判断
	for (int i = 1; i < 6; i++)
	{
		if (i < 5)
			Utils::StringCpy_s(tmp, i * 2 + 1, pszDeviceID);
		else
			Utils::StringCpy_s(tmp, ID_BUF_LEN, pszDeviceID);

		SubInfo_t subInfo;
		if (TRUE == m_oSubscribeMap.Lookup(tmp, subInfo, FALSE))
		{
			nDID = subInfo.did;
			Utils::StringCpy_s(pzsSubscribeID, ID_BUF_LEN, tmp);
			// 订阅未过期
			if (tmCur < subInfo.exp)
			{
				return 0;
			}
			else
			{
				CLog::Log(SIPCOM, LL_NORMAL, "[%s] Subscribe Expired", tmp);
				Del(tmp);
				return 1;
			}
		}
	}

	return -1;
}

// 从订阅列表删除
int CSubscribeList::Del(const char *pszDeviceID)
{
	return m_oSubscribeMap.RemoveKey(pszDeviceID);
}