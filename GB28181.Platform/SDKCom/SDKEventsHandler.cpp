#include "stdafx.h"
#include "SDKEventsHandler.h"
#include "SDKCom.h"


void SiteEventsClient::SetContext(CSDKCom * p_SDKCom, VmsSiteProxy * pVmsSiteProxy, CAllocator<CModMessage>* pMemAllocator)
{
	SiteJobWoker::SetContext(p_SDKCom, pVmsSiteProxy, pMemAllocator);
}

BOOL SiteEventsClient::RegisterToSiteEventsSource()
{

	auto pMsgEvents = m_pVmsSiteProxy->m_pSiteImageAdapter->GetMsgEvents();
	auto  pIMsgEvent = pMsgEvents->GetItem(SiteElementType_Device);

	CLog::Log(SDKCOM, LL_NORMAL, "ISynEvent接口：初始化站点设备事件动态同步.");
	ISynEvent *pISynEvent = nullptr;
	if (FAILED(pIMsgEvent->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pISynEvent))))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "无法取得ISynEvent接口.");
		return FALSE;
	}

	if (FAILED(DispSynEvent::DispEventAdvise(pISynEvent)))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "DispEventAdvise(pISynEvent)失败");
		return FALSE;;
	}
#ifdef SDK_VERSION_4_3
	if (FAILED(DispFactoryEvent::DispEventAdvise(m_pVmsSiteProxy->m_pAdaptorFactory)))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "DispEventAdvise(m_pAdaptorFactory)失败:初始化站点记录查询.");
		return FALSE;
	}
#endif
	return TRUE;
}

BOOL SiteEventsClient::InitRecordSearchContext()
{
#ifdef SDK_VERSION_4_3	
	m_pVmsSiteProxy->InitRecordSearchContext();
#endif
	CLog::Log(SDKCOM, LL_NORMAL, "初始化录像查询接口。");
	return TRUE;
}

int SiteEventsClient::HandleRecordSearchResult() const
{
	auto & mRecord = m_pSDKCom->m_VideoRecordMgr;
	auto & m_memAllocator = m_pSDKCom->m_MemAllocator;
	auto pUnifiedMsg = mRecord.GetSearchMessage();
	CString  strDeviceID = pUnifiedMsg->GetDeviceID();

	CLog::Log(SDKCOM, LL_NORMAL, "录像文件查询完成 设备ID:%s SN:%s 数量:%d", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN(), mRecord.GetRecordCount());

	auto  pResultBuf = m_memAllocator.AllocBodyContentBuf();

	// 把缓存挂载到消息中
	pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));

	DeviceBasicObject::InfoContext_t	tContext;
	CModMessage  oUnifiedpMsg;
	CString  strSN = pUnifiedMsg->GetQuerySN();
	mRecord.SetDeviceID(strDeviceID);

	// 向缓存中写入XML格式的查询结果数据
	while (0 < mRecord.GetCatalogBodyContent(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), &tContext))
	{
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		pUnifiedMsg->SetDeviceID(strDeviceID);
		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);

		// 生成模块消息对象
		pUnifiedMsg = m_memAllocator.AllocModMessage();
		// 生成大文件缓存
		pResultBuf = m_memAllocator.AllocBodyContentBuf();

		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
	}

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);

	// 发送到SIPCom模块
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	mRecord.RemoveHeadSearchMessage();
	mRecord.SetSearchFinish();

	return 0;
}

// 转发更新消息
void SiteEventsClient::ForwardToDevInfo(const char *pszGUID, event_notify_t::e_update_t eUpdateType) const
{
	auto & m_memAllocator = this->m_pSDKCom->m_MemAllocator;
	// 通知DevInfo模块更新数据
	// 生成模块消息对象
	auto pUnifiedMsg = m_memAllocator.AllocModMessage();
	pUnifiedMsg->SetModAction(mod_op_t::ot_devinfo::update_data);
	pUnifiedMsg->SetUpdataGUID(pszGUID);
	pUnifiedMsg->SetNotifyUpdataType(eUpdateType);
	CRouter::PushMsg(DEVINFO, pUnifiedMsg);
}

HRESULT __stdcall SiteEventsClient::OnAddDevice(BSTR strID)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;

	CString strGUID(strID);
	CLog::Log(SDKCOM, LL_NORMAL, "添加设备 虚拟GUID:%s", strGUID);
	GUID guidDevice;
	wstring wstrID = strID;
	if (L'{' != wstrID[0])
		wstrID = L"{" + wstrID + L"}";

	CLSIDFromString(wstrID.c_str(), &guidDevice);

	_ECElementPtr ptrECElement = m_ptrSynAdapter->GetElement(guidDevice);
	if (NULL != ptrECElement)
	{
		_ElementSettingsPtr ptrElementSetting = ptrECElement->GetSettings();
		if (NULL != ptrElementSetting)
		{
			VARIANT varParam;
			BSTR	bstrTargetDeviceID = SysAllocString(L"TargetDeviceID");
			ptrElementSetting->get_Item(bstrTargetDeviceID, &varParam);
			SysFreeString(bstrTargetDeviceID);
			if (NULL == varParam.bstrVal)
			{
				CLog::Log(SDKCOM, LL_NORMAL, "未取得真实GUID 忽略设备添加操作");
				goto error;
			}

			std::basic_string<OLECHAR> strGuid;
			if ('{' == varParam.bstrVal[0])
				strGuid = varParam.bstrVal;
			else
			{
				strGuid.append(1, '{');
				strGuid.append(varParam.bstrVal);
				strGuid.append(1, '}');
			}
			CLSIDFromString(const_cast<LPOLESTR>(strGuid.c_str()), &guidDevice);
			CString strRealGUID;
			Utils::GUIDToCString(guidDevice, strRealGUID);
			CLog::Log(SDKCOM, LL_NORMAL, "添加设备 真实GUID:%s", strRealGUID);
			// 判断该设备是否挂载到gateway
			auto tmpguid = guidDevice;
			if (FALSE == CheckSource(tmpguid))
			{
				CLog::Log(SDKCOM, LL_NORMAL, "当前设备未挂载到Gateway GUID:%s", strRealGUID);
				return S_OK;
			}
		}
	}

	m_DeviceInfoMgr.AddDeviceObject(guidDevice);
	m_pSDKCom->CreateECClient();
	ForwardToDevInfo(strGUID.GetString(), event_notify_t::e_update_t::ut_add);
error:
	return S_OK;
}

HRESULT __stdcall SiteEventsClient::OnDelDevice(BSTR strID)
{
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;
	CString strGUID(strID);
	CLog::Log(SDKCOM, LL_NORMAL, "删除设备 GUID:%s", strGUID);

	// 取得第三个GUID
	int nPos = strGUID.ReverseFind('#');
	if (0 < nPos)
	{
		strGUID = strGUID.Left(nPos);
	}
	nPos = strGUID.ReverseFind('#');
	if (0 < nPos)
	{
		strGUID = strGUID.Right(strGUID.GetLength() - nPos - 1);
	}
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	GUID guidDevice;
	BSTR bstrGUID = strGUID.AllocSysString();
	CLSIDFromString(bstrGUID, &guidDevice);
	SysFreeString(bstrGUID);
	m_DeviceInfoMgr.DeleteDeviceObject(strGUID.GetString());
	ForwardToDevInfo(strGUID.GetString(), event_notify_t::e_update_t::ut_del);
	return S_OK;
}

HRESULT __stdcall SiteEventsClient::OnModDevice(BSTR strID) {
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;
	CString strGUID(strID);
	CLog::Log(SDKCOM, LL_NORMAL, "修改设备 GUID:%s", strGUID);
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";

	GUID guidDevice;
	BSTR bstrGUID = strGUID.AllocSysString();
	CLSIDFromString(bstrGUID, &guidDevice);
	SysFreeString(bstrGUID);
	if (FALSE == CheckSource(guidDevice))
		return S_OK;

	m_DeviceInfoMgr.DeleteDeviceObject(strGUID.GetString());
	wstring wstrID = strID;
	if (L'{' != wstrID[0])
		wstrID = L"{" + wstrID + L"}";
	CLSIDFromString(wstrID.c_str(), &guidDevice);
	m_DeviceInfoMgr.AddDeviceObject(guidDevice);
	m_pSDKCom->CreateECClient();
	ForwardToDevInfo(strGUID.GetString(), event_notify_t::e_update_t::ut_mod);
	return S_OK;
}

HRESULT __stdcall SiteEventsClient::OnVideoDBAction(IUnknown* /*lpSender*/, struct _HisActionArg * pAction) const
{
	auto & m_VideoRecordMgr = this->m_pSDKCom->m_VideoRecordMgr;
	if (pAction->HisActionType == EN_SEARCH_ACTION_HISTORY_SEARCH_RECROD)
	{
		_SiteAppendPtr pAppend;
		pAppend = pAction->BindData;
		CLog::Log(SDKCOM, LL_NORMAL, "%s pAppend = %08x\r\n", __FUNCTION__, pAppend);
		if (pAppend != NULL)	//	Indicate this search record is from web
		{

			DATE stime = pAction->StartTime;
			DATE etime = pAction->EndTime;

			int nReiggerType = pAppend->GetintTriggerType();

			auto pRecordbuf = m_VideoRecordMgr.GetRecordBuf();
			if (pRecordbuf)
			{
				pRecordbuf->SetType(static_cast<record_type_t>(nReiggerType));
				pRecordbuf->SetStartTime(stime);
				pRecordbuf->SetEndTime(etime);
				pRecordbuf->SetAddr(m_VideoRecordMgr.GetSearchMessage()->GetRecordAddr());
				string fileName;
				strcpy_s(pRecordbuf->fileName, fileName.c_str());
				string filePath = "c:\record";
				strcpy_s(pRecordbuf->filePath, filePath.c_str());
				string filesize = "0";
				strcpy_s(pRecordbuf->fileSize, filesize.c_str());
				string recordId = "0";
				strcpy_s(pRecordbuf->recordId, recordId.c_str());
				m_VideoRecordMgr.InserRecord(pRecordbuf);
			}
			return S_OK;
		}
	}
	else if (pAction->HisActionType == EN_SEARCH_ACTION_HISTORY_SEARCH_FINISH)
	{
		//hus4.3 RecordSearch逻辑
		HandleRecordSearchResult();
	}
	else if (pAction->HisActionType != EN_SEARCH_ACTION_HISTORY_SEARCH_ERROR)
	{
	}
	else if (pAction->HisActionType != EN_SEARCH_ACTION_HISTORY_DOWNLOAD_FINISH)
	{
	}
	else if (pAction->HisActionType != EN_SEARCH_ACTION_HISTORY_SEARCH_BEGIN)
	{
	}

	return S_OK;
}

BOOL SiteEventsClient::CheckSource(GUID &guidDevice) const
{
	auto& m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;
	// 取得SourceDeviceID
	VARIANT varParam;
	basic_string<OLECHAR> strGuid;
	CString strSourceID;
	CString strGatewayID;

	auto ptrElementSetting = m_ptrSynAdapter->GetElementSettings(guidDevice);
	if (NULL == ptrElementSetting)
		return FALSE;

	BSTR bstrSourceDeviceID = SysAllocString(L"SourceDeviceID");
	ptrElementSetting->get_Item(bstrSourceDeviceID, &varParam);
	SysFreeString(bstrSourceDeviceID);
	if (NULL == varParam.bstrVal)
		return FALSE;

	strGuid = varParam.bstrVal;
	CLSIDFromString(const_cast<LPOLESTR>(strGuid.c_str()), &guidDevice);
	Utils::GUIDToCString(guidDevice, strSourceID);

	if ('{' == varParam.bstrVal[0])
		strGatewayID = m_DeviceInfoMgr.GetGatewyGUID(TRUE);
	else
		strGatewayID = m_DeviceInfoMgr.GetGatewyGUID(FALSE);
	// SourceDeviceID和GatewayGUID不相等，该设备未挂载到gateway
	if (strSourceID == strGatewayID)
		return TRUE;

	return FALSE;
}