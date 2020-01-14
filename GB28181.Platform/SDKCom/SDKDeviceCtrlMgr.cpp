#include "stdafx.h"
#include <future>
#include <chrono>
#include "ServerConsole.h"
#include "SDKDeviceCtrlMgr.h"
#include "SDKCom.h"

void DeviceController::SetContext(CSDKCom* p_SDKCom, VmsSiteProxy* pVmsSiteProxy, CAllocator<CModMessage> * pMemAllocator)
{

	SiteJobWoker::SetContext(p_SDKCom, pVmsSiteProxy, pMemAllocator);
	m_videoWidth = devInfoConf.m_CameraConfig.videoHeight_Base;
	m_videoHeight = devInfoConf.m_CameraConfig.videoWidth_Base;
}

int DeviceController::DeviceControlResponse(CModMessage * pUnifiedMsg, const char *pszResult) const
{
	CBodyBuilder oSIPBody;

	char* strSn = pUnifiedMsg->GetQuerySN();
	char* strDeviceId = pUnifiedMsg->GetDeviceID();
	if (NULL == pUnifiedMsg || NULL == pszResult)
		return ERROR_INVALID_PARAMETER;

	// 创建Response文件
	auto pUnifiedMsgTmp = m_pMemAllocator->AllocModMessage();
	auto pResponse = m_pMemAllocator->AllocBodyContentBuf();
	*pUnifiedMsgTmp = *pUnifiedMsg;
	oSIPBody.CreateControlResponseBody(pResponse->GetBuffer(), pResponse->GetBufferLen(), strSn, strDeviceId, pszResult);
	pUnifiedMsgTmp->SetNotifyData(reinterpret_cast<void *>(pResponse));

	pUnifiedMsgTmp->SetModAction(mod_op_t::ot_sipcom::notify_reponse);
	// 发送到SIPCom模块
	CRouter::PushMsg(SIPCOM, pUnifiedMsgTmp);

	return 0;
}
int DeviceController::ProcDeviceCtrl(CModMessage * pUnifiedMsg)
{
	DeviceObject deviceInfo;
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;
	CString strCtrlData = pUnifiedMsg->GetCtrlCmd();
	strCtrlData.MakeLower();
	//check ptz first
	if (strCtrlData.Left(2) == "a5" && strCtrlData.GetLength() == 16)
	{
		XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());
		CString szPriority = szParam->strParamVal2;
		delete szParam;
		// 解析PTZ指令
		CString strDeviceID = pUnifiedMsg->GetDeviceID();
		CString strSN = pUnifiedMsg->GetQuerySN();
		pUnifiedMsg->Free();

		GUID guidVirChannel;
		GUID guidVirStreamer;
		GUID guidEC;
		if (m_DeviceInfoMgr.ChannelLookup(strDeviceID, deviceInfo))
		{
			guidVirChannel = deviceInfo.linked.guidVirChannel;
			guidVirStreamer = deviceInfo.linked.guidVirSteamer;
			guidEC = deviceInfo.linked.guidEC;
			m_DeviceInfoMgr.ChannelLookupEnd();
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, "设备PTZ控制失败，未知的GB28181设备ID：%s SN:%s", strDeviceID, strSN);
			m_DeviceInfoMgr.ChannelLookupEnd();
			return -1;
		}

		// 默认动作
		int nItemID = PTZ_STOP;
		int nItemID_Old = PTZ_STOP;
		// 默认速度
		int nStepPan = 1;//水平方向
		int nStepTetil = 1;//垂直方向
		int nStep = 4;//实际步长
		int nStepTmp = 0;
		int nOldStep = 4;//为了支持国标
		const char *pByte = strCtrlData.GetString();

		// zoom in
		if (0 == Utils::StringCmp_s(pByte + 6, "20", 2))
		{
			nItemID = PTZ_ZOOMIN;
		}
		// zoom out
		else if (0 == Utils::StringCmp_s(pByte + 6, "10", 2))
		{
			nItemID = PTZ_ZOOMOUT;
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "81", 2)) //设置预置位
		{
			nItemID = 81;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "82", 2)) //调用预置位
		{
			nItemID = 80;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "83", 2)) //删除预置位
		{
			nItemID = 82;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "44", 2)) // 光圈+
		{
			nItemID = 64;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "48", 2)) // 光圈-
		{
			nItemID = 65;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "42", 2)) // 焦距+
		{
			nItemID = 48;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else if (0 == Utils::StringCmp_s(pByte + 6, "41", 2)) // 焦距-
		{
			nItemID = 49;
			nStep = Utils::strToHex(const_cast<char*>(pByte + 10));
		}
		else
		{
			int pan = 0;  //0 无效 -1 向左 1 向右
			int title = 0; //0 无效 1 向上 -1 向下

			//计算水平方向的步长
			nStepTmp = Utils::strToHex(const_cast<char*>(pByte + 8));
			if (nStepTmp == 0)
			{
				nStepPan = 0;
			}
			if (nStepTmp * 9 >= 255)
			{
				nStepPan = (nStepTmp * 9) / 255;
				nOldStep = nStepPan;
			}
			//计算垂直方向的步长
			nStepTmp = Utils::strToHex(const_cast<char*>(pByte + 10));
			if (nStepTmp == 0)
			{
				nStepTetil = 0;
			}
			else if (nStepTmp * 9 >= 255)
			{
				nStepTetil = (nStepTmp * 9) / 255;
				nOldStep = nStepTetil;
			}
			if (nStepTetil == 0 && nStepPan == 0)
			{
				nOldStep = 0;
			}
			if (0 == memcmp(pByte + 6, "08", 2))
			{
				nItemID_Old = PTZ_UP;
				title = 1;
				nStepPan = 0;
			}
			// down
			else if (0 == memcmp(pByte + 6, "04", 2))
			{
				nItemID_Old = PTZ_DOWN;
				title = -1;
				nStepPan = 0;
			}
			// left
			else if (0 == memcmp(pByte + 6, "02", 2))
			{
				nStepTetil = 0;
				nItemID_Old = PTZ_LEFT;
				pan = -1;
			}
			// right
			else if (0 == memcmp(pByte + 6, "01", 2))
			{
				nStepTetil = 0;
				nItemID_Old = PTZ_RIGHT;
				pan = 1;
			}
			// left up
			else if ((0 == memcmp(pByte + 6, "0a", 2)) || (0 == memcmp(pByte + 6, "0A", 2)))
			{
				pan = -1;
				title = 1;
			}
			//left down 
			else if ((0 == memcmp(pByte + 6, "06", 2)))
			{
				pan = -1;
				title = -1;
			}
			//rigth up 
			else if ((0 == memcmp(pByte + 6, "09", 2)))
			{
				pan = 1;
				title = 1;
			}
			//rigth down 
			else if ((0 == memcmp(pByte + 6, "05", 2)))
			{
				pan = 1;
				title = -1;
			}

			int panSpeed = pan * nStepPan;
			int  titleSpeed = title * nStepTetil;

			nItemID = 7;
			nStep = (((unsigned int)panSpeed << 16) & 0xFFFF0000) | ((unsigned int)titleSpeed & 0xFFFF);
			if (panSpeed == 0 && titleSpeed == 0)
			{
				nItemID = PTZ_STOP;
			}

			CLog::Log(SDKCOM, LL_NORMAL, "%s pan = %d title = %d panSpeed = %d titleSpeed = %d nStep = %d strCtrlData = %s nItemId = %d 1\r\n", __FUNCTION__, pan, title, panSpeed, titleSpeed, nStep, strCtrlData, nItemID);
		}

		PostPTZCommand(guidVirChannel, guidVirStreamer, guidEC, nItemID/*for 8-direction-PTZ*/, nStep, nItemID_Old/*for old PTZ way*/, nOldStep);
	}
	else if ("dragzoomin" == strCtrlData ||
		"dragzoomout" == strCtrlData)
	{
		DragParam_t *szParam = static_cast<DragParam_t*>(pUnifiedMsg->GetCmdParam());

		CString szLength = szParam->strLength;
		CString szWidth = szParam->strWidth;
		CString szMidPointX = szParam->strMidPointX;
		CString szMidPointY = szParam->strMidPointY;
		CString szLengthX = szParam->strLengthX;
		CString szLengthY = szParam->strLengthY;

		/* int screenHeight = atoi(szLength);
		int screenWidth = atoi(szWidth);*/
		int screenHeight = atoi(szWidth);
		int screenWidth = atoi(szLength);
		int point_x = atoi(szMidPointX);
		int point_y = atoi(szMidPointY);
		int areaLength = atoi(szLengthX);
		int areaWidth = atoi(szLengthY);

		delete szParam;

		CString strDeviceID = pUnifiedMsg->GetDeviceID();
		CString strSN = pUnifiedMsg->GetQuerySN();

		GUID guidChannel;
		GUID guidVirDeviceGUID;
		GUID guidEC;
		int nItemId = 8;
		if (m_DeviceInfoMgr.ChannelLookup(strDeviceID, deviceInfo))
		{
			guidChannel = deviceInfo.linked.guidParent;
			guidVirDeviceGUID = deviceInfo.linked.guidVirSteamer;
			guidEC = deviceInfo.linked.guidEC;
			m_DeviceInfoMgr.ChannelLookupEnd();
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, "设备拉框控制失败，未知的GB28181设备ID：%s SN:%s", strDeviceID, strSN);
			m_DeviceInfoMgr.ChannelLookupEnd();
			return -1;
		}

		double x = (point_x - (areaLength*1.0 / 2)) * m_videoWidth / screenWidth;
		double y = (point_y - (areaWidth*1.0 / 2)) * m_videoHeight / screenHeight;
		double width = areaLength*1.0 * m_videoWidth / screenWidth;
		double height = areaWidth*1.0  * m_videoHeight / screenHeight;
		double z = 200;

		if ("DragZoomOut" == strCtrlData)
		{
			z = 50;
		}

		CLog::Log(SDKCOM, LL_NORMAL, "%s screenHeight = %d screenWidth = %d point_x = %d point_y = %d areaLength = %d areaWidth = %d m_videoWidth = %d m_videoHeight = %d\r\n", __FUNCTION__, screenHeight, screenWidth, point_x, point_y, areaLength, areaWidth, m_videoWidth, m_videoHeight);
		// SDK暂不支持
		PostDragZoomCommand(guidChannel, guidVirDeviceGUID, guidEC, nItemId, x, y, width, height, z);
	}
	else if ("record" == strCtrlData)
	{
		XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());
		CString szRecLocation = szParam->strParamVal2;
		delete szParam;
		// 开始设备录像
		if (m_DeviceInfoMgr.ChannelLookup(pUnifiedMsg->GetDeviceID(), deviceInfo))
		{
			CString strGUID;
			Utils::GUIDToCString(deviceInfo.guidDevice, strGUID);
			m_DeviceInfoMgr.ChannelLookupEnd();
			pUnifiedMsg->SetNVRIP(deviceInfo.linked.strNVRIP);
			pUnifiedMsg->SetModAction(mod_op_t::ot_rtsp::record_start);
			pUnifiedMsg->SetPlayGUID(deviceInfo.linked.strStreamerGUID);

			DeviceControlResponse(pUnifiedMsg, "OK");
			CreateWebRecord(deviceInfo, pUnifiedMsg);
			/*
			std::async(launch::async, [&]()->int {
			CreateWebRecord(deviceInfo, pUnifiedMsg);
			}); //async handler
			*/
			CRouter::PushMsg(RTSPCOM, pUnifiedMsg);
		}
		else
		{
			m_DeviceInfoMgr.ChannelLookupEnd();
			CLog::Log(SDKCOM, LL_NORMAL, "开始录像失败 未知的GB28181设备ID:%s", pUnifiedMsg->GetDeviceID());
			DeviceControlResponse(pUnifiedMsg, "ERROR");
			SAFE_FREE_MOD_MSG(pUnifiedMsg);
		}
	}
	else if ("stoprecord" == strCtrlData)
	{
		// 停止设备录像
		if (m_DeviceInfoMgr.ChannelLookup(pUnifiedMsg->GetDeviceID(), deviceInfo))
		{
			CString strGUID;
			Utils::GUIDToCString(deviceInfo.guidDevice, strGUID);
			pUnifiedMsg->SetNVRIP(deviceInfo.linked.strNVRIP);
			m_DeviceInfoMgr.ChannelLookupEnd();
			DeviceControlResponse(pUnifiedMsg, "OK");
			pUnifiedMsg->SetModAction(mod_op_t::ot_rtsp::record_stop);
			pUnifiedMsg->SetPlayGUID(deviceInfo.linked.strStreamerGUID);
			CRouter::PushMsg(RTSPCOM, pUnifiedMsg);
		}
		else
		{
			m_DeviceInfoMgr.ChannelLookupEnd();
			CLog::Log(SDKCOM, LL_NORMAL, "停止录像失败 未知的GB28181设备ID:%s", pUnifiedMsg->GetDeviceID());
			DeviceControlResponse(pUnifiedMsg, "ERROR");
			SAFE_FREE_MOD_MSG(pUnifiedMsg);
		}
	}
	else if ("setguard" == strCtrlData || "resetguard" == strCtrlData)
	{
		DeviceAlarmInfo tInfo;
		// 开始设备布放
		if (m_DeviceInfoMgr.AlarmLookup(pUnifiedMsg->GetDeviceID(), tInfo))
		{
			if ("SetGuard" == strCtrlData)
			{
				tInfo.eStatus = DeviceAlarmInfo::DutyStatus::ONDUTY;
				CLog::Log(SDKCOM, LL_NORMAL, "设备布防 设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
			}
			else
			{
				tInfo.eStatus = DeviceAlarmInfo::DutyStatus::OFFDUTY;
				CLog::Log(SDKCOM, LL_NORMAL, "设备撤防 设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
			}
			m_DeviceInfoMgr.AlarmSetAt(pUnifiedMsg->GetDeviceID(), tInfo, FALSE);
			m_DeviceInfoMgr.AlarmLookupEnd();
			DeviceControlResponse(pUnifiedMsg, "OK");
			SAFE_FREE_MOD_MSG(pUnifiedMsg);
		}
		else
		{
			m_DeviceInfoMgr.AlarmLookupEnd();
			CLog::Log(SDKCOM, LL_NORMAL, "设备布/撤防失败 未知的GB28181设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
			DeviceControlResponse(pUnifiedMsg, "ERROR");
			SAFE_FREE_MOD_MSG(pUnifiedMsg);
		}
	}
	else if ("resetalarm" == strCtrlData)
	{
		// 告警复位
		m_DeviceInfoMgr.RemoveAlarming(pUnifiedMsg->GetDeviceID());
		CLog::Log(SDKCOM, LL_NORMAL, "报警复位 设备ID:%s", pUnifiedMsg->GetDeviceID());
		DeviceAlarmInfo tInfo;

		// 恢复报警状态
		if (m_DeviceInfoMgr.AlarmLookup(pUnifiedMsg->GetDeviceID(), tInfo))
		{
			if (DeviceAlarmInfo::DutyStatus::ALARM == tInfo.eStatus)
			{
				tInfo.eStatus = DeviceAlarmInfo::DutyStatus::ONDUTY;
				m_DeviceInfoMgr.AlarmSetAt(pUnifiedMsg->GetDeviceID(), tInfo, FALSE);
			}
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, "报警复位失败 无效的设备GBID:%s", pUnifiedMsg->GetDeviceID());
		}
		m_DeviceInfoMgr.AlarmLookupEnd();
		DeviceControlResponse(pUnifiedMsg, "OK");
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
	}
	else if ("boot" == strCtrlData)
	{
		// 不支持设备重启
		CLog::Log(SDKCOM, LL_NORMAL, "设备重启 设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return 0;
	}
	else if (strCtrlData == "realplayurl")
	{
		ProcUrlQuery(pUnifiedMsg);
		return 0;
		// pUnifiedMsg->SetModAction(ot_sdk_search_realplayurl);
	}
	else if (strCtrlData == "playbackurl")
	{
		ProcUrlQuery(pUnifiedMsg);
		return 0;
	}
	else if (strCtrlData == "decoderdivision")
	{
		ProcDecoderDivision(pUnifiedMsg);
		return 0;
	}
	else if (strCtrlData == "stopplayurl")
	{
		ProcStopPlayUrl(pUnifiedMsg);
		return 0;
	}
	else if (strCtrlData == "decoderstatus")
	{
		ProcDecoderStatus(pUnifiedMsg);
		return 0;
	}
	else if (strCtrlData == "decoderinfo")
	{
		ProcDecoderInfo(pUnifiedMsg);
		return 0;
	}
	else
	{
		//校时
		DATE tmCurTime;
		SipUnified::GB28181TimeToDATE(strCtrlData, tmCurTime);

		DeviceControlResponse(pUnifiedMsg, "OK");

		SAFE_FREE_MOD_MSG(pUnifiedMsg);
	}

	return 0;
}
void DeviceController::PostPTZCommand(GUID &guidVirChannel, GUID &GUIDVirStreamer, GUID &guidEC, int nItemID, int nStep, int nItemID_Old, int nOldStep)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pAdaptorFactory = this->m_pSDKCom->m_VmsSiteProxy.m_pAdaptorFactory;
	auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString strCommandContent;
	CString strVirChannelGUID;
	CString strVirStreamerGUID;
	CString strCommandGUID;

	Utils::GUIDToCString(guidVirChannel, strVirChannelGUID,FALSE);
	Utils::GUIDToCString(GUIDVirStreamer, strVirStreamerGUID, FALSE);

	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "开始设备PTZ控制 nItemId = %d nStep = %d", nItemID, nStep);

	// 已知被控设备的GUID，拿到其IConfig接口
	GUID guidDeviceType = m_ptrSynAdapter->GetTypeID(guidVirChannel);
	auto pIConfig = m_pAdaptorFactory->GetConfig(guidDeviceType);

	strCommandGUID = strVirChannelGUID;
	// 获取SectionID对应的ConfigSection接口
	_ConfigSectionPtr pConfigSection = pIConfig->getConfigSection(SI_PTZ_COMMAND);
	auto strVirTargetGUID = strVirChannelGUID; //默认channel层
	if (NULL == pConfigSection)
	{
		_ECElementPtr	*arrSubElements = nullptr;
		SAFEARRAY *sarrSubDevices = m_ptrSynAdapter->GetSubElementsArray(guidVirChannel);

		if (FAILED(SafeArrayAccessData(sarrSubDevices, reinterpret_cast<void**>(&arrSubElements))) || 0 == sarrSubDevices->rgsabound[0].cElements)
		{
			CLog::Log(SDKCOM, LL_NORMAL, "SafeArrayAccessData ERROR  DeviceGUID:%s", strVirChannelGUID);
			return;
		}

		GUID guidSub = arrSubElements[0]->GetID();
		Utils::GUIDToCString(guidSub, strVirChannelGUID);

		// 已知被控设备的GUID，拿到其IConfig接口
		GUID guidSubType = m_ptrSynAdapter->GetTypeID(guidSub);
		pIConfig = m_pAdaptorFactory->GetConfig(guidSubType);

		strCommandGUID = strVirStreamerGUID;
		// 获取SectionID对应的ConfigSection接口
		pConfigSection = pIConfig->getConfigSection(SI_PTZ_COMMAND);
		if (NULL == pConfigSection) {
			CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s", strVirChannelGUID, strECGUID);
			return;
		}
		strVirTargetGUID = strVirStreamerGUID; //重置为streamer层
	}
	// 设置ConfigSection中相应ConfigItem的参数
	auto pConfigItem = pConfigSection->getConfigItem(nItemID);
	if (!pConfigItem)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "取得PTZ设备的参数fsdgbhnfyj。nItemId = %d nStep = %d\r\n", nItemID, nStep);
		pConfigItem = pConfigSection->getConfigItem(nItemID_Old);
		nStep = nOldStep; //国标的步长 0-9
		nItemID = nItemID_Old; //only for log
	}
	CLog::Log(SDKCOM, LL_NORMAL, "取得PTZ设备的参数。nItemId = %d nStep = %d\r\n", nItemID, nStep);

	if (pConfigItem)
	{
		pConfigItem->Putvalue(nStep);
		if (80 == nItemID || 81 == nItemID || 82 == nItemID) {
			wchar_t strText[256];
			swprintf_s(strText, L"%d", nStep);
			wstring text = strText;
			pConfigItem->PutText(text.c_str());
		}
	}
	else
	{
		CLog::Log(SDKCOM, LL_DEBUG, "取得PTZ设备的参数in DeviceType by (pConfigItem = pConfigSection->getConfigItem(nItemID)) with nItemId = %d failure nStep = %d pls Import new device type define  to HUS Site \r\n", nItemID, nStep);
		return;
	}

	int nConfigItemCount = 1;
	CComSafeArray<LPDISPATCH> cItems(nConfigItemCount, 0);
	cItems.SetAt(0, pConfigItem);

	_bstr_t strPropertyValue = pConfigItem->SerializeArray(static_cast<LPSAFEARRAY>(cItems), SI_PTZ_COMMAND, pConfigSection->GetTitle(), static_cast<_bstr_t>(strVirChannelGUID));

	CString strMsgID;
	GUID guidMsg;

	CoCreateGuid(&guidMsg);
	Utils::GUIDToCString(guidMsg, strMsgID, FALSE);
	CLog::Log(SDKCOM, LL_DEBUG, "取得HUS Command");

	// 调用ICommandUtility接口，构造commandContent
	ICommandUtility *ptrCommandUtility;
	CString strNULLGUID = _T("00000000-0000-0000-0000-000000000000");
	CoCreateInstance(CLSID_CommandUtility, nullptr, CLSCTX_ALL, IID_ICommandUtility, reinterpret_cast<LPVOID*>(&ptrCommandUtility));
	CLog::Log(SDKCOM, LL_NORMAL, "取得HUS Command 1");
	if (!ptrCommandUtility)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s 取得PTZ设备的参数。createInstance CLSID_CommandUtility failure\r\n", __FUNCTION__);
		return;
	}
	CLog::Log(SDKCOM, LL_NORMAL, "取得HUS Command 2");
	auto commandContent = ptrCommandUtility->GetControlCommand(
		static_cast<_bstr_t>(strVirTargetGUID),
		static_cast<_bstr_t>(strNULLGUID),
		static_cast<_bstr_t>(strMsgID),
		pConfigSection->GetTitle(),
		_T(""), strPropertyValue, _T(""), _T(""), _T(""), _T(""), _T(""));

	CLog::Log(SDKCOM, LL_NORMAL, "%s 取得PTZ设备的参数。createInstance CLSID_CommandUtility suc \r\n", __FUNCTION__);
	// 发送控制命令
	strCommandContent = static_cast<char*>(commandContent);
	CLog::Log(SDKCOM, LL_DEBUG, "HUS Command:%s", strCommandContent);
	m_ECClient.PostCommand(strCommandContent, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "设备PTZ控制 设备DeviceGUID:%s 控制类型:%d", strVirChannelGUID, nItemID);
}

void DeviceController::PostDragZoomCommand(GUID &guidChannel, GUID &guidVirStreamer, GUID &guidEC, int nItemID, double x, double y, double length, double height, double z)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pAdaptorFactory = this->m_pSDKCom->m_VmsSiteProxy.m_pAdaptorFactory;
	auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString strCommandContent;
	CString strDeviceID;
	CString strVirSteamGUID;

	Utils::GUIDToCString(guidChannel, strDeviceID);
	Utils::GUIDToCString(guidVirStreamer, strVirSteamGUID, FALSE);

	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "%s begin nItemId = %d x = %d y = %d length = %d height = %d z = %d\r\n", __FUNCTION__, nItemID, x, y, length, height, z);

	// 已知被控设备的GUID，拿到其IConfig接口
	GUID guidDeviceType = m_ptrSynAdapter->GetTypeID(guidChannel);
	auto pIConfig = m_pAdaptorFactory->GetConfig(guidDeviceType);

	// 获取SectionID对应的ConfigSection接口
	_ConfigSectionPtr pConfigSection = pIConfig->getConfigSection(SI_PTZ_COMMAND);
	//if(NULL == pConfigSection)
	//{
	//	CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s", strDeviceID, strECGUID);
	//	return;
	//}
	if (NULL == pConfigSection)
	{
		_ECElementPtr	*arrSubElements = nullptr;
		SAFEARRAY *sarrSubDevices = m_ptrSynAdapter->GetSubElementsArray(guidChannel);

		if (FAILED(SafeArrayAccessData(sarrSubDevices, reinterpret_cast<void**>(&arrSubElements))) || 0 == sarrSubDevices->rgsabound[0].cElements)
		{
			CLog::Log(SDKCOM, LL_NORMAL, "SafeArrayAccessData ERROR  DeviceGUID:%s", strDeviceID);
			return;
		}

		GUID guidSub = arrSubElements[0]->GetID();
		Utils::GUIDToCString(guidSub, strDeviceID);

		// 已知被控设备的GUID，拿到其IConfig接口
		GUID guidSubType = m_ptrSynAdapter->GetTypeID(guidSub);
		pIConfig = m_pAdaptorFactory->GetConfig(guidSubType);

		// 获取SectionID对应的ConfigSection接口
		pConfigSection = pIConfig->getConfigSection(SI_PTZ_COMMAND);
		if (NULL == pConfigSection) {
			CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s", strDeviceID, strECGUID);
			return;
		}
	}

	char tempBuf[256] = { 0 };
	sprintf_s(tempBuf, "ZoomArea|[%lf,%lf]|[%lf]|[%lf,%lf]", x, y, z, length, height);
	CLog::Log(SDKCOM, LL_NORMAL, "%s 取得PTZ设备的参数 %s\r\n", __FUNCTION__, tempBuf);

	// 设置ConfigSection中相应ConfigItem的参数
	_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(nItemID);
	//   pConfigItem->Putvalue(nStep);
	if (pConfigItem)
	{
		pConfigItem->PutText(static_cast<_bstr_t>(tempBuf));
	}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s version no support", strDeviceID, strECGUID);
		return;
	}

	int nConfigItemCount = 1;
	CComSafeArray<LPDISPATCH> cItems(nConfigItemCount, 0);
	cItems.SetAt(0, pConfigItem);

	_bstr_t strPropertyValue = pConfigItem->SerializeArray(static_cast<LPSAFEARRAY>(cItems), SI_PTZ_COMMAND, pConfigSection->GetTitle(), static_cast<_bstr_t>(strVirSteamGUID));

	CString strMsgID;
	GUID guidMsg;

	CoCreateGuid(&guidMsg);
	Utils::GUIDToCString(guidMsg, strMsgID);
	CLog::Log(SDKCOM, LL_DEBUG, "取得HUS Command");

	// 调用ICommandUtility接口，构造commandContent
	ICommandUtility *ptrCommandUtility;
	CoCreateInstance(CLSID_CommandUtility, nullptr, CLSCTX_ALL, IID_ICommandUtility, reinterpret_cast<LPVOID*>(&ptrCommandUtility));
	CString strNULLGUID = _T("00000000-0000-0000-0000-000000000000");
	_bstr_t commandContent = ptrCommandUtility->GetControlCommand(
		static_cast<_bstr_t>(strVirSteamGUID),
		static_cast<_bstr_t>(strNULLGUID),/*strDeviceID*/
		static_cast<_bstr_t>(strMsgID),
		pConfigSection->GetTitle(),
		_T(""), strPropertyValue, _T(""), _T(""), _T(""), _T(""), _T(""));

	// 发送控制命令
	strCommandContent = static_cast<char*>(commandContent);
	CLog::Log(SDKCOM, LL_DEBUG, "HUS Command:%s", strCommandContent);
	m_ECClient.PostCommand(strCommandContent, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "设备PTZ控制 设备DeviceGUID:%s 控制类型:%d", strDeviceID, nItemID);
}

bool DeviceController::GetDecoderInfo()
{
	return husDecoder.getDecoderInfo();
}

int DeviceController::ProcUrlQuery(CModMessage * pUnifiedMsg)
{
	//auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strID = pUnifiedMsg->GetDeviceID();
	XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());
	CString strChanID = szParam->strParamVal1;

	DeviceObject	pDeviceInfo;

	DeviceObject deviceInfo;
	CString strGUID;
	CString  strNvrIp;
	GUID     guidStreamer;
	GUID     guidDecoder;
	if (m_pSDKCom->m_DeviceInfoMgr.ChannelLookup(pUnifiedMsg->GetDeviceID(), deviceInfo))
	{
		Utils::GUIDToCString(deviceInfo.linked.guidDevice, strGUID);
		guidStreamer = deviceInfo.linked.guidDevice;
		//   pUnifiedMsg->SetNVRIP(pInfo->strNVRIP);
		strNvrIp = deviceInfo.linked.strNVRIP;
		m_pSDKCom->m_DeviceInfoMgr.ChannelLookupEnd();
	}

	if (m_pSDKCom->m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		char *rtspStr = "rtsp://%s?streamid=%s&playtime=%s&endtime=%s";
		char tmpBuf[MAX_PATH * 4] = { 0 };

		auto pResultBuf = m_pMemAllocator->AllocBodyContentBuf();

		CString cmdType = pUnifiedMsg->GetQueryType();
		if (cmdType == "PlayBackUrl")
		{
			CString strStartTime = szParam->strParamVal1;
			CString strEndTime = szParam->strParamVal2;
			CString strRecLoaction = szParam->strParamVal3;
			__time64_t		tmStart = _atoi64(strStartTime);				// 播放开始时间
			__time64_t      tmEnd = _atoi64(strEndTime);
			CString strRang;
			tm tmInfo;
			//    tmInfo = *localtime(&pSession->tmStart);
			_gmtime64_s(&tmInfo, &(tmStart));
			strRang.Format("%d%02d%02dT%02d%02d%02dZ",
				tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
				tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

			tm tmEndInfo;
			//    tmEndInfo = *localtime(&pSession->tmEnd);
			_gmtime64_s(&tmEndInfo, &(tmEnd));
			//   CString strEndTime;
			strEndTime.Format("%d%02d%02dT%02d%02d%02dZ",
				tmEndInfo.tm_year + 1900, tmEndInfo.tm_mon + 1, tmEndInfo.tm_mday,
				tmEndInfo.tm_hour, tmEndInfo.tm_min, tmEndInfo.tm_sec);

			sprintf_s(tmpBuf, rtspStr, strNvrIp, strGUID, strRang, strEndTime);

			//    CString playUrlStr = "sdk no support";
			CString playUrlStr = tmpBuf;
			CString resultStr = "OK";
			//sdk
			CLog::Log(DEVINFO, LL_NORMAL, "%s playUrlStr = %s", __FUNCTION__, playUrlStr);
			/* CBodyBuilder::CreatePlayBackUrlResponse(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(),
			strSN, strID, "TODO");*/

			CBodyBuilder::CreatePlayBackUrlResponseByControl(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(),
				strSN, strID, playUrlStr, resultStr);
		}
		else if (cmdType == "RealPlayUrl")
		{
			rtspStr = "rtsp://%s?streamid=%s";

			//char tmpBuf[MAX_PATH] = {0};
			sprintf_s(tmpBuf, rtspStr, strNvrIp, strGUID);
			//       CString playUrlStr = "sdk no support";
			CString playUrlStr = tmpBuf;
			CString resultStr = "OK";
			//sdk
			CLog::Log(DEVINFO, LL_NORMAL, "%s playUrlStr = %s", __FUNCTION__, playUrlStr);
			CBodyBuilder::CreateRealPlayUrlResponseByContrl(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(),
				strSN, strID, strChanID, playUrlStr, resultStr);
		}

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN);

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);

		if (appConf.nSipComMode)
		{
			string divisonGbId(strChanID);
			husDecoder.g_division_vidioIdInfo[divisonGbId] = strID;
			decoderInfoStruc decoderInfo;

			husDecoder.queryDecoderDivisonInfo(divisonGbId, 0, decoderInfo);
			//todo send decoder;
			string decoderGbId = decoderInfo.gbId;
			DeviceObject	p_deviceInfo;
			if (m_pSDKCom->m_DeviceInfoMgr.DeviceLookup(decoderGbId.c_str(), p_deviceInfo))
			{
				guidDecoder = p_deviceInfo.guidDevice;
				//    CSDKDevInfoMgr::GUIDToCString(deviceInfo->guidDevice, strGUID);
				//   pUnifiedMsg->SetNVRIP(pInfo->strNVRIP);
				//    strNvrIp = pInfo->strNVRIP;
				//   m_DeviceInfoMgr.DecoderLookupEnd();
				m_pSDKCom->m_DeviceInfoMgr.DeviceLookupEnd();

				int chnNum = decoderInfo.decoderChnInfo[0].chnNum;
				int divison = decoderInfo.decoderChnInfo[0].screenInfo[0].screenNum;

				CLog::Log(DEVINFO, LL_NORMAL, "%s chnNum = %d divison = %d gbId = %s divisonGbId = %s", __FUNCTION__, chnNum, divison, decoderGbId.c_str(), divisonGbId.c_str());

				PostDecoderPlayCommand(guidDecoder, guidStreamer, chnNum, divison, FALSE);
			}

			//发送decoder状态
			char *decoderStatus = "<?xml version=\"1.0\"?>\r\n"
				"<Notify>\r\n"
				"<CmdType>DecoderStatus</CmdType>\r\n"
				"<SN>300</SN>\r\n"
				"<DeviceID>%s</DeviceID>\r\n"
				"<VideoDeviceID>%s</VideoDeviceID>\r\n"
				"</Notify>";

			// char tmpBuf[1024] = {0};
			sprintf_s(tmpBuf, decoderStatus, divisonGbId.c_str(), strID);

			//CBigFile *pResultBuf = NULL;
			pUnifiedMsg = m_pMemAllocator->AllocModMessage();
			pResultBuf = m_pMemAllocator->AllocBodyContentBuf();
			pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
			memcpy(pResultBuf->GetBuffer(), tmpBuf, strlen(tmpBuf) + 1);
			pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::decoder_status_notify_result);
			CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s info = %s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, pResultBuf->GetBuffer());

			CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		}
	}
	else
	{
		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询失败，未知的设备ID:%s  SN:%s", pUnifiedMsg->GetDeviceID(), strSN);
		pUnifiedMsg->Free();
	}
	delete szParam;

	return 0;
}

// 处理StopPlayUrl
int DeviceController::ProcStopPlayUrl(CModMessage * pUnifiedMsg)
{
	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strID = pUnifiedMsg->GetDeviceID();

	XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());

	//    DeviceInfo_t	*pDeviceInfo = NULL;

	//    ChannelInfo_t *pInfo = NULL;
	CString strGUID;
	CString  strNvrIp;
	GUID     guidStreamer;
	GUID     guidDecoder;

	decoderInfoStruc decoderInfo;
	string divisonGbId(strID);
	husDecoder.queryDecoderDivisonInfo(divisonGbId, 0, decoderInfo);
	string decoderGbId = decoderInfo.gbId;
	DeviceObject	deviceInfo;

	auto iter = husDecoder.g_division_vidioIdInfo.find(divisonGbId);
	if (iter != husDecoder.g_division_vidioIdInfo.end())
	{
		husDecoder.g_division_vidioIdInfo.erase(iter);
	}
	//todo to decoder
	if (m_pSDKCom->m_DeviceInfoMgr.DeviceLookup(decoderGbId.c_str(), deviceInfo))
	{
		guidDecoder = deviceInfo.guidDevice;
		//    CSDKDevInfoMgr::GUIDToCString(deviceInfo->guidDevice, strGUID);
		//   pUnifiedMsg->SetNVRIP(pInfo->strNVRIP);
		//    strNvrIp = pInfo->strNVRIP;
		//   m_DeviceInfoMgr.DecoderLookupEnd();
		m_pSDKCom->m_DeviceInfoMgr.DeviceLookupEnd();

		int chnNum = decoderInfo.decoderChnInfo[0].chnNum;
		int divison = decoderInfo.decoderChnInfo[0].screenInfo[0].screenNum;

		CLog::Log(DEVINFO, LL_NORMAL, "%s chnNum = %d divison = %d gbId = %s divisonGbId = %s", __FUNCTION__, chnNum, divison, decoderGbId.c_str(), divisonGbId.c_str());

		PostDecoderPlayCommand(guidDecoder, guidStreamer, chnNum, divison, TRUE);
	}

	//   if(m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		char tmpBuf[MAX_PATH * 4] = { 0 };

		auto pResultBuf = m_pMemAllocator->AllocBodyContentBuf();

		CString cmdType = pUnifiedMsg->GetQueryType();

		CString resultStr = "OK";
		CBodyBuilder::CreateStopPlayUrlResponseBody(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(),
			strSN, strID, resultStr);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN);

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);

		//发送decoder状态
		char *decoderStatus = "<?xml version=\"1.0\"?>\r\n"
			"<Notify>\r\n"
			"<CmdType>DecoderStatus</CmdType>\r\n"
			"<SN>300</SN>\r\n"
			"<DeviceID>%s</DeviceID>\r\n"
			"<VideoDeviceID>%s</VideoDeviceID>\r\n"
			"</Notify>";

		//    char tmpBuf[1024] = {0};
		string videoDeviceId;
		sprintf_s(tmpBuf, decoderStatus, divisonGbId.c_str(), videoDeviceId.c_str());

		//        CBigFile *pResultBuf = NULL;
		pUnifiedMsg = m_pMemAllocator->AllocModMessage();
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		memcpy(pResultBuf->GetBuffer(), tmpBuf, strlen(tmpBuf) + 1);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::decoder_status_notify_result);
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s info = %s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, pResultBuf->GetBuffer());

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}

	delete szParam;

	return 0;
}

int DeviceController::ProcDecoderStatus(CModMessage * pUnifiedMsg) const
{
	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strID = pUnifiedMsg->GetDeviceID();

	XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());

	//     DeviceInfo_t	*pDeviceInfo;

	//     ChannelInfo_t *pInfo;
	CString strGUID;
	CString  strNvrIp;

	decoderInfoStruc decoderInfo;
	string divisonGbId(strID);
	husDecoder.queryDecoderDivisonInfo(divisonGbId, 0, decoderInfo);
	string decoderGbId = decoderInfo.gbId;

	//   if(m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		CBigFile *pResultBuf = nullptr;

		char tmpBuf[MAX_PATH * 4] = { 0 };
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();

		//发送decoder状态
		char *decoderStatus = "<?xml version=\"1.0\"?>\r\n"
			"<Response>\r\n"
			"<CmdType>DecoderStatus</CmdType>\r\n"
			"<SN>%s</SN>\r\n"
			"<DeviceID>%s</DeviceID>\r\n"
			"<VideoDeviceID>%s</VideoDeviceID>\r\n"
			"</Response>";

		//    char tmpBuf[1024] = {0};
		string videoDeviceId;
		sprintf_s(tmpBuf, decoderStatus, strSN, divisonGbId.c_str(), videoDeviceId.c_str());

		//        CBigFile *pResultBuf = NULL;
		pUnifiedMsg = m_pMemAllocator->AllocModMessage();
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		memcpy(pResultBuf->GetBuffer(), tmpBuf, strlen(tmpBuf) + 1);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s info = %s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, pResultBuf->GetBuffer());

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}

	delete szParam;

	return 0;
}

int DeviceController::ProcDecoderInfo(CModMessage * pUnifiedMsg) const
{
	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strID = pUnifiedMsg->GetDeviceID();

	XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());

	//     DeviceInfo_t	*pDeviceInfo = NULL;

	//  ChannelInfo_t *pInfo = NULL;
	CString strGUID;
	CString  strNvrIp;

	decoderInfoStruc decoderInfo;
	string divisonGbId(strID);
	husDecoder.queryDecoderDivisonInfo(divisonGbId, 0, decoderInfo);
	string decoderGbId = decoderInfo.gbId;

	//   if(m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		CBigFile *pResultBuf = nullptr;

		char tmpBuf[MAX_PATH * 4] = { 0 };
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();

		//发送decoder状态
		char *decoderStatus = "<?xml version=\"1.0\"?>\r\n"
			"<Response>\r\n"
			"<CmdType>DecoderInfo</CmdType>\r\n"
			"<SN>%s</SN>\r\n"
			"<DeviceID>%s</DeviceID>\r\n"
			"<VideoDeviceID>%s</VideoDeviceID>\r\n"
			"</Response>";

		//    char tmpBuf[1024] = {0};
		string videoDeviceId;
		sprintf_s(tmpBuf, decoderStatus, strSN, divisonGbId.c_str(), videoDeviceId.c_str());

		//        CBigFile *pResultBuf = NULL;
		pUnifiedMsg = m_pMemAllocator->AllocModMessage();
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		memcpy(pResultBuf->GetBuffer(), tmpBuf, strlen(tmpBuf) + 1);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s info = %s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, pResultBuf->GetBuffer());

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}

	delete szParam;

	return 0;
}

// 处理DecoderDivision
int DeviceController::ProcDecoderDivision(CModMessage * pUnifiedMsg)
{
	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strID = pUnifiedMsg->GetDeviceID();
	//    GUID     guidStreamer;
	GUID     guidDecoder;

	XmlParam_t* szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());

	DeviceObject	deviceInfo;

	//    ChannelInfo_t *pInfo = NULL;
	CString strGUID;
	CString  strNvrIp;
	string chnId(strID);
	int divison = atoi(szParam->strParamVal1);
	decoderInfoStruc decoderInfo;
	vector<CatalogItem> cataLogInfo;
	husDecoder.modifDecoderDivisonInfo(chnId, divison, decoderInfo, cataLogInfo);
	int count = cataLogInfo.size();

	string decoderGbId = decoderInfo.gbId;
	if (m_pSDKCom->m_DeviceInfoMgr.DeviceLookup(decoderGbId.c_str(), deviceInfo))
	{
		guidDecoder = deviceInfo.guidDevice;
		m_pSDKCom->m_DeviceInfoMgr.DeviceLookupEnd();

		int chnNum = decoderInfo.decoderChnInfo[0].chnNum;
		//        int divison = decoderInfo.decoderChnInfo[0].screenInfo[0].screenNum;

		CLog::Log(DEVINFO, LL_NORMAL, "%s chnNum = %d  gbId = %s chnId = %s", __FUNCTION__, chnNum, decoderGbId.c_str(), chnId.c_str());

		//   PostDecoderPlayCommand(guidDecoder, guidStreamer, chnNum, divison, TRUE);
		PostDecoderDivisionCommand(guidDecoder, chnNum, divison);
	}
	//  if(m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		CBigFile *pResultBuf = nullptr;

		// char tmpBuf[MAX_PATH] = {0};
		pResultBuf = m_pMemAllocator->AllocBodyContentBuf();

		CString cmdType = pUnifiedMsg->GetQueryType();

		CString resultStr = "OK";
		CBodyBuilder::CreateDecoderDivisonResponseBody(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(),
			strSN, strID, resultStr);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN);

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);

		;
		CatalogCollections *decoderChnnInfo = new CatalogCollections;
		for (int i = 0; i < count; i++)
		{
			CatalogItem *pCatalog = new CatalogItem;
			*pCatalog = cataLogInfo[i];
			decoderChnnInfo->AddCatalog(cataLogInfo[i].GetDeviceID(), *pCatalog);
			delete pCatalog;
		}

		pUnifiedMsg = m_pMemAllocator->AllocModMessage();
		pUnifiedMsg->SetModAction(mod_op_t::ot_devinfo::send_decoder_subscribe);
		pUnifiedMsg->SetSearchData(decoderChnnInfo);
		CRouter::PushMsg(DEVINFO, pUnifiedMsg);
	}
	/* else
	{
	CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询失败，未知的设备ID:%s  SN:%s", pUnifiedMsg->GetDeviceID(), strSN);
	pUnifiedMsg->Free();
	}*/
	delete szParam;

	return 0;
}

void DeviceController::PostDecoderPlayCommand(GUID &guidDecoder, GUID &guidStreamer, int nDecoderChannelID, int nDivisionID, BOOL bStop)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pAdaptorFactory = this->m_pSDKCom->m_VmsSiteProxy.m_pAdaptorFactory;
	auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString strCommandContent;

	CString strDecoderID;
	Utils::GUIDToCString(guidDecoder, strDecoderID);

	//
	GUID guidTarget;
	GUID guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(guidDecoder, &guidTarget);

	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID);
	//    CLog::Log(SDKCOM, LL_NORMAL, "开始设备PTZ控制 nItemId = %d nStep = %d", nItemID, nStep);

	// 已知被控设备的GUID，拿到其IConfig接口
	GUID guidDeviceType = m_ptrSynAdapter->GetTypeID(guidDecoder);
	IConfigPtr pIConfig = m_pAdaptorFactory->GetConfig(guidDeviceType);

	// 获取SectionID对应的ConfigSection接口
	_ConfigSectionPtr pConfigSection = pIConfig->getConfigSection(SI_DECODER_PLAY2);
	if (NULL == pConfigSection)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s", strDecoderID, strECGUID);
		return;
	}

	CLog::Log(SDKCOM, LL_DEBUG, "取得PTZ设备的参数。");
	// 设置ConfigSection中相应ConfigItem的参数

	int nConfigItemCount = 9;
	CComSafeArray<LPDISPATCH> cItems(nConfigItemCount, 0);

	for (int i = 1; i < 5; i++) {
		_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(i);

		if (i == nDecoderChannelID + 1) {
			pConfigItem->Putvalue(1);
		}
		else {
			pConfigItem->Putvalue(0);
		}

		cItems.SetAt(i - 1, pConfigItem);
	}

	// wly
	CString strStreamGUID;
	int nMyIndex;
	Utils::GUIDToCString(guidStreamer, strStreamGUID);
	if (0 == strStreamGUID.CompareNoCase(_T("{2645b75b-15e9-44ff-8f4c-6fbef09a8809}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{e7d12eff-ef47-480a-9fba-75f5d857dbb1}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{21674f0f-f86f-47c3-99d4-d4c3e52f32c1}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{9b9f1c38-77e9-47cf-9fb6-64cc94cb0437}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{aa796a4c-7634-4230-bd55-f210e2d371bd}"))
		) {
		nMyIndex = 1;
	}
	else if (0 == strStreamGUID.CompareNoCase(_T("{6520f662-4206-4525-8b62-4ae2b96f4792}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{e8b9fe16-59ad-48c5-8317-aa726471b2a7}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{59e7eeea-804e-45e4-92fc-9fc90151a685}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{01f6fe88-df4f-4b83-ad48-ae27c260b6e1}")) ||
		0 == strStreamGUID.CompareNoCase(_T("{b50f3f69-0f38-48dc-9afc-c9f30e62b888}"))
		) {
		nMyIndex = 2;
	}
	else {
		nMyIndex = 0;
	}
	// wly

	_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(5);
	if (bStop) {
		pConfigItem->Putvalue(0);
	}
	else {
		pConfigItem->Putvalue(nMyIndex);
	}

	cItems.SetAt(4, pConfigItem);

	for (int i = 6; i < 10; i++) {
		pConfigItem = pConfigSection->getConfigItem(i);
		pConfigItem->Putvalue(nDivisionID);
		cItems.SetAt(i - 1, pConfigItem);
	}

	_bstr_t strPropertyValue = pConfigItem->SerializeArray(static_cast<LPSAFEARRAY>(cItems), SI_DECODER_PLAY2, pConfigSection->GetTitle(), static_cast<_bstr_t>(strDecoderID));

	CString strMsgID;
	GUID guidMsg;

	CoCreateGuid(&guidMsg);
	Utils::GUIDToCString(guidMsg, strMsgID);
	CLog::Log(SDKCOM, LL_DEBUG, "取得HUS Command");

	// 调用ICommandUtility接口，构造commandContent
	ICommandUtility *ptrCommandUtility;
	::CoCreateInstance(CLSID_CommandUtility, nullptr, CLSCTX_ALL, IID_ICommandUtility, reinterpret_cast<LPVOID*>(&ptrCommandUtility));
	_bstr_t commandContent = ptrCommandUtility->GetControlCommand(static_cast<_bstr_t>(strDecoderID), static_cast<_bstr_t>(strDecoderID), static_cast<_bstr_t>(strMsgID), pConfigSection->GetTitle(),
		_T(""), strPropertyValue, _T(""), _T(""), _T(""), _T(""), _T(""));

	// 发送控制命令
	strCommandContent = static_cast<char*>(commandContent);
	CLog::Log(SDKCOM, LL_DEBUG, "HUS Command:%s", strCommandContent);
	m_ECClient.PostCommand(strCommandContent, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "设备PTZ控制 设备DeviceGUID:%s ", strDecoderID);
}

void DeviceController::PostDecoderDivisionCommand(GUID &guidDecoder, int nDecoderChannelID, int nDivisionNumber)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pAdaptorFactory = this->m_pSDKCom->m_VmsSiteProxy.m_pAdaptorFactory;
	auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString strCommandContent;

	CString strDecoderID;
	Utils::GUIDToCString(guidDecoder, strDecoderID);

	//
	GUID guidTarget;
	GUID guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(guidDecoder, &guidTarget);

	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID);
	//    CLog::Log(SDKCOM, LL_NORMAL, "开始设备PTZ控制 nItemId = %d nStep = %d", nItemID, nStep);

	// 已知被控设备的GUID，拿到其IConfig接口
	GUID guidDeviceType = m_ptrSynAdapter->GetTypeID(guidDecoder);
	IConfigPtr pIConfig = m_pAdaptorFactory->GetConfig(guidDeviceType);

	// 获取SectionID对应的ConfigSection接口
	_ConfigSectionPtr pConfigSection = pIConfig->getConfigSection(SI_DECODER_LAYOUT);
	if (NULL == pConfigSection)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "PTZ控制失败 DeviceGUID:%s ECGUID:%s", strDecoderID, strECGUID);
		return;
	}

	CLog::Log(SDKCOM, LL_DEBUG, "取得PTZ设备的参数。");
	// 设置ConfigSection中相应ConfigItem的参数

	int nConfigItemCount = 4;
	CComSafeArray<LPDISPATCH> cItems(nConfigItemCount, 0);

	for (int i = 1; i < 5; i++) {
		_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(i);

		if (i == nDecoderChannelID + 1) {
			wchar_t strOption[256];
			swprintf_s(strOption, L"{1\\/0}{4\\/1}{9\\/2}{16\\/3}");

			wstring option = strOption;
			pConfigItem->PutText(option.c_str());

			if (1 == nDivisionNumber)
				pConfigItem->Putvalue(0);
			else if (4 == nDivisionNumber)
				pConfigItem->Putvalue(1);
			else if (9 == nDivisionNumber)
				pConfigItem->Putvalue(2);
			else if (16 == nDivisionNumber)
				pConfigItem->Putvalue(3);
			else
				pConfigItem->Putvalue(0);
		}
		else {
			pConfigItem->Putvalue(0);
		}

		cItems.SetAt(i - 1, pConfigItem);
	}

	_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(1);

	_bstr_t strPropertyValue = pConfigItem->SerializeArray(static_cast<LPSAFEARRAY>(cItems), SI_DECODER_LAYOUT, pConfigSection->GetTitle(), static_cast<_bstr_t>(strDecoderID));

	CString strMsgID;
	GUID guidMsg;

	CoCreateGuid(&guidMsg);
	Utils::GUIDToCString(guidMsg, strMsgID);
	CLog::Log(SDKCOM, LL_DEBUG, "取得HUS Command");

	// 调用ICommandUtility接口，构造commandContent
	ICommandUtility *ptrCommandUtility;
	::CoCreateInstance(CLSID_CommandUtility, nullptr, CLSCTX_ALL, IID_ICommandUtility, reinterpret_cast<LPVOID*>(&ptrCommandUtility));
	_bstr_t commandContent = ptrCommandUtility->GetControlCommand(static_cast<_bstr_t>(strDecoderID), static_cast<_bstr_t>(strDecoderID), static_cast<_bstr_t>(strMsgID), pConfigSection->GetTitle(),
		_T(""), strPropertyValue, _T(""), _T(""), _T(""), _T(""), _T(""));

	// 发送控制命令
	strCommandContent = static_cast<char*>(commandContent);
	CLog::Log(SDKCOM, LL_DEBUG, "HUS Command:%s", strCommandContent);
	m_ECClient.PostCommand(strCommandContent, strECGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "设备PTZ控制 设备DeviceGUID:%s ", strDecoderID);
}

void DeviceController::PostDecoderCommand(GUID &guidDecoder, GUID &guidEncoder, GUID &guidEC, SectionID eSectionID, const char *pszChannelNumber)
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pAdaptorFactory = this->m_pSDKCom->m_VmsSiteProxy.m_pAdaptorFactory;
	auto & m_pSynClient = this->m_pSDKCom->m_VmsSiteProxy.m_pSynClient;
	auto & m_ECClient = this->m_pSDKCom->m_ECClient;
	CString strCommandContent;
	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID, FALSE);

	CString strDecoderID;
	Utils::GUIDToCString(guidDecoder, strDecoderID, FALSE);

	CString strEncoderID;
	Utils::GUIDToCString(guidEncoder, strEncoderID, FALSE);

	int nConfigItemCount = 6;
	int nItemID[9];

	// 已知被控设备的GUID，拿到其IConfig接口
	GUID guidDeviceType = m_ptrSynAdapter->GetTypeID(guidDecoder);
	IConfigPtr pIConfig = m_pAdaptorFactory->GetConfig(guidDeviceType);

	// 获取SectionID对应的ConfigSection接口
	_ConfigSectionPtr pConfigSection = pIConfig->getConfigSection(eSectionID);
	//SAFEARRAY *pSectionArray = pConfigSection->GetItemArray();

	// 设置ConfigSection中相应ConfigItem的参数
	// 设置解码器编号,文本
	_ConfigItemPtr pConfigItem = pConfigSection->getConfigItem(DII_DECODER_ID);
	pConfigItem->PutText(static_cast<_bstr_t>(pszChannelNumber));
	nItemID[0] = DII_DECODER_ID;

	// 设置编码器编号,文本
	pConfigItem = pConfigSection->getConfigItem(DII_ENCODER_ID);
	pConfigItem->PutText(static_cast<_bstr_t>(strEncoderID));
	nItemID[1] = DII_ENCODER_ID;

	// 设置模式,{选择窗口\/0}{实时PTZ\/1}{实时数字缩放\/2}{回放\/3}{回放数字缩放\/4}
	pConfigItem = pConfigSection->getConfigItem(DII_OPERATE_MODE);
	pConfigItem->Putvalue(0);
	nItemID[2] = DII_OPERATE_MODE;

	// 设置错误码,数字（最小值0）
	pConfigItem = pConfigSection->getConfigItem(DII_ERROR_CODE);
	pConfigItem->Putvalue(0);
	nItemID[3] = DII_ERROR_CODE;

	// 设置错误消息
	pConfigItem = pConfigSection->getConfigItem(DII_ERROR_MSG);
	pConfigItem->PutText(_T(""));
	nItemID[4] = DII_ERROR_MSG;

	// 设置用户名,文本
	pConfigItem = pConfigSection->getConfigItem(DII_USER_NAME);
	pConfigItem->PutText(_T(""));
	nItemID[5] = DII_USER_NAME;

	if (eSectionID == SI_DECODER_PLAY)
	{
		// 设备树信息,文本
		CString strDeviceTree;
		if (FALSE == CreateDeviceTreeForDecoder(strDeviceTree, guidEncoder))
			return;

		pConfigItem = pConfigSection->getConfigItem(DII_DEVICE_TREE);
		pConfigItem->PutText(static_cast<_bstr_t>(strDeviceTree));
		nItemID[6] = DII_DEVICE_TREE;

		// 播放模式,实时\/0}{回放\/1}
		pConfigItem = pConfigSection->getConfigItem(DII_PLAY_MODE);
		pConfigItem->Putvalue(0);
		nItemID[7] = DII_PLAY_MODE;

		// HUS站点地址,文本
		pConfigItem = pConfigSection->getConfigItem(DII_HUSSIE_ADDRESS);
		pConfigItem->PutText(m_pSynClient->GetSiteIP());
		nItemID[8] = DII_HUSSIE_ADDRESS;

		nConfigItemCount = 9;
	}

	// 序列化设置好参数的ConfigItem数组
	//int nConfigItemCount = pSectionArray->cbElements;
	CComSafeArray<LPDISPATCH> cItems(nConfigItemCount, 0);

	for (auto i = 0; i < nConfigItemCount; i++)
	{
		pConfigItem = pConfigSection->getConfigItem(nItemID[i]);
		cItems.SetAt(i, pConfigItem);
		pConfigItem->AddRef();
	}

	auto strPropertyValue = pConfigItem->SerializeArray(static_cast<LPSAFEARRAY>(cItems), eSectionID, pConfigSection->GetTitle(), static_cast<_bstr_t>(strDecoderID));

	CString strMsgID;
	GUID guidMsg;
	CoCreateGuid(&guidMsg);
	Utils::GUIDToCString(guidMsg, strMsgID, FALSE);

	// 调用ICommandUtility接口，构造commandContent
	ICommandUtility *ptrCommandUtility;
	CoCreateInstance(CLSID_CommandUtility, nullptr, CLSCTX_ALL, IID_ICommandUtility, reinterpret_cast<LPVOID*>(&ptrCommandUtility));
	auto commandContent = ptrCommandUtility->GetControlCommand(
		static_cast<_bstr_t>(strDecoderID),
		static_cast<_bstr_t>(strDecoderID),
		static_cast<_bstr_t>(strMsgID),
		pConfigSection->GetTitle(),
		_T(""), strPropertyValue, _T(""), _T(""), _T(""), _T(""), _T(""));

	strCommandContent = static_cast<char*>(commandContent);
	// 发送控制命令
	m_ECClient.PostCommand(strCommandContent, strECGUID);
	ptrCommandUtility->Release();
}

BOOL DeviceController::CreateDeviceTreeForDecoder(CString &strDeviceTree, const GUID &guidDevice) const
{
	auto & m_ptrSynAdapter = this->m_pSDKCom->m_VmsSiteProxy.m_pSiteImageAdapter;
	auto & m_pSynClient = this->m_pSDKCom->m_VmsSiteProxy.m_pSynClient;
	auto & m_DeviceInfoMgr = this->m_pSDKCom->m_DeviceInfoMgr;

	if (NULL == m_ptrSynAdapter || NULL == m_pSynClient)
		return FALSE;

	strDeviceTree = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
		"<VideoSwitch>\r\n";

	const char szFormat[] = "<Device ID=\'%s\' Title=\'%s\' SiteID=\'%s\' IsVirtual='false' "
		"TypeID=\'%s\' TypeTitle=\'%s\' ParentID=\'%s\'>\r\n";
	const char szSettings[] = "<Settings Title=\'%s\' TypeID=\'%s\' Code=\'%s\' Address=\'%s\' "
		"Memo=\'%s\' AlternativeView=\'%s\' ItemType=\'%s\' TypeTitle=\'%s\' "
		"Hiberachy=\'%s\' IsVirtual='False'>\r\n</Settings>\r\n";

	GUID guidCurLayer = guidDevice;
	for (int nLayer = 0; nLayer < 4; nLayer++)
	{
		_ECElementPtr ptrECElement = m_ptrSynAdapter->GetElement(guidCurLayer);
		if (NULL == ptrECElement)
		{
			break;
		}

		CString strDeviceGUID;
		Utils::GUIDToCString(guidCurLayer, strDeviceGUID, FALSE);

		CString strSiteGUID;
		GUID	pSiteID = m_pSynClient->GetSiteID();
		Utils::GUIDToCString(pSiteID, strSiteGUID, FALSE);

		// 组合DeviceTag参数
		auto charTitle = _com_util::ConvertBSTRToString(ptrECElement->GetName());
		CString strTitle = charTitle;
		SAFE_DELETE_ARRAY(charTitle);

		CString strTypeID;
		GUID	guidTypeID = ptrECElement->GetTypeID();
		Utils::GUIDToCString(guidTypeID, strTypeID, FALSE);

		auto charTypeTitle = _com_util::ConvertBSTRToString(m_ptrSynAdapter->GetTypeMark(guidCurLayer));
		CString strTypeTitle = charTypeTitle;
		SAFE_DELETE_ARRAY(charTypeTitle);
		CString strParentID;
		GUID	guidParentID = ptrECElement->GetParentID();
		Utils::GUIDToCString(guidParentID, strParentID, FALSE);
		if (strParentID.IsEmpty() && nLayer != 3)
		{
			Utils::GUIDToCString(guidDevice, strDeviceGUID, FALSE);
			CLog::Log(SDKCOM, LL_NORMAL, "解码器播放失败 无效的Streamer GUID:%s", strDeviceGUID);
			return FALSE;
		}

		CString strDeviceTag;

		// 格式化Device标签
		strDeviceTag.Format(szFormat, strDeviceGUID, strTitle, strSiteGUID, strTypeID, strTypeTitle, strParentID);
		strDeviceTree += strDeviceTag;
		_ElementSettingsPtr ptrElementSetting = ptrECElement->GetSettings();
		if (NULL == ptrElementSetting)
			return FALSE;

		IEnumVARIANTPtr ptrEnumVARIANT = ptrElementSetting->Keys();
		ptrEnumVARIANT->Reset(); //Start from the beginning

		_variant_t vVariant[MAX_PATH];
		ULONG numGot = 0;
		ptrEnumVARIANT->Next(MAX_PATH, vVariant, &numGot);

		CMap<CString, LPCSTR, int, int> oSettingsNameMap;
		for (UINT i = 0; i < numGot; i++)
		{
			CString strValue = static_cast<LPCSTR>(static_cast<bstr_t>(vVariant[i].bstrVal));
			oSettingsNameMap.SetAt(strValue, i);
		}
		oSettingsNameMap.RemoveKey("Title");
		oSettingsNameMap.RemoveKey("TypeID");
		oSettingsNameMap.RemoveKey("Code");
		oSettingsNameMap.RemoveKey("Address");
		oSettingsNameMap.RemoveKey("Memo");
		oSettingsNameMap.RemoveKey("AlternativeView");
		oSettingsNameMap.RemoveKey("ItemType");
		oSettingsNameMap.RemoveKey("TypeTitle");
		oSettingsNameMap.RemoveKey("Hiberachy");
		oSettingsNameMap.RemoveKey("IsVirtual");

		CString strCode;
		CString strAddress;
		CString strMemo;
		CString strAlternativeView;
		CString strItemType;
		CString strHiberachy;
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"Code", strCode);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"Address", strAddress);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"Memo", strMemo);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"AlternativeView", strAlternativeView);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"ItemType", strItemType);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"TypeTitle", strTypeTitle);
		m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, L"Hiberachy", strHiberachy);

		CString strSettingsTag;
		strSettingsTag.Format(szSettings, strTitle, strTypeID, strCode, strAddress, strMemo, strAlternativeView, strItemType, strTypeTitle, strHiberachy);
		strDeviceTree += strSettingsTag;
		CString strParametersTag = "<Parameters";

		POSITION pos = oSettingsNameMap.GetStartPosition();
		while (pos)
		{
			CString strValue;
			CString strData;
			int nIdx;
			oSettingsNameMap.GetNextAssoc(pos, strValue, nIdx);
			m_DeviceInfoMgr.GetSettingsParam(guidCurLayer, static_cast<LPWSTR>(vVariant[nIdx].bstrVal), strData);
			strParametersTag = strParametersTag + " " + strValue + "=" + "\'" + strData + "\'";
		}

		strParametersTag += ">\r\n</Parameters>\r\n";
		strDeviceTree += strParametersTag + "</Device>\r\n";
		guidCurLayer = guidParentID;
	}

	strDeviceTree += "</VideoSwitch>\r\n";

	return TRUE;
}
//设备配置
int DeviceController::ProcDeviceConfig(CModMessage * pUnifiedMsg) const
{
	DeviceObject pDeviceInfo;

	CString	strSN = pUnifiedMsg->GetQuerySN();
	CString strDeviceID = pUnifiedMsg->GetDeviceID();

	ConfigParam_t *szParam = static_cast<ConfigParam_t*>(pUnifiedMsg->GetCmdParam());
	//read config
	delete szParam;

	if (m_pSDKCom->m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), pDeviceInfo))
	{
		// sdk

		DeviceConfigResponse(pUnifiedMsg, "OK");
	}
	else
	{
		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询失败，未知的设备ID:%s  SN:%s", pUnifiedMsg->GetDeviceID(), strSN);
		pUnifiedMsg->Free();
		DeviceConfigResponse(pUnifiedMsg, "ERROR");
	}

	return 0;
}

int DeviceController::DeviceConfigResponse(CModMessage * pUnifiedMsg, const char *pszResult) const
{
	CBodyBuilder oSIPBody;
	CBigFile *pResponse = nullptr;

	if (NULL == pUnifiedMsg || NULL == pszResult)
		return ERROR_INVALID_PARAMETER;

	// 创建Response文件
	pResponse = m_pMemAllocator->AllocBodyContentBuf();
	CBodyBuilder::CreateDeviceConfigResponse(pResponse->GetBuffer(), pResponse->GetBufferLen(), pUnifiedMsg->GetQuerySN(), pUnifiedMsg->GetDeviceID(), pszResult);
	pUnifiedMsg->SetNotifyData(reinterpret_cast<void *>(pResponse));

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::notify_reponse);
	// 发送到SIPCom模块
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	return 0;
}
//向站点插入录像记录
int DeviceController::CreateWebRecord(DeviceObject & device_obj, CModMessage * pUnifiedMsg)
{
	auto deviceLinkOrViewID = device_obj.linked.strStreamerGUID;
	auto StreamServerGUID = device_obj.linked.guidNVR;

	CString strNVRGUID;
	Utils::GUIDToCString(StreamServerGUID, strNVRGUID, FALSE);
	auto start_time = GetNowTimeStamp();
	//time_t end_time = time(NULL);

	char recordtrrigerid[MAX_PATH] = { 0 };
	//save the map
	auto ret = m_pSDKCom->m_VmsVideoRecordMgr.CreateWebVideoRecord(deviceLinkOrViewID, strNVRGUID, start_time, recordtrrigerid);

	if (pUnifiedMsg)
	{
		pUnifiedMsg->SetRecordID(recordtrrigerid);
	}

	return ret;
}

int DeviceController::UpdateWebRecord(CModMessage * pUnifiedMsg)
{
	//DeviceViewLinkID Or StreamerID, must be assgined
	//std::string deviceLinkOrViewID = "FBF4A67D-2AB4-4CC1-99C5-34CBD19A50EC";
	//auto deviceLinkOrViewID = pUnifiedMsg->GetUpdataGUID();
	//StreamerServerID
	//auto StreamServerID = pUnifiedMsg->GetDeviceID();

	auto end_time = GetNowTimeStamp(30); // 30s
	auto recordtrrigerid = pUnifiedMsg->GetRecordID();

	m_pSDKCom->m_VmsVideoRecordMgr.UpdateVideoRecord(recordtrrigerid, end_time);
	return 0;
}

time_t DeviceController::GetNowTimeStamp(int seconds_to_add)
{
	namespace chrono = std::chrono;
	auto time_point_now = chrono::system_clock::now();
	auto time_point_with_add = time_point_now + chrono::seconds(seconds_to_add);
	return chrono::system_clock::to_time_t(time_point_with_add);
}