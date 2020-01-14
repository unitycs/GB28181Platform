#pragma once
#include "SDKCom/HUSSDKUnified.h"
template<class T>
class CRecordBase
{
public:
	// 设置记录查询开始时间
	void SetStartTime(DATE daStart)
	{
		SYSTEMTIME sysStartTime;
		COleDateTime oleStartTime(daStart);
		VariantTimeToSystemTime(oleStartTime, &sysStartTime);
		CTime otmStart(sysStartTime);
		m_tmStart = otmStart.GetTime();

		sprintf_s(m_szStartTime, "%d-%02d-%02dT%02d:%02d:%02d", otmStart.GetYear(),
			otmStart.GetMonth(),
			otmStart.GetDay(),
			otmStart.GetHour(),
			otmStart.GetMinute(),
			otmStart.GetSecond());
	}
	// 设置记录查询结束时间
	void SetEndTime(DATE daEnd)
	{
		SYSTEMTIME sysEndTime;
		COleDateTime oleEndTime(daEnd);
		VariantTimeToSystemTime(oleEndTime, &sysEndTime);
		CTime otmEnd(sysEndTime);
		m_tmEnd = otmEnd.GetTime();

		sprintf_s(m_szEndTime, "%d-%02d-%02dT%02d:%02d:%02d", otmEnd.GetYear(),
			otmEnd.GetMonth(),
			otmEnd.GetDay(),
			otmEnd.GetHour(),
			otmEnd.GetMinute(),
			otmEnd.GetSecond());
		//  CLog::Log(SDKCOM, LL_NORMAL, "%s m_tmStart = %d m_szStartTime = %s m_tmEnd = %d m_szEndTime = %s\r\n", __FUNCTION__, m_tmStart, m_szStartTime, m_tmEnd, m_szEndTime);
		CLog::Log(SDKCOM, LL_NORMAL, "%s m_tmStart = %d m_tmEnd = %d \r\n", __FUNCTION__, m_tmStart, m_tmEnd);
	}

	// 取得查询到的记录的开始时间
	char *GetStartTime(time_t *ptmStart)
	{
		if (ptmStart)
			*ptmStart = m_tmStart;
		return m_szStartTime;
	}

	//取得查询到的记录的结束时间
	char *GetEndTime(time_t *ptmEnd)
	{
		if (ptmEnd)
			*ptmEnd = m_tmEnd;
		return m_szEndTime;
	}

	char		m_szStartTime[TIME_BUF_LEN];
	char		m_szEndTime[TIME_BUF_LEN];
	time_t		m_tmStart;
	time_t		m_tmEnd;
};
template<class T>
class RecordQueryWoker
{
public:
	RecordQueryWoker() : m_oRecordMemMgr(sizeof(T), 1000) {}
	// 等待查询开始
	void WaitSearchStart() const
	{
		DWORD dWaitTm = MAKEWORD(2000, 0);
		WaitForSingleObject(m_oStartEvent.m_hObject, dWaitTm);
	}

	// 开始查询
	void SetSearchStart()
	{
		m_oStartEvent.SetEvent();
	}

	// 等待查询结束
	void WaitSearchFinish() const
	{
		DWORD dWaitTm = MAKEWORD(0, 2000);
		WaitForSingleObject(m_oFinishEvent.m_hObject, dWaitTm);
	}

	// 查询结束
	void SetSearchFinish()
	{
		m_oFinishEvent.SetEvent();
	}

	// 录像文件数量
	int GetRecordCount() const
	{
		return m_oRecordList.GetCount();
	}
	// 取得保存查询结果的缓存
	T* GetRecordBuf()
	{
		T *pRecord = nullptr;
		m_oRecordMemMgr.alloc(reinterpret_cast<CMemPoolUnit **>(&pRecord));
		return pRecord;
	}

	// 开始时间-结束时间为key的录像map
	CList<T*>				m_oRecordList;
	CEvent					m_oStartEvent;
	CEvent					m_oFinishEvent;
private:
	CMemPool				m_oRecordMemMgr;
};