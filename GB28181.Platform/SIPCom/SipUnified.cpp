#include "stdafx.h"
#include "SipUnified.h"

SipUnified::SipUnified()
{
}

SipUnified::~SipUnified()
{
}

int SipUnified::GenerateReportSN(CString &strSN)
{
	static CCriticalSection  oLock;
	static USHORT unSN = 1;

	auto nSN = 0;
	oLock.Lock();
	unSN++;
	nSN = unSN;
	strSN.Format(_T("%d"), unSN);
	oLock.Unlock();
	return nSN;
}

void SipUnified::GB28181TimeToDATE(const char *pszTime, DATE &dateTime)
{
	// 时间格式"2013-03-28T19:46:17"
	__try
	{
		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		int nHour = 0;
		int nMin = 0;
		int nSec = 0;

		sscanf_s(pszTime, "%d-%d-%dT%d:%d:%d", &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec);
		COleDateTime start(nYear, nMonth, nDay, nHour, nMin, nSec);
		dateTime = DATE(start);
	}
	__except (1)
	{
		//CLog::Log(SDKCOM, LL_NORMAL, "GB28181时间格式异常：%s", pszTime);
	}
}

// 时间格式转换
void SipUnified::DATEToGB28181Time(const char *pszTime, DATE &dateTime)
{
	// 时间格式"2013-03-28T19:46:17"
	__try
	{
		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		int nHour = 0;
		int nMin = 0;
		int nSec = 0;

		sscanf_s(pszTime, "%d-%d-%dT%d:%d:%d", &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec);
		COleDateTime start(nYear, nMonth, nDay, nHour, nMin, nSec);
		dateTime = DATE(start);
	}
	__except (1)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "GB28181时间格式异常：%s", pszTime);
	}
}