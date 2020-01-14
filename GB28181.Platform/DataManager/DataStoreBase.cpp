#include "stdafx.h"
#include "DataStoreBase.h"


template<typename T>
BOOL CDataStoreDevice<T>::AddObject(const char * pszKeyStr, T & pDevObj, BOOL isLock)
{
	if (nullptr == pszKeyStr) return FALSE;
	CString mykeyStr(pszKeyStr);
	return	m_ObjectMap.SetAt(mykeyStr, pDevObj, isLock);
}

template<typename T>
BOOL CDataStoreDevice<T>::RemoveObject(const char * pszKeyStr, BOOL isLock)
{
	if (nullptr == pszKeyStr)
	{
		m_ObjectMap.RemoveAll(isLock);
	}

	CString myKeyID(pszKeyStr);
	T tmpDevObj; // FOR LOOKUP
	auto bFind = m_ObjectMap[myKeyID];
	if (bFind)
	{
		return	m_ObjectMap.RemoveKey(pszKeyStr, isLock);
	}
	return TRUE;
}

template<typename T>
int CDataStoreDevice<T>::GetDeviceCount()
{
	return m_ObjectMap.GetSize();
}
template<typename T>
BOOL CDataStoreDevice<T>::LookupObject(const char * pszKeyStr, T & pDevObj, BOOL isManulUnLock = FALSE)
{
	if (nullptr == pszKeyStr) return FALSE;
	CString myGUID(pszKeyStr);
	return m_ObjectMap.Lookup(myGUID, pDevObj, isManulUnLock);
}

template<typename T>
void CDataStoreDevice<T>::ManualUnlockDeviceInfoMap()
{
	m_ObjectMap.ManualUnlock();
}

template<typename T>
T & CDataStoreDevice<T>::operator[](const char * pszKeyStr)
{
	return m_ObjectMap[pszKeyStr];
	// TODO: insert return statement here
}




