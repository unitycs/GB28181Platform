#include "stdafx.h"
#include "DataStore.h"
#include "DataStoreBase.h"
#include "Main/UnifiedMessage.h"

CDataStore * CDataStore::m_pStore = nullptr;

bool CDataStore::HandleMsg(CMemPoolUnit * /*pUnit*/)
{
	return false;
}

CDataStore::CDataStore() :CDataStoreDevice<DeviceObject>()
{
	m_pStore = this;
}

CDataStore::~CDataStore()
{
	//CDataStoreDevice<DeviceObject>();
}

void CDataStore::Init()
{
	// 注册Module消息处理函数
	RegisterProc(pfnQueueProc, this, 1);

	RegisterProc(pfnDbModifyProc, this, 1);

}

void CDataStore::Cleanup()
{
	CString myKey;

	m_DeviceIDMap.RemoveAll();
	m_GUIDMap.RemoveAll();
}

UINT AFX_CDECL CDataStore::pfnDbModifyProc(LPVOID lParam)
{
	auto pDataStore = reinterpret_cast<CDataStore*>(lParam);

	auto paramSets = make_tuple(pDataStore, lParam);
	//todo ...
	// we can do sth here...
	m_pStore->dbm.StartDirtyWorkProc(reinterpret_cast<LPVOID>(&paramSets));

	return 0;
}


void CDataStore::AddDeviceObjectByGUID(const char * pszGUID, DeviceObject & pDevObj, BOOL isLock)
{
	if (nullptr == pszGUID) return;
	m_pStore->m_ObjectMap.SetAt(pszGUID, pDevObj), isLock;

	//m_pStore->AddDevice(pszGUID, pDevObj);
}



CDataStore::HUSDEV_MAP_LOCK_T * CDataStore::GetAllDeviceList()
{

	return &(m_pStore->m_ObjectMap);

}


CatalogItem * CDataStore::GetCatalogByStrGUID(const char * pszGUID)
{
	DeviceObject husDevObject;
	m_pStore->m_ObjectMap.Lookup(pszGUID, husDevObject);
	return husDevObject.GetGBDescription()->GetCatalog();
}



int CDataStore::GetDeviceCount()
{
	return m_pStore->m_ObjectMap.GetSize();
}


void CDataStore::AddDeviceID(const char* pszDeviceID, const char* pszGUID, BOOL isLock)
{
	if (nullptr == pszDeviceID || nullptr == pszGUID) return;
	//m_pStore->RemoveDevice(pszGUID, isLock);会自动覆盖，不用删除

	CString myDeviceID(pszDeviceID);
	CString myGUID(pszGUID);
	m_pStore->m_DeviceIDMap.SetAt(myDeviceID, myGUID, isLock);
	m_pStore->m_GUIDMap.SetAt(myGUID, myDeviceID, isLock);
}

void CDataStore::AddGUID(const char* pszGUID, const char* pszDeviceID, BOOL isLock)
{
	if (nullptr == pszDeviceID || nullptr == pszGUID) return;
	CString myDeviceID(pszDeviceID);
	CString myGUID(pszGUID);
	m_pStore->m_GUIDMap.SetAt(myGUID, myDeviceID, isLock);
	m_pStore->m_DeviceIDMap.SetAt(myDeviceID, myGUID, isLock);
}

void CDataStore::RemoveDevice(const char* pszDeviceID, BOOL isLock)
{
	if (nullptr == pszDeviceID) return;

	CString myDeviceID(pszDeviceID);
	CString tmpGUID;  // FOR LOOKUP
	BOOL bFind = m_pStore->m_DeviceIDMap.Lookup(myDeviceID, tmpGUID);
	if (bFind)
	{
		m_pStore->m_DeviceIDMap.RemoveKey(myDeviceID, isLock);
	}
}

void CDataStore::RemoveDeviceChildLinkByGUID(const char* pszChildGUID, const char* pszParentGUID)
{

	if (nullptr == pszChildGUID || pszParentGUID == nullptr) return;

	CString myChildGUID(pszChildGUID);
	CString myParentGUID(pszParentGUID);
	DeviceObject tmpDevinfo; // FOR LOOKUP

	auto bFind = m_pStore->m_ObjectMap.Lookup(myParentGUID, tmpDevinfo);

	if (bFind)
	{
		auto pSubDevDes = tmpDevinfo.GetVMSDescription();
		if (pSubDevDes->subDevStrGUIDMap.count(myChildGUID.GetString()) > 0)
		{
			pSubDevDes->subDevStrGUIDMap.erase(myChildGUID.GetString());
		}
		m_pStore->m_ObjectMap.SetAt(myParentGUID, tmpDevinfo);
	}

}


//删除单个设备信息,但是不删除关联关系
void CDataStore::RemoveDeviceItemByGUID(const char * pszGUID, BOOL isLock)
{
	if (nullptr == pszGUID) return;
	CString strTargetGUID(pszGUID);
	CString strDeviceID;
	//检查ID映射表
	auto bFind = m_pStore->m_GUIDMap.Lookup(pszGUID, strDeviceID);
	if (bFind)
	{
		m_pStore->m_GUIDMap.RemoveKey(strTargetGUID, isLock);
		m_pStore->m_ObjectMap.RemoveKey(strTargetGUID, isLock);
	}
}

//删除整个关联关系树，包含子设备
void CDataStore::RemoveDeviceByGUID(const char * pszGUID, BOOL isLock)
{
	if (nullptr == pszGUID) return;
	CString strTargetGUID(pszGUID);
	DeviceObject tmpDevinfo; // FOR LOOKUP
	auto bFind = m_pStore->m_ObjectMap.Lookup(strTargetGUID, tmpDevinfo);
	if (bFind)
	{
		//删除全部子设备
		auto pHusDevDes = tmpDevinfo.GetVMSDescription();

		if (pHusDevDes->subDevStrGUIDMap.size() > 0)
			for (auto &item : pHusDevDes->subDevStrGUIDMap)
			{
				RemoveDeviceItemByGUID(item.first.c_str());
			}

		//删除自己
		RemoveDeviceItemByGUID(pszGUID, isLock);
	}

	if (!tmpDevinfo.GetStrGUIDParent().IsEmpty())
	{
		RemoveDeviceChildLinkByGUID(pszGUID, tmpDevinfo.GetStrGUIDParent());
	}

}




BOOL CDataStore::LookupGUID(const char* pszDeviceID, CString& strGUID, BOOL isManulUnlock)
{
	if (nullptr == pszDeviceID || _tcsclen(pszDeviceID) != 20) return FALSE;
	return m_pStore->m_DeviceIDMap.Lookup(pszDeviceID, strGUID, isManulUnlock);
}
BOOL CDataStore::LookupDeviceID(const char * pszGUID, CString & strDeviceID, BOOL isManulUnlock)
{
	if (nullptr == pszGUID) return FALSE;

	return m_pStore->m_GUIDMap.Lookup(pszGUID, strDeviceID, isManulUnlock);
}
BOOL CDataStore::LookupDeviceByGUID(const char * pszGUID, DeviceObject & info, BOOL isManulUnlock)
{
	CString strGUID = pszGUID;
	if (strGUID.IsEmpty()) return FALSE;

	if (m_pStore->m_ObjectMap.Lookup(strGUID, info, isManulUnlock))
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDataStore::LookupDevice(const char * pszDeviceID, DeviceObject & info, BOOL isManulUnlock)
{
	auto strGUID = m_pStore->m_DeviceIDMap[pszDeviceID];
	if (strGUID.IsEmpty()) return FALSE;
	DeviceObject husDeviceInfo;
	if (m_pStore->m_ObjectMap.Lookup(strGUID, husDeviceInfo, isManulUnlock))
	{
		info = husDeviceInfo;
		return TRUE;
	}

	return FALSE;
}

void CDataStore::ManualUnlockDeviceInfoMap()
{
	m_pStore->m_ObjectMap.ManualUnlock();
}
void CDataStore::ManualUnlockGUIDMap()
{
	m_pStore->m_GUIDMap.ManualUnlock();
}

void CDataStore::ManualUnlockDeviceIDMap()
{
	m_pStore->m_DeviceIDMap.ManualUnlock();
}

void CDataStore::SetDeviceAlarmStatus(const char * pszGUID, DeviceAlarmInfo& alarminfo, BOOL isManulUnlock)
{

	if (pszGUID == nullptr) return;

	DeviceObject  tmpdevinfo;
	auto b_ret = m_pStore->m_ObjectMap.Lookup(pszGUID, tmpdevinfo, isManulUnlock);

	if (b_ret)
	{
		auto p_Alarm = tmpdevinfo.GetAlarmInfo();
		*p_Alarm = alarminfo;
		m_pStore->m_ObjectMap.SetAt(pszGUID, tmpdevinfo);
	}
}


void CDataStore::RemoveDeviceAlarmStatus(const char * pszGUID, BOOL isManulUnlock)
{
	DeviceObject  tmpdevinfo;

	auto b_ret = m_pStore->m_ObjectMap.Lookup(pszGUID, tmpdevinfo, isManulUnlock);
	if (b_ret)
	{
		tmpdevinfo.GetAlarmInfo()->Clear();
		m_pStore->m_ObjectMap.SetAt(pszGUID, tmpdevinfo);
	}
}

BOOL CDataStore::LookupDeviceAlarmStatus(const char * pszDeviceID, DeviceAlarmInfo& tServiceInfo, BOOL isManulUnlock)
{
	auto strGUID = m_pStore->m_DeviceIDMap[pszDeviceID];

	if (strGUID.IsEmpty()) return FALSE;
	DeviceObject  tmpdevinfo;
	auto b_ret = m_pStore->m_ObjectMap.Lookup(strGUID, tmpdevinfo, isManulUnlock);
	if (b_ret)
	{
		auto p_Alarm = tmpdevinfo.GetAlarmInfo();
		tServiceInfo = *p_Alarm;
	}
	return b_ret == TRUE;
}


//默认缓存200条记录
void CDataStore::AddAlarmInfo(const char * pszDeviceID, DeviceAlarmInfo& alarminfo)
{

	auto strGUID = m_pStore->m_DeviceIDMap[pszDeviceID];

	auto ncount = m_pStore->m_oAlarmingDeviceMap[strGUID.GetString()].size();
	if (ncount >= 200) return;
	m_pStore->m_oAlarmingDeviceMap[strGUID.GetString()].push_back(alarminfo);

	if (!strGUID.IsEmpty())
	{
		SetDeviceAlarmStatus(strGUID, alarminfo);
	}
}

void CDataStore::RemoveAlarming(const char * pszDeviceID /*= nullptr*/)
{
	if (nullptr == pszDeviceID) return;
	auto strGUID = m_pStore->m_DeviceIDMap[pszDeviceID];
	auto ncount = m_pStore->m_oAlarmingDeviceMap[strGUID.GetString()].size();
	if (ncount == 0) return;
	////remove all
	for (auto &item : m_pStore->m_oAlarmingDeviceMap)
	{
		item.second.clear();
	}
	m_pStore->m_oAlarmingDeviceMap.clear();
}

std::vector<DeviceAlarmInfo> CDataStore::GetRecentAlarmAll()
{
	std::vector<DeviceAlarmInfo> allAlarmInfos;

	for (auto &item : m_pStore->m_oAlarmingDeviceMap)
	{
		allAlarmInfos.push_back(item.second[0]);
	}


	return allAlarmInfos;
}

CDataStore::STR_MAP_LOCK_T_PTR CDataStore::GetDeviceIDMap()
{
	return  &(m_pStore->m_DeviceIDMap);
}

