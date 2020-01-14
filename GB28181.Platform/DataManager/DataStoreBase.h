#pragma once
#include "Memory/MapWithLock.h"
#include "Main/UnifiedMessage.h"
template<typename T = DeviceObject>
class CDataStoreDevice
{
public:
	CDataStoreDevice() {};
	virtual ~CDataStoreDevice() {};


	BOOL AddObject(const char* pszKeyStr, T & devObj, BOOL isLock = TRUE);

	// 从设备表中移除设备对象
	BOOL RemoveObject(const char* pszKeyStr =nullptr, BOOL isLock = TRUE);

	// 获取设备对象数量
	int GetDeviceCount();

	// 获取设备对象
	BOOL LookupObject(const char* pszKeyStr, T & devObj, BOOL isManulUnLock);

	T &  operator[](const char* pszKeyStr);

protected:
	void ManualUnlockDeviceInfoMap();

	// 设备对象，当被实例化为HUS的DeviceObject时,key是HUS的GUID
	// 全部对象的类型信息，包括设备、报警视频通道通道、解码通道
	CMapWithLock<CString, LPCSTR, T, T&> m_ObjectMap;
};
