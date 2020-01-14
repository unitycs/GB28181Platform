#pragma once
#include "HUSSDKUnified.h"
#include "SDKDecoder.h"
class VideoRecordsSeachMgr;
class AlarmRecordsMgr;
class DevicesInfoMgr;


class DeviceController :private SiteJobWoker
{
public:
	DeviceController() = default;
	virtual ~DeviceController() = default;
	void SetContext(CSDKCom* m_pSDKCom, VmsSiteProxy * pVmsSiteProxy = nullptr, CAllocator<CModMessage> * pMemAllocator = nullptr) override;
	int ProcDeviceCtrl(CModMessage* pUnifiedMsg);
	void PostDecoderCommand(GUID& guidDecoder, GUID& guidEncoder, GUID& guidEC, SectionID eSectionID, const char* pszChannelNumber);
	int ProcUrlQuery(CModMessage* pUnifiedMsg);
	int ProcStopPlayUrl(CModMessage* pUnifiedMsg);
	int ProcDecoderStatus(CModMessage* pUnifiedMsg) const;
	int ProcDecoderInfo(CModMessage* pUnifiedMsg) const;
	int ProcDecoderDivision(CModMessage* pUnifiedMsg);
	bool GetDecoderInfo();
	int ProcDeviceConfig(CModMessage* pUnifiedMsg) const;

	int UpdateWebRecord(CModMessage* pUnifiedMsg);

private:

	int DeviceControlResponse(CModMessage * pUnifiedMsg, const char *pszResult) const;
	void PostPTZCommand(GUID& guidVirChannel, GUID &guidVirStreamer, GUID& guidEC, int nItemID, int nStep, int nItemID_Old, int nOldStep);
	void PostDragZoomCommand(GUID& guidChannel, GUID &guidVirStreamer, GUID& guidEC, int nItemID, double x, double y, double length, double height, double z);

	void PostDecoderPlayCommand(GUID& guidDecoder, GUID& guidStreamer, int nDecoderChannelID, int nDivisionID, BOOL bStop);
	void PostDecoderDivisionCommand(GUID& guidDecoder, int nDecoderChannelID, int nDivisionNumber);

	BOOL CreateDeviceTreeForDecoder(CString& strDeviceTree, const GUID& guidDevice) const;

	int DeviceConfigResponse(CModMessage* pUnifiedMsg, const char* pszResult) const;

	SDKDecoder husDecoder;

	int     m_videoWidth;
	int     m_videoHeight;

	int CreateWebRecord(DeviceObject& device_obj, CModMessage * pUnifiedMsg);

	time_t GetNowTimeStamp(int seconds_to_add = 0);
};
