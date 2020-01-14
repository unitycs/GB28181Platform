#pragma once

//////////////////////////////////////////////////////////////////////////
// CTaskImpl: Thread task base class.
#include "threadSync.h"
template<class T>
class CTaskImpl : HUS::noncopyable, public HUS::thread_functor {
	CTaskImpl(const HUS::mutex& m_mtx_thread, const HUS::condition& m_cnd_work, bool m_b_running, bool m_b_exiting, bool m_b_suspended)
		: m_mtxThread(m_mtx_thread),
		m_cndWork(m_cnd_work),
		m_bRunning(m_b_running),
		m_bExiting(m_b_exiting),
		m_bSuspended(m_b_suspended)
	{
	}
	CTaskImpl() : m_bRunning(false), m_bExiting(false), m_bSuspended(false)
	{};
	virtual ~CTaskImpl() {
		Stop(FALSE, TRUE);
	}

public:
	typedef HUS::mutex::scoped_lock scoped_lock;
	typedef HUS::condition condition;
	typedef HUS::mutex mutex;

	bool is_running() {
		mutex::scoped_lock lock(m_mtxThread);
		return m_bRunning;
	}

	DWORD Suspend() {
		mutex::scoped_lock lock(m_mtxThread);
		if (m_bSuspended) {
			return 0;
		}

		m_bSuspended = true;
		/*if (m_lpThread != NULL) {
			return m_lpThread->suspend();
		}
		*/
		return 0;
	}

	DWORD Resume() {
		mutex::scoped_lock lock(m_mtxThread);
		if (!m_bSuspended) {
			return 0;
		}

		m_bSuspended = false;
		//if (m_lpThread != NULL) {
		//	return m_lpThread->resume();
		//}

		return 0;
	}

public:
	BOOL Start() {
		mutex::scoped_lock lock(m_mtxThread);

		if (!m_bRunning) {
			// The thread is stopping or has terminated,
			// Confirm this thread has terminated completely.
			Stop(FALSE, TRUE);
		}

		// Already create the thread.
		//if (m_lpThread != NULL) {
		//	DWORD dwResume = 0;
		//	while ((dwResume = m_lpThread->resume()) > 0);
		//	return TRUE;
		//}

		// Previously, the thread has terminated completely,
		// Create new thread to run this task.
		m_bExiting = false, m_bRunning = false, m_bSuspended = false;
		//		ATLTRY(m_lpThread = new thread(*this, m_pOnThreadTermnate));
		return TRUE;
	}

public:
	BOOL SignalStop() {
		mutex::scoped_lock lock(m_mtxThread);

		// set exit flag
		m_bExiting = true;

		// signal to wakeup thread
		m_cndWork.notify_one();
		return true;
	}

protected:
	BOOL Stop(BOOL /*bMsgp*/, BOOL /*bForce*/) {
		mutex::scoped_lock lock(m_mtxThread);

		// set exit flag
		m_bExiting = true;

		// signal to wakeup thread
		m_cndWork.notify_one();

		//if (m_lpThread != NULL) {
		//	DWORD dwResume = 0;
		//	while ((dwResume = m_lpThread->resume()) > 0);

		//	// wait until thread exited
		//	m_lpThread->kill(bMsgp, bForce);

		//	// cleanup
		//	ATLTRY(delete m_lpThread);
		//	m_bSuspended = false;
		//	m_bRunning = false;
		//	m_lpThread = NULL;
		//}

		return TRUE;
	}

private:
	void operator()(void) {
		m_bRunning = true;

		// Call correct task loop.
		T& pThis = static_cast<T&>(*this);
		pThis._TaskProc();

		m_bRunning = false;
	}

protected:
	//lpthread m_lpThread;

	mutex m_mtxThread;		// task thread operation mutext.
	condition m_cndWork;	// task queue not empty condition

	//OnThreadTermnate m_pOnThreadTermnate;
	bool m_bRunning;		// thread running flag
	bool m_bExiting;		// thread exiting flag
	bool m_bSuspended;		// thread suspended flag
	friend  T;
};
