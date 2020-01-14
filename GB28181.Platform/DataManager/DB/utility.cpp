// utility.cpp: HUS module's utilities.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "utility.h"
#include "time.h"

//////////////////////////////////////////////////////////////////////////
//
__int64 CUtility::GetMiniTime() {
	SYSTEMTIME pSysTime;
	::GetLocalTime(&pSysTime);
	__int64 nSecTime = time(nullptr);

	return ((nSecTime << 32) + pSysTime.wMilliseconds);
}

__int64 CUtility::NormalMiniTime(__int64 nTime) {
	__int64 nS = SecsFromMiniTime(nTime), nMS = MinisFromMiniTime(nTime);
	return (nS * 1000 + nMS);
}

time_t CUtility::GetSecTime() {
	return time(nullptr);
}

__int64 CUtility::Differs(__int64 nMini1, __int64 nMini2) {
	__int64 nS1 = SecsFromMiniTime(nMini1), nS2 = SecsFromMiniTime(nMini2);
	__int64 nMS1 = MinisFromMiniTime(nMini1), nMS2 = MinisFromMiniTime(nMini2);
	return ((nS1 * 1000 + nMS1) - (nS2 * 1000 + nMS2));
}

long CUtility::SecsFromMiniTime(__int64 nTmMini) {
	return static_cast<long>(nTmMini >> 32);
}

long CUtility::MinisFromMiniTime(__int64 nTmMini) {
	return static_cast<long>(nTmMini & 0XFFFFFFFF);
}

//////////////////////////////////////////////////////////////////////////
//
static void ConvertGUIDName(const GUID& pID, _bstr_t& strRetID) {
	BSTR strID;

	// Convert it.
	StringFromIID(pID, &strID);
	strRetID = _bstr_t(strID);
	::CoTaskMemFree(strID);
}

//////////////////////////////////////////////////////////////////////////
// CWaitableMonitorTimer:

#define _SECOND 10000000