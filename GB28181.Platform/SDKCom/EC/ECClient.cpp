#include "StdAfx.h"
#include "ECClient.h"
#include "SDKCom/SDKCom.h"

ECClient::~ECClient(void)
{
	if (m_pConnectionManagerProvider) {
		DispEventUnadvise(reinterpret_cast<IConnectionManagerEvents*>(m_pConnectionManagerProvider));
		m_pConnectionManagerProvider->Release();
		m_pConnectionManagerProvider = nullptr;
	}
}

BOOL ECClient::CheckConection(CString strECGUID)
{
	if (!m_Connections.empty() && m_Connections.find(strECGUID.GetString()) != m_Connections.cend())
		return TRUE;
	return FALSE;

}

BOOL ECClient::AddServer(LPCTSTR lpstrServerIP, long nServerPort, LPCTSTR lpstrServerID) {
	if (NULL == m_pConnectionManagerProvider) {
		if (FAILED(CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_ALL, IID_IConnectionManagerProvider, reinterpret_cast<LPVOID*>(&m_pConnectionManagerProvider)))) {
			return FALSE;
		}

		DispEventAdvise(reinterpret_cast<IConnectionManagerEvents *>(m_pConnectionManagerProvider));
	}

	if (VARIANT_FALSE == m_pConnectionManagerProvider->GetAddServer(nServerPort, lpstrServerIP, lpstrServerID)) {
		return FALSE;
	}
	m_Connections[lpstrServerID].pszECIP = lpstrServerIP;
	m_Connections[lpstrServerID].b_Connected = false;
	return TRUE;
}

void ECClient::RemoveServer(LPCTSTR lpstrServerID)
{
	if (NULL == m_pConnectionManagerProvider || NULL == lpstrServerID) {
		return;
	}

	m_pConnectionManagerProvider->RemoveServer(lpstrServerID);
	m_Connections[lpstrServerID].pszDeviceSets.clear();
	m_Connections.erase(lpstrServerID);
}

INT ECClient::Connect(LPCTSTR /*lpstrName*/, LPCTSTR /*lpstrPassword*/, LPCTSTR lpstrServerID) {
	if (NULL == m_pConnectionManagerProvider || NULL == lpstrServerID) {
		return FALSE_FAIELD;
	}
	if (m_Connections[lpstrServerID].b_Connected)  return TRUE_DUP;
	if (VARIANT_FALSE == m_pConnectionManagerProvider->Connect(lpstrServerID)) {
		return FALSE_FAIELD;
	}
	m_Connections[lpstrServerID].b_Connected = true;
	return TRUE_OK;
}

void ECClient::DisConnect(LPCTSTR lpstrServerID)
{
	if (NULL == m_pConnectionManagerProvider || NULL == lpstrServerID) {
		return;
	}
	m_Connections[lpstrServerID].b_Connected = false;
	m_pConnectionManagerProvider->Disconnect(lpstrServerID);
}

INT  ECClient::ListenTo(LPCTSTR lpstrDeviceID, BOOL bListenTo, LPCTSTR lpstrServerID) {
	if (NULL == m_pConnectionManagerProvider || NULL == lpstrDeviceID || NULL == lpstrServerID) {
		return FALSE_FAIELD;
	}

	if (bListenTo) {
		auto &device_sets = m_Connections[lpstrServerID].pszDeviceSets;
		auto b_listend = device_sets.find(lpstrDeviceID);
		if (!device_sets.empty() && b_listend != device_sets.cend()) return TRUE_DUP;

		if (VARIANT_FALSE == m_pConnectionManagerProvider->ListenTo(lpstrDeviceID, lpstrServerID)) {
			return FALSE_FAIELD;
		}
		m_Connections[lpstrServerID].pszDeviceSets.emplace(lpstrDeviceID);
	}
	else {
		if (VARIANT_FALSE == m_pConnectionManagerProvider->CancelListenTo(lpstrDeviceID, lpstrServerID)) {
			return FALSE_FAIELD;
		}
		else {
			m_Connections[lpstrServerID].pszDeviceSets.erase(lpstrDeviceID);
		}
	}
	return TRUE_OK;
}

void __stdcall ECClient::OnOfflineEvent(VARIANT /*bstrGUID*/, IDispatch FAR* bstrDetail) {
	if (NULL == bstrDetail) {
		return;
	}

	DISPPARAMS dispparamsNoArgs = { nullptr, nullptr, 0, 0 };
	CComVariant varResult;
	CComExcepInfo tEI;
	DISPID dispid = DISPID_UNKNOWN;
	HRESULT hRet;

	OLECHAR FAR* szMember = L"ServerId";
	if (S_OK == bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult,
			&tEI, nullptr);
		if (hRet == DISP_E_EXCEPTION) {
			if (tEI.pfnDeferredFillIn != nullptr) {
				(*tEI.pfnDeferredFillIn)(&tEI);
			}
		}
		else if (hRet == S_OK && varResult.vt == VT_BSTR) {
			_bstr_t ServerID = varResult.bstrVal;
		}
	}

	BOOL bServerOnline;
	szMember = L"ServerOnline";
	hRet = bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if (hRet == S_OK) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
			&dispparamsNoArgs, &varResult, nullptr, nullptr);
		if (hRet == S_OK && varResult.vt == VT_BOOL) {
			bServerOnline = (varResult.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
		}
	}

	bstrDetail->Release();
}

void __stdcall ECClient::OnOnlineEvent(VARIANT /*bstrGUID*/, IDispatch FAR* bstrDetail) {
	if (NULL == bstrDetail) {
		return;
	}

	DISPPARAMS dispparamsNoArgs = { nullptr, nullptr, 0, 0 };
	CComVariant varResult;
	CComExcepInfo tEI;
	DISPID dispid = DISPID_UNKNOWN;
	HRESULT hRet;

	OLECHAR FAR* szMember = L"ServerId";
	if (S_OK == bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult,
			&tEI, nullptr);
		if (hRet == DISP_E_EXCEPTION) {
			if (tEI.pfnDeferredFillIn != nullptr) {
				(*tEI.pfnDeferredFillIn)(&tEI);
			}
		}
		else if (hRet == S_OK && varResult.vt == VT_BSTR) {
			_bstr_t ServerID = varResult.bstrVal;
		}
	}

	BOOL bServerOnline;
	szMember = L"ServerOnline";
	hRet = bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if (hRet == S_OK) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
			&dispparamsNoArgs, &varResult, nullptr, nullptr);
		if (hRet == S_OK && varResult.vt == VT_BOOL) {
			bServerOnline = (varResult.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
		}
	}

	bstrDetail->Release();

}

void __stdcall ECClient::OnMessageArrived(VARIANT /*bstrGUID*/, IDispatch FAR* bstrDetail) {
	if (NULL == bstrDetail) {
		return;
	}

	DISPPARAMS dispparamsNoArgs = { nullptr, nullptr, 0, 0 };
	CComVariant varResult;
	CComExcepInfo tEI;
	DISPID dispid = DISPID_UNKNOWN;
	HRESULT hRet;

	OLECHAR FAR* szMember = L"ServerId";
	if (S_OK == bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult,
			&tEI, nullptr);
		if (hRet == DISP_E_EXCEPTION) {
			if (tEI.pfnDeferredFillIn != nullptr) {
				(*tEI.pfnDeferredFillIn)(&tEI);
			}
		}
		else if (hRet == S_OK && varResult.vt == VT_BSTR) {
			_bstr_t ServerID = varResult.bstrVal;
		}
	}

	szMember = L"MessageContent";
	hRet = bstrDetail->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if (hRet == S_OK) {
		hRet = bstrDetail->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
			&dispparamsNoArgs, &varResult, nullptr, nullptr);
		if (hRet == S_OK && varResult.vt == VT_BSTR)
		{
			IECMessageObject *ptrIECMessageObject = nullptr;
			HRESULT hr = CoCreateInstance(CLSID_ECMessageObject, nullptr, CLSCTX_ALL, IID_IECMessageObject, reinterpret_cast<void**>(&ptrIECMessageObject));
			if (hr != S_OK || ptrIECMessageObject == nullptr) {
				return;
			}

			if (S_OK != ptrIECMessageObject->Initial(varResult.bstrVal)) {
				ptrIECMessageObject->Release();
				ptrIECMessageObject = nullptr;
				return;
			}

			auto strCommandType = ptrIECMessageObject->GetCommandType();
			// 设备GUID
			auto strSenderGUID = ptrIECMessageObject->GetOption(_T("device_id"));
			// 取得报警类型
			auto strAlarmCode = ptrIECMessageObject->GetOption(_T("alarm_type_code"));
			CString strType = static_cast<LPCSTR>(strAlarmCode);
			// 设备上线
			CLog::Log(SDKCOM, LL_DEBUG, "%s strType = %s\r\n", __FUNCTION__, strType);
			//     m_pParent->SetOnlineStatusByGUID((LPCSTR)strSenderGUID, 0);
			if (0 == strType.Compare("/1000/1"))
			{
				m_pParent->SetOnlineStatusByGUID(static_cast<LPCSTR>(strSenderGUID), 0);
			}
			// 设备下线
			else if (0 == strType.Compare("/1000/2"))
			{
				m_pParent->SetOnlineStatusByGUID(static_cast<LPCSTR>(strSenderGUID), 1);
			}
			// 设备报警
			else
			{
				int nAlarmType = 0;
				int nAlarmMethord = 0;
				CString strAlarmStatus = "Occur";
				if (0 == strType.Compare("/1/1")) {  //运动开始
					nAlarmType = 16;
					nAlarmMethord = 2;
				}
				else if (0 == strType.Compare("/1/2")) {	//运动结束
					nAlarmType = 16;
					nAlarmMethord = 2;
					strAlarmStatus = "Restore";
				}
				else if (0 == strType.Compare("/5/1")) {	//视频遮挡开始
					nAlarmType = 3;
					nAlarmMethord = 1;
				}
				else if (0 == strType.Compare("/5/2")) {	//视频遮挡结束
					nAlarmType = 3;
					nAlarmMethord = 1;
					strAlarmStatus = "Restore";
				}
				else if (0 == strType.Compare("/15/1")) {	//视频丢失开始
					nAlarmType = 1;
					nAlarmMethord = 1;
				}
				else if (0 == strType.Compare("/15/2")) {	//视频丢失开始
					nAlarmType = 2;
					nAlarmMethord = 1;
					strAlarmStatus = "Restore";
				}
				// 取得报警描述
				auto strAlarmTypeName = ptrIECMessageObject->GetOption(_T("alarm_type_name"));
				// 取得报警级别
				auto strAlarmSeverity = ptrIECMessageObject->GetOption(_T("alarm_severity"));
				// 事件发生时间
				auto strAlarmTime = ptrIECMessageObject->GetOption(_T("time"));

				m_pParent->HandleAlarmInfoFromDevice(static_cast<LPCSTR>(strSenderGUID), static_cast<LPCSTR>(strAlarmTypeName), static_cast<LPCSTR>(strAlarmSeverity), static_cast<LPCSTR>(strAlarmTime), nAlarmType, nAlarmMethord, strAlarmStatus);
			}
			ptrIECMessageObject->Release();
			ptrIECMessageObject = nullptr;
		}
	}
	bstrDetail->Release();
}

// 向指定EC服务发送控制命令
BOOL ECClient::PostCommand(CString &sCommandContent, CString &sServerDeviceID) const
{
	VARIANT_BOOL bResult;
	CComBSTR strCommandContent(sCommandContent);
	CComBSTR strServerID(sServerDeviceID);
	//HRESULT hr = m_ptrConnectionManagerProvider->PostCommand(strCommandContent, strServerID, &bResult);
	bResult = m_pConnectionManagerProvider->PostCommand(static_cast<BSTR>(strCommandContent), static_cast<BSTR>(strServerID));
	if (/*hr!=S_OK || */bResult != VARIANT_TRUE)
	{
		return FALSE;
	}
	return TRUE;
}
