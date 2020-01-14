#ifdef SDK_VERSION_5_0
#include "simobject.h"
#include "main/UnifiedMessage.h"
class SiteEventsClient;
class VideoRecordSearcher :private SiteJobWoker
{
private:
	// encapsulation of search result callback
	class RecordCallback : public CComSimDispatchCoClass < RecordCallback, ISearchCallback > {
	public:
		RecordCallback() {
			ptr_record_searcher_ = NULL;
		}

		bool SetVideoRecordSearcher(VideoRecordSearcher* ptr_record_searcher) {
			if (NULL == ptr_record_searcher) return false;

			ptr_record_searcher_ = ptr_record_searcher;
			return true;
		}

		HRESULT __stdcall raw_ReportResult(ISearchResult* result) {
			if (NULL == ptr_record_searcher_) return S_FALSE;
			ptr_record_searcher_->ReportResult(result);
			return S_OK;
		}

	private:
		VideoRecordSearcher* ptr_record_searcher_;
	};

public:
	virtual ~VideoRecordSearcher() = default;

	VideoRecordSearcher() {
		ptr_callback_object_ = NULL;
		HRESULT hr = ComObjectCreate(ptr_callback_object_, *(&iptr_record_callback_));
		if (SUCCEEDED(hr)) {
			if (NULL != ptr_callback_object_) {
				ptr_callback_object_->SetVideoRecordSearcher(this);
			}
		}
	}

public:

	void SetContext(SiteEventsClient *pEventClient, CSDKCom* m_pSDKCom, VmsSiteProxy * pVmsSiteProxy = nullptr, CAllocator<CModMessage> * pMemAllocator = nullptr);

	void InitVideoSearch(GUID guidStream, HUSDevice_T video_type = HUSDevice_T::NVR_DVR);

	bool SearchRecord(DATE start_time, DATE end_time, long priority);

	// search result callback
	virtual void ReportResult(ISearchResult * result);

private:
	// callback Objects
	CComObject<RecordCallback>* ptr_callback_object_;
	ISearchCallbackPtr iptr_record_callback_;
	SAFEARRAY * guid_safe_arry = nullptr;
	// search id
	GUID streamerid_to_search = GUID_NULL;
	HUSDevice_T   video_type_to_search;
	SiteEventsClient *m_pEventClient = nullptr;

	void addSingleRecord(DATE recordStartTime, DATE recordEndTime, int triggleType) const;

	IVideoRecordSearcherPtr ptr_video_record_searcher = nullptr;
	IVideoRecordSearcherPtr GetDVRVideoRecordSearcher(GUID guidStream);
	IVideoRecordSearcherPtr GetHUSVideoRecordSearcher(GUID guidStream);
};

#endif