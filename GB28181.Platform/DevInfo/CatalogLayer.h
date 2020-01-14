#pragma once
#include "Memory/MapWithLock.h"
#include "Afxtempl.h"

typedef CList<CString, LPCSTR> CivilList;
typedef CMap<CString, LPCSTR, INT, INT&> CCatalogList;
typedef CMapWithLock<CString, LPCSTR, CCatalogList*, CCatalogList*&> CounterMap;
typedef CMapWithLock<CString, LPCSTR, INT, INT&> DeviceQueue;

class CCatalogLayer {
public:
	CCatalogLayer();
	~CCatalogLayer();

	// 加入设备ID创建目录层级结构
	// 只保存父设备
	int AddDevice(const char *pszDeviceID);

	// 删除设备及设备对应的目录层级结构
	int DelDevice(const char *pszDeviceID);

	// 查询目录层级
	int FindDeviceLayer(const char *pszCivilCode, const char* pszGWID, CivilList &oCivilList);
protected:
	void ErgodicRead(const char *pszCivilCode, CivilList &oCivilList);
private:
	CounterMap m_oProvinceCount;
	CounterMap m_oCityCount;
	CounterMap m_oDistrictCount;
	CounterMap m_oStationCount;
	DeviceQueue m_oDeviceQueue;
	CCriticalSection m_oLock;
};