#include "StdAfx.h"
#include "SDKDevInfo.h"
#include "Main/MainThread.h"
#include "ServerConsole.h"
#include "Common/Utils.h"
#include "DataManager/DataStore.h"
#include "SDKCom.h"

CCatalog  g_testAlarmCatalog;
bool      g_addAlarmFlag;
//初始化
CSDKDevInfo * CSDKDevInfo::m_pCSDKDevInfo = nullptr;

CSDKDevInfo::CSDKDevInfo(void)
	:
	m_pAdaptorFactory(nullptr),
	m_ptrSynAdapter(nullptr),
	m_pSynClient(nullptr)
{
	m_pCSDKDevInfo = this;
	auto m_Gatewayid = appGlobal_ConfInfo.m_LocalPlatform.str_ID;
	m_GBID_Creater.initial(m_Gatewayid.GetString());
}

CSDKDevInfo::~CSDKDevInfo(void)
{
	if (m_pAdaptorFactory)
		m_pAdaptorFactory->Release();

	if (m_pSynClient)
		m_pSynClient->Release();
}

int CSDKDevInfo::Init(_Factory *&pAdaptorFactory, _SynClient *&pSynClient)
{
	pAdaptorFactory = nullptr;
	pSynClient = nullptr;

	if (FALSE == ReadDeviceTypeDef())
		return -1;

	if (FALSE == InitSynClient())
		return -1;

	pAdaptorFactory = m_pAdaptorFactory;
	pSynClient = m_pSynClient;
	m_oDecoderPairMap.SetExpiry(10);
	return 0;
}

void CSDKDevInfo::Cleanup()
{
	HUSDeviceLinkInfo *pChannelInfo = nullptr;

	auto pos = m_oDecoderMap.GetStartPos();
	while (pos)
	{
		CString strID;
		m_oDecoderMap.GetNext(pos, strID, pChannelInfo);
		if (pChannelInfo)
		{
			delete pChannelInfo;
			pChannelInfo = nullptr;
		}
	}

	if (m_pAdaptorFactory)
		m_pAdaptorFactory->Release();

	if (m_pSynClient)
		m_pSynClient->Release();
}

BOOL CSDKDevInfo::InitSynClient()
{
	CLog::Log(SDKCOM, LL_NORMAL, "连接VSM……");

	_AdaptorFactoryWrapperPtr pAdaptorFactoryWrapper;

	HRESULT hr;
	if (SUCCEEDED(hr = pAdaptorFactoryWrapper.CreateInstance(__uuidof(AdaptorFactoryWrapper))))
	{
		m_pAdaptorFactory = pAdaptorFactoryWrapper->GetAdaptorFactoryInstance();
	}
	else
	{
		if (FAILED(hr = CoCreateInstance(CLSID_Factory, nullptr, CLSCTX_INPROC_SERVER, IID__Factory, reinterpret_cast<LPVOID*>(&m_pAdaptorFactory))))
		{
			CLog::Log(SDKCOM, LL_NORMAL, "_Factory接口创建失败。");
			return FALSE;
		}
	}

	// 创建SynClient
	if (FAILED(hr = CoCreateInstance(CLSID_SynClient, nullptr, CLSCTX_INPROC_SERVER, IID__SynClient, reinterpret_cast<LPVOID*>(&m_pSynClient))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "_SynClient接口创建失败。");
		return FALSE;
	}
	// 在调用SynClient的其他方法之前，需设置服务类型；针对GB28181 Gateway。需要定义一个新的字符串，如“GB28181GW”；
	//目前将其定义为："GBGateway"
	CString strServiceTag = "GBGateway";

	m_pSynClient->PutServerType(strServiceTag.GetString());
	m_guidGateway = m_pSynClient->GetConfigID(strServiceTag.GetString());
	m_pAdaptorFactory->SiteAddress = m_pSynClient->GetSiteIP();
	// 初始化SynClient，需传入Gateway的GUID
	// 初始化过程会从VMS站点获取设备数据写到SiteTree文件（C:\Program Files (x86)\Common Files\Honeywell\HUS\SynchronizeFiles\...xml），比较耗时

	if (!m_pSynClient->Initialize(m_guidGateway))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "_SynClient初始化失败。。");
		return FALSE;
	}

	// 拿到SiteImageAdaptor对象
	m_ptrSynAdapter = m_pSynClient->SiteImageAdaptor;
	_bstr_t strProperties = m_ptrSynAdapter->GetDeviceProperties();
	m_pAdaptorFactory->DeviceTypeFromFile = FALSE;
	m_pAdaptorFactory->DeviceTypeData = strProperties;

	//	pSynAdapter->QueryInterface(IID_IDeviceConfig, (LPVOID*)&pDeviceConfig);
	//	pSynAdapter = pDeviceConfig;
	// 获取设备列表，参数是Gateway的GUID，返回值是设备数组
	// 这里返回的设备列表包括所有挂接在当前Gateway下的设备
	SAFEARRAY* arDevices = m_ptrSynAdapter->GetLinks(m_guidGateway);
	_ECElementPtr *arECElements = nullptr;

	if (FAILED(SafeArrayAccessData(arDevices, reinterpret_cast<void**>(&arECElements))))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "SafeArrayAccessData失败。");
		return FALSE;;
	}

	CLog::Log(SDKCOM, LL_NORMAL, "获取%d个设备对象的信息……", arDevices->rgsabound[0].cElements);
	for (UINT i = 0; i < arDevices->rgsabound[0].cElements; i++)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "访问设备[%d]", i);
		IniDeviveProperty(arECElements[i]);
	}

	SafeArrayUnaccessData(arDevices);
	SafeArrayDestroy(arDevices);
	CLog::Log(SDKCOM, LL_NORMAL, "信息获取完成。");
	return TRUE;
}

BOOL CSDKDevInfo::ChannelLookup(const char *pszKey, DeviceObject_t & husdeviceInfo)
{
	DeviceObject_t husDevceInfo;
	if (CDataStore::LookupHUSDeviceByDeviceID(pszKey, husDevceInfo, TRUE))
	{
		husdeviceInfo = husDevceInfo;
		return TRUE;
	}
	return FALSE;
	//return m_oChannelGBIDMap.Lookup(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::ChannelLookupEnd()
{
	CDataStore::ManualUnlockHUSDeviceInfoMap();
	//m_oChannelGBIDMap.ManualUnlock();
}

BOOL CSDKDevInfo::DecoderLookup(const char *pszKey, HUSDeviceLinkInfo *pInfo)
{
	return m_oDecoderMap.Lookup(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::DecoderLookupEnd()
{
	m_oDecoderMap.ManualUnlock();
}

BOOL CSDKDevInfo::DecoderPairRemove(const char *pszKey, DecoderPairInfo_t &pInfo)
{
	return m_oDecoderPairMap.RemoveKey(pszKey, pInfo);
}

void CSDKDevInfo::AlarmingReomve(const char *pszKey)
{
	if (nullptr == pszKey)
		m_oAlarmingDeviceMap.RemoveAll();
	else
		m_oAlarmingDeviceMap.RemoveKey(pszKey);
}

// 取得设备对应的EC和NVR信息
void CSDKDevInfo::SetOnlineStatusByGUID(const char *pszGUID, int isOffline)
{
	DeviceObject tInfo;
	CString strGUID = pszGUID;
	CString strID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	if (!CDataStore::LookupDeviceID(strGUID, strID)) return;
	DeviceObject deviceInfo;
	if (CDataStore::LookupHUSDevice(strGUID, deviceInfo, TRUE))
	{
		if (isOffline)
		{
			deviceInfo.extDevInfo.SetOnlineStatus(_T("OFFLINE"));
			INT evttype = EventNotify::UpdateType::ut_off;
			CDataStore::AddHUSDeviceObj(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(strGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "设备下线 GUID:%s", strID);
			p_oSDKCom->NoticeDevInfo(strGUID, EventNotify::UpdateType::ut_off, strID, ot_devinfo_subscribe_notify);
		}
		else
		{
			deviceInfo.extDevInfo.SetOnlineStatus(_T("ONLINE"));
			INT evttype = EventNotify::UpdateType::ut_on;
			CDataStore::AddHUSDeviceObj(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(pszGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "设备上线 GUID:%s", strID);
			p_oSDKCom->NoticeDevInfo(strGUID, EventNotify::UpdateType::ut_on, strID, ot_devinfo_subscribe_notify);
		}
	}
	else
		CLog::Log(SDKCOM, LL_NORMAL, _T("无GBID的设备状态变更 GUID:%s 当前设备上次状态码（0未离线,1离线）:%d"), strGUID, isOffline);

	CDataStore::ManualUnlockHUSDeviceInfoMap();
}

// 删除设备对象
void CSDKDevInfo::DeleteDeviceObject(const char *pszGUID)
{
	CString deviceGBID;

	if (CDataStore::LookupDeviceID(pszGUID, deviceGBID, TRUE))
	{
		//移除设备ID映射关系
		CDataStore::RemoveHUSDevice(pszGUID);
		CDataStore::RemoveDevice(deviceGBID);
	}
	else
	{
		CDataStore::ManualUnlockGUIDMap();
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备对象失败，未知的GUID：%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockGUIDMap();
	//删除HUS设备信息
	DeviceObject deviceToDelete;
	if (CDataStore::LookupHUSDevice(pszGUID, deviceToDelete, TRUE))
	{
		CDataStore::RemoveHUSDeviceObj(pszGUID);
	}
	else
	{
		CDataStore::ManualUnlockHUSDeviceInfoMap();
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备对象失败，未知的GUID：%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockHUSDeviceInfoMap();

	// 删除子设备
	if (deviceToDelete.eGBDevType == GBDeviceType::OT_DEVICE)
		while (0 < deviceToDelete.oSubDevGUIDList.GetCount())
		{
			CString strGUID;
			GUID guidSubDev = deviceToDelete.oSubDevGUIDList.RemoveHead();
			Utils::GUIDToCString(guidSubDev, strGUID);
			DeviceObject subDeviceToDelete;

			if (CDataStore::LookupHUSDevice(strGUID, subDeviceToDelete, TRUE))
			{
				CString strGUIDtoDelete;
				if (GBDeviceType::OT_CAMERA == subDeviceToDelete.eGBDevType)
				{
					if (CDataStore::LookupGUID(subDeviceToDelete.strDeviceID, strGUIDtoDelete, TRUE))
					{
						CDataStore::RemoveDevice(deviceToDelete.strDeviceID);
						CDataStore::RemoveHUSDevice(strGUIDtoDelete);
					}
					CDataStore::ManualUnlockDeviceIDMap();
				}
				else if (GBDeviceType::OT_ALARM == deviceToDelete.eGBDevType)
				{
					//m_oAlarmGBIDMap.RemoveKey(deviceToDelete.strDeviceID);
					CDataStore::RemoveDeviceAlarmStatus(deviceToDelete.strDeviceID);
				}
				else if (GBDeviceType::OT_DECODER == deviceToDelete.eGBDevType)
				{
					HUSDeviceLinkInfo_t* p_DeviceLinkInfo = nullptr;
					if (m_oDecoderMap.Lookup(deviceToDelete.strDeviceID, p_DeviceLinkInfo, TRUE))
					{
						m_oDecoderMap.RemoveKey(deviceToDelete.strDeviceID);
						delete p_DeviceLinkInfo;
						p_DeviceLinkInfo = nullptr;
					}
				}
			}
			CDataStore::ManualUnlockHUSDeviceInfoMap();
		}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备时忽略通道或流 设备的GUID：%s", pszGUID);
	}
}

// 更新设备信息
void CSDKDevInfo::UpdateGBDeviceIDByGUID(const char *pszGUID)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s", __FUNCTION__);
	CString strConfigName = appGlobal_ConfInfo.m_strDevConfPath + _T("\\") + pszGUID;

	// 取得最新的GBID
	CMap<CString, LPCSTR, CString, CString&> strDeviceIDMap;
	char szData[MAX_PATH];
	CString strDeviceType;
	CString strDeviceID;
	GetPrivateProfileString(_T("CATALOG"), _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
	strDeviceID = szData;
	strDeviceIDMap.SetAt(pszGUID, strDeviceID);
	//update Device RelationShip

	// 取得设备类型
	GetPrivateProfileString(_T("INFO"), _T("DeviceType"), _T("DeviceType"), szData, MAX_PATH, strConfigName);
	strDeviceType = szData;

	// 取得子设备ID和GUID
	CString strSection;
	CString strChannelID;
	CString strGUID;
	for (int j = 0; j < MAX_CHANNEL; j++)
	{
		strSection.Format(_T("CHANNEL_CATALOG%d"), j);
		GetPrivateProfileString(strSection, _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		strChannelID = szData;

		GetPrivateProfileString(strSection, _T("GUID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		// GUID为空，不存在该通道
		if (0 == strcmp(szData, _T("NaN")))
			break;

		strGUID = szData;
		strGUID.MakeLower();
		if (0 == j && 0 == strDeviceType.CompareNoCase(_T("ipc")))
		{
			strDeviceIDMap.SetAt(strGUID, strDeviceID);
			continue;
		}
		strDeviceIDMap.SetAt(strGUID, strChannelID);
	}

	POSITION pos = strDeviceIDMap.GetStartPosition();
	while (pos)
	{
		strDeviceIDMap.GetNextAssoc(pos, strGUID, strDeviceID);
		DeviceObject tNewInfo, tOldInfo;
		if (!CDataStore::LookupHUSDevice(strGUID, tOldInfo, TRUE))
		{
			CDataStore::ManualUnlockHUSDeviceInfoMap();
			CLog::Log(SDKCOM, LL_NORMAL, "更新设备信息失败，未知的GUID:%s", strGUID);
			continue;
		}
		tNewInfo = tOldInfo;
		tNewInfo.strDeviceID = strDeviceID;
		CDataStore::AddHUSDeviceObj(strGUID, tNewInfo, FALSE);
		CDataStore::ManualUnlockHUSDeviceInfoMap();
		Utils::StringCpy_s(szData, MAX_PATH, strDeviceID);
		CString	oldstrDeviceID = tOldInfo.strDeviceID; //get old deviceID to delete
		GBDeviceType	eGBDevType = tOldInfo.eGBDevType;

		// GBID没有更新，不用替换
		if (0 == oldstrDeviceID.Compare(szData))
		{
			continue;
		}

		// 没有有效的GBID
		if (ID_LEN != oldstrDeviceID.GetLength())
		{
			// 用GUID做key查询
			oldstrDeviceID = strGUID;
		}

		// 替换原有信息
		HUSDeviceLinkInfo *deviceLinkInfo = nullptr;
		if (GBDeviceType::OT_DEVICE == eGBDevType || GBDeviceType::OT_CAMERA == eGBDevType)
		{
			CString husDeviceGUID;
			if (CDataStore::LookupGUID(oldstrDeviceID, husDeviceGUID, TRUE)) //更新映射关系
			{
				CDataStore::RemoveDevice(oldstrDeviceID, FALSE);
				CDataStore::AddDeviceID(strDeviceID, husDeviceGUID, FALSE);
			}
			CDataStore::ManualUnlockGUIDMap();
		}
		else if (GBDeviceType::OT_ALARM == eGBDevType)
		{
			AlarmInfo_t tServiceInfo;
			if (CDataStore::LookupDeviceAlarmStatus(oldstrDeviceID, tServiceInfo, TRUE))
			{
				CDataStore::RemoveDeviceAlarmStatus(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					CDataStore::SetDeviceAlarmStatus(strGUID, tServiceInfo, FALSE);
				else
					CDataStore::SetDeviceAlarmStatus(szData, tServiceInfo, FALSE);
			}
			//m_oAlarmGBIDMap.ManualUnlock();
			//CDataStore::ManualUnlockDeviceIDMap();
			CDataStore::ManualUnlockDeviceInfoMap();
		}
		else if (GBDeviceType::OT_DECODER == eGBDevType)
		{
			if (m_oDecoderMap.Lookup(oldstrDeviceID, deviceLinkInfo, TRUE))
			{
				m_oDecoderMap.RemoveKey(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					m_oDecoderMap.SetAt(strGUID, deviceLinkInfo, FALSE);
				else
					m_oDecoderMap.SetAt(szData, deviceLinkInfo, FALSE);
			}
			m_oDecoderMap.ManualUnlock();
		}
	}
}

void CSDKDevInfo::DecoderPairTimeout()
{
	m_oDecoderPairMap.CheckTimeOut();
}

DeviceChangedMap &CSDKDevInfo::GetStatusChangedMap()
{
	return m_oStatusChangeMap;
}

CSDKDevInfo::AlarmingDeviceMap &CSDKDevInfo::GetAlarming()
{
	return m_oAlarmingDeviceMap;
}

BOOL CSDKDevInfo::AlarmLookup(const char *pszKey, AlarmInfo_t &pInfo)
{
	return CDataStore::LookupDeviceAlarmStatus(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::AlarmLookupEnd()
{
	//m_oAlarmGBIDMap.ManualUnlock();
	//CDataStore::ManualUnlockDeviceIDMap();
	CDataStore::ManualUnlockDeviceInfoMap();
}

void CSDKDevInfo::AlarmSetAt(const char *pszKey, AlarmInfo_t &pInfo, BOOL bIsLock)
{
	CDataStore::SetDeviceAlarmStatus(pszKey, pInfo, bIsLock);
}

BOOL CSDKDevInfo::DecoderIPLookup(const char *pszKey, CString &pszID)
{
	return m_oDecoderIPtoIDMap.Lookup(pszKey, pszID, FALSE);
}

void CSDKDevInfo::DecoderIPLookupEnd()
{
	m_oDecoderIPtoIDMap.ManualUnlock();
}

void CSDKDevInfo::DecoderPairPush(const char *pszKey, DecoderPairInfo_t &info)
{
	m_oDecoderPairMap.Push(pszKey, info);
}

BOOL CSDKDevInfo::DeviceLookup(const char *pszKey, DeviceObject_t *&pInfo)
{
	return CDataStore::LookupHUSDeviceByDeviceID(pszKey, *pInfo, TRUE);
}

void CSDKDevInfo::DeviceLookupEnd()
{
	CDataStore::ManualUnlockHUSDeviceInfoMap();
}

// 添加正在报警的设备信息
int CSDKDevInfo::AddAlarmList(const char *pszGUID, const char *pszDescribe, const char *pszLevel  /*same with：strAlarmSeverity*/, const char *pszTime, int nAlarmType, int nAlarmMethord, int nAlarmStatus)
{
	if (0 == strlen(pszGUID))
		return -1;
	CLog::Log(SDKCOM, LL_DEBUG, _T("设备告警，设备GUID:%s 告警级别:%s 告警信息:%s"), pszGUID, pszLevel, pszDescribe);
	CString			strDeviceGBID;
	DeviceObject	tInfo;
	CString strGUID = pszGUID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	// 取得设备对应的GBID
	CDataStore::LookupDeviceID(strGUID.GetString(), strDeviceGBID, FALSE);
	if (!strDeviceGBID.IsEmpty())
	{
		AlarmInfo_t		tAlarmInfo;
		if (CDataStore::LookupDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, TRUE)) //查找设备
		{
			// 判断布撤防状态
			if (AlarmInfo_t::DutyStatus::OFFDUTY == tAlarmInfo.eStatus)
			{
				//m_oAlarmGBIDMap.ManualUnlock();
				//CDataStore::ManualUnlockDeviceIDMap();
				CDataStore::ManualUnlockDeviceInfoMap();
				CLog::Log(SDKCOM, LL_DEBUG, _T("设备已撤防，告警不上报，设备ID：%s"), strDeviceGBID);
				return 0;
			}
			if (AlarmInfo_t::DutyStatus::ONDUTY == tAlarmInfo.eStatus)
			{
				tAlarmInfo.eStatus = AlarmInfo_t::DutyStatus::ALARM;
				tAlarmInfo.strDeviceGBID = strDeviceGBID;
				tAlarmInfo.m_nAlarmMethord = nAlarmMethord;
				tAlarmInfo.m_nAlarmType = nAlarmType;
				tAlarmInfo.m_nAlarmStatus = nAlarmStatus;
				tAlarmInfo.strDescribe = pszDescribe;
				tAlarmInfo.strLevel = pszLevel;
				tAlarmInfo.strTime = pszTime;
				//m_oAlarmGBIDMap.SetAt(strDeviceGBID, tAlarmInfo, FALSE);
				CDataStore::SetDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, FALSE);
				CLog::Log(SDKCOM, LL_DEBUG, _T("设备已布防，告警上报，设备ID：%s"), strDeviceGBID);
				this->p_oSDKCom->NoticeDevInfo(pszGUID, EventNotify::UpdateType::ut_alarm);
			}
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, _T("报警上报失败，未知的GB28181设备ID：%s"), strDeviceGBID);
			//m_oAlarmGBIDMap.ManualUnlock();
			//CDataStore::ManualUnlockDeviceIDMap();
			CDataStore::ManualUnlockDeviceInfoMap();
			return 0;
		}
		//m_oAlarmGBIDMap.ManualUnlock();
		//CDataStore::ManualUnlockDeviceIDMap();
		CDataStore::ManualUnlockDeviceInfoMap();
		// 添加到报警队列
		AlarmInfo_t alarmInfo(strDeviceGBID, nAlarmMethord, nAlarmType, nAlarmStatus, pszDescribe, pszLevel, pszTime);
		m_oAlarmingDeviceMap.SetAt(strDeviceGBID, alarmInfo);
	}
	return 0;
}

BOOL CSDKDevInfo::ReadDeviceTypeDef()
{
	CStdioFile oFile;
	if (FALSE == oFile.Open(appGlobal_ConfInfo.m_strTypeDefFileName, CStdioFile::modeRead))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "类型定义文件%s打开失败！", appGlobal_ConfInfo.m_strTypeDefFileName);
		return FALSE;
	}

	CString strContent;
	while (TRUE == oFile.ReadString(strContent))
	{
		int nPos = strContent.Find(",");
		if (0 < nPos)
		{
			CString strFront = strContent.Left(nPos);
			strContent = strContent.Right(strContent.GetLength() - nPos - 1);
			strFront.MakeLower();
			m_oDeviceTypeDefMap.SetAt(strFront, strContent);
		}
	}
	return TRUE;
}

void CSDKDevInfo::IniDeviveProperty(_ECElementPtr ptrECElement)
{
	GUID		guidEC;
	GUID		guidDevice;
	CString		strPath;
	strPath = appGlobal_ConfInfo.m_strDevConfPath + _T("\\");
	// 遍历设备数组
	guidDevice = ptrECElement->GetID();  // 设备的GUID
	_bstr_t strTypeMark = m_ptrSynAdapter->GetTypeMark(guidDevice);  // 设备类型，如"DVR", "Channel", "RStreamer"
	_bstr_t strName = ptrECElement->GetName();  // 设备名称
	// 取得设备类型GUID
	auto guidType = m_ptrSynAdapter->GetTypeID(guidDevice);
	CString strGUID;
	Utils::GUIDToCString(guidType, strGUID, FALSE);
	CString strType;
	// 取得对应的GB设备类型的
	if (m_oDeviceTypeDefMap.Lookup(strGUID, strType))
	{
		strTypeMark = strType;
	}
	// 获取当前设备所对应的EC服务的GUID
	GUID guidTarget;
	guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(guidDevice, &guidTarget);

	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID, FALSE);
	m_oECToDevMap.SetAt(strDeviceGUID, guidEC);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID);

	// 取得设备的IP
	CString pszDevIP;
	GetSettingsParam(GUID_NULL, L"IP", pszDevIP, ptrECElement);
	// 取得设备的Port
	CString pszDevPort;
	GetSettingsParam(GUID_NULL, L"Port", pszDevPort, ptrECElement);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s pszDevIp = %s pszDevPort = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID, pszDevIP, pszDevPort);
	WriteDeviceInfoConfig(strPath.GetString(), strTypeMark, strName, guidDevice, guidEC, pszDevIP, pszDevPort);
}

BOOL CSDKDevInfo::GetSettingsParam(GUID guidDevice, WCHAR *pwszParamName, CString &strParmaValue, _ECElementPtr  p_EleSetting) const
{
	if (nullptr == m_ptrSynAdapter && p_EleSetting == nullptr)
		return FALSE;

	_ElementSettingsPtr pSetting = nullptr;

	if (p_EleSetting != nullptr&& guidDevice == GUID_NULL)
		pSetting = p_EleSetting->GetSettings();
	else
		pSetting = m_ptrSynAdapter->GetElementSettings(guidDevice);
	if (nullptr == pSetting)
		return FALSE;
	// 取得参数名称
	VARIANT varParam;
	TCHAR *pszValue;
	auto bstrParamName = SysAllocString(pwszParamName);
	pSetting->get_Item(bstrParamName, &varParam);
	SysFreeString(bstrParamName);

	pszValue = _com_util::ConvertBSTRToString(varParam.bstrVal);
	strParmaValue = pszValue;
	SAFE_DELETE_ARRAY(pszValue);
	return TRUE;
}

void CSDKDevInfo::AddToDataStore(DeviceObject & objec2add, DeviceObject* parentObject)
{
	if (parentObject != nullptr)
		parentObject->oSubDevGUIDList.AddTail(objec2add.guidDevice);
	CDataStore::AddHUSDeviceObj(objec2add.strGuidDevice, objec2add);
	CDataStore::AddGUID(objec2add.strGuidDevice, objec2add.strDeviceID);
}
void CSDKDevInfo::FillSubDevInfoByparent(DeviceObject & sub_device, const DeviceObject& parent_device)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(sub_device.guidDevice, strDeviceGUID);
	sub_device.strGuidDevice = strDeviceGUID;
	sub_device.strIP = parent_device.strIP;
	sub_device.strPort = parent_device.strPort;
	sub_device.linkedInfo.strDeviceName = sub_device.strName; //标明链接信息所属的Onwer
	sub_device.linkedInfo.guidDevice = sub_device.guidDevice;
	sub_device.linkedInfo.guidParent = parent_device.guidDevice;
	sub_device.linkedInfo.guidEC = parent_device.linkedInfo.guidEC;
	sub_device.linkedInfo.strDeviceName = sub_device.strName; //标明链接的Onwer
	sub_device.linkedInfo.guidDevice = sub_device.guidDevice;

}

void CSDKDevInfo::WriteDeviceInfoConfig(const char *pszConfigPath, _bstr_t &bstrType, _bstr_t &bstrName, const GUID &guidDevice, const GUID &guidEC, const char *pszDevIP, const char *pszDevPort)
{
	// 取得设备类型
	CString strType = static_cast<char *>(bstrType);
	// 只处理顶层设备（DVR，decoder,device(ipc,encoder)
	HUSDeviceType _husDevType = HUSDeviceType::DT_NULL;
	auto  _gb_deviceType = GetDeviceTypeByName(strType, _husDevType, TRUE);
	if (_gb_deviceType == OT_NONE)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "忽略子设备 Type:%s ,继续处理下一个设备", strType);
		return;
	}
	CString strDeviceName = static_cast<char *>(bstrName);
	CString strDeviceIP = pszDevIP;
	CString strDevicePort = pszDevPort;
	//转换DVR设备的GUID为字符串
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "读入设备信息 ,设备GUID:%s Type:%s", strDeviceGUID, strType);

	// 生成保存设备和报警通道的数据结构
	DeviceObject newdeviceInfo;
	newdeviceInfo.eGBDevType = _gb_deviceType; //对应的GB设备类型
	newdeviceInfo.guidDevice = guidDevice;
	newdeviceInfo.strGuidDevice = strDeviceGUID;
	newdeviceInfo.strName = strDeviceName;
	newdeviceInfo.strDeviceTyeMark = strType; //字符串标记，同时用于GB和HUS的设备类型
	newdeviceInfo.strIP = strDeviceIP;
	newdeviceInfo.strPort = strDevicePort;
	newdeviceInfo.linkedInfo.guidDevice = guidDevice;
	newdeviceInfo.linkedInfo.strDeviceName = strDeviceName;
	newdeviceInfo.linkedInfo.guidParent = GUID_NULL;
	newdeviceInfo.linkedInfo.guidEC = guidEC;
	newdeviceInfo.extDevInfo.SetIOType(DT_DVR); //HUS的DVR层对象

	CString strConfigName = pszConfigPath + strDeviceGUID;
	WriteDvrDeviceConfig(strConfigName, newdeviceInfo);
	//dvr设备最后添加

	_ECElementPtr	*arrSubElements = nullptr;
	auto sarrSubDevices = m_ptrSynAdapter->GetSubElementsArray(guidDevice);
	if (FAILED(SafeArrayAccessData(sarrSubDevices, reinterpret_cast<void**>(&arrSubElements))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice： %s configPath = %s  Access SubChannel Info failure, but dvr Object init OK !\r\n", __FUNCTION__, strConfigName);

		return	AddToDataStore(newdeviceInfo);
	}
	auto channel_count = sarrSubDevices->rgsabound[0].cElements; //通道的数量
	CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice: %s configPath = %s ,Access SubChannel Info OK, SubChannel Count = %d\r\n", __FUNCTION__, pszConfigPath, channel_count);
	auto nElementIndex = 0;
	// 遍历子设备
	for (auto j = 0UL; j < channel_count; j++)
	{
		auto			guid_Channel = arrSubElements[j]->GetID();
		CString			strChannelMark = static_cast<LPCTSTR>(m_ptrSynAdapter->GetTypeMark(guid_Channel));
		auto            bstr_ChannelName = arrSubElements[j]->GetName();
		auto            strChannelName = CString(static_cast<LPCTSTR>(bstr_ChannelName));
		CString			ChannelOrderID;
		//判断是不是编码器的通道，是的话做相应处理
		if (_gb_deviceType == OT_ENCODER)
		{
			GetSettingsParam(guid_Channel, L"Param", ChannelOrderID); 			//获取EV4通道序号
			auto channelorder = _ttoi(ChannelOrderID);
			if (ChannelOrderID.GetLength() != 1)
			{
				GetSettingsParam(guid_Channel, L"ChannelID", ChannelOrderID);	//获取E8X,E4X通道序号
				nElementIndex = channelorder;
			}
			else
				nElementIndex = channelorder - 1;
		}
		DeviceObject subChannelObject;
		subChannelObject.guidDevice = guid_Channel;
		subChannelObject.strName = strChannelName;
		subChannelObject.strDeviceTyeMark = strChannelMark;
		FillSubDevInfoByparent(subChannelObject, newdeviceInfo);
		subChannelObject.linkedInfo.strDeviceName = strChannelName;
		_husDevType = HUSDeviceType::DT_NULL;
		auto  _gb_sub_deviceType = GetDeviceTypeByName(strChannelMark, _husDevType);
		subChannelObject.eGBDevType = _gb_sub_deviceType;
		subChannelObject.extDevInfo.SetIOType(_husDevType);

		if (_husDevType == HUSDeviceType::DT_CHANNEL)
		{
			// 将Channel添加为虚拟Alarm。
			//(在HUS设备概念中，IPC 的Channel同时也是报警通道和PTZ通道,另外Streamer层对象也是有报警的)。。。
			// channel同时也是一个虚拟的Alarm，所以需要将Channel 作为一个对象来添加。
			WriteAlarmChannelConfig(nElementIndex + channel_count, strConfigName, subChannelObject);
			// 初始化报警通道状态
			//HUS的Stremaer 在GB看来是一个IPC,需要单独添加一个对象，并且该对象在逻辑上与Channel并列存在。
			DeviceObject	subVideoObject;
			subVideoObject.eGBDevType = GBDeviceType::OT_CAMERA;
			subVideoObject.extDevInfo.SetIOType(DT_STREAMER);
			FillSubDevInfoByparent(subVideoObject, subChannelObject);
			if (newdeviceInfo.eGBDevType == GBDeviceType::OT_IPC)
			{
				subVideoObject.strDeviceID = newdeviceInfo.strDeviceID;
				subVideoObject.eGBDevType = GBDeviceType::OT_IPC;
			}
			auto  result = WriteVideoChannelConfig(nElementIndex++, strConfigName, subVideoObject);

			if (newdeviceInfo.eGBDevType == GBDeviceType::OT_IPC)
			{
				newdeviceInfo.linkedInfo.guidEC = subVideoObject.linkedInfo.guidEC;
				newdeviceInfo.linkedInfo.guidNVR = subVideoObject.linkedInfo.guidNVR;
				newdeviceInfo.linkedInfo.guidVirSteamer = subVideoObject.linkedInfo.guidVirSteamer;
				newdeviceInfo.linkedInfo.strNVRIP = subVideoObject.linkedInfo.strNVRIP;
				newdeviceInfo.linkedInfo.StrStreamerGUID = subVideoObject.strGuidDevice;
				newdeviceInfo.linkedInfo.guidParent = subVideoObject.guidDevice;//ptz控制对应的stream层的GUID
				newdeviceInfo.linkedInfo.guidVirSteamer = subVideoObject.linkedInfo.guidVirSteamer;
			}
			if (result == S_FALSE && j < channel_count) continue;
			if (newdeviceInfo.eGBDevType != GBDeviceType::OT_IPC)///ignore Streamer OF IPC for HUS Device.
			{
				subVideoObject.linkedInfo.StrStreamerGUID = subVideoObject.strGuidDevice;
				subVideoObject.linkedInfo.strDeviceName = subChannelObject.strName;
				subVideoObject.linkedInfo.guidParent = subVideoObject.guidDevice;
				AddToDataStore(subVideoObject, &newdeviceInfo);
			}
		}
		else if (_husDevType == HUSDeviceType::DT_ALARM_CHANNEL)
		{
			WriteAlarmChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}
		else if (_husDevType == HUSDeviceType::DT_DECODER_CHANNEL)
		{
			WriteDecorderChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}
		AddToDataStore(subChannelObject, &newdeviceInfo);
	}
	AddToDataStore(newdeviceInfo);
}

void CSDKDevInfo::WriteDvrDeviceConfig(const CString & strConfigName, DeviceObject & newDVRDeviceObject)
{
	TCHAR szData[MAX_PATH] = { 0 };

	// 写入设备类型和设备名
	WritePrivateProfileString("INFO", "DeviceType", newDVRDeviceObject.strDeviceTyeMark, strConfigName);
	WritePrivateProfileString("CATALOG", "Name", newDVRDeviceObject.strName, strConfigName);

	//获取设备ID,第一次是获取不到，因为deviceManager未配置,要自动生成GBID
	GetPrivateProfileString("CATALOG", "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString CurrentDeviceID(szData);  //生成DVR的GBID.
	if (CurrentDeviceID.GetLength() != ID_LEN)
	{
		CurrentDeviceID = m_GBID_Creater.Create_DevIDByType(newDVRDeviceObject.eGBDevType).c_str();
		Utils::StringCpy_s(szData, MAX_PATH, CurrentDeviceID);
		WritePrivateProfileString("CATALOG", "DeviceID", szData, strConfigName);
	}
	CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s szData = %s strlen(szData) = %d ID_LEN = %d\r\n", __FUNCTION__, strConfigName, CurrentDeviceID, CurrentDeviceID.GetLength(), ID_LEN);

	newDVRDeviceObject.strDeviceID = CurrentDeviceID;
	newDVRDeviceObject.extDevInfo.SetDeviceID(CurrentDeviceID);
}

HRESULT CSDKDevInfo::WriteVideoChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subVideoObject)
{
	auto sarrStreamer = m_ptrSynAdapter->GetSubElementsArray(subVideoObject.linkedInfo.guidParent);
	_ECElementPtr *arrStreamer = nullptr;
	if (FAILED(SafeArrayAccessData(sarrStreamer, reinterpret_cast<void**>(&arrStreamer))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s SafeArrayAccessData failure no channel\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	GUID			guid_Streamer;
	// 获取主码流的GUID，尝试判断属于GB的NVR
	guid_Streamer = arrStreamer[0]->GetID();
	CString			strStreamerMark = static_cast<LPCTSTR>(m_ptrSynAdapter->GetTypeMark(guid_Streamer));
	auto bstr_streamername = arrStreamer[0]->GetName();
	auto strStreamerName = CString(static_cast<LPCTSTR>(bstr_streamername));
	CString			strStreamerGUID;
	Utils::GUIDToCString(guid_Streamer, strStreamerGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "初始化HUS码流对象,设备GUID:%s Type:%s", strStreamerGUID, strStreamerMark);

	subVideoObject.strName = strStreamerName;
	subVideoObject.strDeviceTyeMark = strStreamerMark;
	subVideoObject.guidDevice = guid_Streamer;
	subVideoObject.strGuidDevice = strStreamerGUID;
	// 获取当前设备所对应的NVR服务的GUID
	// 这里返回的是GUID数组，因为一个设备可能挂接在多个NVR服务下
	// 对于GB28181 Gateway来说，选择第一个NVR即可
	GUID *arguidNVR = nullptr;
	auto arNVR = m_ptrSynAdapter->GetSServerIDArrayByElementID(subVideoObject.guidDevice);

	if (arNVR != nullptr && FAILED(SafeArrayAccessData(arNVR, reinterpret_cast<void**>(&arguidNVR))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s access Device nvr info failure\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	else if (arNVR == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s No NVR Linked to streamer: {%s}  \r\n", __FUNCTION__, strConfigName, strStreamerGUID);
		return S_FALSE;
	}
	GetSettingsParam(arguidNVR[0], L"IP", subVideoObject.linkedInfo.strNVRIP);
	subVideoObject.linkedInfo.guidNVR = arguidNVR[0]; //只保存第一个NVR的GUID.
// 设备基本信息写入设备配置文件。

	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder); //ENCODER(E4V/E8X等)，DECODER的子通道较多  DVR
	WritePrivateProfileString(strSection, "GUID", subVideoObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", "Camera", strConfigName); //streamer 的类型为Camera.
	WritePrivateProfileString(strSection, "Name", subVideoObject.strName, strConfigName); //write streamer name
	// 当设备是IPC时，读取CATALOG的ID
	TCHAR szData[MAX_PATH] = { 0 };
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	//处理 Steamer的GBID.
	CString CurrentStreamerID(szData);
	if (CurrentStreamerID.GetLength() != ID_LEN)
	{
		if (subVideoObject.eGBDevType == GBDeviceType::OT_IPC)
			CurrentStreamerID = subVideoObject.strDeviceID;
		else
			CurrentStreamerID = m_GBID_Creater.CreateSubDeviceid(subVideoObject.eGBDevType, subVideoObject.strDeviceID, indexOrder).c_str();
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s add device szData = %s\r\n", __FUNCTION__, strConfigName, szData);
		// 添加GBID 到HUS的GUID的映射
		WritePrivateProfileString(strSection, "DeviceID", CurrentStreamerID, strConfigName);
	}
	subVideoObject.strDeviceID = CurrentStreamerID;

	//处理设备链接到的EC的ID.
	if (GUID_NULL == subVideoObject.linkedInfo.guidEC)  //当前的DVR未连接到EC
	{
		GUID tmpguidTarget;
		subVideoObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subVideoObject.linkedInfo.guidParent, &tmpguidTarget);
	}
	m_ptrSynAdapter->GetECServerIDByElementID_2(subVideoObject.guidDevice, &(subVideoObject.linkedInfo.guidVirSteamer));
	CString strVirStreamer;
	Utils::GUIDToCString(subVideoObject.linkedInfo.guidVirSteamer, strVirStreamer, FALSE);
	m_oECToDevMap.SetAt(strVirStreamer, subVideoObject.linkedInfo.guidEC);
	CLog::Log(SDKCOM, LL_DEBUG, _T("添加虚视频流到EC映射队列 GUID：%s VirGUID:%s 队列长度：%d"), subVideoObject.strGuidDevice, strVirStreamer, m_oECToDevMap.GetSize());
	return S_OK;
}

void CSDKDevInfo::WriteAlarmChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subAlarmObject)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);
	WritePrivateProfileString(strSection, "GUID", subAlarmObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", subAlarmObject.strDeviceTyeMark, strConfigName);
	WritePrivateProfileString(strSection, "Name", subAlarmObject.strName, strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString strAlarmDeviceID(szData);
	if (ID_LEN != strAlarmDeviceID.GetLength())
	{
		Utils::StringCpy_s(szData, MAX_PATH, m_GBID_Creater.Create_AlarmDevID().c_str());
		WritePrivateProfileString(strSection, "DeviceID", szData, strConfigName);
		CLog::Log(SDKCOM, LL_NORMAL, "%s AlarmChannel deviceId = %s AlarmChannel Guid = %s\r\n", __FUNCTION__, szData, subAlarmObject.strGuidDevice);
	}
	if (GUID_NULL == subAlarmObject.linkedInfo.guidEC)
	{
		GUID tmpguidTarget;
		subAlarmObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subAlarmObject.guidDevice, &tmpguidTarget);
	}

	subAlarmObject.strDeviceID = strAlarmDeviceID;
	subAlarmObject.strAlarmID = strAlarmDeviceID;
}

void CSDKDevInfo::WriteDecorderChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subDisplayObject /*Streamer OF the Channel */)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);
	WritePrivateProfileString(strSection, "GUID", subDisplayObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", subDisplayObject.strDeviceTyeMark, strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "NaN", szData, MAX_PATH, strConfigName);
	CString strDecoderDisplayDeviceID(szData);
	if (ID_LEN != strDecoderDisplayDeviceID.GetLength())
	{
		strDecoderDisplayDeviceID = m_GBID_Creater.Create_DecoderID().c_str();
		Utils::StringCpy_s(szData, MAX_PATH, strDecoderDisplayDeviceID);
		WritePrivateProfileString(strSection, "DeviceID", strDecoderDisplayDeviceID, strConfigName);
	}
	if (GUID_NULL == subDisplayObject.linkedInfo.guidEC)
	{
		GUID tmpguidTarget;
		subDisplayObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subDisplayObject.guidDevice, &tmpguidTarget);
	}
	// 取得通道编号
	CString strMonitorNum;
	CString strChannelID;
	GetSettingsParam(subDisplayObject.guidDevice, L"Monitor", strMonitorNum);
	GetSettingsParam(subDisplayObject.guidDevice, L"ChannelID", strChannelID);

	auto  p_LinkedInfo = new HUSDeviceLinkInfo();
	p_LinkedInfo->strChannelNum = strMonitorNum + ":" + strChannelID;
	p_LinkedInfo->strDevIP = subDisplayObject.strIP;
	p_LinkedInfo->strDevPort = subDisplayObject.strPort;

	// 用GBID做key
	m_oDecoderMap.SetAt(strDecoderDisplayDeviceID, p_LinkedInfo);

	CString strAddress = subDisplayObject.strIP + ":" + subDisplayObject.strPort;
	subDisplayObject.strDeviceID = strDecoderDisplayDeviceID;
	m_oDecoderIPtoIDMap.SetAt(strAddress, subDisplayObject.strDeviceID);
}

// 添加设备对象
void CSDKDevInfo::AddDeviceObject(const GUID &guidDevcie)
{
	if (nullptr == m_ptrSynAdapter)
		return;

	// 获取设备列表，参数是Gateway的GUID，返回值是设备数组
	// 这里返回的设备列表包括所有挂接在当前Gateway下的设备
	_ECElementPtr ptrECElement = m_ptrSynAdapter->GetElement(guidDevcie);
	if (nullptr == ptrECElement)
		return;

	IniDeviveProperty(ptrECElement);
}

ECInfoMap &CSDKDevInfo::GetECToDevMap()
{
	return m_oECToDevMap;
}

void CSDKDevInfo::DecoderBind(const GUID &guidDecoder, const GUID &guidChannel)
{
	HUSDeviceLinkInfo *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);

	DeviceObject tObjInfo;
	CString deviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, deviceID);
	if (m_oDecoderMap.Lookup(deviceID, pInfo, TRUE))
	{
		pInfo->guidParent = guidChannel;
	}
	m_oDecoderMap.ManualUnlock();
}

void CSDKDevInfo::GUIDTranslatedIntoGBID(const GUID &guidDevice, CString &strGBID)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CString strDeviceID;
	if ("{" != strDeviceGUID.Left(1))
		strDeviceGUID = "{" + strDeviceGUID + "}";
	CDataStore::LookupDeviceID(strDeviceGUID, strDeviceID);
	strGBID = strDeviceID;
}

// 解除解码器视频流绑定
void CSDKDevInfo::DecoderUnbind(const GUID &guidDecoder)
{
	HUSDeviceLinkInfo *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);
	CString strDeviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, strDeviceID);

	if (m_oDecoderMap.Lookup(strDeviceID, pInfo, TRUE))
	{
		memset(&(pInfo->guidParent), 0, sizeof(pInfo->guidParent));
	}
	m_oDecoderMap.ManualUnlock();
}

int CSDKDevInfo::GetAllGUID(CList<GUID> &guidList) const
{
	//实际读入的是DVR的GUID.
	return CDataStore::GetAllDeviceGUID(guidList);
}

CString CSDKDevInfo::GetGatewyGUID(BOOL bIsWithBraces) const
{
	CString strGatewayGUID;
	Utils::GUIDToCString(m_guidGateway, strGatewayGUID, bIsWithBraces);
	return  strGatewayGUID;
}

void CSDKDevInfo::AddHUSDeviceLinkInfo(const TCHAR* pszGUID, HUSDeviceLinkInfo& deviceLinkinfo)
{
	//查不到设备信息则添加
	DeviceObject tmpDeviceInfo;
	if (!CDataStore::LookupHUSDevice(pszGUID, tmpDeviceInfo, TRUE))
	{
		tmpDeviceInfo.linkedInfo = deviceLinkinfo;
		CDataStore::AddHUSDeviceObj(pszGUID, tmpDeviceInfo);
	}
	CDataStore::ManualUnlockHUSDeviceInfoMap();
}
void CSDKDevInfo::SetOwner(CSDKCom* p_SDKCom)
{
	this->p_oSDKCom = p_SDKCom;
}

GBDeviceType CSDKDevInfo::GetDeviceTypeByName(CString & strType, HUSDeviceType & husDevType, BOOL isDvrLayer)
{
	//Channel  Contains: channel/alarm/decoder
	strType.MakeLower();
	GBDeviceType _gbdt = GBDeviceType::OT_NONE;

	if (isDvrLayer)
	{
		husDevType = DT_DVR;
		if (strType.Find(_T("ipc")) >= 0)
			_gbdt = GBDeviceType::OT_IPC;
		else if (strType.Find(_T("dvr")) >= 0)
			_gbdt = GBDeviceType::OT_DVR;
		else if (strType.Find(_T("encoder")) >= 0)
			_gbdt = GBDeviceType::OT_ENCODER;
		else if (strType.Find(_T("nvr")) >= 0)
			_gbdt = GBDeviceType::OT_NVR;
		return _gbdt;
	}

	if (strType.Find(_T("channel")) >= 0)
	{
		_gbdt = GBDeviceType::OT_ALARM;
		husDevType = DT_CHANNEL;
	}
	else if (strType.Find(_T("alarm")) >= 0)
	{
		_gbdt = GBDeviceType::OT_ALARM;
		husDevType = HUSDeviceType::DT_ALARM_CHANNEL;
	}
	else if (strType.Find(_T("decoder")) >= 0)
	{
		_gbdt = GBDeviceType::OT_DISPLAY;
		husDevType = HUSDeviceType::DT_DECODER_CHANNEL;
	}
	else if (strType.Find(_T("streamer")) >= 0)
	{
		_gbdt = GBDeviceType::OT_CAMERA;
		husDevType = DT_STREAMER;
	}
	return _gbdt;
}