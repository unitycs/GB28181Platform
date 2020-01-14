#pragma once
#include "Main/UnifiedMessage.h"
#include "SDKCom/SDKInfoSearcherMgr.h"

class SiteEventsClient :private SiteJobWoker,
	public IDispEventImpl<0, SiteEventsClient, &DIID_ISynEvent, &LIBID_SiteImageAdaptor, 1, 0> //动态同步的com事件
#ifdef SDK_VERSION_4_3
	, public IDispEventImpl<1, SiteEventsClient, &DIID_IVideoActionEvent, &LIBID_AdaptorFactory, 1, 0> //站点video记录查询的com事件
#endif
{
	typedef IDispEventImpl<0, SiteEventsClient, &DIID_ISynEvent, &LIBID_SiteImageAdaptor, 1, 0> DispSynEvent;
#ifdef SDK_VERSION_4_3
	typedef IDispEventImpl<1, SiteEventsClient, &DIID_IVideoActionEvent, &LIBID_AdaptorFactory, 1, 0> DispFactoryEvent;
#endif
public:
	SiteEventsClient() = default;
	virtual ~SiteEventsClient() = default;

	void SetContext(CSDKCom* p_SDKCom, VmsSiteProxy * pVmsSiteProxy = nullptr, CAllocator<CModMessage> * pMemAllocator = nullptr) override;

	BOOL RegisterToSiteEventsSource();

	BOOL InitRecordSearchContext();


	int HandleRecordSearchResult() const;


	void ForwardToDevInfo(const char* pszGUID, event_notify_t::e_update_t eUpdateType) const;

	BEGIN_SINK_MAP(SiteEventsClient)
		SINK_ENTRY_INFO(0, DIID_ISynEvent, 1, OnAddDevice, &OnEventInfo)
		SINK_ENTRY_INFO(0, DIID_ISynEvent, 2, OnModDevice, &OnEventInfo)
		SINK_ENTRY_INFO(0, DIID_ISynEvent, 3, OnDelDevice, &OnEventInfo)
#ifdef SDK_VERSION_4_3
		SINK_ENTRY_INFO(1, DIID_IVideoActionEvent, 3, OnVideoDBAction, &OnFactoryEventInfo)
#endif
	END_SINK_MAP()

	HRESULT __stdcall OnAddDevice(BSTR strID);
	HRESULT __stdcall OnDelDevice(BSTR strID);
	HRESULT __stdcall OnModDevice(BSTR strID);
	HRESULT __stdcall OnVideoDBAction(IUnknown* lpSender, struct _HisActionArg * pAction) const;
	//	HRESULT __stdcall OnAlarmDBAction(IUnknown* lpSender, struct _HisActionArg * pAction) const;
	BOOL CheckSource(GUID& guidDevice) const;

};
