#include "StdAfx.h"
#include "SDKDevInfo.h"
#include "ServerConsole.h"
#include "DataManager/DataStore.h"
#include "SDKCom.h"

//初始化
DevicesInfoMgr * DevicesInfoMgr::m_pCSDKDevInfo = nullptr;

DevicesInfoMgr::DevicesInfoMgr(void)
{
	m_pCSDKDevInfo = this;
	auto m_Gatewayid = appConf.m_Current.str_ID;
	//初始化
	m_GBID_Creater.initial(m_Gatewayid.GetString(), appConf.strDevInfoConfPath.GetString(), &devInfoConf.m_GBIDCreatorInfo);

	m_DeviceTypeDef = std::move(devInfoConf.m_DeviceTypeDef);
}


void DevicesInfoMgr::Cleanup()
{
	//保存生成的各类id的信息
	m_GBID_Creater.SerializeToFile();

	HUSDeviceConnect_T *pChannelInfo = nullptr;

	auto pos = m_oDecoderMap.GetStartPos();
	while (pos)
	{
		CString strID;
		m_oDecoderMap.GetNext(pos, strID, pChannelInfo);
		if (pChannelInfo)
		{
			delete pChannelInfo;
			pChannelInfo = nullptr;
		}
	}
}

BOOL DevicesInfoMgr::ChannelLookup(const char *pszKey, DeviceObject & husdeviceInfo)
{
	DeviceObject husDevceInfo;
	if (CDataStore::LookupDevice(pszKey, husDevceInfo, TRUE))
	{
		husdeviceInfo = husDevceInfo;
		return TRUE;
	}
	return FALSE;
	//return m_oChannelGBIDMap.Lookup(pszKey, pInfo, TRUE);
}

void DevicesInfoMgr::ChannelLookupEnd()
{
	CDataStore::ManualUnlockDeviceInfoMap();
	//m_oChannelGBIDMap.ManualUnlock();
}

BOOL DevicesInfoMgr::DecoderLookup(const char *pszKey, HUSDeviceConnect_T *pInfo)
{
	return m_oDecoderMap.Lookup(pszKey, pInfo, TRUE);
}

void DevicesInfoMgr::DecoderLookupEnd()
{
	m_oDecoderMap.ManualUnlock();
}

BOOL DevicesInfoMgr::DecoderPairRemove(const char *pszKey, DecoderPairInfo_t &pInfo)
{
	return m_oDecoderPairMap.RemoveKey(pszKey, pInfo);
}

void DevicesInfoMgr::RemoveAlarming(const char *pszKey)
{
	CDataStore::RemoveAlarming(pszKey);
}

// 取得设备对应的EC和NVR信息
void DevicesInfoMgr::SetOnlineStatusByGUID(const char *pszGUID, int isOffline)
{
	DeviceObject tInfo;
	CString strGUID = pszGUID;
	CString strID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	if (!CDataStore::LookupDeviceID(strGUID, strID)) return;
	DeviceObject deviceInfo;
	if (CDataStore::LookupDeviceByGUID(strGUID, deviceInfo, TRUE))
	{
		if (isOffline)
		{
			deviceInfo.tDevStateInfoAll.SetOnlineStatus(_T("OFFLINE"));
			INT evttype = event_notify_t::e_update_t::ut_off;
			CDataStore::AddDeviceObjectByGUID(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(strGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "设备下线 GUID:%s", strID);
			m_pSDKCom->NoticeDevInfo(strGUID, event_notify_t::e_update_t::ut_off, strID, mod_op_t::ot_devinfo::subscribe_notify);
		}
		else
		{
			deviceInfo.tDevStateInfoAll.SetOnlineStatus(_T("ONLINE"));
			INT evttype = event_notify_t::e_update_t::ut_on;
			CDataStore::AddDeviceObjectByGUID(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(pszGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "设备上线 GUID:%s", strID);
			m_pSDKCom->NoticeDevInfo(strGUID, event_notify_t::e_update_t::ut_on, strID, mod_op_t::ot_devinfo::subscribe_notify);
		}
	}
	else
		CLog::Log(SDKCOM, LL_NORMAL, _T("无GBID的设备状态变更 GUID:%s 当前设备上次状态码（0未离线,1离线）:%d"), strGUID, isOffline);

	CDataStore::ManualUnlockDeviceInfoMap();
}

// 删除设备对象
void DevicesInfoMgr::DeleteDeviceObject(const char *pszGUID)
{
	CString deviceGBID;

	if (CDataStore::LookupDeviceID(pszGUID, deviceGBID, TRUE))
	{
		//移除设备ID映射关系
		CDataStore::RemoveDeviceItemByGUID(pszGUID);
		CDataStore::RemoveDevice(deviceGBID);
	}
	else
	{
		CDataStore::ManualUnlockGUIDMap();
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备对象失败，未知的GUID：%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockGUIDMap();
	//删除HUS设备信息
	DeviceObject deviceToDelete;
	if (CDataStore::LookupDeviceByGUID(pszGUID, deviceToDelete, TRUE))
	{
		CDataStore::RemoveDeviceByGUID(pszGUID);
	}
	else
	{
		CDataStore::ManualUnlockDeviceInfoMap();
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备对象失败，未知的GUID：%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockDeviceInfoMap();

	auto pHUSDevSubMap = deviceToDelete.GetVMSDescription();
	// 删除子设备
	if (deviceToDelete.GetGBType() == GBDevice_T::DEVICE)
		while (0 < pHUSDevSubMap->subDevStrGUIDMap.size())
		{
			auto it = pHUSDevSubMap->subDevStrGUIDMap.begin();
			auto strGUID = it->first;

			DeviceObject subDeviceToDelete;

			if (CDataStore::LookupDeviceByGUID(strGUID.c_str(), subDeviceToDelete, TRUE))
			{
				CString strGUIDtoDelete;
				if (GBDevice_T::CAMERA == subDeviceToDelete.GetGBType())
				{
					if (CDataStore::LookupGUID(subDeviceToDelete.m_strDeviceID, strGUIDtoDelete, TRUE))
					{
						CDataStore::RemoveDevice(deviceToDelete.m_strDeviceID);
						CDataStore::RemoveDeviceItemByGUID(strGUIDtoDelete);
					}
					CDataStore::ManualUnlockDeviceIDMap();
				}
				else if (GBDevice_T::ALARM == deviceToDelete.GetGBType())
				{
					//m_oAlarmGBIDMap.RemoveKey(deviceToDelete.strDeviceID);
					CDataStore::RemoveDeviceAlarmStatus(deviceToDelete.m_strDeviceID);
				}
				else if (GBDevice_T::DECODER == deviceToDelete.GetGBType())
				{
					HUSDeviceConnect_T* p_DeviceLinkInfo = nullptr;
					if (m_oDecoderMap.Lookup(deviceToDelete.m_strDeviceID, p_DeviceLinkInfo, TRUE))
					{
						m_oDecoderMap.RemoveKey(deviceToDelete.m_strDeviceID);
						delete p_DeviceLinkInfo;
						p_DeviceLinkInfo = nullptr;
					}
				}
			}
			CDataStore::ManualUnlockDeviceInfoMap();
		}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "删除设备时忽略通道或流 设备的GUID：%s", pszGUID);
	}
}

// 更新设备信息
void DevicesInfoMgr::UpdateGBDeviceIDByGUID(const char *pszGUID)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s", __FUNCTION__);
	CString strConfigName = appConf.strDevConfDir + _T("\\") + pszGUID;

	// 取得最新的GBID
	CMap<CString, LPCSTR, CString, CString&> strDeviceIDMap;
	char szData[MAX_PATH];
	CString strDeviceType;
	CString strDeviceID;
	GetPrivateProfileString(_T("CATALOG"), _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
	strDeviceID = szData;
	strDeviceIDMap.SetAt(pszGUID, strDeviceID);
	//update Device RelationShip

	// 取得设备类型
	GetPrivateProfileString(_T("INFO"), _T("DeviceType"), _T("DeviceType"), szData, MAX_PATH, strConfigName);
	strDeviceType = szData;

	// 取得子设备ID和GUID
	CString strSection;
	CString strChannelID;
	CString strGUID;
	for (int j = 0; j < 64; j++)
	{
		strSection.Format(_T("CHANNEL_CATALOG%d"), j);
		GetPrivateProfileString(strSection, _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		strChannelID = szData;

		GetPrivateProfileString(strSection, _T("GUID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		// GUID为空，不存在该通道
		if (0 == strcmp(szData, _T("NaN")))
			break;

		strGUID = szData;
		strGUID.MakeLower();
		if (0 == j && 0 == strDeviceType.CompareNoCase(_T("ipc")))
		{
			strDeviceIDMap.SetAt(strGUID, strDeviceID);
			continue;
		}
		strDeviceIDMap.SetAt(strGUID, strChannelID);
	}

	POSITION pos = strDeviceIDMap.GetStartPosition();
	while (pos)
	{
		strDeviceIDMap.GetNextAssoc(pos, strGUID, strDeviceID);
		DeviceObject tNewInfo, tOldInfo;
		if (!CDataStore::LookupDeviceByGUID(strGUID, tOldInfo, TRUE))
		{
			CDataStore::ManualUnlockDeviceInfoMap();
			CLog::Log(SDKCOM, LL_NORMAL, "更新设备信息失败，未知的GUID:%s", strGUID);
			continue;
		}
		tNewInfo = tOldInfo;
		tNewInfo.m_strDeviceID = strDeviceID;
		CDataStore::AddDeviceObjectByGUID(strGUID, tNewInfo, FALSE);
		CDataStore::ManualUnlockDeviceInfoMap();
		Utils::StringCpy_s(szData, MAX_PATH, strDeviceID);
		CString	oldstrDeviceID = tOldInfo.m_strDeviceID; //get old deviceID to delete
		auto	eGBDevType = tOldInfo.GetGBType();

		// GBID没有更新，不用替换
		if (0 == oldstrDeviceID.Compare(szData))
		{
			continue;
		}

		// 没有有效的GBID
		if (ID_LEN != oldstrDeviceID.GetLength())
		{
			// 用GUID做key查询
			oldstrDeviceID = strGUID;
		}

		// 替换原有信息
		HUSDeviceConnect_T *deviceLinkInfo = nullptr;
		if (GBDevice_T::DEVICE == eGBDevType || GBDevice_T::CAMERA == eGBDevType)
		{
			CString husDeviceGUID;
			if (CDataStore::LookupGUID(oldstrDeviceID, husDeviceGUID, TRUE)) //更新映射关系
			{
				CDataStore::RemoveDevice(oldstrDeviceID, FALSE);
				CDataStore::AddDeviceID(strDeviceID, husDeviceGUID, FALSE);
			}
			CDataStore::ManualUnlockGUIDMap();
		}
		else if (GBDevice_T::ALARM == eGBDevType)
		{
			DeviceAlarmInfo tServiceInfo;
			if (CDataStore::LookupDeviceAlarmStatus(oldstrDeviceID, tServiceInfo, TRUE))
			{
				CDataStore::RemoveDeviceAlarmStatus(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					CDataStore::SetDeviceAlarmStatus(strGUID, tServiceInfo, FALSE);
				else
					CDataStore::SetDeviceAlarmStatus(szData, tServiceInfo, FALSE);
			}
			//m_oAlarmGBIDMap.ManualUnlock();
			//CDataStore::ManualUnlockDeviceIDMap();
			CDataStore::ManualUnlockDeviceInfoMap();
		}
		else if (GBDevice_T::DECODER == eGBDevType)
		{
			if (m_oDecoderMap.Lookup(oldstrDeviceID, deviceLinkInfo, TRUE))
			{
				m_oDecoderMap.RemoveKey(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					m_oDecoderMap.SetAt(strGUID, deviceLinkInfo, FALSE);
				else
					m_oDecoderMap.SetAt(szData, deviceLinkInfo, FALSE);
			}
			m_oDecoderMap.ManualUnlock();
		}
	}
}

void DevicesInfoMgr::DecoderPairTimeout()
{
	m_oDecoderPairMap.CheckTimeOut();
}

DeviceChangedMap &DevicesInfoMgr::GetStatusChangedMap()
{
	return m_oStatusChangeMap;
}

//pszKey是strDeviceID
BOOL DevicesInfoMgr::AlarmLookup(const char *pszKey, DeviceAlarmInfo &pInfo)
{
	return CDataStore::LookupDeviceAlarmStatus(pszKey, pInfo, TRUE);
}

void DevicesInfoMgr::AlarmLookupEnd()
{
	CDataStore::ManualUnlockDeviceInfoMap();
}

void DevicesInfoMgr::AlarmSetAt(const char *pszKey, DeviceAlarmInfo &pInfo, BOOL bIsLock)
{
	CDataStore::SetDeviceAlarmStatus(pszKey, pInfo, bIsLock);
}

BOOL DevicesInfoMgr::DecoderIPLookup(const char *pszKey, CString &pszID)
{
	return m_oDecoderIPtoIDMap.Lookup(pszKey, pszID, FALSE);
}

void DevicesInfoMgr::DecoderIPLookupEnd()
{
	m_oDecoderIPtoIDMap.ManualUnlock();
}

void DevicesInfoMgr::DecoderPairPush(const char *pszKey, DecoderPairInfo_t &info)
{
	m_oDecoderPairMap.Push(pszKey, info);
}

BOOL DevicesInfoMgr::DeviceLookup(const char *pszKey, DeviceObject &pInfo)
{
	return CDataStore::LookupDevice(pszKey, pInfo, FALSE);
}

void DevicesInfoMgr::DeviceLookupEnd()
{
	CDataStore::ManualUnlockDeviceInfoMap();
}

// 添加正在报警的设备信息
int DevicesInfoMgr::HandleAlarmInfoFromDevice(const char *pszGUID, const char *pszDescribe, const char *pszLevel  /*same with：strAlarmSeverity*/, const char *pszTime, int nAlarmType, int nAlarmMethord, CString& strAlarmStatus)
{
	if (0 == strlen(pszGUID))
		return -1;
	CLog::Log(SDKCOM, LL_DEBUG, _T("设备告警，设备GUID:%s 告警级别:%s 告警信息:%s"), pszGUID, pszLevel, pszDescribe);
	CString			strDeviceGBID;
	DeviceObject	tInfo;
	CString strGUID = pszGUID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	// 取得设备对应的GBID
	auto bFind = CDataStore::LookupDeviceID(strGUID, strDeviceGBID, FALSE);

	if (!bFind) return -1;

	DeviceAlarmInfo		tAlarmInfo;
	if (CDataStore::LookupDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, TRUE)) //查找设备
	{
		// 判断布撤防状态
		if (DeviceAlarmInfo::DutyStatus::OFFDUTY == tAlarmInfo.eStatus)
		{
			CDataStore::ManualUnlockDeviceInfoMap();
			CLog::Log(SDKCOM, LL_DEBUG, _T("设备已撤防，告警不上报，设备ID：%s"), strDeviceGBID);
			return 0;
		}
		if (DeviceAlarmInfo::DutyStatus::ONDUTY == tAlarmInfo.eStatus)
		{
			tAlarmInfo.eStatus = DeviceAlarmInfo::DutyStatus::ALARM;
			tAlarmInfo.m_StrGUID = pszGUID;
			tAlarmInfo.strDeviceID = strDeviceGBID;
			tAlarmInfo.m_nAlarmMethord = nAlarmMethord;
			tAlarmInfo.m_nAlarmType = nAlarmType;
			tAlarmInfo.m_strAlarmStatus = strAlarmStatus;
			tAlarmInfo.m_Description = pszDescribe;
			tAlarmInfo.m_strPriority = pszLevel;
			tAlarmInfo.m_strTime = pszTime;
			CDataStore::SetDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, FALSE);
			CLog::Log(SDKCOM, LL_DEBUG, _T("设备已布防，告警上报，设备ID：%s"), strDeviceGBID);
			this->m_pSDKCom->NoticeDevInfo(pszGUID, event_notify_t::e_update_t::ut_alarm);
		}
	}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, _T("报警上报失败，未知的GB28181设备ID：%s"), strDeviceGBID);
		CDataStore::ManualUnlockDeviceInfoMap();
		return 0;
	}
	CDataStore::ManualUnlockDeviceInfoMap();
	// 添加到报警队列缓存
	DeviceAlarmInfo alarmInfo(strDeviceGBID, nAlarmMethord, nAlarmType, strAlarmStatus, pszDescribe, pszLevel, pszTime);
	CDataStore::AddAlarmInfo(strDeviceGBID, alarmInfo);
	return 0;
}

void DevicesInfoMgr::IniDeviveProperty(_ECElementPtr ptrECElement)
{
	GUID		guidEC;
	GUID		guidDevice;
	CString		strPath;
	strPath = appConf.strDevConfDir + _T("\\");
	// 遍历设备数组
	guidDevice = ptrECElement->GetID();  // 设备的GUID
	_bstr_t strTypeMark = m_pVmsSiteProxy->GetTypeMark(guidDevice);  // 设备类型，如"DVR", "Channel", "RStreamer"
	_bstr_t strName = ptrECElement->GetName();  // 设备名称
	// 取得设备类型GUID
	auto guidType = m_pVmsSiteProxy->GetTypeID(guidDevice);
	CString strGUID;
	Utils::GUIDToCString(guidType, strGUID, FALSE);
	std::string  strType="";

	auto value = m_DeviceTypeDef[strGUID.MakeUpper().GetString()];

	if (strType=="")
	{
		return;
	}
	//auto  strType = m_DeviceTypeDef[strGUID.GetString()];
	assert(!strType.empty());
	// 取得对应的GB设备类型的
	strTypeMark = strType.c_str();
	// 获取当前设备所对应的EC服务的GUID
	guidEC = m_pVmsSiteProxy->GetECServerID(guidDevice);

	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID, FALSE);
	m_oECToDevMap.SetAt(strDeviceGUID, guidEC);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID);

	// 取得设备的IP
	CString pszDevIP;
	GetSettingsParam(GUID_NULL, L"IP", pszDevIP, ptrECElement);
	// 取得设备的Port
	CString pszDevPort;
	GetSettingsParam(GUID_NULL, L"Port", pszDevPort, ptrECElement);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s pszDevIp = %s pszDevPort = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID, pszDevIP, pszDevPort);
	WriteDeviceInfoConfig(strPath.GetString(), strTypeMark, strName, guidDevice, guidEC, pszDevIP, pszDevPort);
}

BOOL DevicesInfoMgr::GetSettingsParam(GUID guidDevice, const WCHAR *pwszParamName, CString &strParmaValue, _ECElementPtr  p_EleSetting)
{
	return m_pVmsSiteProxy->GetDeviceSettingsInfo(guidDevice, pwszParamName, strParmaValue, p_EleSetting);
}

void DevicesInfoMgr::AddToDataStore(DeviceObject & objec2add, DeviceObject* parentObject)
{
	if (parentObject != nullptr)
	{
		auto pHUSDevSubMap = parentObject->GetVMSDescription();
		pHUSDevSubMap->subDevStrGUIDMap.emplace(parentObject->strDeviceGUID, parentObject->guidDevice);
	}

	CDataStore::AddDeviceObjectByGUID(objec2add.strDeviceGUID, objec2add);
	CDataStore::AddGUID(objec2add.strDeviceGUID, objec2add.m_strDeviceID);
}
void DevicesInfoMgr::FillSubDevInfoByparent(DeviceObject & sub_device, const DeviceObject& parent_device)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(sub_device.guidDevice, strDeviceGUID);
	sub_device.strDeviceGUID = strDeviceGUID;
	sub_device.strIP = parent_device.strIP;
	sub_device.strPort = parent_device.strPort;
	sub_device.linked.strDeviceName = sub_device.strName; //标明链接信息所属的Onwer
	sub_device.linked.guidDevice = sub_device.guidDevice;
	sub_device.linked.guidParent = parent_device.guidDevice;
	sub_device.linked.guidEC = parent_device.linked.guidEC;
	sub_device.linked.strDeviceName = sub_device.strName; //标明链接的Onwer
	sub_device.linked.guidDevice = sub_device.guidDevice;
}

void DevicesInfoMgr::WriteDeviceInfoConfig(const char *pszConfigPath, _bstr_t &bstrType, _bstr_t &bstrName, const GUID &guidDevice, const GUID &guidEC, const char *pszDevIP, const char *pszDevPort)
{
	// 取得设备类型
	CString strType = static_cast<char *>(bstrType);
	// 只处理顶层设备（DVR，decoder,device(ipc,encoder)
	auto husDevType = HUSDevice_T::NONE;
	auto gbdeviceType = GBDevice_T::NONE;
	std::tie(gbdeviceType, husDevType) = ParserDevTypeByStrMark(strType, TRUE);
	if (gbdeviceType == GBDevice_T::NONE)

	{
		CLog::Log(SDKCOM, LL_NORMAL, "忽略子设备 Type:%s ,继续处理下一个设备", strType);
		return;
	}
	CString strDeviceName = static_cast<char *>(bstrName);
	CString strDeviceIP = pszDevIP;
	CString strDevicePort = pszDevPort;
	//转换DVR设备的GUID为字符串
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "读入设备信息 ,设备GUID:%s Type:%s", strDeviceGUID, strType);

	// 生成保存设备和报警通道的数据结构
	DeviceObject newDvrObject;
	newDvrObject.SetGBType(gbdeviceType); //对应的GB设备类型 //如果是6100 则此处是OT_NVR
	newDvrObject.guidDevice = guidDevice;
	newDvrObject.strDeviceGUID = strDeviceGUID;
	newDvrObject.strName = strDeviceName;
	newDvrObject.strDeviceTyeMark = strType; //字符串标记，同时用于GB和HUS的设备类型
	newDvrObject.strIP = strDeviceIP;
	newDvrObject.strPort = strDevicePort;
	newDvrObject.linked.guidDevice = guidDevice;
	newDvrObject.linked.strDeviceName = strDeviceName;
	newDvrObject.linked.guidParent = GUID_NULL;
	newDvrObject.linked.guidEC = guidEC;
	CString strConfigName = pszConfigPath + strDeviceGUID;
	WriteDvrDeviceConfig(strConfigName, newDvrObject);  //当是IPS-NVR的时候也会写入配置
	//dvr设备最后添加

	auto arrayChannel = m_pVmsSiteProxy->GetSubElementsArray(guidDevice);
	if (arrayChannel.size() <= 0)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice： %s configPath = %s  Access SubChannel Info failure, but dvr Object init OK !\r\n", __FUNCTION__, strConfigName);
		return	AddToDataStore(newDvrObject);
	}
	CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice: %s configPath = %s ,Access SubChannel Info OK, SubChannel Count = %d\r\n", __FUNCTION__, pszConfigPath, arrayChannel);
	auto nElementIndex = 0;
	// 遍历子设备
	for (auto j = 0UL; j < arrayChannel.size(); j++)
	{
		auto			guid_Channel = arrayChannel[j]->GetID();
		CString			strChannelMark = m_pVmsSiteProxy->GetTypeMark(guid_Channel);
		auto            bstr_ChannelName = arrayChannel[j]->GetName();
		auto            strChannelName = CString(static_cast<LPCTSTR>(bstr_ChannelName));
		CString			ChannelOrderID;
		//判断是不是编码器的通道，是的话做相应处理
		if (gbdeviceType == GBDevice_T::ENCODER)
		{
			GetSettingsParam(guid_Channel, L"Param", ChannelOrderID); 			//获取EV4通道序号
			auto channelorder = _ttoi(ChannelOrderID);
			if (ChannelOrderID.GetLength() != 1)
			{
				GetSettingsParam(guid_Channel, L"ChannelID", ChannelOrderID);	//获取E8X,E4X通道序号
				nElementIndex = channelorder;
			}
			else
				nElementIndex = channelorder - 1;
		}
		DeviceObject subChannelObject;
		subChannelObject.guidDevice = guid_Channel;
		subChannelObject.strName = strChannelName;
		subChannelObject.strDeviceTyeMark = strChannelMark;
		FillSubDevInfoByparent(subChannelObject, newDvrObject);
		subChannelObject.linked.strDeviceName = strChannelName;

		auto subdevType = ParserDevTypeByStrMark(strChannelMark);
		auto  _gb_sub_deviceType = std::get<0>(subdevType);
		subChannelObject.SetGBType(_gb_sub_deviceType);
		auto  husSubDevType = std::get<1>(subdevType);

		if (husSubDevType == HUSDevice_T::CHANNEL)
		{
			if (newDvrObject.GetGBType() == GBDevice_T::NVR)
			{
				//这个是6100 
				//要处理全部同步通道
				//1、处理NVR的关联关系
				//2、处理EC的关联关系
				//3、重新处理对象ID
				subChannelObject.eHUSDevType = HUSDevice_T::NVR_DVR;
				subChannelObject.SetGBType(GBDevice_T::CAMERA);
				WriteNvrVideoChannelConfig(nElementIndex++, strConfigName, subChannelObject, newDvrObject);
				subChannelObject.linked.guidParent = subChannelObject.guidDevice;//ptz时应该传GUID过去
				subChannelObject.linked.strDeviceName = subChannelObject.strName;
				subChannelObject.linked.strStreamerGUID = subChannelObject.strDeviceGUID;
				subChannelObject.linked.guidParent = subChannelObject.guidDevice;   //当前对象channel层对象的parentID.				
				subChannelObject.linked.guidNVR = newDvrObject.guidDevice;  //6100的DVR层的guid就是当前6100NVR的ID.
				subChannelObject.linked.strNVRIP = newDvrObject.strIP;     //6100的DVR层的guid就是当前6100NVR的IP.
				subChannelObject.linked.guidEC = subChannelObject.linked.guidEC;
				subChannelObject.linked.guidStreamer = subChannelObject.guidDevice;
				subChannelObject.linked.guidVirSteamer = subChannelObject.linked.guidVirSteamer;//ptz控制对应的stream层的GUID

				AddToDataStore(subChannelObject, &newDvrObject);
				continue;
			}
			// 将Channel添加为虚拟Alarm。
			//(在HUS设备概念中，IPC 的Channel同时也是报警通道和PTZ通道,另外Streamer层对象也是有报警的)。。。
			//channel同时也是一个虚拟的Alarm，所以需要将Channel 作为一个对象来添加到配置文件中。
			WriteAlarmChannelConfig(nElementIndex + arrayChannel.size(), strConfigName, subChannelObject);
			DeviceObject	subStreamerObject;
			subStreamerObject.SetGBType(GBDevice_T::CAMERA);

			FillSubDevInfoByparent(subStreamerObject, subChannelObject);
			if (newDvrObject.GetGBType() == GBDevice_T::IPC)
			{
				subStreamerObject.SetGBType(GBDevice_T::CAMERA);
				subStreamerObject.eHUSDevType = HUSDevice_T::STREAMER;
				//对于IPC 类型的GB设备需要重新修正对象字的逻辑关系
				subStreamerObject.SetDeviceID(newDvrObject.GetDeviceID());
				subStreamerObject.SetGBType(GBDevice_T::IPC);
				newDvrObject.linked.guidNVR = subStreamerObject.linked.guidNVR;
				newDvrObject.linked.guidVirSteamer = subStreamerObject.linked.guidVirSteamer;
				newDvrObject.linked.strNVRIP = subStreamerObject.linked.strNVRIP;
				newDvrObject.linked.strStreamerGUID = subStreamerObject.strDeviceGUID;
				newDvrObject.linked.guidParent = subChannelObject.guidDevice;
				newDvrObject.linked.guidStreamer = subStreamerObject.guidDevice;
				newDvrObject.linked.guidVirSteamer = subStreamerObject.linked.guidVirSteamer;//ptz控制对应的stream层的GUID
			}
			auto  result = WriteVideoChannelConfig(nElementIndex++, strConfigName, subStreamerObject);

			if (result == S_FALSE && j < arrayChannel.size()) continue;
			if (newDvrObject.GetGBType() == GBDevice_T::IPC)
			{
				newDvrObject.linked.guidEC = subStreamerObject.linked.guidEC;
				newDvrObject.linked.guidNVR = subStreamerObject.linked.guidNVR;
				newDvrObject.linked.strNVRIP = subStreamerObject.linked.strNVRIP;
				newDvrObject.linked.strStreamerGUID = subStreamerObject.strDeviceGUID;
				newDvrObject.linked.guidParent = subChannelObject.guidDevice;
				newDvrObject.linked.guidStreamer = subStreamerObject.guidDevice;
				newDvrObject.linked.guidVirSteamer = subStreamerObject.linked.guidVirSteamer;//ptz控制对应的stream层的GUID
				break; //如果是IPC则直接跳出for 循环，保存DVR对象
			}
			else if (newDvrObject.GetGBType() == GBDevice_T::NVR) //如果是IPS6100-NVR  这里处理
			{
				//1、处理NVR的关联关系
				//2、处理EC的关联关系
				//3、重新处理对象ID
				subChannelObject.linked.strDeviceName = subStreamerObject.strName;
				subChannelObject.linked.strStreamerGUID = subStreamerObject.strDeviceGUID;
				subChannelObject.linked.guidParent = subChannelObject.guidDevice;   //当前对象channel层对象的parentID.
				subChannelObject.linked.guidNVR = newDvrObject.guidDevice;  //6100的DVR层的guid就是当前6100NVR的ID.
				subChannelObject.linked.strNVRIP = newDvrObject.strIP;     //6100的DVR层的guid就是当前6100NVR的IP.
				subChannelObject.linked.guidEC = subStreamerObject.linked.guidEC;
				newDvrObject.linked.guidStreamer = subStreamerObject.guidDevice;
				subChannelObject.linked.guidVirSteamer = subStreamerObject.linked.guidVirSteamer;//ptz控制对应的stream层的GUID
				AddToDataStore(subStreamerObject, &subChannelObject);
			}
			else
			{
				//如果编码器/解码器
				subStreamerObject.linked.strStreamerGUID = subStreamerObject.strDeviceGUID;
				subStreamerObject.linked.strDeviceName = subChannelObject.strName;
				subStreamerObject.linked.guidParent = subStreamerObject.guidDevice;
				subStreamerObject.linked.guidStreamer = subStreamerObject.guidDevice;
				//AddToDataStore(subStreamerObject, &newdeviceInfo);
				subChannelObject = subStreamerObject;
				//subStreamerObject.SetDeviceID(subStreamerObject.strDeviceID);
			}

		}
		else if (husSubDevType == HUSDevice_T::ALARM_CHANNEL)
		{
			WriteAlarmChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}
		else if (husSubDevType == HUSDevice_T::DECODER_CHANNEL)
		{
			WriteDecorderChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}

		//如果是编码器/解码器/DVR/则需要保存当前通道对象。
		if (newDvrObject.GetGBType() != GBDevice_T::IPC)
		{
			//IPC 在GB设备类中只保存一个对象,将DVR和Streamer合并成一个IPC对象，不保存channel 
			//这里保存通道的camare对象
			//如果不是IPC channel 与camera合并保存成一个camera对象有报警
			//不再为channle单独抽象一个alarm对象。
			AddToDataStore(subChannelObject, &newDvrObject);
		}
	}
	//保存HUS的DVR层对象,不保存NVR.
	if (newDvrObject.GetGBType() != GBDevice_T::NVR)
	{
		AddToDataStore(newDvrObject);
	}
}

void DevicesInfoMgr::WriteDvrDeviceConfig(const CString & strConfigName, DeviceObject & newDVRDeviceObject)
{
	TCHAR szData[MAX_PATH] = { 0 };

	// 写入设备类型和设备名
	WritePrivateProfileString("INFO", "DeviceType", newDVRDeviceObject.strDeviceTyeMark, strConfigName);
	WritePrivateProfileString("CATALOG", "Name", newDVRDeviceObject.strName, strConfigName);

	//获取设备ID,第一次是获取不到，因为deviceManager未配置,要自动生成GBID
	GetPrivateProfileString("CATALOG", "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString CurrentDeviceID(szData);  //生成DVR的GBID.
	if (CurrentDeviceID.GetLength() != ID_LEN)
	{
		CurrentDeviceID = m_GBID_Creater.Create_DevIDByType(newDVRDeviceObject.GetGBType()).c_str();
		Utils::StringCpy_s(szData, MAX_PATH, CurrentDeviceID);
		WritePrivateProfileString("CATALOG", "DeviceID", szData, strConfigName);
	}
	CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s szData = %s strlen(szData) = %d ID_LEN = %d\r\n", __FUNCTION__, strConfigName, CurrentDeviceID, CurrentDeviceID.GetLength(), ID_LEN);

	newDVRDeviceObject.m_strDeviceID = CurrentDeviceID;
	newDVRDeviceObject.tDevStateInfoAll.SetDeviceID(CurrentDeviceID);
}

HRESULT DevicesInfoMgr::WriteVideoChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subVideoObject)
{

	auto arrayStreamer = m_pVmsSiteProxy->GetSubElementsArray(subVideoObject.linked.guidParent);

	//TODO... 增加主码流的判断逻辑
	if (arrayStreamer.size() <= 0)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s SafeArrayAccessData failure no channel\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	GUID			guid_Streamer;
	// 获取主码流的GUID，尝试判断属于GB的NVR
	guid_Streamer = arrayStreamer[0]->GetID();
	CString			strStreamerMark = m_pVmsSiteProxy->GetTypeMark(guid_Streamer);
	auto bstr_streamername = arrayStreamer[0]->GetName();
	auto strStreamerName = CString(static_cast<LPCTSTR>(bstr_streamername));
	CString			strStreamerGUID;
	Utils::GUIDToCString(guid_Streamer, strStreamerGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "初始化HUS码流对象,设备GUID:%s Type:%s", strStreamerGUID, strStreamerMark);

	subVideoObject.strName = strStreamerName;
	subVideoObject.strDeviceTyeMark = strStreamerMark;
	subVideoObject.guidDevice = guid_Streamer;
	subVideoObject.strDeviceGUID = strStreamerGUID;

	auto arguidNVR = m_pVmsSiteProxy->GetLinkedFirstSServerID(subVideoObject.guidDevice);

	if (arguidNVR == GUID_NULL)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s Maybe No NVR Linked to streamer: {%s}  \r\n", __FUNCTION__, strConfigName, strStreamerGUID);
		return FALSE;
	}

	GetSettingsParam(arguidNVR, L"IP", subVideoObject.linked.strNVRIP);
	subVideoObject.linked.guidNVR = arguidNVR; //只保存第一个NVR的GUID.
// 设备基本信息写入设备配置文件。

	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder); //ENCODER(E4V/E8X等)，DECODER的子通道较多  DVR
	subVideoObject.tDevinfoInConf.strSection = strSection;

	WritePrivateProfileString(strSection, "GUID", subVideoObject.strDeviceGUID, strConfigName);
	WritePrivateProfileString(strSection, "Model", "Camera", strConfigName); //streamer 的类型为Camera.
	WritePrivateProfileString(strSection, "Name", subVideoObject.strName, strConfigName); //write streamer name
	// 当设备是IPC时，读取CATALOG的ID
	TCHAR szData[MAX_PATH] = { 0 };
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	//处理 Steamer的GBID.
	CString CurrentStreamerID(szData);
	if (CurrentStreamerID.GetLength() != ID_LEN)
	{
		if (subVideoObject.GetGBType() == GBDevice_T::IPC)
			CurrentStreamerID = subVideoObject.m_strDeviceID;
		else
			//CurrentStreamerID = m_GBID_Creater.CreateSubDeviceid(subVideoObject.eGBDevType, subVideoObject.strDeviceID, indexOrder).c_str();
			CurrentStreamerID = m_GBID_Creater.Create_CameraID().c_str();
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s add device szData = %s\r\n", __FUNCTION__, strConfigName, szData);
		// 添加GBID 到HUS的GUID的映射
		WritePrivateProfileString(strSection, "DeviceID", CurrentStreamerID, strConfigName);
	}
	subVideoObject.m_strDeviceID = CurrentStreamerID;

	//处理设备链接到的EC的ID.
	if (GUID_NULL == subVideoObject.linked.guidEC)  //当前的DVR未连接到EC
	{
		subVideoObject.linked.guidEC = m_pVmsSiteProxy->GetECServerID(subVideoObject.linked.guidParent);
		subVideoObject.linked.guidVirChannel = m_pVmsSiteProxy->GetVirtualECTargetGUID(subVideoObject.linked.guidParent);
	}
	subVideoObject.linked.guidVirSteamer = m_pVmsSiteProxy->GetVirtualECTargetGUID(subVideoObject.guidDevice);

	CString strVirStreamer;
	Utils::GUIDToCString(subVideoObject.linked.guidVirSteamer, strVirStreamer, FALSE);
	m_oECToDevMap.SetAt(strVirStreamer, subVideoObject.linked.guidEC);
	CLog::Log(SDKCOM, LL_DEBUG, _T("添加虚视频流到EC映射队列 GUID：%s VirGUID:%s 队列长度：%d"), subVideoObject.strDeviceGUID, strVirStreamer, m_oECToDevMap.GetSize());
	return S_OK;
}

HRESULT DevicesInfoMgr::WriteNvrVideoChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subVideoObject, DeviceObject &DvrObject)
{
	auto arrayStreamer = m_pVmsSiteProxy->GetSubElementsArray(subVideoObject.guidDevice);
	if (arrayStreamer.size() <= 0)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s SafeArrayAccessData failure no channel\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	GUID			guid_Streamer;
	// 获取主码流的GUID，尝试判断属于GB的NVR
	guid_Streamer = arrayStreamer[0]->GetID();
	CString			strStreamerMark = static_cast<LPCTSTR>(m_pVmsSiteProxy->GetTypeMark(guid_Streamer));
	auto bstr_streamername = arrayStreamer[0]->GetName();
	auto strStreamerName = CString(static_cast<LPCTSTR>(bstr_streamername));
	CString			strStreamerGUID;
	Utils::GUIDToCString(guid_Streamer, strStreamerGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "初始化HUS码流对象,设备GUID:%s Type:%s", strStreamerGUID, strStreamerMark);

	subVideoObject.strName = strStreamerName;
	subVideoObject.strDeviceTyeMark = strStreamerMark;
	//subVideoObject.guidDevice = guid_Streamer;
	subVideoObject.strDeviceGUID = strStreamerGUID;
	// 获取当前设备所对应的NVR服务的GUID
	// 这里返回的是GUID数组，因为一个设备可能挂接在多个NVR服务下
	// 对于GB28181 Gateway来说，选择第一个NVR即可
	subVideoObject.linked.strNVRIP = DvrObject.strIP;     //6100的DVR层的guid就是当前6100NVR的IP.
	subVideoObject.linked.guidNVR = DvrObject.guidDevice;;

	// 设备基本信息写入设备配置文件。

	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder); //ENCODER(E4V/E8X等)，DECODER的子通道较多  DVR
	WritePrivateProfileString(strSection, "GUID", subVideoObject.strDeviceGUID, strConfigName);
	WritePrivateProfileString(strSection, "Model", "Camera", strConfigName); //streamer 的类型为Camera.
	WritePrivateProfileString(strSection, "Name", subVideoObject.strName, strConfigName); //write streamer name
																						  // 当设备是IPC时，读取CATALOG的ID
	TCHAR szData[MAX_PATH] = { 0 };
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	//处理 Steamer的GBID.
	CString CurrentStreamerID(szData);
	if (CurrentStreamerID.GetLength() != ID_LEN)
	{
		CurrentStreamerID = m_GBID_Creater.Create_CameraID().c_str();
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s add device szData = %s\r\n", __FUNCTION__, strConfigName, szData);
		// 添加GBID 到HUS的GUID的映射
		WritePrivateProfileString(strSection, "DeviceID", CurrentStreamerID, strConfigName);
	}
	subVideoObject.SetDeviceID(CurrentStreamerID);

	//处理设备链接到的EC的ID.
	if (GUID_NULL == subVideoObject.linked.guidEC)  //当前的DVR未连接到EC
	{
		subVideoObject.linked.guidEC = m_pVmsSiteProxy->GetECServerID(subVideoObject.guidDevice);
	}
	subVideoObject.linked.guidVirSteamer = m_pVmsSiteProxy->GetECServerID(subVideoObject.guidDevice);
	CString strVirStreamer;
	Utils::GUIDToCString(subVideoObject.linked.guidVirSteamer, strVirStreamer, FALSE);
	m_oECToDevMap.SetAt(strVirStreamer, subVideoObject.linked.guidEC);
	CLog::Log(SDKCOM, LL_DEBUG, _T("添加虚视频流到EC映射队列 GUID：%s VirGUID:%s 队列长度：%d"), subVideoObject.strDeviceGUID, strVirStreamer, m_oECToDevMap.GetSize());
	return S_OK;
}

void DevicesInfoMgr::WriteAlarmChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subAlarmObject)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);

	subAlarmObject.tDevinfoInConf.strSection = strSection;

	WritePrivateProfileString(strSection, "GUID", subAlarmObject.strDeviceGUID, strConfigName);
	WritePrivateProfileString(strSection, "Model", "VirAlarm", strConfigName);
	WritePrivateProfileString(strSection, "Name", subAlarmObject.strName, strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString strAlarmDeviceID(szData);
	if (ID_LEN != strAlarmDeviceID.GetLength())
	{
		Utils::StringCpy_s(szData, MAX_PATH, m_GBID_Creater.Create_AlarmDevID().c_str());
		WritePrivateProfileString(strSection, "DeviceID", szData, strConfigName);
		CLog::Log(SDKCOM, LL_NORMAL, "%s AlarmChannel deviceId = %s AlarmChannel Guid = %s\r\n", __FUNCTION__, szData, subAlarmObject.strDeviceGUID);
	}
	if (GUID_NULL == subAlarmObject.linked.guidEC)
	{
		subAlarmObject.linked.guidEC = m_pVmsSiteProxy->GetECServerID(subAlarmObject.guidDevice);
	}

	subAlarmObject.m_strDeviceID = strAlarmDeviceID;
	subAlarmObject.strAlarmID = strAlarmDeviceID;
}

void DevicesInfoMgr::WriteDecorderChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subDisplayObject /*Streamer OF the Channel */)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);
	subDisplayObject.tDevinfoInConf.strSection = strSection;
	WritePrivateProfileString(strSection, "GUID", subDisplayObject.strDeviceGUID, strConfigName);
	WritePrivateProfileString(strSection, "Model", "Display", strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "NaN", szData, MAX_PATH, strConfigName);
	CString strDecoderDisplayDeviceID(szData);
	if (ID_LEN != strDecoderDisplayDeviceID.GetLength())
	{
		strDecoderDisplayDeviceID = m_GBID_Creater.Create_DecoderID().c_str();
		Utils::StringCpy_s(szData, MAX_PATH, strDecoderDisplayDeviceID);
		WritePrivateProfileString(strSection, "DeviceID", strDecoderDisplayDeviceID, strConfigName);
	}
	if (GUID_NULL == subDisplayObject.linked.guidEC)
	{
		subDisplayObject.linked.guidEC = m_pVmsSiteProxy->GetECServerID(subDisplayObject.guidDevice);
	}
	// 取得通道编号
	CString strMonitorNum;
	CString strChannelID;
	GetSettingsParam(subDisplayObject.guidDevice, L"Monitor", strMonitorNum);
	GetSettingsParam(subDisplayObject.guidDevice, L"ChannelID", strChannelID);

	auto  p_LinkedInfo = new HUSDeviceConnect_T();
	p_LinkedInfo->strChannelNum = strMonitorNum + ":" + strChannelID;
	p_LinkedInfo->strDevIP = subDisplayObject.strIP;
	p_LinkedInfo->strDevPort = subDisplayObject.strPort;

	// 用GBID做key
	m_oDecoderMap.SetAt(strDecoderDisplayDeviceID, p_LinkedInfo);

	CString strAddress = subDisplayObject.strIP + ":" + subDisplayObject.strPort;
	subDisplayObject.m_strDeviceID = strDecoderDisplayDeviceID;
	m_oDecoderIPtoIDMap.SetAt(strAddress, subDisplayObject.m_strDeviceID);
}

bool DevicesInfoMgr::IsMainStreamer(GUID guidDevice)
{
	//superhd 和 profileS 
	CString streamerType;
	std::wstring lpStreamerParam = L"MediaProfile"; //	profile /stream /
	GetSettingsParam(guidDevice, lpStreamerParam.c_str(), streamerType);
	if (streamerType != "")
	{
		if (streamerType.Find("1") >= 0 || streamerType.Find("Primary")
			|| streamerType.Find("main"))
		{
			return true;
		}
	}

	//编解码器
	streamerType = "";
	lpStreamerParam = L"Stream";
	GetSettingsParam(guidDevice, lpStreamerParam.c_str(), streamerType);
	if (streamerType != "")
	{
		if (streamerType.Find("0") >= 0 || streamerType.Find("1") >= 0 || streamerType.Find("2") >= 0)
		{
			return true;
		}
	}
	return false;
}

// 添加设备对象
BOOL DevicesInfoMgr::AddDeviceObject(const GUID &guidDevcie)
{
	auto ptrECElement = m_pVmsSiteProxy->GetECElement(guidDevcie);
	if (ptrECElement)
	{
		IniDeviveProperty(ptrECElement);
		//将自动生成的ID序列写进配置文件
		m_GBID_Creater.SerializeToFile();
		return TRUE;
	}
	return FALSE;
}

ECInfoMap &DevicesInfoMgr::GetECToDevMap()
{
	return m_oECToDevMap;
}

void DevicesInfoMgr::DecoderBind(const GUID &guidDecoder, const GUID &guidChannel)
{
	HUSDeviceConnect_T *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);

	DeviceObject tObjInfo;
	CString deviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, deviceID);
	if (m_oDecoderMap.Lookup(deviceID, pInfo, TRUE))
	{
		pInfo->guidParent = guidChannel;
	}
	m_oDecoderMap.ManualUnlock();
}

void DevicesInfoMgr::GUIDTranslatedIntoGBID(const GUID &guidDevice, CString &strGBID)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CString strDeviceID;
	if ("{" != strDeviceGUID.Left(1))
		strDeviceGUID = "{" + strDeviceGUID + "}";
	CDataStore::LookupDeviceID(strDeviceGUID, strDeviceID);
	strGBID = strDeviceID;
}

// 解除解码器视频流绑定
void DevicesInfoMgr::DecoderUnbind(const GUID &guidDecoder)
{
	HUSDeviceConnect_T *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);
	CString strDeviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, strDeviceID);

	if (m_oDecoderMap.Lookup(strDeviceID, pInfo, TRUE))
	{
		memset(&(pInfo->guidParent), 0, sizeof(pInfo->guidParent));
	}
	m_oDecoderMap.ManualUnlock();
}


const CString DevicesInfoMgr::GetSiteIP()
{
	return m_pVmsSiteProxy->GetSiteIP();
}

const CString DevicesInfoMgr::GetGatewyGUID(BOOL bIsWithBraces)
{
	if (m_guidGateway == GUID_NULL)
	{
		m_guidGateway = m_pVmsSiteProxy->GetGatewayGUID();
	}
	CString strGatewayGUID;
	Utils::GUIDToCString(m_guidGateway, strGatewayGUID, bIsWithBraces);
	return  strGatewayGUID;
}

void DevicesInfoMgr::AddHUSDeviceLinkInfo(const TCHAR* pszGUID, HUSDeviceConnect_T& deviceLinkinfo)
{
	//查不到设备信息则添加
	DeviceObject tmpDeviceInfo;
	if (!CDataStore::LookupDeviceByGUID(pszGUID, tmpDeviceInfo, TRUE))
	{
		tmpDeviceInfo.linked = deviceLinkinfo;
		CDataStore::AddDeviceObjectByGUID(pszGUID, tmpDeviceInfo);
	}
	CDataStore::ManualUnlockDeviceInfoMap();
}
void DevicesInfoMgr::SetContext(CSDKCom* p_SDKCom, VmsSiteProxy * pVmsSiteProxy, CAllocator<CModMessage> * pMemAllocator)
{
	SiteJobWoker::SetContext(p_SDKCom, pVmsSiteProxy, pMemAllocator);
	m_oDecoderPairMap.SetExpiry(10);
}

std::tuple<GBDevice_T, HUSDevice_T>  DevicesInfoMgr::ParserDevTypeByStrMark(CString & strType, BOOL isDvrLayer)
{
	//Channel  Contains: channel/alarm/decoder
	strType.MakeLower();
	GBDevice_T gbDevType = GBDevice_T::NONE;
	HUSDevice_T husDevType = HUSDevice_T::NONE;
	if (isDvrLayer)
	{
		husDevType = HUSDevice_T::DVR;
		if (strType.Find(_T("ipc")) >= 0)
			gbDevType = GBDevice_T::IPC;
		else if (strType.Find(_T("dvr")) >= 0)
			gbDevType = GBDevice_T::DVR;
		else if (strType.Find(_T("encoder")) >= 0)
			gbDevType = GBDevice_T::ENCODER;
		else if (strType.Find(_T("nvr")) >= 0)
			gbDevType = GBDevice_T::NVR;
		return std::make_tuple(gbDevType, husDevType);
	}

	if (strType.Find(_T("channel")) >= 0)
	{
		gbDevType = GBDevice_T::ALARM;
		husDevType = HUSDevice_T::CHANNEL;
	}
	else if (strType.Find(_T("alarm")) >= 0)
	{
		gbDevType = GBDevice_T::ALARM;
		husDevType = HUSDevice_T::ALARM_CHANNEL;
	}
	else if (strType.Find(_T("decoder")) >= 0)
	{
		gbDevType = GBDevice_T::DISPLAY;
		husDevType = HUSDevice_T::DECODER_CHANNEL;
	}
	else if (strType.Find(_T("streamer")) >= 0)
	{
		gbDevType = GBDevice_T::CAMERA;
		husDevType = HUSDevice_T::STREAMER;
	}
	return std::make_tuple(gbDevType, husDevType);
}