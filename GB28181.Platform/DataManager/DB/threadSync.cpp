#include "stdafx.h"
#include "threadSync.h"
#include <chrono>

//#include <PROCESS.H>
#ifndef LONG_MAX
#	define LONG_MAX 2147483647L
#endif // LONG_MAX

namespace HUS {
	//////////////////////////////////////////////////////////////////////////
	//
	xtime::xtime() : sec(0), nsec(0) {
	}

	xtime::xtime(const xtime& xt) : sec(xt.sec), nsec(xt.nsec) {
	}

	int xtime::Setcurrent(int clock_type) {
		if (clock_type == TIME_UTC) {
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);

			const __int64 TIMESPEC_TO_FILETIME_OFFSET = 0;//((__int64)27111902UL << 32) + (__int64)3577643008UL;
			sec = static_cast<int>((*reinterpret_cast<__int64*>(&ft) - TIMESPEC_TO_FILETIME_OFFSET) / 10000000);
			nsec = static_cast<int>((*reinterpret_cast<__int64*>(&ft) - TIMESPEC_TO_FILETIME_OFFSET - (static_cast<__int64>(sec) * static_cast<__int64>(10000000))) * 100);
			return clock_type;
		}

		return 0;
	}

	void xtime::Settime(int milliseconds) {
		int res;
		res = Setcurrent(TIME_UTC);
		ATLASSERT(res == TIME_UTC);

		sec += (milliseconds / MILLISECONDS_PER_SECOND);
		nsec += ((milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND);

		if (nsec > static_cast<const int>(NANOSECONDS_PER_SECOND)) {
			++sec;
			nsec -= NANOSECONDS_PER_SECOND;
		}
	}

	int xtime::Getduration() const {
		xtime cur, xt(*this);
		int milliseconds, res = cur.Setcurrent(TIME_UTC);

		ATLASSERT(res == TIME_UTC);

		if (xtime::cmp(xt, cur) <= 0) {
			milliseconds = 0;
		}
		else {
			if (cur.nsec > xt.nsec) {
				xt.nsec += NANOSECONDS_PER_SECOND;
				--xt.sec;
			}

			milliseconds = static_cast<int>((xt.sec - cur.sec) * MILLISECONDS_PER_SECOND) + (((xt.nsec - cur.nsec) + (NANOSECONDS_PER_MILLISECOND / 2)) / NANOSECONDS_PER_MILLISECOND);
		}

		return milliseconds;
	}

	int xtime::Getmicroduration() const {
		xtime cur;
		int microseconds, res = cur.Setcurrent(TIME_UTC);
		ATLASSERT(res == TIME_UTC);

		if (cur.Setcurrent(TIME_UTC) <= 0) {
			microseconds = 0;
		}
		else {
			microseconds = static_cast<int>((sec - cur.sec) * MICROSECONDS_PER_SECOND) + (((nsec - cur.nsec) + (NANOSECONDS_PER_MICROSECOND / 2)) / NANOSECONDS_PER_MICROSECOND);
		}

		return microseconds;
	}

	int xtime::cmp(const xtime& xt1, const xtime& xt2) {
		int res = static_cast<int>(xt1.sec - xt2.sec);
		if (res == 0) res = static_cast<int>(xt1.nsec - xt2.nsec);
		return res;
	}

	mutex::mutex() : m_mutex(0) {
		try {
			m_mutex = reinterpret_cast<void*>(new CRITICAL_SECTION);
		}
		catch (...) {
		}

		if (!m_mutex) throw thread_resource_error();
		InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
	}

	mutex::~mutex() {
		DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
		//		ATLTRY(delete reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
	}

	void mutex::do_lock() const
	{
		EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
	}

	void mutex::do_unlock() const
	{
		LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
	}

	void mutex::do_lock(cv_state&) const
	{
		do_lock();
	}

	void mutex::do_unlock(cv_state&) const
	{
		do_unlock();
	}

	BOOL mutex::TryEnter() const
	{
		return TryEnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_mutex));
	}
} // namespace HUS