#pragma once
#include <functional>
#include<vector>
class CSDKCom;
class VmsSiteProxy
{
	using DELEGATE_FUNC_CB_T = std::function<void(_ECElementPtr)>;
	DELEGATE_FUNC_CB_T InitDeviceItemCallBack = nullptr;

public:
	VmsSiteProxy()
	{
		m_pAdaptorFactory = nullptr;
		m_pSynClient = nullptr;
		m_pSiteImageAdapter = nullptr;
	}
	~VmsSiteProxy() = default;

	VmsSiteProxy & operator=(VmsSiteProxy &vmsSiteProxy);

	BOOL InitSynClient(CSDKCom* pSdkCom = nullptr);

	BOOL InitRecordSearchContext();

	void RefreshDataIfNeed(_bstr_t inParam);

	BOOL GetAllDeviceFromSite();

	void ReleaseComObjects();

	const CString GetTypeMark(const GUID& guid_device);

	const CString  GetSiteIP();

	const GUID GetTypeID(const GUID& guid_device);

	const GUID & GetGatewayGUID();

	const CString GetGatewayStrGUID(BOOL bIsWithBraces = FALSE);

	const GUID GetECServerID(const GUID & guid_device);

	const GUID GetVirtualECTargetGUID(const GUID & guid_device);

	const GUID GetLinkedFirstSServerID(const GUID & guid_device);

	_ECElementPtr GetECElement(const GUID & guidDevcie);

	std::vector<_ECElementPtr> GetSubElementsArray(const GUID & guidDevcie);

	BOOL GetDeviceSettingsInfo(GUID guidDevice, const WCHAR *pwszParamName, CString &strParmaValue, _ECElementPtr  p_EleSetting);

	_SiteImageAdaptorPtr & GetSiteImageAdaptorPtr();

	void SetInitDeviceItemCallBack(DELEGATE_FUNC_CB_T pCallBack = nullptr);

#ifdef SDK_VERSION_5_0
	IRealDevicePtr GetIRealDevice(const GUID guidStream);
#endif

	// 在调用SynClient的其他方法之前，需设置服务类型；针对GB28181 Gateway。需要定义一个新的字符串，如“GB28181GW”；
	//目前将其定义为："GBGateway"
	CString strServiceTag = "GBGateway";
	_FactoryPtr  m_pAdaptorFactory;
	_SiteImageAdaptorPtr m_pSiteImageAdapter;
	_SynClientPtr m_pSynClient;
#ifdef SDK_VERSION_5_0
	IDataClientPtr  pDataClient = NULL;
	IRealDevicePtr  pRetDevcie = NULL;
#endif
#ifdef SDK_VERSION_4_3
	_VideoStoreSitePtr m_PVideoStoreSite = nullptr;
	IVideoSearchPtr m_pIVideoSearch = nullptr;
	_SiteImageAdaptorExPtr m_pSiteImageAdaptorEx = nullptr;
#endif

	GUID m_GuidGateway;
	CString m_StrSiteIP;
	_bstr_t strProperties;

	bool b_Initialized = false;
	CSDKCom* m_pSdkCom = nullptr;
};
