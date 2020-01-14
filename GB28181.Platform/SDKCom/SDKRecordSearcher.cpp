#include "StdAfx.h"

#ifdef SDK_VERSION_5_0
#include "SDKRecordSearcher.h"
#include "SDKCom/SDKCom.h"

void VideoRecordSearcher::SetContext(SiteEventsClient *pEventClient, CSDKCom* pSDKCom, VmsSiteProxy * pVmsSiteProxy /*= nullptr*/, CAllocator<CModMessage> * pMemAllocator /*= nullptr*/)
{
	m_pEventClient = pEventClient;
	SiteJobWoker::SetContext(pSDKCom, pVmsSiteProxy, pMemAllocator);

}

void VideoRecordSearcher::InitVideoSearch(GUID guidStream, HUSDevice_T video_type)
{
	if (video_type == HUSDevice_T::NVR_DVR)
	{
		ptr_video_record_searcher = GetDVRVideoRecordSearcher(guidStream);
	}
	else
	{
		ptr_video_record_searcher = GetHUSVideoRecordSearcher(guidStream);
	}
	video_type_to_search = video_type;
}

bool VideoRecordSearcher::SearchRecord(DATE start_time, DATE end_time, long priority)
{
	auto result = true;
	CLog::Log(SDKCOM, LL_NORMAL, "%s begin\r\n", __FUNCTION__);

	if (FAILED(ptr_video_record_searcher->RetrieveResult(start_time, end_time, iptr_record_callback_, streamerid_to_search, priority)))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s failure\r\n", __FUNCTION__);
		result = false;
	}
	CLog::Log(SDKCOM, LL_NORMAL, "%s succ\r\n", __FUNCTION__);
	if (guid_safe_arry != nullptr)
	{
		SafeArrayDestroy(guid_safe_arry);
	}
	return result;
}

void VideoRecordSearcher::ReportResult(ISearchResult * presult)
{

	CComPtr<ISearchResult> result = presult;

	// if the result doesn't match current searching id, just discard it
	if (result->GetIdentity() != streamerid_to_search) return;

	// get the status of result to judgment
	auto search_status = result->GetStatus();

	switch (search_status)
	{

	case HUS_DataManager_Search_Contract::SearchResultStatus_InProgress:
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s 开始处理录像查询的callback 数据!\r\n", __FUNCTION__);

		IVideoRecordSearchResultPtr record_search_result = NULL;
		result->QueryInterface(IID_PPV_ARGS(&record_search_result));
		// get detailed record info
		CComSafeArray<LPUNKNOWN> safearray_records = record_search_result->GetClipResults();
		if (safearray_records.GetCount() == 0)
		{
			//......
			CLog::Log(SDKCOM, LL_NORMAL, "%s 本次callback的数据为 0,将不做处理. \r\n", __FUNCTION__);
			return;
		}
		//   list<VideoNode> list_video_node;
		auto low_bound1 = safearray_records.GetLowerBound();
		auto up_bound1 = safearray_records.GetUpperBound();
		for (auto i = low_bound1; i <= up_bound1; i++) {
			auto ptr_unknown = safearray_records.GetAt(i);
			if (ptr_unknown == nullptr)	continue;

			IVideoClipResultPtr ptr_video_clip = NULL;
			if (FAILED(ptr_unknown->QueryInterface(IID_PPV_ARGS(&ptr_video_clip)))) {
				//......
				break;
			}

			CComSafeArray<LPUNKNOWN> safearray_time_range = ptr_video_clip->GetVideoExistsTimeRanges();

			if (safearray_time_range.GetCount() == 0)	continue;

			auto low_bound2 = safearray_time_range.GetLowerBound();
			auto up_bound2 = safearray_time_range.GetUpperBound();
			for (auto j = low_bound2; j <= up_bound2; j++)
			{
				auto ptr_unknow = safearray_time_range.GetAt(j);
				if (ptr_unknown == nullptr)	continue;


				ITimeRangePtr ptr_time_range = NULL;
				if (FAILED(ptr_unknow->QueryInterface(IID_PPV_ARGS(&ptr_time_range))))
				{
					//......
					break;
				}

				DATE startTime = 0;
				DATE endTime = 0;
				ptr_time_range->get_StartTime(&startTime);
				ptr_time_range->get_EndTime(&endTime);

				addSingleRecord(startTime, endTime, 0);
			}
		}
	}
	break;
	case HUS_DataManager_Search_Contract::SearchResultStatus_Completed:
	{
		//hus5.x RecordSearch逻辑
		//通知 SiteEventsClient处理record search.
		m_pEventClient->HandleRecordSearchResult();
	}
	break;
	default:
		CLog::Log(SDKCOM, LL_NORMAL, "%s 搜索录像文件时候，出现异常，错误代码是：%d! \r\n", __FUNCTION__, search_status);
		break;
	}

}

void VideoRecordSearcher::addSingleRecord(DATE recordStartTime, DATE recordEndTime, int triggleType) const
{
	auto pRecordbuf = m_pSDKCom->m_VideoRecordMgr.GetRecordBuf();
	if (pRecordbuf)
	{
		pRecordbuf->SetType(static_cast<record_type_t>(triggleType));
		pRecordbuf->SetStartTime(recordStartTime);
		pRecordbuf->SetEndTime(recordEndTime);
		pRecordbuf->SetAddr(m_pSDKCom->m_VideoRecordMgr.GetSearchMessage()->GetRecordAddr());
		m_pSDKCom->m_VideoRecordMgr.InserRecord(pRecordbuf);

		CLog::Log(SDKCOM, LL_NORMAL, "%s startTime = %s endTime = %s", __FUNCTION__, pRecordbuf->m_szStartTime, pRecordbuf->m_szEndTime);
	}
}

IVideoRecordSearcherPtr VideoRecordSearcher::GetDVRVideoRecordSearcher(GUID guidStream)
{
	IDVRVideoRecordSearchContextPtr hus_record_search_context = NULL;
	HRESULT hr = hus_record_search_context.CreateInstance(CLSID_DVRVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_search_context != NULL);
	if (FAILED(hr))	CLog::Log(SDKCOM, LL_NORMAL, "%s  创建 DVRVideoRecordSearchContext 失败!\r\n", __FUNCTION__);

	hus_record_search_context->PutTriggerType(1);
	auto irealDevice = m_pVmsSiteProxy->GetIRealDevice(guidStream);
	hus_record_search_context->PutRefRealDevice(irealDevice);

	//设置IP/ID
	auto bstrSiteID = m_pVmsSiteProxy->m_pSynClient->GetSiteID();
	auto bstrSiteIP = m_pVmsSiteProxy->m_pSynClient->GetSiteIP();
	ISearchContextPtr searchContext = NULL;
	hus_record_search_context->QueryInterface(IID_PPV_ARGS(&searchContext));
	searchContext->PutSiteId(bstrSiteID);
	searchContext->PutSiteIP(bstrSiteIP);

	IDVRVideoRecordSearcherPtr hus_record_searcher = NULL;
	hr = hus_record_searcher.CreateInstance(CLSID_DVRVideoRecordSearcher, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_searcher != NULL);
	if (FAILED(hr))	CLog::Log(SDKCOM, LL_NORMAL, "%s siteIp = %s 创建 HUSVideoRecordSearcher失败!\r\n", __FUNCTION__, bstrSiteIP);
	hus_record_searcher->SetContext(hus_record_search_context);
	CLog::Log(SDKCOM, LL_NORMAL, "ready to call SearchRecord:");
	return hus_record_searcher;
}

IVideoRecordSearcherPtr VideoRecordSearcher::GetHUSVideoRecordSearcher(GUID guidStream)
{
	IHUSVideoRecordSearchContextPtr hus_record_search_context = NULL;
	HRESULT hr = hus_record_search_context.CreateInstance(CLSID_HUSVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_search_context != NULL);
	if (FAILED(hr)) CLog::Log(SDKCOM, LL_NORMAL, "%s 创建 HUSVideoRecordSearchContext 失败!\r\n", __FUNCTION__);

	// fetch the IRecordInfo Interface
	IRecordInfoPtr pRecordInfo = NULL;
	hr = GetRecordInfoFromGuids(__uuidof(mscorlib::__mscorlib), 2, 0, 0, __uuidof(Guid), &pRecordInfo);
	ASSERT(SUCCEEDED(hr) && pRecordInfo);

	//create safe array with one element
	SAFEARRAYBOUND sab = { 1, 0 };
	guid_safe_arry = SafeArrayCreateEx(VT_RECORD, 1, &sab, pRecordInfo);
	GUID *iguid = NULL;
	hr = SafeArrayAccessData(guid_safe_arry, reinterpret_cast<PVOID*>(&iguid));
	ASSERT(SUCCEEDED(hr) && iguid);
	iguid[0] = guidStream;
	SafeArrayUnaccessData(guid_safe_arry);
	streamerid_to_search = guidStream;  //Save the streamer 
	hus_record_search_context->PutDeviceIds(guid_safe_arry);
	hus_record_search_context->PutPrecision(1);

	ISearchContextPtr searchContext = NULL;
	hus_record_search_context->QueryInterface(IID_PPV_ARGS(&searchContext));

	auto bstrSiteID = m_pVmsSiteProxy->m_pSynClient->GetSiteID();
	auto bstrSiteIP = m_pVmsSiteProxy->m_pSynClient->GetSiteIP();

	searchContext->PutSiteId(bstrSiteID);
	searchContext->PutSiteIP(bstrSiteIP);

	CLog::Log(SDKCOM, LL_NORMAL, "%s  siteIp = %s \r\n", __FUNCTION__, bstrSiteIP);

	IHUSVideoRecordSearcherPtr hus_record_searcher = NULL;
	hr = hus_record_searcher.CreateInstance(CLSID_HUSVideoRecordSearcher, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_searcher != NULL);
	if (FAILED(hr))	CLog::Log(SDKCOM, LL_NORMAL, "%s 创建 HUSVideoRecordSearcher失败!\r\n", __FUNCTION__);
	hus_record_searcher->SetContext(hus_record_search_context);
	CLog::Log(SDKCOM, LL_NORMAL, "ready to call SearchRecord:");

	return hus_record_searcher;
}

#endif