#pragma once
#include "Memory/MapWithLock.h"
#include "Main/UnifiedMessage.h"
#include "DataStoreBase.h"
#include "DB/SQliteORM.h"
class CDataStore :public CModuleWithIQ, public CDataStoreDevice<DeviceObject>
{
public:
	CDataStore();
	~CDataStore();

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("DATASTORE");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};
	using STR_MAP_LOCK_T_PTR = CMapWithLock<CString, LPCSTR, CString, CString&> *;
	using HUSDEV_MAP_LOCK_T = CMapWithLock<CString, LPCSTR, DeviceObject, DeviceObject&>;


	// 初始化数据
	void Init() override;

	// 清理数据
	void Cleanup() override;

	static UINT pfnDbModifyProc(LPVOID lParam);


	static void AddDeviceObjectByGUID(const char* pszGUID, DeviceObject & pDevObj, BOOL isLock = TRUE);


	// 从设备表中移除设备对象
	static void RemoveDeviceChildLinkByGUID(const char* pszChildGUID, const char* pszParentGUID);

	static  HUSDEV_MAP_LOCK_T * GetAllDeviceList();

	static  CatalogItem * GetCatalogByStrGUID(const char* pszGUID);

	// 获取设备对象数量
	static int GetDeviceCount();

	// 添加国标ID与GUID的映射(覆盖)，会自动执行AddGUID()
	static void AddDeviceID(const char* pszDeviceID, const char* pszGUID, BOOL isLock = TRUE);

	// 添加GUID与国标ID的映射(覆盖)，会自动执行AddDeviceID()
	static void AddGUID(const char* pszGUID, const char* pszDeviceID, BOOL isLock = TRUE);

	static void RemoveDevice(const char* pszDeviceID, BOOL isLock = TRUE);

	static void RemoveDeviceItemByGUID(const char* pszGUID, BOOL isLock = TRUE);

	static void RemoveDeviceByGUID(const char * pszGUID, BOOL isLock = TRUE);

	// 获取设备GUID
	static BOOL LookupGUID(const char* pszDeviceID, CString& strGUID, BOOL isManulUnlock = FALSE);
	// 获取设备DeviceID (即20位GBID)
	static BOOL LookupDeviceID(const char* pszGUID, CString& strDeviceID, BOOL isManulUnlock = FALSE);

	static BOOL LookupDeviceByGUID(const char * pszGUID, DeviceObject & info, BOOL isManulUnlock = FALSE);

	static BOOL LookupDevice(const char * pszDeviceID, DeviceObject & info, BOOL isManulUnlock = FALSE);

	static void ManualUnlockDeviceInfoMap();
	static void ManualUnlockGUIDMap();
	static void ManualUnlockDeviceIDMap();

	static void SetDeviceAlarmStatus(const char* pszGUID, DeviceAlarmInfo& alarminfo, BOOL isManulUnlock = FALSE);

	static void RemoveDeviceAlarmStatus(const char * pszGUID, BOOL isManulUnlock = FALSE);

	static BOOL LookupDeviceAlarmStatus(const char * pszDeviceID, DeviceAlarmInfo& tServiceInfo, BOOL isManulUnlock = FALSE);


	static void AddAlarmInfo(const char * pszDeviceID, DeviceAlarmInfo& alarminfo);

	static void RemoveAlarming(const char * pszDeviceID = nullptr);

	static std::vector<DeviceAlarmInfo> GetRecentAlarmAll();


	static STR_MAP_LOCK_T_PTR GetDeviceIDMap();


protected:
	bool HandleMsg(CMemPoolUnit* pUnit) override;

private:


	CMapWithLock<CString, LPCSTR, CString, CString&> m_DeviceIDMap; // 设备表 key: DeviceID value: GUID
	CMapWithLock<CString, LPCSTR, CString, CString&> m_GUIDMap; // 设备反向查找表 key: GUID(HUS) value: DeviceID(GB)

	CMap<INT64, INT64, CString, LPCSTR> m_oSubIDMap;


	//报警信息的缓存
	std::unordered_map<std::string, std::vector<DeviceAlarmInfo>> m_oAlarmingDeviceMap;

	//CDataStoreDevice<INT> m_oStatusChangeMap;
	DataBaseMgr dbm;
	static CDataStore * m_pStore;
	friend class DevicesInfoMgr;
};
