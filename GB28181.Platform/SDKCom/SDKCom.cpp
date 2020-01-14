#include "StdAfx.h"
#include "ServerConsole.h"
#include "SDKCom.h"
#ifdef SDK_VERSION_5_0
#include "SDKRecordSearcher.h"
#endif

using namespace std;
CSDKCom::CSDKCom(void)
	:m_oRecordMemMgr(sizeof(VideoRecordItem), MEMPOOL_BLOCK_SUM),
	m_pShareUpdateMsg(nullptr)
{
	this->m_DsEventClient.SetContext(this, &m_VmsSiteProxy);
	this->m_DeviceController.SetContext(this, &m_VmsSiteProxy, &m_MemAllocator);
	this->m_DeviceInfoMgr.SetContext(this, &m_VmsSiteProxy);
}

#pragma region Life Circle Stuff

void CSDKCom::Init(void)
{
	m_ECClient.SetParent(&m_DeviceInfoMgr);
	ReadConfig();

	//husDeviceController.GetDecoderInfo();  //pjs add
	try
	{
		CLog::Log(SDKCOM, LL_NORMAL, "SynClient初始化");
		if (0 != InitSDKInterface())
			exit(0);
		CreateECClient();
	}
	catch (const std::exception& e)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "SynClient初始化失败: %s", e.what());
		exit(0);
	}

	// 注册Module消息处理函数
	RegisterProc(pfnQueueProc, this, 1);
	RegisterProc(pfnUpdateData, this, 1);
	//RegisterProc(pfnReportAlarmProc, this, 1);
	RegisterProc(pfnSearchVideoRecordProc, this, 1);
}

// 读取配置文件
int CSDKCom::ReadConfig()
{
	//进程间通信,共享内存配置
	m_pShareUpdateMsg = new CSharedVarQueue(appConf.m_SharedMemory.DevMgmtProc_To_MainProc.str_Name);
	m_pShareUpdateMsg->Init(MB2B(DEFAULT_SHARED_MEMORY_SIZE_MB), false);
	return 0;
}
int CSDKCom::InitSDKInterface()
{
	//连接站点获取设备数据信息

	if (!m_VmsSiteProxy.InitSynClient(this)) return FALSE;

	CLog::Log(SDKCOM, LL_NORMAL, "初始Site同步对象完成，下一步获取站点全部设备信息!");

	//绑定初始化callback事件
	auto callback = std::bind(&DevicesInfoMgr::IniDeviveProperty, &m_DeviceInfoMgr, std::placeholders::_1);
	m_VmsSiteProxy.SetInitDeviceItemCallBack(callback);
	if (!m_VmsSiteProxy.GetAllDeviceFromSite()) return FALSE;

	CLog::Log(SDKCOM, LL_NORMAL, "获取站点全部设备信息，完成!");

	m_DsEventClient.RegisterToSiteEventsSource();

	m_DsEventClient.InitRecordSearchContext();

	// 通知DevInfo模块，初始化站点设备
	auto gatewayid = m_VmsSiteProxy.GetGatewayStrGUID();
	NoticeDevInfo(gatewayid, event_notify_t::e_update_t::ut_init);

	//get the site ip here.
	//this->m_VmsVideoRecordMgr.initialize(m_VmsSiteProxy.GetSiteIP());

	return 0;
}

void CSDKCom::CreateECClient()
{
	ECInfoMap &refChannelMap = m_DeviceInfoMgr.GetECToDevMap();

	CString strID;
	CString strDeviceGUID;
	CString strECIP;
	CString strECPort;
	GUID guidEC;

	refChannelMap.ManualLock();
	POSITION pos = refChannelMap.GetStartPos();

	while (pos)
	{
		refChannelMap.GetNext(pos, strDeviceGUID, guidEC);
		// 取得EC的IP和端口
		m_DeviceInfoMgr.GetSettingsParam(guidEC, L"IP", strECIP);
		m_DeviceInfoMgr.GetSettingsParam(guidEC, L"Port", strECPort);

		if (strECIP.IsEmpty() || strECPort.IsEmpty()) continue;;

		CString strECGUID;
		Utils::GUIDToCString(guidEC, strECGUID, FALSE);
		if (!m_ECClient.CheckConection(strECGUID))
		{
			CLog::Log(SDKCOM, LL_NORMAL, "创建EC客户端 GUID:%s IP:%s PORT:%s strDeviceGUID：%s", strECGUID, strECIP, strECPort, strDeviceGUID);
			// 创建EC控制客户端
			long nPort = atoi(strECPort);
			auto INT_RES = m_ECClient.AddServer(CString(strECIP), nPort, strECGUID);
			if (FALSE_FAIELD == INT_RES) {
				CLog::Log(SDKCOM, LL_NORMAL, "Add EC Server Faield: %s ,GUID %s strDeviceGUID：%s", strECIP, strECGUID, strDeviceGUID);
				continue;
			}
			else if (TRUE_OK == INT_RES) {
				CLog::Log(SDKCOM, LL_NORMAL, "Add EC Server OK: %s ,GUID %s strDeviceGUID：%s", strECIP, strECGUID, strDeviceGUID);
			}
			INT_RES = m_ECClient.Connect(CString(""), CString(""), strECGUID);
			if (FALSE_FAIELD == INT_RES) {
				CLog::Log(SDKCOM, LL_NORMAL, "Connect EC Server Faield: %s ,GUID %s strDeviceGUID：%s", strECIP, strECGUID, strDeviceGUID);
				continue;
			}
			else if (TRUE_OK == INT_RES) {
				CLog::Log(SDKCOM, LL_NORMAL, "Connect EC Server OK: %s ,GUID %s strDeviceGUID：%s", strECIP, strECGUID, strDeviceGUID);
			}
		}

		auto  INT_RES = m_ECClient.ListenTo(strDeviceGUID, TRUE, strECGUID);
		if (FALSE_FAIELD == INT_RES) {
			CLog::Log(SDKCOM, LL_NORMAL, "ListenTo Failed: %s", strDeviceGUID);
			continue;
		}
		else if (TRUE_OK == INT_RES) {
			CLog::Log(SDKCOM, LL_NORMAL, "ListenTo OK: %s", strDeviceGUID);
		}
	}
	refChannelMap.RemoveAll(FALSE);
	refChannelMap.ManualUnlock();
	CLog::Log(SDKCOM, LL_NORMAL, "EC客户端创建完成。");
}

BOOL CSDKCom::ConnectToEC(GUID guidEC)
{
	if (m_listConnectedEC.Find(guidEC)) { // already connected
		return TRUE;
	}
	CString strECIP;
	CString strECPort;
	m_DeviceInfoMgr.GetSettingsParam(guidEC, L"IP", strECIP);
	m_DeviceInfoMgr.GetSettingsParam(guidEC, L"Port", strECPort);

	CString strECGUID;
	Utils::GUIDToCString(guidEC, strECGUID, FALSE);

	if (strECIP.IsEmpty() || strECPort.IsEmpty()) {
		CLog::Log(SDKCOM, LL_NORMAL, "Fail to connect to EC: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);
		return FALSE;
	}

	CLog::Log(SDKCOM, LL_DEBUG, "Connect to EC: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);

	// 创建EC控制客户端
	long nPort = atoi(strECPort);
	if (!m_ECClient.AddServer(strECIP, nPort, strECGUID)) {
		CLog::Log(SDKCOM, LL_NORMAL, "Fail to add EC Server: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);
		return FALSE;
	}
	CLog::Log(SDKCOM, LL_DEBUG, "AddServer Success: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);

	if (!m_ECClient.Connect(CString(""), CString(""), strECGUID)) {
		CLog::Log(SDKCOM, LL_NORMAL, "Connect faild: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);
		m_ECClient.RemoveServer(strECGUID);
		return FALSE;
	}
	CLog::Log(SDKCOM, LL_DEBUG, "Connect Success: GUID: %s; IP: %s; PORT: %s", strECGUID, strECIP, strECPort);

	m_listConnectedEC.AddTail(guidEC);
	return TRUE;
}

void CSDKCom::Cleanup(void)
{
	m_DeviceInfoMgr.Cleanup();
	m_VmsSiteProxy.ReleaseComObjects();
	if (m_pShareUpdateMsg)
		delete m_pShareUpdateMsg;
}

#pragma endregion

// 处理内存队列消息的函数
bool CSDKCom::HandleMsg(CMemPoolUnit * pUnit)
{
	auto pUnifiedMsg = reinterpret_cast<CModMessage *>(pUnit);
	auto	eOperateType = pUnifiedMsg->GetModAction();

	switch (eOperateType.action_sdk)
	{
		// 设备动作控制
	case mod_op_t::ot_sdk::ctrl_action:
		ProcDeviceCtrl(pUnifiedMsg);
		break;
		// 解码器播放请求
	case mod_op_t::ot_sdk::decoder_invite:
		ProcDecorderPlayRequest(pUnifiedMsg);
		break;

		// 解码器播放开始
	case mod_op_t::ot_sdk::decoder_ack:
		ProcDecorderPlayAck(pUnifiedMsg);
		break;
		// 停止解码器播放
	case mod_op_t::ot_sdk::decoder_bye:
		ProcDecorderStop(pUnifiedMsg);
		break;
		// 设备状态
	case mod_op_t::ot_sdk::search_status:
		ProcStatusSearch(pUnifiedMsg);
		break;
		// 手动录像，更新记录
	case mod_op_t::ot_sdk::update_video_record:
		ProcUpdateRecord(pUnifiedMsg);
		break;
	case mod_op_t::ot_sdk::search_record:
		ProcRecordSearch(pUnifiedMsg);
		break;
		// 平台保活失败，停止报警
	case mod_op_t::ot_sdk::stop_alarm:
		m_DeviceInfoMgr.RemoveAlarming();
		break;

	case mod_op_t::ot_sdk::search_config:
		ProcConfigSearch(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::device_config:
		ProcDeviceConfig(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::search_realplayurl:
	case mod_op_t::ot_sdk::search_playbackurl:
		ProcUrlQuery(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::stopPlayUrl:
		ProcStopPlayUrl(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::DecoderStatus:
		ProcDecoderStatus(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::DecoderInfo:
		ProcDecoderInfo(pUnifiedMsg);
		break;

	case mod_op_t::ot_sdk::DecoderDivision:
		ProcDecoderDivision(pUnifiedMsg);
		break;
	case mod_op_t::ot_sdk::subscribe_alarm:
		//ProcAlarmSubscribe(pUnifiedMsg);
	default:
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		break;
	}

	return true;
}

// 处理数据更新线程
UINT AFX_CDECL CSDKCom::pfnUpdateData(LPVOID lParam)
{
	CSDKCom *pSDKCom = reinterpret_cast<CSDKCom*>(lParam);
	TCHAR	szGUID[GUID_BUF_LEN];
	TCHAR	*pData = nullptr;

	while (!pSDKCom->m_bIsExit)
	{
		if (WAIT_TIMEOUT != WaitForSingleObject(pSDKCom->m_pShareUpdateMsg->GetEventHandle(), 1000))
		{
			while (pSDKCom->m_pShareUpdateMsg->PopAlloc(reinterpret_cast<void**>(&pData)))
			{
				USHORT nSize = 0;
				memcpy(&nSize, pData, sizeof(nSize));

				// 读取消息内容
				if (GUID_LEN > nSize)
				{
					memcpy(szGUID, pData + sizeof(nSize), nSize);
				}
				else
				{
					memcpy(szGUID, pData + sizeof(nSize), GUID_LEN);
					szGUID[GUID_LEN] = 0;
				}
				pSDKCom->m_pShareUpdateMsg->PopDone(pData);
				CString strGUID = szGUID;
				strGUID.MakeLower();

				pSDKCom->NoticeDevInfo(strGUID.GetString(), event_notify_t::e_update_t::ut_mod);
				CLog::Log(SDKCOM, LL_NORMAL, _T("更新设备GB信息 设备GUID:%s"), strGUID);
				pSDKCom->m_DeviceInfoMgr.UpdateGBDeviceIDByGUID(strGUID.GetString());
			}
		}
	}

	return 0;
}

// 处理设备控制
int CSDKCom::ProcDeviceCtrl(CModMessage * pUnifiedMsg)
{
	return m_DeviceController.ProcDeviceCtrl(pUnifiedMsg);
}

int CSDKCom::ProcDecorderPlayRequest(CModMessage * pUnifiedMsg)
{
	CDevSDP			oDevSDP;
	HUSDeviceConnect_T	*pInfo = nullptr;
	if (m_DeviceInfoMgr.DecoderLookup(pUnifiedMsg->GetDeviceID(), pInfo))
	{
		oDevSDP.SetMediaIP(pInfo->strDevIP);
		oDevSDP.SetMediaPort(pInfo->strDevPort);
		m_DeviceInfoMgr.DecoderLookupEnd();

		auto pSDP = m_MemAllocator.AllocBodyContentBuf();
		oDevSDP.SetSessionName("RTSP Session");
		oDevSDP.SetDeviceID(pUnifiedMsg->GetDeviceID());

		oDevSDP.GetCatalogBodyContent(pSDP->GetBuffer(), pSDP->GetBufferLen(), nullptr, nullptr);

		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::invite_reponse);
		pUnifiedMsg->SetPlayData(pSDP);

		CLog::Log(SDKCOM, LL_NORMAL, "解码器播放请求 GB28181设备ID:%s", pUnifiedMsg->GetDeviceID());
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	else
	{
		m_DeviceInfoMgr.DecoderLookupEnd();
		CLog::Log(SDKCOM, LL_NORMAL, "解码器播放请求失败 未知的GB28181设备ID：%s", pUnifiedMsg->GetDeviceID());
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
		return -1;
	}
	return 0;
}
int CSDKCom::ProcDecorderPlayAck(CModMessage * pUnifiedMsg)
{
	DecoderPairInfo_t oDecoderPairInfo;
	if (0 == m_DeviceInfoMgr.DecoderPairRemove(pUnifiedMsg->GetDeviceID(), oDecoderPairInfo))
	{
		// 发送Decoder播放指令到EC
		m_DeviceController.PostDecoderCommand(oDecoderPairInfo.guidDecoder, oDecoderPairInfo.guidEncoder, oDecoderPairInfo.guidEC, SI_DECODER_PLAY, oDecoderPairInfo.strchannelIndex);
		m_DeviceInfoMgr.DecoderBind(oDecoderPairInfo.guidDecoder, oDecoderPairInfo.guidEncoder);
		CLog::Log(SDKCOM, LL_NORMAL, "解码器播放开始 GB28181设备ID:%s", pUnifiedMsg->GetDeviceID());
	}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "解码器播放失败 无法匹配的GB28181解码器设备 ID:%s", pUnifiedMsg->GetDeviceID());
	}
	SAFE_FREE_MOD_MSG(pUnifiedMsg);
	return 0;
}

int CSDKCom::ProcDecorderStop(CModMessage * pUnifiedMsg)
{
	HUSDeviceConnect_T	*pInfo = nullptr;
	GUID	guidDecoder;
	GUID	guidEncoder;
	GUID	guidEC;
	CString strchannelIndex;
	CString strDeviceID = pUnifiedMsg->GetDeviceID();

	if (m_DeviceInfoMgr.DecoderLookup(strDeviceID, pInfo))
	{
		guidDecoder = pInfo->guidDevice;
		guidEncoder = pInfo->guidParent;
		guidEC = pInfo->guidEC;
		strchannelIndex = pInfo->strChannelNum;
	}
	m_DeviceInfoMgr.DecoderLookupEnd();

	if (pUnifiedMsg->GetPlayData())
	{
		CBigFile *pSDPFile = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
		pSDPFile->Free();
	}
	pUnifiedMsg->Free();

	if (!strchannelIndex.IsEmpty())
	{
		// 发送Decoder播放指令到EC
		m_DeviceController.PostDecoderCommand(guidDecoder, guidEncoder, guidEC, SI_DECODER_STOP, strchannelIndex);
		m_DeviceInfoMgr.DecoderUnbind(guidDecoder);
		CLog::Log(SDKCOM, LL_NORMAL, "停止解码器播放 GB28181解码器设备 ID：%s", strDeviceID);
	}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "停止解码器播放失败 无法匹配的GB28181解码器设备 ID：%s", strDeviceID);
		return -1;
	}
	return 0;
}

// 处理设备状态查询
int CSDKCom::ProcStatusSearch(CModMessage * pUnifiedMsg)
{
	DeviceObject	husObjectInfo;

	CString strCmdType = pUnifiedMsg->GetQueryType();
	CString strSN = pUnifiedMsg->GetQuerySN();
	CString strDeviceID = pUnifiedMsg->GetDeviceID();
	// 设备状态查询
	if (0 == strCmdType.Compare("DeviceStatus"))
	{
		if (m_DeviceInfoMgr.DeviceLookup(strDeviceID, husObjectInfo))
		{
			auto	pResultBuf = m_MemAllocator.AllocBodyContentBuf();
			// 把缓存挂载到消息中
			pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));

			time_t tmCur;
			time(&tmCur);
			tm tmInfo;
			localtime_s(&tmInfo, &tmCur);
			CString strCurTime;
			strCurTime.Format("%d-%02d-%02dT%02d:%02d:%02d", tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday, tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

			DeviceBasicObject::InfoContext_t tContext;
			// 向缓存中写入XML格式的查询结果数据
			CBodyBuilder builder;
			auto len = pResultBuf->GetBufferLen();
			builder.CreateStatusBody(pResultBuf->GetBuffer(), len, strSN, strDeviceID,
				"OK", "Online", "ON", "no", "no", strCurTime, 1, false);

			pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);

			CLog::Log(SDKCOM, LL_NORMAL, "设备状态查询 设备ID：%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());

			// 发送到SIPCom模块
			CRouter::PushMsg(SIPCOM, pUnifiedMsg);
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, "状态查询失败，未知的GB28181设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN());
			SAFE_FREE_MOD_MSG(pUnifiedMsg);
		}
	}
	// 其它查询
	else
	{
		// 转发到DevInfo模块处理
		pUnifiedMsg->SetModAction(mod_op_t::ot_devinfo::query_info);
		// 发送到DevInfo
		CRouter::PushMsg(DEVINFO, pUnifiedMsg);
	}

	return 0;
}

int CSDKCom::ProcRecordSearch(CModMessage *pUnifiedMsg)
{
	DeviceObject deviceInfo;
	if (m_DeviceInfoMgr.ChannelLookup(pUnifiedMsg->GetDeviceID(), deviceInfo))
	{
		//GUID guidDevice = deviceInfo.linked.guidParent;
		auto guidStreamer = deviceInfo.linked.guidStreamer;
		auto guidNVR = deviceInfo.linked.guidNVR;
		auto nvrIp = deviceInfo.linked.strNVRIP;
		pUnifiedMsg->eHusDevice = deviceInfo.eHUSDevType;

		strcpy_s(pUnifiedMsg->m_szNvrIP, nvrIp);
		m_DeviceInfoMgr.ChannelLookupEnd();
		auto pszStartTime = pUnifiedMsg->GetSearchStartTime();
		auto pszEndTime = pUnifiedMsg->GetSearchEndTime();
		DATE tmStart;
		DATE tmEnd;
		// int cur;
		auto curTime = CTime::GetCurrentTime();
		auto cur = curTime.GetTime();
		time_t	startTime;
		time_t	endTime;
		SipUnified::GB28181TimeToDATE(pszStartTime, tmStart);
		CLog::Log(SDKCOM, LL_NORMAL, " %s nvrIp = %s 4\r\n", __FUNCTION__, nvrIp);

		{
			int nYear = 0;
			int nMonth = 0;
			int nDay = 0;
			int nHour = 0;
			int nMin = 0;
			int nSec = 0;

			sscanf_s(pszStartTime, "%d-%d-%dT%d:%d:%d", &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec);
			CTime start(nYear, nMonth, nDay, nHour, nMin, nSec);
			startTime = start.GetTime();
		}

		CLog::Log(SDKCOM, LL_NORMAL, " %s nvrIp = %s 5\r\n", __FUNCTION__, nvrIp);
		SipUnified::GB28181TimeToDATE(pszEndTime, tmEnd);
		{
			int nYear = 0;
			int nMonth = 0;
			int nDay = 0;
			int nHour = 0;
			int nMin = 0;
			int nSec = 0;
			sscanf_s(pszEndTime, "%d-%d-%dT%d:%d:%d", &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec);
			CTime end(nYear, nMonth, nDay, nHour, nMin, nSec);
			endTime = end.GetTime();
		}

		CLog::Log(SDKCOM, LL_NORMAL, " %s  6\r\n", __FUNCTION__);
		if (startTime >= cur)
		{
			CLog::Log(SDKCOM, LL_NORMAL, " %s deviceId = %s 录像文件时间: startTime = %s orginEndTime = %s time out cur return 0\r\n", __FUNCTION__, pUnifiedMsg->GetDeviceID(), pszStartTime, pszEndTime);
			auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();

			// 把缓存挂载到消息中
			pUnifiedMsg->SetSearchData(reinterpret_cast<DeviceBasicObject *>(pResultBuf));
			//  CInfo::InfoContext_t	tContext;
			CModMessage  oUnifiedpMsg;
			CBodyBuilder m_oBodyBuilder;
			auto strSN = pUnifiedMsg->GetQuerySN();
			auto strDeviceName = deviceInfo.linked.strDeviceName;
			auto nCompleteLen = pResultBuf->GetBufferLen();
			CString strDeviceID = pUnifiedMsg->GetDeviceID();

			// 向缓存中写入XML格式的查询结果数据
			m_oBodyBuilder.CreateRecordHead(pResultBuf->GetBuffer(), nCompleteLen, strSN, strDeviceID.GetString(), strDeviceName.GetString(), 0, 0);

			// 添加录像文件xml的文件尾
			m_oBodyBuilder.CompleteRecord(pResultBuf->GetBuffer() + nCompleteLen, pResultBuf->GetBufferLen() - nCompleteLen);

			pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);
			pUnifiedMsg->SetDeviceID(strDeviceID);

			// 发送到SIPCom模块
			CRouter::PushMsg(SIPCOM, pUnifiedMsg);

			return 0;
		}

		if (endTime > cur || endTime <= 0)
		{
			tmEnd = static_cast<DATE>(cur);
			//  char tmpBuf[128] ={0};
			CString curTimeStr = curTime.Format("%Y-%m-%dT%H:%M:%S");;
			//   VideoRecordItem::DATEToGB28181Time(tmpBuf, tmEnd);
			pUnifiedMsg->SetSearchEndTime(curTimeStr);
			CLog::Log(SDKCOM, LL_NORMAL, " %s deviceId = %s 录像文件时间: startTime = %s orginEndTime = %s curendTime = %s", __FUNCTION__, pUnifiedMsg->GetDeviceID(), pszStartTime, pszEndTime, curTimeStr);
		}

		//查询消息带到查询队列中，不需删除
		m_VideoRecordMgr.SetSearchMessage(pUnifiedMsg, guidStreamer, guidNVR);
		m_VideoRecordMgr.SetDeviceName(deviceInfo.linked.strDeviceName);
		m_VideoRecordMgr.SetSearchStart();
	}
	else
	{
		m_DeviceInfoMgr.ChannelLookupEnd();
		CLog::Log(SDKCOM, LL_NORMAL, "录像文件查询失败，未知的GB28181设备ID：%s", pUnifiedMsg->GetDeviceID());
		pUnifiedMsg->Free();
	}

	return 0;
}

UINT AFX_CDECL CSDKCom::pfnSearchVideoRecordProc(LPVOID lParam)
{
	CSDKCom *pSDKCom = reinterpret_cast<CSDKCom *>(lParam);
	while (!pSDKCom->m_bIsExit)
	{
		GUID guidStream;
		GUID guidNVR;
		CModMessage *pUnifiedMsg = pSDKCom->m_VideoRecordMgr.GetSearchMessage(guidStream, guidNVR);
		if (NULL == pUnifiedMsg)
		{
			pSDKCom->m_VideoRecordMgr.WaitSearchStart();
		}
		else
		{
			auto pszStartTime = pUnifiedMsg->GetSearchStartTime();
			auto pszEndTime = pUnifiedMsg->GetSearchEndTime();
			auto eRecordType = pUnifiedMsg->GetRecordType();
			// const char *pszRecLocation = pUnifiedMsg->GetRecLocation();
			//TODO

			DATE tmStart = 0;
			DATE tmEnd = 0;

			SipUnified::GB28181TimeToDATE(pszStartTime, tmStart);
			SipUnified::GB28181TimeToDATE(pszEndTime, tmEnd);

			CString strGUIDStreamID;
			CString strNVRID;

			Utils::GUIDToCString(guidStream, strGUIDStreamID, FALSE);
			Utils::GUIDToCString(guidNVR, strNVRID, FALSE);

			//pSDKCom->m_VideoRecordMgr.SetDeviceName(pszDeviceName);
			CLog::Log(SDKCOM, LL_NORMAL, "开始录像文件查询 设备ID:%s SN:%s StreamGUID:%s NVR_GUID:%s 录像类型:%d 开始时间:%s 结束时间:%s",
				pUnifiedMsg->GetDeviceID(), pUnifiedMsg->GetQuerySN(), strGUIDStreamID, strNVRID, eRecordType, pszStartTime, pszEndTime);
#ifdef SDK_VERSION_5_0
			auto husDeviceType = pUnifiedMsg->GetDeviceType();
			VideoRecordSearcher videoRecordSearcher;
			videoRecordSearcher.SetContext(&pSDKCom->m_DsEventClient, pSDKCom, &pSDKCom->m_VmsSiteProxy);
			videoRecordSearcher.InitVideoSearch(guidStream, husDeviceType);
			videoRecordSearcher.SearchRecord(tmStart, tmEnd, 1);
#else
			pSDKCom->m_VmsSiteProxy.RefreshDataIfNeed(strGUIDStreamID.GetString());
			pSDKCom->m_VmsSiteProxy.m_pIVideoSearch->SearchVideo_2(guidStream, guidNVR, eRecordType, tmStart, tmEnd, 300);
#endif // SDK_VERSION_5_0
			pSDKCom->m_VideoRecordMgr.WaitSearchFinish();

		}
	}
	return 0;
}

// 转发更新消息
void CSDKCom::NoticeDevInfo(
	const char *pszGUID,
	event_notify_t::e_update_t eUpdateType,
	const char *pszDeviceID,
	mod_op_t::ot_devinfo devOperateType)
{
	// 通知DevInfo模块更新数据
	// 生成模块消息对象
	auto pUnifiedMsg = m_MemAllocator.AllocModMessage();
	pUnifiedMsg->SetModAction(devOperateType);
	if (pszDeviceID != nullptr)
	{
		pUnifiedMsg->SetDeviceID(pszDeviceID);
	}
	pUnifiedMsg->SetUpdataGUID(pszGUID);
	pUnifiedMsg->SetNotifyUpdataType(eUpdateType);
	CRouter::PushMsg(DEVINFO, pUnifiedMsg);
}

int CSDKCom::ProcConfigSearch(CModMessage * pUnifiedMsg)
{
	DeviceObject deviceInfo;

	CString	strSN = pUnifiedMsg->GetQuerySN();
	XmlParam_t *szParam = static_cast<XmlParam_t*>(pUnifiedMsg->GetCmdParam());
	CString strConfigType = szParam->strParamVal1;
	delete szParam;

	if (m_DeviceInfoMgr.DeviceLookup(pUnifiedMsg->GetDeviceID(), deviceInfo))
	{
		CString strGUID = deviceInfo.strDeviceGUID;
		auto pResultBuf = m_MemAllocator.AllocBodyContentBuf();

		char *pSearchBuf = pResultBuf->GetBuffer();
		int nBufLen = pResultBuf->GetBufferLen();
		int nBufUseLen = nBufLen;
		int nBufCount = 0;

		CBodyBuilder::CreateConfigSearchHead(pSearchBuf, nBufUseLen,
			pUnifiedMsg->GetQuerySN(), pUnifiedMsg->GetDeviceID(), "OK");

		nBufCount += nBufUseLen;
		nBufUseLen = nBufLen - nBufCount;
		if (strConfigType.Find("BasicParam") != -1)
		{
			CBodyBuilder::AddBasicParamConfig(pSearchBuf + nBufCount, nBufUseLen,
				deviceInfo.strName,
				pUnifiedMsg->GetDeviceID(),
				appConf.m_Current.str_ID,
				appConf.m_Current.str_IP,
				std::to_string(appConf.m_Current.nPort).c_str(),
				appConf.m_Current.str_Name,
				std::to_string(appConf.m_Current.ExpiryTime).c_str(),
				appConf.m_Current.str_Password,
				std::to_string(appConf.m_Current.KeepAliveInterval).c_str(),
				"3");

			nBufCount += nBufUseLen;
			nBufUseLen = nBufLen - nBufCount;
		}
		if (strConfigType.Find("VideoParamOpt") != -1)
		{
			//sdk
		}
		if (strConfigType.Find("VideoParamConfig") != -1)
		{
			//sdk
		}
		if (strConfigType.Find("AudioParamOpt") != -1)
		{
			//sdk
		}
		if (strConfigType.Find("AudioParamConfig") != -1)
		{
			//sdk
		}
		CBodyBuilder::CompleteConfigSearch(pSearchBuf + nBufCount, nBufUseLen);

		// 设置操作类型
		pUnifiedMsg->SetModAction(mod_op_t::ot_sipcom::query_result);

		// 把查询结果挂载到消息中
		pUnifiedMsg->SetSearchData(reinterpret_cast<void *>(pResultBuf));
		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询, 设备ID:%s SN:%s", pUnifiedMsg->GetDeviceID(), strSN);

		// 转发消息到SIPCom模块
		CRouter::PushMsg(SIPCOM, pUnifiedMsg);
	}
	else
	{
		CLog::Log(DEVINFO, LL_NORMAL, "设备信息查询失败，未知的设备ID:%s  SN:%s", pUnifiedMsg->GetDeviceID(), strSN);
		SAFE_FREE_MOD_MSG(pUnifiedMsg);
	}

	return 0;
}

int CSDKCom::ProcDeviceConfig(CModMessage * pUnifiedMsg) const
{
	return m_DeviceController.ProcDeviceConfig(pUnifiedMsg);
}

int CSDKCom::ProcUrlQuery(CModMessage* pUnifiedMsg)
{
	return m_DeviceController.ProcUrlQuery(pUnifiedMsg);
}

int CSDKCom::ProcStopPlayUrl(CModMessage* pUnifiedMsg)
{
	return m_DeviceController.ProcStopPlayUrl(pUnifiedMsg);
}

int CSDKCom::ProcDecoderStatus(CModMessage* pUnifiedMsg) const
{
	return m_DeviceController.ProcDecoderStatus(pUnifiedMsg);
}

int CSDKCom::ProcDecoderInfo(CModMessage* pUnifiedMsg) const
{
	return m_DeviceController.ProcDecoderInfo(pUnifiedMsg);
}

int CSDKCom::ProcDecoderDivision(CModMessage* pUnifiedMsg)
{
	return m_DeviceController.ProcDecoderDivision(pUnifiedMsg);
}

int CSDKCom::ProcUpdateRecord(CModMessage* pUnifiedMsg)
{
	return m_DeviceController.UpdateWebRecord(pUnifiedMsg);
}