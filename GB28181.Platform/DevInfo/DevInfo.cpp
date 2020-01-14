#include "StdAfx.h"
#include "ServerConsole.h"
#include "DevInfo.h"
#include "SIPCom/XMLBodyBuilder.h"
#include "Memory/BigFile.h"
//#include "RTSPParser.h"

// 创建并发送应答文件

CDevInfo::CDevInfo(void)
{
	str_GatewayName = appConf.m_Current.str_Name;
	m_strRemotePltID = appConf.m_UpperList[0].str_ID;
	m_CatalogPush = std::move(devInfoConf.m_CatalogPush);
}

void CDevInfo::Init(void)
{
	// 读取设备配置文件
	if (0 != InitDevComBaseInfo())
		exit(0);

	// 默认初始化虚拟目录列表
	InitVirtualCatalog();

	//ParserCatalogList();
	// 注册Module消息处理函数
	RegisterProc(pfnQueueProc, this, 1);
	//RegisterProc(pfnSearchAlarmRecordProc, this, 1);
}

// 在预先设定的文件夹中读取设备信息的配置文件
int CDevInfo::InitDevComBaseInfo()
{
	//初始化GB设备类型和名称的映射
	m_GBDevTypeNameMap[GBDevice_T::IPC] = "ipc";
	m_GBDevTypeNameMap[GBDevice_T::CAMERA] = "camera";
	m_GBDevTypeNameMap[GBDevice_T::ENCODER] = "encoder";
	m_GBDevTypeNameMap[GBDevice_T::DECODER] = "decorder";
	m_GBDevTypeNameMap[GBDevice_T::ALARM] = "alarm";
	m_GBDevTypeNameMap[GBDevice_T::NVR] = "nvr";
	m_GBDevTypeNameMap[GBDevice_T::DVR] = "dvr";
	m_GBDevTypeNameMap[GBDevice_T::DISPLAY] = "decorder_channel";
	m_GBDevTypeNameMap[GBDevice_T::DEVICE] = "device";

	return 0;
}

void CDevInfo::Cleanup(void)
{

}


// 处理内存队列消息的函数
bool CDevInfo::HandleMsg(CMemPoolUnit * pUnit)
{
	auto pUnifiedMsg = reinterpret_cast<CModMessage *>(pUnit);
	auto  eOperateType = pUnifiedMsg->GetModAction();
	CString strCmdType = pUnifiedMsg->GetQueryType();

	if (mod_op_t::ot_devinfo::query_info == eOperateType.action_devinfo) // SIP
	{
		if (strCmdType == "DeviceInfo")
		{
			// 处理设备信息查询
			HandlePropertyQuery(pUnifiedMsg);
			return true;
		}
		if (strCmdType == "Catalog")
		{
			// 设备目录查询
			HandleCatalogQuery(pUnifiedMsg);
			return true;
		}
		if (strCmdType == "RecordInfo")
		{
			// 录像查询消息，转发给SDK模块处理
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::search_record);
		}
		else if (strCmdType == "Alarm")
		{
			// 告警查询，本模块直接处理
			HandleAlarmQuery(pUnifiedMsg);
		}
		else if (strCmdType == "DeviceStatus")
		{
			// 状态查询消息，转发给SDK模块处理
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::search_status);
		}
		else if (strCmdType == "ConfigDownload")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::search_config);
		}
		else if (strCmdType == "DeviceConfig")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::device_config);
		}
		else if (strCmdType == "PersetQuery")
		{
			HandlePersetSearch(pUnifiedMsg);
			return true;
			//pUnifiedMsg->SetModAction(ot_sdk_search_perset);
		}
		else if (strCmdType == "RealPlayUrl")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::search_realplayurl);
		}
		else if (strCmdType == "PlayBackUrl")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::search_playbackurl);
		}
		else if (strCmdType == "StopPlayUrl")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::stopPlayUrl);
		}
		else if (strCmdType == "DecoderDivision")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::DecoderDivision);
		}
		else if (strCmdType == "DecoderStatus")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::DecoderStatus);
		}
		else if (strCmdType == "DecoderInfo")
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sdk::DecoderInfo);
		}

		CRouter::PushMsg(SDKCOM, pUnifiedMsg);
	}
	else if (mod_op_t::ot_devinfo::update_data == eOperateType.action_devinfo) // SDK (SDKEventHandler?)
	{
		// 更新设备信息
		//如果是从sdkcom模块来的消息
		//在原始逻辑中是逐条读取DeviceConfig内的设备文件初始化GB设备信息。
		HandleUpdateData(pUnifiedMsg);
	}
	else if (mod_op_t::ot_devinfo::subscribe == eOperateType.action_devinfo) // SIP
	{
		HandleSubscribe(pUnifiedMsg);
	}
	else if (mod_op_t::ot_devinfo::send_decoder_subscribe == eOperateType.action_devinfo) // SDKDeviceControl
	{
		HandleDecoderStatusSubscribeNotify(pUnifiedMsg);
	}
	else if (mod_op_t::ot_devinfo::subscribe_notify == eOperateType.action_devinfo) //catalog notify ,such as:  on/offline
	{
		HandleSubscribeNotify(pUnifiedMsg);
	}
	else if (mod_op_t::ot_sdk::subscribe_alarm == eOperateType.action_sdk)
	{
		HandleAlarmSubscribe(pUnifiedMsg);
	}
	else
	{
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
	}
	return true;
}

// 处理设备信息查询
int CDevInfo::HandlePropertyQuery(CModMessage * pUnifiedMsg)
{

	CString strGUID;							// 设备的GUID
	CString	strSN = pUnifiedMsg->GetQuerySN();	// 取得SN号
	CString strDevID = pUnifiedMsg->GetDeviceID();
	DeviceObject deviceinfo;

	auto bFind = CDataStore::LookupDevice(strDevID, deviceinfo);

	if (!bFind)
	{
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return -1;
	}
	auto pDevProperty = deviceinfo.GetProperty();
	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
	pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));// 把查询结果挂载到消息中

	// 创建设备信息包体
	int contentLen = 0;
	CDeviceInfoContent diContent(strDevID, strSN, pDevProperty);
	BodyBuilder::CreateContent(XBT_Response, &diContent, pResultBuf->GetBuffer(), contentLen);

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result); // 设置操作类型
	CRouter::PushMsg(SIPCOM, pUnifiedMsg); // 转发消息到SIPCom模块
	CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询, 设备ID:%s SN:%s", strDevID, strSN);
	return 0;
}

int CDevInfo::HandleDecoderStatusSubscribeNotify(CModMessage * pUnifiedMsg)
{
	CLog::Log(DEVINFO, LL_NORMAL, " %s ", __FUNCTION__);
	DeviceBasicObject		*pUpdateData = reinterpret_cast<DeviceBasicObject *>(pUnifiedMsg->GetSearchData());
	CatalogCollections *channelInfo = static_cast<CatalogCollections*>(pUpdateData);
	subscribeCatalogReport(channelInfo, "MOD");
	delete channelInfo;
	return 1;
}

int CDevInfo::SendAllCatalog(CModMessage *pUnifiedMsg)
{
	int deviceCount = m_CatalogCollections.GetRecordCount();
	int recordCount = 1 + deviceCount;

#ifndef NO_VIRTUAL_CATALOG
	int virtualCount = m_devVirtual.GetRecordCount();
	if (virtualCount > 0)
	{
		recordCount += virtualCount;
	}
#endif /* NO_VIRTUAL_CATALOG */

	// 推送平台目录
	CLog::Log(DEVINFO, LL_NORMAL, " %s Ready to send platform catalog", __FUNCTION__);
	SendPlatformCatalog(pUnifiedMsg, recordCount);
	Sleep(50);

	// 推送虚拟目录
	CLog::Log(DEVINFO, LL_NORMAL, " %s Ready to send virtual catalog", __FUNCTION__);
	SendVirtualCatalog(pUnifiedMsg, recordCount);
	Sleep(50);

	// 推送设备目录
	CLog::Log(DEVINFO, LL_NORMAL, " %s Ready to send device catalog", __FUNCTION__);

	SendDeviceCatalogAll(pUnifiedMsg, recordCount);
	Sleep(50);

	return 0;
}

int CDevInfo::SendPlatformCatalog(CModMessage *pUnifiedMsg, int recordCount)
{
	CString strSN = pUnifiedMsg->GetQuerySN();
	CString strDeviceID = pUnifiedMsg->GetDeviceID();
	int contentLen = 0;

	std::string civilCodeStr = strDeviceID;
	std::string civilCode = civilCodeStr.substr(0, 6);

	//m_oModuleMsgMemMgr.alloc(reinterpret_cast<CMemPoolUnit**>(&pUnifiedMsg)); // 生成模块消息对象
	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
	pUnifiedMsg->SetSearchData(reinterpret_cast<DeviceBasicObject *>(pResultBuf));// 把缓存挂载到消息中

	CPlatformCatalogContent ptContent(str_GatewayName, pUnifiedMsg->GetDeviceID(), strSN, civilCode.c_str(), recordCount);
	BodyBuilder::CreateContent(XBT_Response, &ptContent, pResultBuf->GetBuffer(), contentLen);

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	return 0;
}

#ifndef NO_VIRTUAL_CATALOG
int CDevInfo::SendVirtualCatalog(CModMessage *pUnifiedMsg, int recordCount)
{
	CModMessage *tmpUnifiedMsg = nullptr;
	CBigFile *pResultBuf = nullptr;
	CString strSN = pUnifiedMsg->GetQuerySN();
	DeviceBasicObject::InfoContext_t tContext; // 数据索引
	CatalogCollections *virtualInfo = reinterpret_cast<CatalogCollections*>(pUnifiedMsg->GetSearchData());

	while (nullptr != virtualInfo)
	{
		auto curCatalog = virtualInfo->GetNextCatalog(&tContext);
		if (nullptr == curCatalog) break;

		pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		tmpUnifiedMsg = m_MemAllocator.CloneMessage(pUnifiedMsg);
		tmpUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf)); // 把缓存挂载到消息中

		int contentLen = 0;
		CVirtualCatalogContent vcContent(tmpUnifiedMsg->GetDeviceID(), strSN, recordCount);
		vcContent.AddCatalog(*curCatalog);
		BodyBuilder::CreateContent(XBT_Response, &vcContent, pResultBuf->GetBuffer(), contentLen);

		tmpUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, tmpUnifiedMsg);
		tmpUnifiedMsg = nullptr;
		Sleep(50);
	}
	return 0;
}
#endif /* NO_VIRTUAL_CATALOG */

int CDevInfo::SendDeviceCatalogAll(CModMessage *pMsg, int recordCount, int eveType)
{
	CString strSN = pMsg->GetQuerySN();
	DeviceBasicObject::InfoContext_t tContext; // 数据索引
	auto myType = static_cast<CatalogEventType_t>(eveType);
	auto xmlType = XBT_Response;
	if (myType > CET_NONE && myType < CET_ERROR) xmlType = XBT_Notify;

	if (recordCount == 0)
	{
		recordCount = m_CatalogCollections.GetRecordCount();
	}
	auto curCatalog = m_CatalogCollections.GetNextCatalog(&tContext);
	while (nullptr != curCatalog)
	{
		auto strName = m_GBDevTypeNameMap[curCatalog->GetType()];

		if (m_CatalogPush[strName])
		{
			auto pBodyBuf = m_MemAllocator.AllocBodyContentBuf();
			auto pModMsg = m_MemAllocator.CloneMessage(pMsg);
			pModMsg->SetSearchData(reinterpret_cast<void *>(pBodyBuf)); // 把缓存挂载到消息中

			int contentLen = 0;
			CDeviceCatalog dcContent(pModMsg->GetDeviceID(), strSN, recordCount);
			dcContent.AddCatalog(*curCatalog);
			dcContent.SetEvent(myType);
			BodyBuilder::CreateContent(xmlType, &dcContent, pBodyBuf->GetBuffer(), contentLen);
			pModMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
			// 发送到SIPCom模块
			CRouter::PushMsg(SIPCOM, pModMsg);
			curCatalog = m_CatalogCollections.GetNextCatalog(&tContext);
			Sleep(50);
		}
	}
	return 0;
}

int CDevInfo::SendSingeDecoderCatalog(CModMessage * pUnifiedMsg)
{
	string      deviceId = pUnifiedMsg->GetDeviceID();
	string      remoteId = pUnifiedMsg->GetRemoteID();
	vector<CatalogItem> cataLogInfo;
	CString					strSN = pUnifiedMsg->GetQuerySN();
	decoder.getOneDeocderAllCatalog(deviceId, cataLogInfo);
	int decoderCataLogCount = cataLogInfo.size();
	CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s count = %d", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, decoderCataLogCount);
	for (int i = 0; i < decoderCataLogCount; i++)
	{
		// 生成模块消息对象
		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));

		deviceId = m_strRemotePltID.GetString();
		decoder.packSingleDecoderChnOrDivisonInfo(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), deviceId, decoderCataLogCount, &cataLogInfo[i]);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		::Sleep(50);
	}
	return 1;
}

int CDevInfo::SendSingeDecoderOneChnCatalog(CModMessage * pUnifiedMsg)
{
	string      deviceId = pUnifiedMsg->GetDeviceID();
	string      remoteId = pUnifiedMsg->GetRemoteID();
	vector<CatalogItem> cataLogInfo;
	CString					strSN = pUnifiedMsg->GetQuerySN();
	decoder.getOneDecoderChnAllCatalog(deviceId, cataLogInfo);
	int decoderCataLogCount = cataLogInfo.size();
	CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s count = %d", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, decoderCataLogCount);
	for (int i = 0; i < decoderCataLogCount; i++)
	{
		// 生成模块消息对象
		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		deviceId = m_strRemotePltID.GetString();
		decoder.packSingleDecoderChnOrDivisonInfo(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), deviceId, decoderCataLogCount, &cataLogInfo[i]);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	return 1;
}

int CDevInfo::SendSingeDecoderOneChnOneDivisonCatalog(CModMessage * pUnifiedMsg)
{
	string      deviceId = pUnifiedMsg->GetDeviceID();
	string      remoteId = pUnifiedMsg->GetRemoteID();
	vector<CatalogItem> cataLogInfo;
	CString					strSN = pUnifiedMsg->GetQuerySN();
	decoder.getOneDecoderChnOneDivisonCatalog(deviceId, cataLogInfo);
	int decoderCataLogCount = cataLogInfo.size();
	CLog::Log(DEVINFO, LL_NORMAL, "%s, 设备ID:%s SN:%s count = %d", __FUNCTION__, pUnifiedMsg->GetDeviceID(), strSN, decoderCataLogCount);

	for (int i = 0; i < decoderCataLogCount; i++)
	{
		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();

		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));

		deviceId = m_strRemotePltID.GetString();
		decoder.packSingleDecoderChnOrDivisonInfo(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), deviceId, decoderCataLogCount, &cataLogInfo[i]);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	return 1;
}

int CDevInfo::SendCatalogSubscribeResult(CModMessage * pUnifiedMsg, char *type)
{
	DeviceBasicObject		*pUpdateData = reinterpret_cast<DeviceBasicObject *>(pUnifiedMsg->GetSearchData());
	pUpdateData->SetDeviceID(pUnifiedMsg->GetDeviceID());

	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();

	// 把缓存挂载到消息中
	pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));

	DeviceBasicObject::InfoContext_t	tContext;		// 数据索引
	CString					strSN = pUnifiedMsg->GetQuerySN();

	CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d strsn = %s", __FUNCTION__, pResultBuf->GetBufferLen(), strSN);
	// 向缓存中写入XML格式的查询结果数据
	while (static_cast<CatalogCollections*>(pUpdateData)->GetNotifyCatalogInfo(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), &tContext, type))
	{
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_notify);

		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		// 生成大文件缓存
		pResultBuf = m_MemAllocator.AllocBodyContentBuf();

		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
	}

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_notify);

	// 发送到SIPCom模块
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	return 0;
}

// 处理目录查询
int CDevInfo::HandleCatalogQuery(CModMessage * pUnifiedMsg)
{
	CString		strSN = pUnifiedMsg->GetQuerySN();// 取得SN号
	CString		strDevID = pUnifiedMsg->GetDeviceID();

	// 查询平台目录
	if (appConf.m_Current.str_ID == strDevID)
	{
		SendAllCatalog(pUnifiedMsg);
		CLog::Log(DEVINFO, LL_NORMAL, "本地平台目录查询, 设备ID:%s SN:%s", strDevID, strSN);
		return 0;
	}

	// TODO:请设计者添加注释
	if (appConf.nSipComMode)
	{
		string deviceId = pUnifiedMsg->GetDeviceID();
		string deviceTypStr = deviceId.substr(12, 3);
		if (deviceTypStr == "009")
		{
			SendSingeDecoderCatalog(pUnifiedMsg);
			return 0;
		}

		if (deviceTypStr == "017")
		{
			SendSingeDecoderOneChnCatalog(pUnifiedMsg);
			return 0;
		}
		if (deviceTypStr == "021")
		{
			SendSingeDecoderOneChnOneDivisonCatalog(pUnifiedMsg);
			return 0;
		}
	}

	// 查询设备目录
	if (!CheckDeviceObj(strDevID))
	{
		pUnifiedMsg->Free();
		return -1;
	}

	SendDeviceCatalogAll(pUnifiedMsg, m_CatalogCollections.GetRecordCount()); // 生成xml格式的查询结果

	SAFE_FREE_MOD_MSG(pUnifiedMsg);
	CLog::Log(DEVINFO, LL_NORMAL, "%s 设备目录查询, 设备ID:%s SN:%s", __FUNCTION__, strDevID, strSN);
	return 0;
}

// 更新数据
int CDevInfo::HandleUpdateData(CModMessage * pUnifiedMsg)
{
	auto strGUID = pUnifiedMsg->GetUpdataGUID();
	auto eUpdateType = pUnifiedMsg->GetUpdataType();
	m_oLock.Lock();
	if (event_notify_t::ut_add == eUpdateType) AddDeviceInfo(strGUID);
	else if (event_notify_t::ut_mod == eUpdateType) UpdateDeviceInfo(strGUID);
	else if (event_notify_t::ut_del == eUpdateType) RemoveDeviceInfo(strGUID);
	else if (event_notify_t::ut_init == eUpdateType)InitAllDeviceInfo();
	else if (event_notify_t::ut_alarm == eUpdateType)AlarmNotify(strGUID);
	SAFE_FREE_MOD_MSG(pUnifiedMsg);
	m_oLock.Unlock();
	return 0;
}
int CDevInfo::InitAllDeviceInfo()
{
	auto pAllDevices = CDataStore::GetAllDeviceList();

	auto pos = pAllDevices->GetStartPos();
	DeviceObject deviceInfo;
	auto nCount = 0;
	CString strGUID;
	while (pos)
	{
		pAllDevices->GetNext(pos, strGUID, deviceInfo);
		InitSigleDeviceInfo(appConf.strDevConfDir + _T("\\"), strGUID, nullptr, &deviceInfo);
		nCount++;
	}
	return nCount;
}

int CDevInfo::AlarmNotify(CString strGUID)
{
	CString			strDeviceGBID;
	DeviceObject deviceInfo;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";

	auto bfind = CDataStore::LookupDeviceID(strGUID, strDeviceGBID);
	if (!bfind) return -1;

	auto subscriber = m_oSubAlrmascribeMap[strDeviceGBID.GetString()];
	if (subscriber.remoteId.IsEmpty()) return 0;
	bfind = CDataStore::LookupDeviceByGUID(strDeviceGBID, deviceInfo);
	if (!bfind) return -1;
	SendAlarm(deviceInfo.GetAlarmInfo(), strDeviceGBID);
	return 0;
}

//将设备初始化成一个复合对象
int CDevInfo::InitSigleDeviceInfo(const char *strFilePath, CString strGuid, InfoChangedList *pChangeList, DeviceObject *pDeviceObject)
{
	DeviceObject husDevObject;

	ChangInfo_t oCatalogTmp;
	CString DeviceStatus = "ON";//设备状态默认状态为在线

	if ("{" != strGuid.Left(1))
		strGuid = "{" + strGuid + "}";
	//查找之前初始化的HUS对象
	if (pDeviceObject)
	{
		husDevObject = *pDeviceObject;
	}
	else if (!CDataStore::LookupDeviceByGUID(strGuid, husDevObject))
	{
		return -1;
	}

	auto pGBDevdes = husDevObject.GetGBDescription();

	CString		strFileAllName = strFilePath + strGuid; // Absolute file path.
	// 初始化目录信息
	auto	pCatalog = pGBDevdes->GetCatalog();//目录有哪些信息是需要对象初始化的，怎么和HUSInfo对应
	pCatalog->SetGUID(strGuid);
	pCatalog->SetType(husDevObject.GetGBType());
	//获取目录信息
	int nRet = ReadCatalog(strFileAllName, _T("CATALOG"), pCatalog, nullptr, nullptr);

	if (pCatalog->GetParental().IsEmpty()) pCatalog->SetParental(_T("0"));
	if (pCatalog->GetParentID().IsEmpty()) pCatalog->SetParentID(appConf.m_Current.str_ID);

	if (nullptr != pChangeList)
	{
		oCatalogTmp.oBefore = *pCatalog;
		if (0 != nRet && (!oCatalogTmp.oBefore.GetDeviceID().IsEmpty() || !pCatalog->GetDeviceID().IsEmpty()))
		{
			oCatalogTmp.nChangeType = nRet;
			oCatalogTmp.oAfter = *pCatalog;
			pChangeList->AddTail(oCatalogTmp);
		}
	}
	// 取得设备信息
	auto pDevProperty = pGBDevdes->GetProperty();
	// 取得设备类型名
	if (husDevObject.strDeviceTyeMark)
		pDevProperty->SetDeviceType(husDevObject.strDeviceTyeMark);
	pDevProperty->SetManufacturer("Honeywell");
	pDevProperty->SetModel(pDevProperty->GetDeviceType());
	pDevProperty->SetFirmware(_T("Honeywell Firmware"));

	pGBDevdes->SetDeviceID(pCatalog->GetDeviceID());
	pDevProperty->SetDeviceID(pCatalog->GetDeviceID());
	pGBDevdes->SetGUID(strGuid);

	if (pCatalog->GetModel().MakeLower() == _T("model"))
	{
		pCatalog->SetModel(pDevProperty->GetDeviceType());
	}
	auto devicetype = pDevProperty->GetDeviceType();
	auto devicetypestr = devicetype.MakeLower();
	if (devicetypestr.Find(_T("dvr")) >= 0 || devicetypestr.Find(_T("encoder")) >= 0)
	{
		pCatalog->SetParental(_T("1"));
	}

	if (pDevProperty->GetDeviceType().MakeLower() == _T("decoder"))
	{
		pCatalog->SetParental(_T("1"));

		int count = decoder.g_decoderCatalogInfo.size();
		bool findFlag = false;
		for (int i = 0; i < count; i++)
		{
			if (decoder.g_decoderCatalogInfo[i].GetDeviceID() == pCatalog->GetDeviceID())
			{
				decoder.g_decoderCatalogInfo[i] = *pCatalog;  ////pjs modify
				findFlag = true;
			}
		}

		if (!findFlag)
		{
			decoder.g_decoderCatalogInfo.push_back(*pCatalog);
		}
	}

	CDataStore::AddDeviceObjectByGUID(strGuid, husDevObject);

	//添加目录
	m_CatalogCollections.AddCatalog(husDevObject.m_strDeviceID, *pCatalog);

	//存在子设备时候开始读取
	if (husDevObject.tDevinfoInConf.oSubStrGUIDSecNameMap.size() > 0)
	{
		// 读取子设备
		auto pSubCatalogs = pGBDevdes->GetSubCatalogs();
		pSubCatalogs->Clear();
		auto parentID = pCatalog->GetDeviceID(); //get ParentID for current chaannelID.

		CString			strChannelIndex;
		auto videoChannel = 1;
		auto alarmChannel = 1;

		for (auto& item : husDevObject.tDevinfoInConf.oSubStrGUIDSecNameMap)
		{
			DeviceObject subhusobject;
			CDataStore::LookupDeviceByGUID(item.first.c_str(), subhusobject);

			auto pSubDevObj = subhusobject.GetGBDescription();
			auto childCatalog = pSubDevObj->GetCatalog();
			childCatalog->SetType(subhusobject.GetGBType());

			auto ret = ReadCatalog(strFileAllName, item.second.c_str(), childCatalog);

			if (ret < 0)	continue;

			childCatalog->SetParentID(parentID); //Setup ParentID.
			childCatalog->SetParental(_T("0"));  //Set Parental as 0
		childCatalog->SetStatus(pCatalog->GetStatus());
			auto DevieModelTypeName = childCatalog->GetModel();
			DevieModelTypeName = DevieModelTypeName.MakeLower();

			if (childCatalog->GetName().MakeLower() == _T("name"))
			{
				CString str;
				auto namestr = pCatalog->GetName();
				//FOR ENCODER AND DVR
				if (DevieModelTypeName == _T("camera"))
				{
					str.Format(namestr + "_" + DevieModelTypeName + "_%d", videoChannel++);
				}
				else if (DevieModelTypeName == _T("viralarm"))
				{
					str.Format(namestr + "_" + DevieModelTypeName + "_%d", alarmChannel++);
				}
				childCatalog->SetName(str);
			}
			auto subDeviceID = childCatalog->GetDeviceID();

			auto pSubDevProp = pSubDevObj->GetProperty();
			pSubDevProp->SetDeviceType(childCatalog->GetModel()); // device type -> catalog model
			pSubDevProp->SetManufacturer(pDevProperty->GetManufacturer());// device manufacturer -> parent manufacturer
			pSubDevProp->SetModel(childCatalog->GetModel());// device model -> catalog model
			pSubDevProp->SetFirmware(_T("Honeywell Firmware"));
			pSubDevObj->SetDeviceID(subDeviceID);
			pSubDevProp->SetDeviceID(subDeviceID);
			pSubDevObj->SetGUID(item.first.c_str());

			pSubCatalogs->AddCatalog(subDeviceID, *childCatalog); // add subchannel device

			m_CatalogCollections.AddCatalog(subDeviceID, *childCatalog);  //add to Platform device

			CDataStore::AddDeviceObjectByGUID(item.first.c_str(), husDevObject);
			continue;
		}
		
	}

	return 0;
}

int CDevInfo::InitCalog(CatalogItem *pCatalog, CString szinfo, CString guid, const char * /*pParentType*/, const char * /*pszParentGUID*/)
{
	CString Guid = guid;
	CString szInfo = szinfo;
	int nRet = 0;
	if (0 == _stricmp("NaN", szInfo))
		return -1;
	if (0 != pCatalog->GetDeviceID().Compare(szInfo)) nRet |= 1;
	pCatalog->SetDeviceID(szInfo);
	if (0 != pCatalog->GetName().Compare("Name")) nRet |= 2;
	pCatalog->SetName("Name");
	if (0 != pCatalog->GetManufacturer().Compare("Honeywell")) nRet |= 2;
	pCatalog->SetManufacturer("Honeywell");

	if (0 != pCatalog->GetModel().Compare("Model")) nRet |= 2;
	pCatalog->SetModel("Model");

	if (0 != pCatalog->GetModel().Compare("Owner")) nRet |= 2;
	pCatalog->SetModel("Owner");

	char strTmp[MAX_PATH] = "CivilCode";
	if (memcmp(strTmp, "CivilCode", 3) == 0)
	{
		auto tmpstr = pCatalog->GetDeviceID().Mid(0, 6);
		Utils::StringCpy_s(strTmp, MAX_PATH, tmpstr);
	}
	CString strCivilCode = strTmp;
	if (0 != strCivilCode.GetLength() % 2) {
		strCivilCode += "0";
	}
	if (0 != pCatalog->GetCivilCode().Compare(strTmp)) nRet |= 2;
	pCatalog->SetCivilCode(strTmp);

	if (0 != pCatalog->GetBlock().Compare("Block")) nRet |= 2;
	pCatalog->SetBlock("Block");

	if (0 != pCatalog->GetAddress().Compare("Address")) nRet |= 2;
	pCatalog->SetAddress("Address");

#ifndef NO_VIRTUAL_CATALOG
	/*GetPrivateProfileString(strSectionName, "VirtualDirID", "", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetVirtualDirID().Compare(szInfo)) nRet |= 2;
	pCatalog->SetVirtualDirID(szInfo);
	pCatalog->SetParentID(szInfo);*/
	if (0 != pCatalog->GetVirtualDirID().Compare("")) nRet |= 2;
	pCatalog->SetVirtualDirID("");
	pCatalog->SetParentID("");
#endif /* NO_VIRTUAL_CATALOG */
	if (0 != pCatalog->GetSafetyWay().Compare("0")) nRet |= 2;
	pCatalog->SetSafetyWay("0");

	if (0 != pCatalog->GetRegisterWay().Compare("1")) nRet |= 2;
	pCatalog->SetRegisterWay("1");

	if (0 != pCatalog->GetCertNum().Compare("0")) nRet |= 2;
	pCatalog->SetCertNum("0");

	if (0 != pCatalog->GetCertifiable().Compare("0")) nRet |= 2;
	pCatalog->SetCertifiable("0");

	if (0 != pCatalog->GetErrcode().Compare(szInfo)) nRet |= 2;
	pCatalog->SetErrcode("0");

	if (0 != pCatalog->GetEndTime().Compare("now")) nRet |= 2;
	pCatalog->SetEndTime("now");

	if (0 != pCatalog->GetSecrecy().Compare("0")) nRet |= 2;
	pCatalog->SetSecrecy("0");

	if (0 != pCatalog->GetIPAddress().Compare("0.0.0.0")) nRet |= 2;
	pCatalog->SetIPAddress("0.0.0.0");

	if (0 != pCatalog->GetPort().Compare("0")) nRet |= 2;
	pCatalog->SetPort("0");

	if (0 != pCatalog->GetPassword().Compare("12345678")) nRet |= 2;
	pCatalog->SetPassword("12345678");

	if (0 != pCatalog->GetLongitude().Compare("0")) nRet |= 2;
	pCatalog->SetLongitude("0");

	if (0 != pCatalog->GetLatitude().Compare("0")) nRet |= 2;
	pCatalog->SetLatitude("0");

	if (0 != pCatalog->GetPTZType().Compare("1")) nRet |= 2;
	pCatalog->SetPTZType("1");

	if (0 != pCatalog->GetPlayUrl().Compare("PlayUrl")) nRet |= 2;
	pCatalog->SetPlayUrl("PlayUrl");
	return nRet;
}
int CDevInfo::AddDeviceInfo(const char* pszGUID)
{
	//return InitSigleDeviceInfo(pszGUID, nullptr);
	return InitSigleDeviceInfo(appConf.strDevConfDir + _T("\\"), pszGUID, nullptr);
}

int CDevInfo::UpdateDeviceInfo(const char* pszGUID)
{
	ChangInfo_t*		pChangeInfo = nullptr;
	InfoChangedList		oInfoChanged;
	if (!InitSigleDeviceInfo(appConf.strDevConfDir + _T("\\"), pszGUID, &oInfoChanged))
		//if (!InitSigleDeviceInfo(pszGUID, &oInfoChanged))
		return -1;
	POSITION pos = oInfoChanged.GetHeadPosition();
	while (pos)
	{
		pChangeInfo = &oInfoChanged.GetNext(pos);
		int nChType = pChangeInfo->nChangeType;
		if (nChType > 0)
		{
			if ((nChType & 0x01) != 0)
			{
				RemoveDeviceInfoNotify(&(pChangeInfo->oBefore));
				UpdateDeviceInfoNotify(&(pChangeInfo->oAfter), TRUE);
			}
			else
			{
				UpdateDeviceInfoNotify(&(pChangeInfo->oAfter), FALSE);
			}
		}
	}
	oInfoChanged.RemoveAll();
	CLog::Log(DEVINFO, LL_DEBUG, "更新设备信息 设备GUID:%s", pszGUID);
	return 0;
}

//直接去设备集合里面查找删除
int CDevInfo::RemoveDeviceInfo(const char* pszGUID)
{
	DeviceObject deviceinfo;
	CString strDeviceId;
	CString strGUID = pszGUID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";

	CDataStore::RemoveDeviceByGUID(strGUID);

	CLog::Log(DEVINFO, LL_DEBUG, "删除设备信息 设备GUID:%s", pszGUID);
	return 0;
}

int CDevInfo::UpdateDeviceInfoNotify(CatalogItem *pNewCatalog, BOOL bNew)
{
	if (nullptr == pNewCatalog) return -1;

	SubInfoList subList;
	CString strDeviceID(pNewCatalog->GetDeviceID());
	if (strDeviceID.GetLength() != ID_LEN) return -1;

	BOOL bFind = GetSubscribeInfo(strDeviceID, &subList);
	if (!bFind) return -1;

	auto pos = subList.GetHeadPosition();
	while (pos != nullptr)
	{
		auto subInfo = subList.GetNext(pos);
		SendCatalogSubscribeNotify(subInfo, pNewCatalog, (bNew ? (CET_ADD) : (CET_UPDATE)));
	}

	return 0;
}

int CDevInfo::RemoveDeviceInfoNotify(CatalogItem *pOldCatalog)
{
	if (nullptr == pOldCatalog) return -1;

	SubInfoList subList;
	CString strDeviceID(pOldCatalog->GetDeviceID());
	if (strDeviceID.GetLength() != ID_LEN) return -1;

	BOOL bFind = GetSubscribeInfo(strDeviceID, &subList);
	if (!bFind) return -1;
	auto pos = subList.GetHeadPosition();
	while (pos != nullptr)
	{
		auto subInfo = subList.GetNext(pos);
		SendCatalogSubscribeNotify(subInfo, pOldCatalog, CET_DEL);
	}
	return 0;
}

int CDevInfo::OnLineInfoNotify(CatalogItem *pCatalog, int eveType)
{
	if (nullptr == pCatalog) return -1;
	SubInfoList subList;
	CString strDeviceID = pCatalog->GetDeviceID();
	if (strDeviceID.GetLength() != ID_LEN) return -1;

	BOOL bFind = GetSubscribeInfo(strDeviceID, &subList);
	if (!bFind) return -1;

	auto pos = subList.GetHeadPosition();
	while (pos != nullptr)
	{
		auto subInfo = subList.GetNext(pos);
		SendCatalogSubscribeNotify(subInfo, pCatalog, eveType);
	}

	return 0;
}

int CDevInfo::SendCatalogSubscribeNotify(SubInfo_t subInfo, CatalogItem* pCatalog, int eveType)
{
	if (nullptr == pCatalog) return -1;

	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
	auto pUnifiedMsg = m_MemAllocator.AllocModMessage();
	pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf)); // 把缓存挂载到消息中

	CString strDeviceID(pCatalog->GetDeviceID());
	auto myType = static_cast<CatalogEventType_t>(eveType);

	int contentLen = 0;
	CDeviceCatalog dcContent(subInfo.szToDeviceID, subInfo.szSN, 1);
	dcContent.AddCatalog(*pCatalog);
	dcContent.SetEvent(myType);
	BodyBuilder::CreateContent(XBT_Notify, &dcContent, pResultBuf->GetBuffer(), contentLen);

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_notify);
	pUnifiedMsg->SetCallDialogID(subInfo.nCallID);
	pUnifiedMsg->SetQuerySN(subInfo.szSN);

	CRouter::PushMsg(SIPCOM, pUnifiedMsg); // 发送到SIPCom模块
	return 0;
}

BOOL CDevInfo::GetSubscribeInfo(const char *pszDeviceID, SubInfoList *subInfolist)
{
	CString		strGUID;
	CString		strDeviceID(pszDeviceID);
	INT64		nDialogID;
	SubInfo_t	*pSubInfo = nullptr;
	CString		strSubID;

	BOOL bFind = CDataStore::LookupGUID(pszDeviceID, strGUID);
	if (!bFind) return FALSE;

	//bFind = CDataStore::LookupDeviceObj(strDeviceID, pDevObj);
	if (!bFind) return FALSE;

	m_catalogSubscribeLock.Lock();
	POSITION posStart = m_oSubscribeMap.GetStartPosition();

	while (posStart != nullptr)
	{
		m_oSubscribeMap.GetNextAssoc(posStart, nDialogID, pSubInfo);
		if (nullptr == pSubInfo) continue;

		strSubID = pSubInfo->szToDeviceID;
		// 如果平台被订阅 或者 设备本身被订阅 或者 (TODO:虚拟目录，行政区划, 业务分组)，则关注。
		if ((strSubID == appConf.m_Current.str_ID) || (strSubID == strDeviceID))
		{
			SubInfo_t tmpInfo;
			tmpInfo = *pSubInfo;
			subInfolist->AddTail(tmpInfo);
		}
	}

	m_catalogSubscribeLock.Unlock();
	return TRUE;
}

// for decoder
void CDevInfo::subscribeCatalogReport(CatalogCollections *chnInfo, char *type)
{
	static	UINT		nSN = 1;

	//CBigFile *pResultBuf = nullptr;
	CString strSN;
	CString deviceId = chnInfo->GetDeviceID();
	SubInfo_t subInfo;
	strSN.Format("%d", nSN);

	auto pUnifiedMsg = m_MemAllocator.AllocModMessage();
	pUnifiedMsg->SetDeviceID(subInfo.szToDeviceID);
	pUnifiedMsg->SetSearchData(chnInfo);
	pUnifiedMsg->SetCallDialogID(subInfo.nCallID);
	pUnifiedMsg->SetRemoteID(subInfo.remoteId.GetString());
	this->SendCatalogSubscribeResult(pUnifiedMsg, type);

	nSN++;
	CLog::Log(SDKCOM, LL_NORMAL, "%s 发送订阅目录通知信息 设备ID:%s type = %s", __FUNCTION__, subInfo.szToDeviceID, type);
}

void CDevInfo::InitVirtualCatalog()
{
	for (auto& item : devInfoConf.m_DeviceVirtualDir)
	{
		CatalogItem virCatalog;
		ws_other_cvt_t ws_gb2312_cvt(new facet_wc_t(GB2312_LOCALE_NAME));
		auto strgb2312 = ws_gb2312_cvt.to_bytes(item.first.c_str());
		virCatalog.SetName(strgb2312.c_str());
		virCatalog.SetDeviceID(item.second.c_str());
		m_devVirtual.AddCatalog(item.second.c_str(), virCatalog);
	}
}
// 读取配置文件中的目录信息
// -1：无法读取DeviceID 0：没有变更 1：GBID变更 2：其他信息变更 3：GBID和其他信息均有变更
int CDevInfo::ReadCatalog(const CString &strFileName, const CString &strSectionName, CatalogItem *pCatalog, const char *pParentType, const char *pszParentGUID)
{
	int nRet = 0;
	char szInfo[MAX_PATH];
	// 此设备是子设备
	if (nullptr != pszParentGUID && 0 < strlen(pszParentGUID))
	{
		CString strParentGUID = pszParentGUID;
		// 设置父子GUID映射
		m_oGUIDToGUIDMap.SetAt(strSectionName, strParentGUID);
		pCatalog->SetGUID(strSectionName);
	}
	CLog::Log(DEVINFO, LL_NORMAL, "%s strFileName = %s strChannelIndex = %s\r\n", __FUNCTION__, strFileName, strSectionName);

	// 取得设备GBID
	if (pParentType && 0 == _stricmp(pParentType, _T("IPC")) && 0 == _stricmp(pCatalog->GetModel(), _T("Camera")))
	{
		GetPrivateProfileString("CATALOG", "DeviceID", "NaN", szInfo, MAX_PATH, strFileName);
	}
	else
	{
		GetPrivateProfileString(strSectionName, "DeviceID", "NaN", szInfo, MAX_PATH, strFileName);
	}
	if (0 == _stricmp("", szInfo) || 0 == _stricmp("NaN", szInfo)) return -1; // 连DeviceID都读不到，玩不下去了
	if (0 != pCatalog->GetDeviceID().Compare(szInfo)) nRet |= 1;
	pCatalog->SetDeviceID(szInfo);

	GetPrivateProfileString(strSectionName, "Name", "Name", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetName().Compare(szInfo)) nRet |= 2;
	pCatalog->SetName(szInfo);

	pCatalog->SetManufacturer("Honeywell");

	GetPrivateProfileString(strSectionName, "Model", "Model", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetModel().Compare(szInfo)) nRet |= 2;
	pCatalog->SetModel(szInfo);

	pCatalog->SetOwner("Owner");

	GetPrivateProfileString(strSectionName, "CivilCode", "CivilCode", szInfo, MAX_PATH, strFileName);
	if (memcmp(szInfo, "CivilCode", 3) == 0)
	{
		auto tmpstr = pCatalog->GetDeviceID().Mid(0, 6);
		Utils::StringCpy_s(szInfo, MAX_PATH, tmpstr);
	}
	CString strCivilCode = szInfo;
	if (0 != strCivilCode.GetLength() % 2) {
		strCivilCode += "0";
	}
	if (0 != pCatalog->GetCivilCode().Compare(szInfo)) nRet |= 2;
	pCatalog->SetCivilCode(szInfo);
	pCatalog->SetBlock(szInfo);
	pCatalog->SetAddress("Address");

#ifndef NO_VIRTUAL_CATALOG
	GetPrivateProfileString(strSectionName, "VirtualDirID", "", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetVirtualDirID().Compare(szInfo)) nRet |= 2;
	pCatalog->SetVirtualDirID(szInfo);
	pCatalog->SetParentID(szInfo);
#endif /* NO_VIRTUAL_CATALOG */

	GetPrivateProfileString(strSectionName, "SafetyWay", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetSafetyWay().Compare(szInfo)) nRet |= 2;
	pCatalog->SetSafetyWay(szInfo);

	GetPrivateProfileString(strSectionName, "RegisterWay", "1", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetRegisterWay().Compare(szInfo)) nRet |= 2;
	pCatalog->SetRegisterWay(szInfo);

	pCatalog->SetCertNum("0");

	GetPrivateProfileString(strSectionName, "Certifiable", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetCertifiable().Compare(szInfo)) nRet |= 2;
	pCatalog->SetCertifiable(szInfo);

	GetPrivateProfileString(strSectionName, "ErrCode", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetErrcode().Compare(szInfo)) nRet |= 2;
	pCatalog->SetErrcode(szInfo);

	GetPrivateProfileString(strSectionName, "EndTime", "now", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetEndTime().Compare(szInfo)) nRet |= 2;
	pCatalog->SetEndTime(szInfo);

	GetPrivateProfileString(strSectionName, "Secrecy", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetSecrecy().Compare(szInfo)) nRet |= 2;
	pCatalog->SetSecrecy(szInfo);

	pCatalog->SetIPAddress("0.0.0.0");

	pCatalog->SetPort("0");
	pCatalog->SetPassword("12345678");

	GetPrivateProfileString(strSectionName, "Longitude", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetLongitude().Compare(szInfo)) nRet |= 2;
	pCatalog->SetLongitude(szInfo);

	GetPrivateProfileString(strSectionName, "Latitude", "0", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetLatitude().Compare(szInfo)) nRet |= 2;
	pCatalog->SetLatitude(szInfo);

	// info add !部分info信息已经被删除了,我就不复原了[Mork mark]!
	GetPrivateProfileString(strSectionName, "PTZType", "1", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetPTZType().Compare(szInfo)) nRet |= 2;
	pCatalog->SetPTZType(szInfo);
	GetPrivateProfileString(strSectionName, "PlayUrl", "PlayUrl", szInfo, MAX_PATH, strFileName);
	if (0 != pCatalog->GetPlayUrl().Compare(szInfo)) nRet |= 2;
	pCatalog->SetPlayUrl(szInfo);

	return nRet;
}


// 处理订阅请求： 200 OK 已经在SIPCOM回复，因此这里无需再发送
int CDevInfo::HandleSubscribe(CModMessage * pUnifiedMsg)
{
	CString szSubID = pUnifiedMsg->GetDeviceID();
	INT64   nCallID = pUnifiedMsg->GetCallDialogID();
	CString szSN = pUnifiedMsg->GetQuerySN();
	CString szSubExpiry = pUnifiedMsg->GetSubExpires();
	CString remoteId = pUnifiedMsg->GetRemoteID();
	SubInfo_t* pSubInfo = nullptr;
	CString strGUID;

	CLog::Log(DEVINFO, LL_NORMAL, "%s szSubExpiry = %s remoteId = %s\r\n", __FUNCTION__, szSubExpiry, remoteId);

	BOOL bSubFind = m_oSubscribeMap.Lookup(nCallID, pSubInfo);
	int nSubExpiry = atoi(szSubExpiry);

	// 处理超时 OR 取消订阅
	if (szSubExpiry == "Timeout" || 0 == nSubExpiry)
	{
		// 若map表里有记录，则删除
		if (bSubFind)
		{
			delete pSubInfo;
			pSubInfo = nullptr;
			m_oSubscribeMap.RemoveKey(nCallID);
		}

		if ("Timeout" == szSubExpiry) // 超时需要发送停止通知
		{
			pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_timeout_notify);
			CRouter::PushMsg(SIPCOM, pUnifiedMsg);
			return 0;
		}

		pUnifiedMsg->Free();
		return 0;
	}

	//处理订阅
	BOOL bFind = FALSE;
	//	int findDevice = -1;
	if (appConf.m_Current.str_ID == szSubID) bFind = TRUE;
	else bFind = CheckDeviceObj(szSubID);

	// 无效的DeviceID
	if (!bFind)
	{
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_noresrc_notify);
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		return -1;
	}

	//CString szToDeviceID;
	// 若map表里没有记录，则添加
	CDataStore::LookupGUID(szSubID, strGUID);
	CTime curTime = CTime::GetCurrentTime();
	if (!bSubFind)
	{
		pSubInfo = new SubInfo_t;
		pSubInfo->deviceIdType = 0;
		pSubInfo->expireTime = nSubExpiry;
		pSubInfo->startTime = static_cast<long>(curTime.GetTime());
		pSubInfo->endTime = pSubInfo->startTime + pSubInfo->expireTime;
		pSubInfo->nCallID = nCallID;
		pSubInfo->szToDeviceID = szSubID;
		pSubInfo->szSN = szSN;
		pSubInfo->remoteId = remoteId;
		pSubInfo->guid = strGUID;

		m_oSubscribeMap.SetAt(nCallID, pSubInfo);
	}
	else
	{
		pSubInfo->expireTime = nSubExpiry;
		pSubInfo->startTime = (long)curTime.GetTime();
		pSubInfo->endTime = pSubInfo->startTime + pSubInfo->expireTime;
	}

	pUnifiedMsg->Free();

	CLog::Log(DEVINFO, LL_NORMAL, "%s szSubExpiry = %s remoteId = %s send test subscribeInfo\r\n", __FUNCTION__, szSubExpiry, remoteId);
	return 0;
}

int CDevInfo::HandleSubscribeNotify(CModMessage * pUnifiedMsg)
{
	auto updatetype = pUnifiedMsg->GetUpdataType();
	if (updatetype & (event_notify_t::ut_on | event_notify_t::ut_off))
	{
		HandleOnlineStatus(pUnifiedMsg);
		return 0;
	}
	return -1;
}

int CDevInfo::HandleAlarmSubscribe(CModMessage * pUnifiedMsg)
{
	CString szSubID = pUnifiedMsg->GetDeviceID();
	INT64   nCallID = pUnifiedMsg->GetCallDialogID();
	CString szSN = pUnifiedMsg->GetQuerySN();
	CString szSubExpiry = pUnifiedMsg->GetSubExpires();
	XmlParam_t     *xmlParam = static_cast<XmlParam_t *>(pUnifiedMsg->GetCmdParam());
	CString         alarmMethod = xmlParam->strParamVal3;
	int          startAlarmProperty = atoi(xmlParam->strParamVal4);
	int          endAlarmProperty = atoi(xmlParam->strParamVal5);
	delete xmlParam;

	SubAlarmInfo_t* pSubInfo = nullptr;

	CBodyBuilder bodyBuider;
	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
	bodyBuider.createSubscribeResponse(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), szSN, szSubID, "Alarm", "OK");
	pUnifiedMsg->SetSearchData(pResultBuf);
	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::subscribe_reponse);
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	CLog::Log(SDKCOM, LL_NORMAL, "%s szSubExpiry = %s deviceId = %s", __FUNCTION__, szSubExpiry, szSubID);


	if (szSubExpiry == "0" ||
		szSubExpiry == "TimeOut")
	{
		CString szToDeviceID;
		if (m_oSubIDMap.Lookup(nCallID, szToDeviceID))
		{

			if (m_oSubAlrmascribeMap.count(szToDeviceID.GetString()) > 0)
			{
				m_oSubAlrmascribeMap.erase(szToDeviceID.GetString());

			}
			m_oSubIDMap.RemoveKey(nCallID);
		}

		if (szSubExpiry == "0")
		{
			//    pUnifiedMsg->SetSubResult(0);
			//    pUnifiedMsg->SetModAction(ot_sip_subscribe_reponse);
			//   CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		}
		else
		{
			//    pUnifiedMsg->Free();
		}

		return 0;
	}

	CString szToDeviceID;
	if (m_oSubIDMap.Lookup(nCallID, szToDeviceID))
	{

		if (m_oSubAlrmascribeMap.count(szToDeviceID.GetString()) > 0)
		{
			m_oSubAlrmascribeMap.erase(szToDeviceID.GetString());
		}

	}


	pSubInfo = new SubAlarmInfo_t;
	pSubInfo->alarmMethod = alarmMethod;
	pSubInfo->deviceIdType = 0;
	pSubInfo->expireTime = atoi(pUnifiedMsg->GetSubExpires());
	CTime curTime = CTime::GetCurrentTime();
	pSubInfo->startTime = curTime.GetTime();
	pSubInfo->endTime = pSubInfo->startTime + pSubInfo->expireTime;

	pSubInfo->nCallID = nCallID;
	pSubInfo->szToDeviceID = szSubID;
	pSubInfo->szSN = szSN;
	pSubInfo->remoteId = pUnifiedMsg->GetRemoteID();
	pSubInfo->startProperty = startAlarmProperty;
	pSubInfo->endProperty = endAlarmProperty;

	memset(&pSubInfo->guid, 0, sizeof(pSubInfo->guid));

	m_oSubAlrmascribeMap[szSubID.GetString()] = *pSubInfo;

	m_oSubIDMap.SetAt(nCallID, szToDeviceID);

	CLog::Log(SDKCOM, LL_NORMAL, "%s deviceId = %s szSN = %s startAlarmProperty = %d endAlarmProperty = %d alarmMethod = %s endTime = %d", __FUNCTION__, szSubID, szSN, startAlarmProperty, endAlarmProperty, alarmMethod, pSubInfo->endTime);


	CString strCurTime;
	time_t tmCur1;
	time(&tmCur1);
	tm tmInfo1;
	localtime_s(&tmInfo1, &tmCur1);
	strCurTime.Format("%d-%02d-%02dT%02d:%02d:%02d",
		tmInfo1.tm_year + 1900, tmInfo1.tm_mon + 1, tmInfo1.tm_mday,
		tmInfo1.tm_hour, tmInfo1.tm_min, tmInfo1.tm_sec);
	//发送全部的报警信息

	auto allRencentAlarm = CDataStore::GetRecentAlarmAll();

	for (auto &item : allRencentAlarm)
	{
		CString DeviceId;
		CString strAlarmTime;
		BOOL bRet = false;

		strAlarmTime = item.m_strTime;
		bRet = judgeTime(tmInfo1, strAlarmTime);
		if (bRet)
		{
			SendAlarm(&item, DeviceId);
		}
	}

	return 0;
}
bool CDevInfo::judgeTime(tm tmInfo1, CString strAlarmTime)
{
	if (19 != strAlarmTime.GetLength()) return false;
	char *str = (char *)malloc(30);
	string tmpTime = strAlarmTime.GetString();
	const char* strTmp = tmpTime.c_str();
	Utils::StringCpy_s(str, 4, strTmp);
	if ((tmInfo1.tm_year + 1900) == atoi(str))
	{
		strTmp += 5;
		str += 4;
		Utils::StringCpy_s(str, 2, strTmp);
		if ((tmInfo1.tm_mon + 1) == atoi(str))
		{
			strTmp += 3;
			str += 2;
			Utils::StringCpy_s(str, 2, strTmp);
			if ((tmInfo1.tm_mday) == atoi(str))
			{
				strTmp += 3;
				str += 2;
				Utils::StringCpy_s(str, 2, strTmp);
				if ((tmInfo1.tm_hour) == atoi(str))
				{
					strTmp += 3;
					str += 2;
					Utils::StringCpy_s(str, 2, strTmp);
					if ((tmInfo1.tm_min - atoi(str)) < 2)
					{
						free(str);
						str = nullptr;
						return true;
					}
					else if ((tmInfo1.tm_min - atoi(str)) == 2)
					{
						strTmp += 3;
						str += 2;
						Utils::StringCpy_s(str, 2, strTmp);
						if (tmInfo1.tm_sec == atoi(str))
						{
							free(str);
							str = nullptr;
							return true;
						}
						else
						{
							free(str);
							str = nullptr;
							return false;
						}
					}
					else
					{
						free(str);
						str = nullptr;
						return false;
					}
				}
				else if ((tmInfo1.tm_hour - atoi(str)) == 1)
				{
					if (tmInfo1.tm_min >= 2)
					{
						free(str);
						str = nullptr;
						return false;
					}
					strTmp += 3;
					str += 2;
					Utils::StringCpy_s(str, 2, strTmp);
					if (atoi(str) <= 57)
					{
						free(str);
						str = nullptr;
						return false;
					}
					if (tmInfo1.tm_min == 0)
					{
						if (atoi(str) == 58)
						{
							strTmp += 3;
							str += 2;
							Utils::StringCpy_s(str, 2, strTmp);
							if (atoi(str) >= tmInfo1.tm_sec)
							{
								free(str);
								str = nullptr;
								return true;
							}
							else
							{
								free(str);
								str = nullptr;
								return false;
							}
						}
						else
						{
							free(str);
							str = nullptr;
							return true;
						}
					}
					if (tmInfo1.tm_min == 1)
					{
						if (atoi(str) == 58)
						{
							free(str);
							str = nullptr;
							return false;
						}
						else
						{
							strTmp += 3;
							str += 2;
							Utils::StringCpy_s(str, 2, strTmp);
							if (atoi(str) >= tmInfo1.tm_sec)
							{
								free(str);
								str = nullptr;
								return true;
							}
							else
							{
								free(str);
								str = nullptr;
								return false;
							}
						}
					}
				}
			}
		}
	}
	free(str);
	str = nullptr;
	return false;
}
void CDevInfo::SendAlarm(DeviceAlarmInfo* AlaInfo, CString DeviceId)
{

	AlarmReport(AlaInfo->strDeviceID.GetString(), AlaInfo->m_Description.GetString(), AlaInfo->m_strPriority, AlaInfo->m_strTime, AlaInfo->m_nAlarmMethord, AlaInfo->m_nAlarmType, AlaInfo->m_strAlarmStatus);
}
// 报警上报
int CDevInfo::AlarmReport(const char *pszGBID, const char *pszDescription, CString strLevel, const char *pszTime, int alarmMethod, int alarmType, CString strAlarmStatus)
{
	static	UINT nSN = 1;
	CBodyBuilder bodyBuider;
	CString strSN;
	CString latitude;
	CString longtitude;
	CString alarmPriority;
	SubAlarmInfo_t subAlarmInfo;
	//alarmPriority.Format("%d", nLevel);
	alarmPriority = strLevel;
	strSN.Format("%d", nSN);
	//  int ret = 1;

	if (m_oSubAlrmascribeMap.count(pszGBID))
	{

		CLog::Log(SDKCOM, LL_NORMAL, "%s gbId = %s  alarmMethod = %d strLevel = %s ", __FUNCTION__, pszGBID, alarmMethod, strLevel);

		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		auto pUnifiedMsg = m_MemAllocator.AllocModMessage();
		CLog::Log(SDKCOM, LL_NORMAL, "%s begin \r\n", __FUNCTION__);
		strSN = subAlarmInfo.szSN;
		longtitude = "";
		latitude = "";
		bodyBuider.CreateAlarmBodyNew(
			pResultBuf->GetBuffer(),
			pResultBuf->GetBufferLen(),
			strSN,
			pszGBID,
			alarmPriority,
			std::to_string(alarmMethod).c_str(),
			pszTime,
			pszDescription,
			longtitude, latitude,
			std::to_string(alarmType).c_str(),
			strAlarmStatus);
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::alarm_request);
		strSN.Format("%d", nSN);
		pUnifiedMsg->SetNotifyData(pResultBuf, strSN);
		//    pUnifiedMsg->SetCallDialogID(subAlarmInfo.nCallID);
		pUnifiedMsg->SetDeviceID(subAlarmInfo.szToDeviceID);
		pUnifiedMsg->SetCallDialogID(subAlarmInfo.nCallID);
		pUnifiedMsg->SetRemoteID(subAlarmInfo.remoteId);
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	nSN++;
	CLog::Log(SDKCOM, LL_DEBUG, "发送报警信息 设备ID:%s 告警级别:%s 告警信息:%s", pszGBID, strLevel, pszDescription);
	return 0;
}

// 处理设备上下线
int CDevInfo::HandleOnlineStatus(CModMessage * pUnifiedMsg)
{
	CatalogItem* pCatalog = nullptr;
	CString strGUID = pUnifiedMsg->GetUpdataGUID();
	CString strDeviceID = pUnifiedMsg->GetDeviceID();

	auto updateType = pUnifiedMsg->GetUpdataType();
	switch (updateType)
	{
	case event_notify_t::ut_on:
	{
		pCatalog = m_CatalogCollections.GetCatalogByGUID(strDeviceID);
		pCatalog->SetStatus("ON");
		OnLineInfoNotify(pCatalog, CET_ON);
		break;
	}
	case event_notify_t::ut_off:
	{
		pCatalog = m_CatalogCollections.GetCatalogByGUID(strDeviceID);
		pCatalog->SetStatus("OFF");
		OnLineInfoNotify(pCatalog, CET_OFF);
		break;
	}
	default: // 其他状态是否包含还未知
		break;
	}
	SAFE_FREE_MOD_MSG(pUnifiedMsg);
	return 0;
}

BOOL CDevInfo::CheckDeviceObj(const char *pszDeviceId)
{
	CString strDevID(pszDeviceId);
	CString strGUID;
	BOOL bFind = FALSE;
	bFind = CDataStore::LookupGUID(strDevID, strGUID);
	if (!bFind || strGUID.IsEmpty())
	{
		CLog::Log(DEVINFO, LL_NORMAL, "%s 设备查询失败，未知的GB28181设备ID:%s", __FUNCTION__, strDevID);
		return FALSE;
	}
	return TRUE;
}

//预置位列表查询
int CDevInfo::HandlePersetSearch(CModMessage * pUnifiedMsg)
{
	DeviceObject	deviceInfo;
	CString	strSN = pUnifiedMsg->GetQuerySN();

	if (CDataStore::LookupDevice(pUnifiedMsg->GetDeviceID(), deviceInfo))
	{
		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		char xml[1024] = { 0 };
		sprintf_s(xml,
			"<?xml version=\"1.0\"?>\r\n"
			"<Response>\r\n"
			"<CmdType>%s</CmdType>\r\n"
			"<SN>%s</SN>\r\n"
			"<DeviceID>%s</DeviceID>\r\n"
			"<PresetList Num=\"0\">\r\n"
			//"<Item>\r\n"
			//"<PresetID></PresetID>\r\n"
			//"<PresetName></PresetName>\r\n"
			//"</Item>\r\n"
			"</PresetList>\r\n"
			"</Response>\r\n", pUnifiedMsg->GetQueryType(), pUnifiedMsg->GetQuerySN(), pUnifiedMsg->GetDeviceID());

		memcpy(pResultBuf->GetBuffer(), xml, strlen(xml));

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);

		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询, 设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), strSN);

		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	else
	{
		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询失败，未知的设备ID:%s  SN:%s", pUnifiedMsg->GetDeviceID(), strSN);
		pUnifiedMsg->Free();
	}

	return 0;
}

UINT AFX_CDECL CDevInfo::pfnSearchAlarmRecordProc(LPVOID lParam)
{
	CDevInfo *pDevInfoCom = reinterpret_cast<CDevInfo *>(lParam);
	while (!pDevInfoCom->m_bIsExit)
	{
		GUID guidStream;
		GUID gudiNVR;

		auto pUnifiedMsg = pDevInfoCom->m_oDevAlarmRecord.GetSearchMessage();

		if (NULL == pUnifiedMsg)
		{
			pDevInfoCom->m_oDevAlarmRecord.WaitSearchStart();
		}
		else
		{
			//TODO
			auto param = static_cast<XmlParam_t *>(pUnifiedMsg->GetCmdParam());
			const char *StartAlarmPriority = param->strParamVal1;
			const char *EndAlarmPriority = param->strParamVal2;
			const char *AlarmMethod = param->strParamVal3;
			const char *StartAlarmTime = param->strParamVal4;
			const char *EndAlarmTime = param->strParamVal5;

			DATE tmStart;
			DATE tmEnd;

			SipUnified::GB28181TimeToDATE(StartAlarmTime, tmStart);
			SipUnified::GB28181TimeToDATE(EndAlarmTime, tmEnd);

			CString	strStreamID;
			CString strNVRID;

			Utils::GUIDToCString(guidStream, strStreamID, FALSE);
			Utils::GUIDToCString(gudiNVR, strNVRID, FALSE);

			CLog::Log(SDKCOM, LL_NORMAL, "%s 告警查询: 开始时间: %s 结束时间: %s StartAlarmPriority = %s EndAlarmPriority = %s AlarmMethod = %s\r\n", __FUNCTION__, StartAlarmTime, EndAlarmTime, StartAlarmPriority, EndAlarmPriority, AlarmMethod);
			pDevInfoCom->m_oDevAlarmRecord.WaitSearchFinish();
		}
	}
	return 0;
}

//处理报警查询，先去查询缓存
int CDevInfo::HandleAlarmQuery(CModMessage * pUnifiedMsg)
{
	DeviceAlarmInfo devAlarmInfo;
	auto bFind = CDataStore::LookupDeviceAlarmStatus(pUnifiedMsg->GetDeviceID(), devAlarmInfo, FALSE);

	if (!bFind)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "告警信息查询失败，未知的GB28181设备ID：%s", pUnifiedMsg->GetDeviceID());
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return -1;
	}

	AlarmRecordsMgr alarmInfoMgr;
	alarmInfoMgr.SetSearchMessage(pUnifiedMsg);
	alarmInfoMgr.SetDeviceName("test_alarm");
	alarmInfoMgr.SetDeviceID(pUnifiedMsg->GetDeviceID());


	auto pRecordbuf = m_oDevAlarmRecord.GetRecordBuf();
	pRecordbuf->FromDeviceAlarmInfo(devAlarmInfo);
	pRecordbuf->m_strDeviceName = "honeywell_ipc";
	alarmInfoMgr.InserRecord(pRecordbuf);

	SendAlarmRecordQueryResult(alarmInfoMgr);

	return 0;
}

int CDevInfo::SendAlarmRecordQueryResult(AlarmRecordsMgr& oRecord)
{
	auto pUnifiedMsg = oRecord.GetSearchMessage();
	CString			strDeviceID = pUnifiedMsg->GetDeviceID();

	CLog::Log(SDKCOM, LL_NORMAL, "告警查询完成 设备ID:%s SN:%s 数量:%d", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN(), oRecord.GetRecordCount());
	auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();

	// 把缓存挂载到消息中
	pUnifiedMsg->SetSearchData(reinterpret_cast<DeviceBasicObject *>(pResultBuf));

	DeviceBasicObject::InfoContext_t	tContext;
	CModMessage			oUnifiedpMsg;
	CString					strSN = pUnifiedMsg->GetQuerySN();

	oRecord.SetDeviceID(strDeviceID);

	// 向缓存中写入XML格式的查询结果数据
	while (0 < oRecord.GetCatalogBodyContent(pResultBuf->GetBuffer(), pResultBuf->GetBufferLen(), strSN.GetString(), &tContext))
	{
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
		pUnifiedMsg->SetDeviceID(strDeviceID);

		// 发送到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);

		pResultBuf = m_MemAllocator.AllocBodyContentBuf();
		pUnifiedMsg = m_MemAllocator.AllocModMessage();
		// 把缓存挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
	}

	pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);

	// 发送到SIPCom模块
	CRouter::PushMsg(SIPCOM, pUnifiedMsg);

	oRecord.RemoveHeadSearchMessage();
	oRecord.SetSearchFinish();

	return 0;
}