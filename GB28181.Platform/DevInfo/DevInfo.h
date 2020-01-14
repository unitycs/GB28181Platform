#pragma once
#include "SDKCom/SDKDecoder.h"
#include "CatalogLayer.h"
#include "SubscribeList.h"
#include "DataManager/DataStore.h"
#include "Memory/ObjectAllocator.hpp"
#include "SDKCom/SDKInfoSearcherMgr.h"
typedef struct SubInfoStruct
{
	INT64    nCallID;
	CString  szToDeviceID;
	CString  szSN;
	CString  remoteId;
	int      deviceIdType;
	long     startTime;
	long     endTime;
	long     expireTime;
	CString  guid;
}SubInfo_t;

typedef CList<SubInfo_t, SubInfo_t&> SubInfoList;
class CDevInfo :
	public CModuleWithIQ
{
public:
	CDevInfo(void);
	~CDevInfo(void) = default;

	//using STR_MAP_LOCK_T_PTR = CDataStore::STR_MAP_LOCK_T_PTR;
	void Init(void) override;
	void Cleanup(void) override;

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("DEVINFO");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};

private:

	CMap<CString, LPCSTR, CString, CString&> m_oGUIDToGUIDMap;
	std::unordered_map<GBDevice_T, std::string>  m_GBDevTypeNameMap;
	std::unordered_map<std::string, const char*>  m_CatalogPush; //为了快速取值
	CAllocator<CModMessage> m_MemAllocator;
	CatalogCollections m_CatalogCollections;

	CSubscribeList	m_oSubscribeList;

	CCatalogLayer	m_oCatalogLayer;

	//CCivilCode		m_oCivilCodeInfo; //TODO
	int		m_nCivilLevel = 0;

	CString str_GatewayName;
	CString m_strRemotePltID;

#ifndef NO_VIRTUAL_CATALOG

	CatalogCollections m_devVirtual;
#endif /* NO_VIRTUAL_CATALOG */

	//
	CMap<INT64, INT64, SubInfo_t*, SubInfo_t*&> m_oSubscribeMap; // key:dialogid;	value:subinfo

	std::unordered_map<std::string, SubAlarmInfo_t> m_oSubAlrmascribeMap;

	CMap<INT64, INT64, CString, LPCSTR> m_oSubIDMap;

	CCriticalSection m_catalogSubscribeLock;

	CCriticalSection m_oLock;

	SDKDecoder decoder;

	static UINT AFX_CDECL pfnSearchAlarmRecordProc(LPVOID lParam);

protected:

	int InitSigleDeviceInfo(const char *strFilePath, CString strGuid, InfoChangedList *pChangeList = nullptr, DeviceObject *pDeviceObject = nullptr);
	//int InitSigleDeviceInfo(CString DeviceId, InfoChangedList *pChangeList);
	static int InitCalog(CatalogItem *pCatalog, CString szinfo, CString guid, const char *pParentType = nullptr, const char * = nullptr);
	
	int InitDevComBaseInfo();

	int ReadCatalog(const CString &strFileName, const CString &strSection, CatalogItem *pCatalog, const char *pParentType = nullptr, const char *pszParentGUID = nullptr);

	bool HandleMsg(CMemPoolUnit * pUnit) override;

	int HandlePropertyQuery(CModMessage * pUnifiedMsg);

	int SendAllCatalog(CModMessage *pUnifiedMsg);

	int SendPlatformCatalog(CModMessage *pUnifiedMsg, int recordCount);

#ifndef NO_VIRTUAL_CATALOG
	int SendVirtualCatalog(CModMessage * pUnifiedMsg, int recordCount);
#endif /* NO_VIRTUAL_CATALOG */

	int SendDeviceCatalogAll(CModMessage *pMsg, int recordCount, int eveType = -1);

	int SendSingeDecoderCatalog(CModMessage * pUnifiedMsg);

	int SendSingeDecoderOneChnCatalog(CModMessage * pUnifiedMsg);

	int SendSingeDecoderOneChnOneDivisonCatalog(CModMessage * pUnifiedMsg);

	int SendCatalogSubscribeResult(CModMessage * pUnifiedMsg, char *type);

	int HandleCatalogQuery(CModMessage * pUnifiedMsg);

#ifndef NO_VIRTUAL_CATALOG

	void InitVirtualCatalog();
#endif /* NO_VIRTUAL_CATALOG */

	int HandleUpdateData(CModMessage * pUnifiedMsg);

	int InitAllDeviceInfo();

	int AlarmNotify(CString strGUID);

	int AddDeviceInfo(const char* pszGUID);

	int UpdateDeviceInfo(const char* pszGUID);

	int RemoveDeviceInfo(const char* pszGUID);

	int UpdateDeviceInfoNotify(CatalogItem *pNewCatalog, BOOL bNew);

	int RemoveDeviceInfoNotify(CatalogItem *pOldCatalog);

	int OnLineInfoNotify(CatalogItem *pCatalog, int eveType);

	int SendCatalogSubscribeNotify(SubInfo_t subInfo, CatalogItem* pCatalog, int eveType);

	int HandleSubscribe(CModMessage * pUnifiedMsg);

	int HandleSubscribeNotify(CModMessage * pUnifiedMsg);

	int HandleAlarmSubscribe(CModMessage* pUnifiedMsg);

	int HandleOnlineStatus(CModMessage* pUnifiedMsg);

	int HandleDecoderStatusSubscribeNotify(CModMessage * pUnifiedMsg);

	int HandlePersetSearch(CModMessage * pUnifiedMsg);

	int HandleAlarmQuery(CModMessage * pUnifiedMsg);

	int SendAlarmRecordQueryResult(AlarmRecordsMgr& oRecord);

	void subscribeCatalogReport(CatalogCollections *chnInfo, char *type);

	BOOL GetSubscribeInfo(const char *pszDeviceID, SubInfoList *subInfolist);

	bool judgeTime(tm tmInfo1, CString strAlarmTime);
	
	void SendAlarm(DeviceAlarmInfo* AlaInfo, CString DeviceId);

	int AlarmReport(const char *pszGBID, const char *pszDescription, CString strLevel, const char *pszTime, int alarmMethod, int alarmType, CString strAlarmStatus);



	BOOL CheckDeviceObj(const char *deviceId);

	AlarmRecordsMgr m_oDevAlarmRecord;
};
