#include "StdAfx.h"
#include "SDKSiteProxy.h"
#include "SDKCom.h"

BOOL VmsSiteProxy::InitSynClient(CSDKCom* pSdkCom)
{
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) {
		CLog::Log(SDKCOM, LL_NORMAL, "COM环境初始化失败,请重新注册COM组件……");
		return FALSE;
	}

	CLog::Log(SDKCOM, LL_NORMAL, "连接VSM……");

	_AdaptorFactoryWrapperPtr pAdaptorFactoryWrapper;

	HRESULT hr;
	if (SUCCEEDED(hr = pAdaptorFactoryWrapper.CreateInstance(CLSID_AdaptorFactoryWrapper)))
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

	_SynClientPtr pSynClientPtr;
	// 创建SynClient
	if (SUCCEEDED(hr = pSynClientPtr.CreateInstance(CLSID_SynClient)))
	{
		m_pSynClient = pSynClientPtr;
	}
	else
	{
		if (FAILED(hr = CoCreateInstance(CLSID_SynClient, nullptr, CLSCTX_INPROC_SERVER, IID__SynClient, reinterpret_cast<LPVOID*>(&m_pSynClient))))
		{
			CLog::Log(SDKCOM, LL_NORMAL, "_SynClient接口创建失败。");
			return FALSE;
		}
	}

	m_pSynClient->PutServerType(strServiceTag.GetString());
	m_pAdaptorFactory->PutSiteAddress(m_pSynClient->GetSiteIP());

	// 初始化SynClient，需传入Gateway的GUID
	// 初始化过程会从VMS站点获取设备数据写到SiteTree文件（C:\Program Files (x86)\Common Files\Honeywell\HUS\SynchronizeFiles\...xml），比较耗时

	if (!m_pSynClient->Initialize(m_GuidGateway))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "_SynClient初始化失败，无法获取站点Sitetree。");
		return FALSE;
	}
#ifdef SDK_VERSION_4_3

	m_pSiteImageAdaptorEx->HoldSyncClient(m_pSynClient);
#endif // SDK_VERSION_4_3

	m_pSynClient->PutServerType(strServiceTag.GetString());
	m_GuidGateway = m_pSynClient->GetConfigID(strServiceTag.GetString());
	m_pSiteImageAdapter = m_pSynClient->GetSiteImageAdaptor();
	strProperties = m_pSiteImageAdapter->GetDeviceProperties();
	m_pAdaptorFactory->DeviceTypeFromFile = FALSE;
	m_pAdaptorFactory->PutDeviceTypeData(strProperties);
	b_Initialized = true;
	//保存SDKCom指针
	m_pSdkCom = pSdkCom;
	return TRUE;
}

#ifdef SDK_VERSION_5_0

IRealDevicePtr VmsSiteProxy::GetIRealDevice(const GUID guidStream)
{
	static bool bLogin = false;

	if (bLogin)
	{
		return pRetDevcie;
	}
	IWellKnownStringPtr  pWellKnownString = NULL;
	auto hr = pWellKnownString.CreateInstance(CLSID_WellKnownString, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && pWellKnownString != NULL);

	hr = pDataClient.CreateInstance(CLSID_DataClient, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && pDataClient != NULL);

	_bstr_t ServerTagTemp = strServiceTag;
	if (!bLogin)
	{
		hr = pDataClient->Initialize_2(ServerTagTemp, "http");
	}
	if (!FAILED(hr))
	{
		bLogin = true;
	}
	pRetDevcie = pDataClient->GetHUSSite()->GetRealDeviceEntitySet()->GetById(guidStream);

	return pRetDevcie;
}

#endif // SDK_VERSION_5_0

BOOL  VmsSiteProxy::InitRecordSearchContext()
{
	if (!b_Initialized)  return FALSE;
#ifdef SDK_VERSION_4_3
	m_PVideoStoreSite.CreateInstance(CLSID_VideoStoreSite);
	m_PVideoStoreSite->PutSiteName(m_pSynClient->GetSiteName());
	m_PVideoStoreSite->PutSiteXML(m_pSynClient->GetSiteIP());
	// 初始化录像查询接口
	auto pIHistoryVideoDB = m_pAdaptorFactory->GetIHistoricalVideoDBCreator()->CreateHistoricalVideoDB(m_PVideoStoreSite, true);
	m_pIVideoSearch = pIHistoryVideoDB->GetIVideoSearch();

	//增加扩展类型,用以获取设备相关的站点信息
	m_pSiteImageAdaptorEx.CreateInstance(CLSID_SiteImageAdaptorEx);
	//hold info from SynClient
	ASSERT(m_pSynClient != nullptr);
	m_pSiteImageAdaptorEx->HoldSyncClient(m_pSynClient);
#endif // SDK_VERSION_4_3
	return TRUE;
}

void VmsSiteProxy::RefreshDataIfNeed(_bstr_t szGUID)
{
#ifdef SDK_VERSION_4_3
	auto siteinfo = m_pSiteImageAdaptorEx->GetSiteDescrition(szGUID);

	std::string currentSiteip = m_PVideoStoreSite->GetSiteXML();

	if (currentSiteip.compare(siteinfo->GetIP()) != 0)	//不是同一个站点
	{
		m_PVideoStoreSite->PutSiteXML(siteinfo->GetIP());
	}
	auto pIHistoryVideoDB = m_pAdaptorFactory->GetIHistoricalVideoDBCreator()->CreateHistoricalVideoDB(m_PVideoStoreSite, true);
	m_pIVideoSearch = pIHistoryVideoDB->GetIVideoSearch();
#endif // SDK_VERSION_4_3
}

BOOL VmsSiteProxy::GetAllDeviceFromSite()
{
	//	pSynAdapter->QueryInterface(IID_IDeviceConfig, (LPVOID*)&pDeviceConfig);
	//	pSynAdapter = pDeviceConfig;
	// 获取设备列表，参数是Gateway的GUID，返回值是设备数组
	// 这里返回的设备列表包括所有挂接在当前Gateway下的设备
	CComSafeArray<LPUNKNOWN> safearry_ec_element_device = m_pSiteImageAdapter->GetLinks(m_GuidGateway);
	auto count = safearry_ec_element_device.GetCount();
	if (count == 0)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "SafeArrayAccessData Failed :GB平台挂载的设备信息获取失败,或者未挂载设备。");
		return FALSE;
	}
	CLog::Log(SDKCOM, LL_NORMAL, "共计获取了[%d]个设备对象的信息……", count);
	auto low_bound = safearry_ec_element_device.GetLowerBound();
	auto up_bound = safearry_ec_element_device.GetUpperBound();

	for (auto i = low_bound; i <= up_bound; i++)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "访问第[%d]设备的信息..", i);
		auto ptr_unknown = safearry_ec_element_device.GetAt(i);
		_ECElementPtr arECElements = nullptr;
		if (FAILED(ptr_unknown->QueryInterface(IID_PPV_ARGS(&arECElements)))) {
			//......
			break;
		}
		InitDeviceItemCallBack(arECElements);
	}

	CLog::Log(SDKCOM, LL_NORMAL, "GB平台挂载的设备信息获取完成。");
	return TRUE;
}

VmsSiteProxy & VmsSiteProxy::operator=(VmsSiteProxy &vmsSiteProxy)
{
	m_pAdaptorFactory = vmsSiteProxy.m_pAdaptorFactory;
	m_pSynClient = vmsSiteProxy.m_pSynClient;

	return (*this);
}

const CString  VmsSiteProxy::GetTypeMark(const GUID& guid_device)
{
	auto typemark = m_pSiteImageAdapter->GetTypeMark(guid_device);
	return static_cast<LPCTSTR>(typemark);
}

const GUID  VmsSiteProxy::GetTypeID(const GUID& guid_device)
{
	return m_pSiteImageAdapter->GetTypeID(guid_device);
}

const CString VmsSiteProxy::GetSiteIP()
{
	if (m_StrSiteIP.IsEmpty())
	{
		auto  strSiteIP = m_pSynClient->GetSiteIP();
		m_StrSiteIP = (LPCSTR)strSiteIP;
	}
	return m_StrSiteIP;
}

const GUID & VmsSiteProxy::GetGatewayGUID()
{
	if (m_GuidGateway == GUID_NULL)
	{
		m_GuidGateway = m_pSynClient->GetConfigID(strServiceTag.GetString());
	}

	return m_GuidGateway;
}

const CString VmsSiteProxy::GetGatewayStrGUID(BOOL bIsWithBraces)
{
	if (m_GuidGateway == GUID_NULL)
	{
		m_GuidGateway = m_pSynClient->GetConfigID(strServiceTag.GetString());
	}

	CString strGatewayGUID;
	Utils::GUIDToCString(m_GuidGateway, strGatewayGUID, bIsWithBraces);
	return  strGatewayGUID;
}

const GUID VmsSiteProxy::GetECServerID(const GUID& guid_device)
{
	GUID vitualGuidTarget;
	return m_pSiteImageAdapter->GetECServerIDByElementID_2(guid_device, &vitualGuidTarget);
}

const GUID VmsSiteProxy::GetVirtualECTargetGUID(const GUID& guid_device)
{
	GUID vitualGuidTarget = GUID_NULL;
	m_pSiteImageAdapter->GetECServerIDByElementID_2(guid_device, &vitualGuidTarget);
	return vitualGuidTarget;
}

// 获取当前设备所对应的NVR服务的GUID
// 这里返回的是GUID数组，因为一个设备可能挂接在多个NVR服务下
// 对于GB28181 Gateway来说，选择第一个NVR即可
const GUID VmsSiteProxy::GetLinkedFirstSServerID(const GUID& guid_device)
{
	Guid *guidArray = nullptr;
//	CComSafeArray<LPUNKNOWN> arNVRArray = m_pSiteImageAdapter->GetSServerIDArrayByElementID(guid_device);
	auto arNVRArray = m_pSiteImageAdapter->GetSServerIDArrayByElementID(guid_device);

	SafeArrayAccessData(arNVRArray, reinterpret_cast<void**>(&guidArray));

	//if (arNVRArray.GetCount() <= 0) return GUID_NULL;
	////获取第一个元素
	//auto pUnknownElemnt = arNVRArray.GetAt(arNVRArray.GetLowerBound());

	//pUnknownElemnt->QueryInterface(&guidArray);

	if (guidArray == nullptr)
	{
		return GUID_NULL;
	}
	//返回第一个NVR
	auto returnedGuid = reinterpret_cast<GUID*>(guidArray);

	return returnedGuid[0];

}

_ECElementPtr VmsSiteProxy::GetECElement(const GUID &guidDevcie)
{
	if (!b_Initialized) return nullptr;
	// 获取设备列表，参数是Gateway的GUID，返回值是设备数组
	// 这里返回的设备列表包括所有挂接在当前Gateway下的设备

	return m_pSiteImageAdapter->GetElement(guidDevcie);
}

std::vector<_ECElementPtr> VmsSiteProxy::GetSubElementsArray(const GUID &guidDevcie)
{
	std::vector<_ECElementPtr> arrayECElementPtr;
	//auto count = 0;
	CComSafeArray <LPUNKNOWN> sarrSubDevices = m_pSiteImageAdapter->GetSubElementsArray(guidDevcie);

	if (sarrSubDevices.GetCount() <= 0)
	{
		return arrayECElementPtr;
	}
	for (auto i = sarrSubDevices.GetLowerBound(); i <= sarrSubDevices.GetUpperBound(); i++)
	{
		auto pUnknownElemnt = sarrSubDevices.GetAt(i);
		if (pUnknownElemnt == nullptr) continue;
		_ECElementPtr pElement = nullptr;
		auto hr = pUnknownElemnt->QueryInterface(IID_PPV_ARGS(&pElement));
		if (FAILED(hr)) continue;
		arrayECElementPtr.push_back(pElement);
	}
	return arrayECElementPtr;
}

BOOL VmsSiteProxy::GetDeviceSettingsInfo(GUID guidDevice, const WCHAR * pwszParamName, CString & strParmaValue, _ECElementPtr p_EleSetting)
{
	if (!b_Initialized && p_EleSetting == nullptr)
		return FALSE;

	_ElementSettingsPtr pSetting = nullptr;

	if (p_EleSetting != nullptr&& guidDevice == GUID_NULL)
		pSetting = p_EleSetting->GetSettings();
	else
		pSetting = m_pSiteImageAdapter->GetElementSettings(guidDevice);
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

_SiteImageAdaptorPtr & VmsSiteProxy::GetSiteImageAdaptorPtr()
{
	if (!m_pSiteImageAdapter)
	{
		// 拿到SiteImageAdaptor对象
		m_pSiteImageAdapter = m_pSynClient->SiteImageAdaptor;
	}

	return m_pSiteImageAdapter;
}

void VmsSiteProxy::ReleaseComObjects()
{
	SAFE_DELETE_PTR(m_pAdaptorFactory);
	SAFE_DELETE_PTR(m_pSynClient);
	CoUninitialize();
}

void VmsSiteProxy::SetInitDeviceItemCallBack(DELEGATE_FUNC_CB_T pCallBack)
{
	if (pCallBack)
	{
		this->InitDeviceItemCallBack = pCallBack;
	}
}
