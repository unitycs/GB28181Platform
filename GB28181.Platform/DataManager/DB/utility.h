// utility.h: HUS module's utilities.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "../../stdafx.h"
#include <list>
#include <map>
#include <mutex>
#include "threadSync.h"
#include <condition_variable>
//////////////////////////////////////////////////////////////////////////
// CRefPtrT:
template<class Ty>
class CRefPtrT {
public:
	typedef Ty* LPTy;

public:
	CRefPtrT() : m_lpTy(nullptr) {
		AddRef();
	}

	CRefPtrT(const Ty& lpTy) : m_lpTy(&const_cast<Ty&>(lpTy)) {
		AddRef();
	}

	CRefPtrT(const CRefPtrT& pRef) : m_lpTy(pRef.m_lpTy) {
		AddRef();
	}

	~CRefPtrT() {
		Release();
	}

	void Lock() const {
		if (m_lpTy != nullptr) {
			m_lpTy->Lock();
		}
	}

	void Unlock()const {
		if (m_lpTy != nullptr) {
			m_lpTy->Unlock();
		}
	}

	const Ty& operator*() const {
		if (m_lpTy == nullptr) {
			ATLASSERT(FALSE);
			return *static_cast<LPTy>(nullptr);
		}

		return (*m_lpTy);
	}

	LPTy operator&() const {
		return m_lpTy;
	}

public:
	void operator=(const CRefPtrT& pRef) {
		Release();

		const_cast<LPTy&>(m_lpTy) = pRef.m_lpTy;
		AddRef();
	}

private:
	void AddRef() const {
		if (m_lpTy != nullptr) {
			const_cast<Ty&>(*m_lpTy).AddRef();
		}
	}

	void Release() const {
		if (m_lpTy == nullptr) return;

		if (const_cast<Ty&>(*m_lpTy).Release() <= 0) {
			delete static_cast<LPTy>(m_lpTy);
			const_cast<LPTy&>(m_lpTy) = nullptr;
		}
	}

private:
	const LPTy m_lpTy;
};

//////////////////////////////////////////////////////////////////////////
// CRefT:
template<class T>
class CRefT {
public:
	CRefT(T& pDat) : m_pDat(pDat) {
	}

public:
	void Rebind(const T& pSrc) {
		*static_cast<T**>(this) = static_cast<T*>(&pSrc);
	}

public:
	T* operator&() {
		return &m_pDat;
	}

	T& operator*() {
		return m_pDat;
	}

	T& operator=(const T& pSrc) {
		Rebind(pSrc);
		return m_pDat;
	}

public:
	T& m_pDat;
};

class IFile {
public:
	virtual ~IFile()
	{
	}

	virtual BOOL OpenFile(LPCTSTR strFile, BOOL bWritable) = 0;
	virtual BOOL CreateFile(LPCTSTR strFile) = 0;
	virtual BOOL Close() = 0;

	virtual ULONG Size() const = 0;
	virtual BOOL Seek(long nOffset) = 0;
	virtual long Read(void* lpBuf, long nLen) = 0;
	virtual long Write(void* lpBuf, long nLen) = 0;
	virtual BOOL isFileSupport() const = 0;
	virtual BOOL Flush() = 0;
	virtual LPCTSTR FileName() const = 0;
	virtual BOOL SetLength(ULONG nNewSize) = 0;
};

//////////////////////////////////////////////////////////////////////////
// GUID_Less: GUID compare method.
class GUID_Less {
public:
	bool operator()(const GUID& x, const GUID& y) const {
		return (memcmp(&x, &y, sizeof(GUID)) < 0);
	}
};

//////////////////////////////////////////////////////////////////////////
// CConnectorPool:
template<class T, class TVal, class TQueue>
class CQueueImpl : public std::mutex {
public:
	typedef CQueueImpl<T, TVal, TQueue> _SuperBase;
	typedef  TQueue _Queue;
	typedef  TVal _Val;

	typedef std::unique_lock<std::mutex> _MutexScopeLocker;

	int Count() /*const*/ {
		_MutexScopeLocker pLock(*static_cast<CQueueImpl*>(this));
		return m_pQueue.size();
	}

	int Add(const _Val& pVal) {
		_MutexScopeLocker pLock(*this);
		static_cast<T&>(*this)._Add(m_pQueue, pVal);
		return m_pQueue.size();
	}

	int Remove(const _Val& pVal) {
		_MutexScopeLocker pLock(*this);
		static_cast<T&>(*this)._Remove(m_pQueue, pVal);

		return m_pQueue.size();
	}

protected:
	template<class TQ>
	void _Add(const TQ& pQ, const _Val& pVal) {
		m_pQueue.push(pVal);
	}

	void _Add(const std::list<_Val>& pQ, const _Val& pVal) {
		m_pQueue.push_back(pVal);
	}

	template<class TQ>
	void _Remove(const TQ& pQ, const _Val& pVal) {
		const_cast<_Val&>(pVal) = m_pQueue.top();
		m_pQueue.pop();
	}

	void _Remove(const std::list<_Val>& pQ, const _Val& pVal) {
		m_pQueue.remove(pVal);
	}

	_Queue m_pQueue;

	friend class CStreamer;
};

//////////////////////////////////////////////////////////////////////////
// CSynQueueT: Synchronized queue.
template<class TVal>
class CSynQueueT : public std::mutex {
public:
	typedef std::list<TVal> _Queue;
	typedef std::unique_lock<std::mutex> _MutexScopeLocker;
	typedef   typename std::list<TVal>::iterator  _Iterator;

	int Count() const {
		_MutexScopeLocker pLock(const_cast<CSynQueueT&>(*this));
		return m_pQueue.size();
	}

	void push(const TVal& pVal) {
		_MutexScopeLocker pLock(*this);
		m_pQueue.push_back(pVal);
		cv.notify_one();
	}

	//void Remove(const TVal& pVal) {
	//	_MutexScopeLocker pLock(*this);
	//	m_pQueue.remove(pVal);
	//}
	void clear()
	{
		_MutexScopeLocker pLock(*this);
		m_pQueue.clear();
	}
	bool Pop(TVal& tval)
	{
		_MutexScopeLocker pLock(*this);
		cv.wait(pLock);
		try
		{
			tval = std::move(m_pQueue.front());
			m_pQueue.pop_front();
		}
		catch (const std::exception&)
		{
			return false;
		}
		return true;
	}

private:
	_Queue m_pQueue;
	std::condition_variable  cv;
};

//////////////////////////////////////////////////////////////////////////
// CSynMapT: Synchronized queue.
template<class TKey, class TVal, class TKeyCmp>
class CSynMapT : public HUS::mutex {
public:
	typedef std::map<TKey, TVal, TKeyCmp> _Map;

	typedef HUS::scoped_lock<CSynMapT> _MutexScopeLocker;

	int Count() const {
		//_MutexScopeLocker pLock(const_cast<CSynMapT&>(*this));
		return m_pMap.size();
	}

	BOOL Add(const TKey& pKey, const TVal& pVal) {
		_MutexScopeLocker pLock(*this);

		if (m_pMap.find(pKey) != m_pMap.end()) {
			return FALSE;
		}

		m_pMap.insert(std::make_pair(pKey, pVal));
		return TRUE;
	}

	TVal& Add(const TKey& pKey) {
		_MutexScopeLocker pLock(*this);
		return m_pMap[pKey];
	}

	BOOL Remove(const TKey& pKey) {
		_MutexScopeLocker pLock(*this);

		if (m_pMap.find(pKey) == m_pMap.end()) {
			return FALSE;
		}

		m_pMap.erase(pKey);
		return TRUE;
	}

	BOOL Exist(const TKey& pKey) {
		_MutexScopeLocker pLock(*this);
		return (m_pMap.find(pKey) != m_pMap.end());
	}

public:
	_Map m_pMap;
};

//////////////////////////////////////////////////////////////////////////
// CUtility:
class CUtility {
public:
	static __int64 NormalMiniTime(__int64 nTime);
	static __int64 GetMiniTime();
	static time_t GetSecTime();
	static __int64 Differs(__int64 nMini1, __int64 nMini2);

	static long SecsFromMiniTime(__int64 nTmMini);
	static long MinisFromMiniTime(__int64 nTmMini);
};

//////////////////////////////////////////////////////////////////////////
// System memory information
class CSysMemory {
public:
	typedef struct MemoryStatusEx {
		DWORD dwLength;
		DWORD dwMemoryLoad;
		DWORDLONG ullTotalPhys;
		DWORDLONG ullAvailPhys;
		DWORDLONG ullTotalPageFile;
		DWORDLONG ullAvailPageFile;
		DWORDLONG ullTotalVirtual;
		DWORDLONG ullAvailVirtual;
		DWORDLONG ullAvailExtendVirtual;
	} *LPMemoryStatusEx;

	typedef BOOL(WINAPI *GetMemoryStatusEx)(LPMemoryStatusEx lpBuffer);

	CSysMemory() : m_funcGetMemoryStatusEx(nullptr), m_hKernal(nullptr), m_bAsStatusEx(FALSE) {
		m_hKernal = ::LoadLibraryEx(_T("kernel32.dll"), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

		if (m_hKernal != nullptr) {
			m_funcGetMemoryStatusEx = reinterpret_cast<GetMemoryStatusEx>(GetProcAddress(m_hKernal, "GlobalMemoryStatusEx"));
		}

		m_pMemoryStatusEx.dwLength = sizeof(m_pMemoryStatusEx);
		m_pMemoryStatus.dwLength = sizeof(m_pMemoryStatus);
		UpdateMemoryInfo();
	}

	~CSysMemory() {
		if (m_hKernal != nullptr) {
			::FreeLibrary(m_hKernal);
		}
	}

public:
	void UpdateMemoryInfo() {
		m_bAsStatusEx = FALSE;
		if (m_funcGetMemoryStatusEx != nullptr) {
			m_bAsStatusEx = m_funcGetMemoryStatusEx(&m_pMemoryStatusEx);
		}

		if (!m_bAsStatusEx) {
			GlobalMemoryStatus(&m_pMemoryStatus);
		}
	}

protected:
	GetMemoryStatusEx m_funcGetMemoryStatusEx;
	HMODULE m_hKernal;

protected:
	BOOL m_bAsStatusEx;
	MEMORYSTATUS m_pMemoryStatus;
	MemoryStatusEx m_pMemoryStatusEx;
};
//////////////////////////////////////////////////////////////////////////
// CBufferT:
template<class E, long Len, BOOL Fixed>
class CBufferT {
public:
	typedef E* LPE;

	enum SeekDir {
		BEGIN = 0,
		END = 1,
		CUR = 2
	};

public:
	CBufferT() : m_lpHeader(nullptr), m_lpPos(nullptr), m_lpEnd(nullptr), m_bInternalMemory(FALSE) {
		if (Len > 0) {
			Allocate(Len);
		}
	}

	virtual ~CBufferT() {
		clear();
	}

public:
	LPE header(long nPos = 0) const {
		if (m_lpHeader == nullptr) {
			if (Fixed) {
				const_cast<CBufferT<E, Len, Fixed>*>(this)->Allocate(Len);
			}
			else {
				return nullptr;
			}
		}

		if ((m_lpEnd - m_lpHeader) < nPos) return nullptr;
		return (m_lpHeader + nPos);
	}

	BOOL append(LPE lpData, long nCount) {
		long nNeedCount = ((m_lpPos - m_lpHeader) + nCount);
		long nCurrentCount = (m_lpEnd - m_lpHeader);

		if (nNeedCount > nCurrentCount) {
			if (Fixed) {
				if (m_lpHeader == nullptr && nNeedCount < Len) {
					if (!Allocate(Len)) {
						return FALSE;
					}
				}
				else {
					return FALSE;
				}
			}
			else {
				if (!Allocate(nNeedCount)) {
					return FALSE;
				}
			}
		}

		if (m_lpHeader == nullptr) return FALSE;
		memcpy(m_lpPos, lpData, (sizeof(E) * nCount));
		m_lpPos += nCount;
		return TRUE;
	}

	long size() const {
		return (m_lpPos - m_lpHeader);
	}

	void clear() {
		if (m_bInternalMemory && m_lpHeader != nullptr) {
			delete[] m_lpHeader;
			m_lpHeader = nullptr;
		}

		m_bInternalMemory = FALSE;
		m_lpHeader = nullptr;
		m_lpEnd = nullptr;
		m_lpPos = nullptr;
	}

	BOOL seek(long nPos, SeekDir eDir = BEGIN) {
		long nAllCount = (m_lpEnd - m_lpHeader);
		nPos = (eDir == BEGIN ? nPos : (eDir == END ? (nAllCount + nPos) : ((m_lpPos - m_lpHeader) + nPos)));

		if (nPos < 0 || nPos >= nAllCount) {
			ATLASSERT(FALSE);
			return FALSE;
		}

		m_lpPos = (m_lpHeader + nPos);
		return TRUE;
	}

	BOOL shift(long nDPos, long nSPos, long nCount) {
		long nAllCount = (m_lpEnd - m_lpHeader);
		if ((nDPos + nCount) >= nAllCount) {
			ATLASSERT(FALSE);
			return FALSE;
		}

		if ((nSPos + nCount) >= nAllCount) {
			ATLASSERT(FALSE);
			return FALSE;
		}

		memmove((m_lpHeader + nDPos), (m_lpHeader + nSPos), sizeof(E) * nCount);
		return TRUE;
	}

	E& operator[](long nIndex) {
		long nAllCount = (m_lpEnd - m_lpHeader);
		if (nIndex >= nAllCount) {
			ATLASSERT(FALSE);
		}

		return (*(m_lpHeader + nIndex));
	}

public:
	BOOL assign(LPE lpHeader, long nLen, long nPos) {
		if (m_bInternalMemory) {
			ATLASSERT(FALSE);
			return FALSE;
		}

		m_lpHeader = lpHeader;
		m_lpPos = (lpHeader + nPos);
		m_lpEnd = (lpHeader + nLen);
		return TRUE;
	}

protected:
	BOOL Allocate(long nLen) {
		nLen = max(nLen, 5);

		if (nLen > (m_lpEnd - m_lpHeader)) {
			if (m_lpHeader != nullptr && !m_bInternalMemory) {
				ATLASSERT(FALSE);
				return FALSE;
			}

			LPE lpHeader = new E[nLen];
			m_bInternalMemory = TRUE;

			if (m_lpHeader != nullptr) {
				memcpy(lpHeader, m_lpHeader, (sizeof(E) * (m_lpPos - m_lpHeader)));
			}

			m_lpPos = (lpHeader + (m_lpPos - m_lpHeader));
			m_lpEnd = lpHeader + nLen;
			m_lpHeader = lpHeader;
		}

		return TRUE;
	}

protected:
	LPE	m_lpHeader, m_lpPos, m_lpEnd;
	BOOL m_bInternalMemory;
};

static long long GetUtcFromDatatime(const INT64 data_time) {
	CTime tm(static_cast<time_t>(data_time >> 32));

	SYSTEMTIME st;
	tm.GetAsSystemTime(st);
	st.wMilliseconds = static_cast<WORD>(data_time);
	FILETIME ft, ft_utc;
	::SystemTimeToFileTime(&st, &ft);
	::LocalFileTimeToFileTime(&ft, &ft_utc);
	return *reinterpret_cast<long long *>(&ft_utc);
}

static INT64 GetDatatimeFromUtc(long long utc_time) {
	FILETIME ft_local;
	::FileTimeToLocalFileTime(reinterpret_cast<FILETIME*>(&utc_time), &ft_local);
	SYSTEMTIME st;
	::FileTimeToSystemTime(&ft_local, &st);
	CTime ct(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	INT64 datetime_64;
	datetime_64 = (static_cast<INT64>(ct.GetTime()) << 32) | st.wMilliseconds;
	return datetime_64;
}