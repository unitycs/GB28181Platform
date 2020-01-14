#pragma once
#include "EC/ECClient.h"
#include "Memory/SharedVarQueue.h"
#include "SDKDevInfo.h"
#include "SDKEventsHandler.h"
#include "SDKDeviceCtrlMgr.h"
#include "WebServiceSDK.h"
#include "SDKSiteProxy.h"
class CSDKCom : public CModuleWithIQ
{
public:

	CSDKCom(void);
	virtual ~CSDKCom(void) = default;

	void Init(void) override;
	void Cleanup(void) override;

	// 处理内存队列消息的函数
	bool HandleMsg(CMemPoolUnit * pUnit) override;

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("SDKCOM");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};



	// 初始化SDK接口
	int InitSDKInterface();

	// 处理数据更新线程
	static UINT AFX_CDECL pfnUpdateData(LPVOID lParam);

	// ReportAlarm线程
	//static UINT AFX_CDECL pfnReportAlarmProc(LPVOID lParam);

	// 视频录像查询处理线程
	static UINT AFX_CDECL pfnSearchVideoRecordProc(LPVOID lParam);

	// 处理设备控制
	int ProcDeviceCtrl(CModMessage * pUnifiedMsg);

	int ProcDecorderPlayRequest(CModMessage* pUnifiedMsg);
	int ProcDecorderPlayAck(CModMessage* pUnifiedMsg);
	int ProcDecorderStop(CModMessage* pUnifiedMsg);
	// 处理设备状态查询
	int ProcStatusSearch(CModMessage * pUnifiedMsg);

	// 处理录像文件查询
	int ProcRecordSearch(CModMessage * pUnifiedMsg);

	// 处理设备配置查询 GB14 add
	int ProcConfigSearch(CModMessage * pUnifiedMsg);

	// 处理设备配置 GB14 add
	int ProcDeviceConfig(CModMessage * pUnifiedMsg) const;

	// 处理Url查询 STB add real/playback
	int ProcUrlQuery(CModMessage * pUnifiedMsg);

	// 处理StopPlayUrl
	int ProcStopPlayUrl(CModMessage * pUnifiedMsg);

	// 处理StopPlayUrl
	int ProcDecoderStatus(CModMessage * pUnifiedMsg) const;

	int ProcDecoderInfo(CModMessage *pUnifiedMsg) const;

	int ProcUpdateRecord(CModMessage* pUnifiedMsg);

	// 处理DecoderDivision
	int ProcDecoderDivision(CModMessage * pUnifiedMsg);

	// 转发更新消息
	void NoticeDevInfo(
		const char *pszGUID,
		event_notify_t::e_update_t eUpdateType,
		const char *pszDeviceID = nullptr,
		mod_op_t::ot_devinfo devOperateType = mod_op_t::ot_devinfo::update_data);

	// 创建EC SDK客户端
	void CreateECClient();
	BOOL ConnectToEC(GUID guidEC);
	// 读取配置文件
	int ReadConfig();

protected:

	// 保存录像信息的内存管理器
	CMemPool	m_oRecordMemMgr;

	CAllocator<CModMessage> m_MemAllocator;

	// 数据更新消息队列
	CSharedVarQueue  *m_pShareUpdateMsg;

	// 录像查询结果集
	VideoRecordsSeachMgr m_VideoRecordMgr;

	// 设备基本信息
	DevicesInfoMgr m_DeviceInfoMgr;


	CList<GUID> m_listConnectedEC;

	//设备信息的动态事件(动态同步)，比如增加e，修改删除等，
	SiteEventsClient m_DsEventClient;

	DeviceController m_DeviceController;

	VMSVideoManager m_VmsVideoRecordMgr;

	VmsSiteProxy m_VmsSiteProxy;

	ECClient	m_ECClient;
private:
	friend class SiteEventsClient;
	friend class DeviceController;
	friend class VideoRecordSearcher;
	friend class DevicesInfoMgr;
};
