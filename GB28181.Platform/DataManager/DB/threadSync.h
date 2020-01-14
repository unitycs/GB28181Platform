#pragma once
#include <windef.h>
#include <stdexcept>
#define DOUBLE_BUFFER_QUEUE_MAX_SIZE 1000

namespace HUS {
	class lock_error : public std::logic_error {
	public:
		lock_error() : std::logic_error("thread lock error") {
		}
	};

	class thread_resource_error : public std::runtime_error {
	public:
		thread_resource_error() : std::runtime_error("thread resource error") {
		}
	};

	const int MILLISECONDS_PER_SECOND = 1000;
	const int NANOSECONDS_PER_SECOND = 1000000000;
	const int NANOSECONDS_PER_MILLISECOND = 1000000;
	const int MICROSECONDS_PER_SECOND = 1000000;
	const int NANOSECONDS_PER_MICROSECOND = 1000;

	class xtime {
	public:
		xtime();
		xtime(const xtime& xt);
		int Setcurrent(int clock_type);
		void Settime(int milliseconds);
		int Getduration() const;
		int Getmicroduration() const;
		static int cmp(const xtime& xt1, const xtime& xt2);

	protected:
		__int64 sec;
		long nsec;
	};

	class noncopyable {
	protected:
		noncopyable() {}
		virtual	~noncopyable() {}

	private:
		noncopyable(const noncopyable&) = delete;
	};

	template <typename T>
	class lock_ops : noncopyable {
	public:
		typedef typename T::cv_state lock_state;

	private:
		lock_ops() { }

	public:
		static void lock(T& m) { m.do_lock(); }
		static void unlock(T& m) { m.do_unlock(); }
		static BOOL trylock(T& m) { return m.do_trylock(); }
		static void lock(T& m, lock_state& state) { m.do_lock(state); }
		static void unlock(T& m, lock_state& state) { m.do_unlock(state); }
		static BOOL timedlock(T& m, const xtime& xt) { return m.do_timedlock(xt); }
	};

	template<typename T>
	class scoped_lock : private noncopyable {
	public:
		typedef T mutex_type;

	public:
		explicit scoped_lock(T& mx, BOOL initially_locked = TRUE) : m_mutex(mx), m_locked(FALSE) {
			if (initially_locked) lock();
		}

		~scoped_lock() {
			if (m_locked) unlock();
		}

	public:
		void lock() {
			if (m_locked) throw lock_error();
			lock_ops<T>::lock(m_mutex);
			m_locked = TRUE;
		}

		void unlock() {
			if (!m_locked) throw lock_error();
			lock_ops<T>::unlock(m_mutex);
			m_locked = FALSE;
		}

	public:
		BOOL locked() const { return m_locked; }
		operator const void*() const { return (m_locked ? this : 0); }

	private:
		friend class condition;

	private:
		T& m_mutex;
		BOOL m_locked;
	};

	class mutex : noncopyable {
	public:
		typedef scoped_lock<mutex> scoped_lock;
		typedef void* cv_state;

		mutex();
		~mutex();

		void do_lock() const;
		void do_unlock() const;
		void do_lock(cv_state&) const;
		void do_unlock(cv_state&) const;

		BOOL TryEnter() const;

	private:
		void* m_mutex;

		friend class lock_ops<mutex>;
	};

	template<BOOL bMsgPach, BOOL bInitialOwner = FALSE>
	class kernel_mutex : noncopyable {
	public:
		typedef scoped_lock<kernel_mutex> scoped_lock;
		typedef void* cv_state;

		kernel_mutex() : m_hMtx(nullptr) {
			try {
				m_hMtx = CreateMutex(nullptr, bInitialOwner, nullptr);
			}
			catch (...) {
			}

			if (!m_hMtx) throw thread_resource_error();
		}

		~kernel_mutex() {
			do_unlock();

			::CloseHandle(m_hMtx);
			m_hMtx = nullptr;
		}

	private:
		void do_lock()  const {
			if (!m_hMtx) throw thread_resource_error();
			auto result = WAIT_OBJECT_0;

			if (bMsgPach) {
				while (1) {
					if ((result = MsgWaitForMultipleObjects(1, &m_hMtx, FALSE, INFINITE, QS_ALLINPUT)) == WAIT_OBJECT_0) {
						break;
					}
					else {
						MSG msg;
						::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
				}
			}
			else {
				WaitForSingleObject(m_hMtx, INFINITE);
			}
		}

		void do_unlock()  const {
			if (!m_hMtx) throw thread_resource_error();
			ReleaseMutex(m_hMtx);
		}

		void do_lock(cv_state&)  const {
			do_lock();
		}

		void do_unlock(cv_state&) const
		{
			do_unlock();
		}

	protected:
		HANDLE m_hMtx;

	private:
		friend class lock_ops<kernel_mutex>;
	};
	//////////////////////////////////////////////////////////////////////////
} // namespace HUS
