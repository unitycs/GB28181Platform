// ServerConsole.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Main/WinService.h"
#include "ServerConsole.h"
#include "Json/json.hpp"
using json = nlohmann::json;
#include <iostream>
#include <fstream>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#ifdef WIN_SERVICE
// 隐藏窗口
#pragma   comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

//CComModule _Module;
// 唯一的应用程序对象
//CWinApp theApp;
APP_SETTING_T	appConf;
DEVICE_INFO_SETTING_T devInfoConf;

auto read_domain_info(json & item,bool needProt2)
{
	APP_SETTING_T::PLT_CONFIG_INFO_T infoitem;

	infoitem.str_Name = item["name"].get<std::string>().c_str();
	infoitem.str_ID = item["device_id"].get<std::string>().c_str();
	infoitem.str_IP = item["ip"].get<std::string>().c_str();
	auto username = item["username"].get<std::string>();
	infoitem.str_Username = username.c_str();
	if (username.empty())
	{
		infoitem.str_Username = infoitem.str_ID;
		_tprintf("配置信息读取:本地UserName为空, 使用本地网关id,作为用户名。");
	}
	infoitem.str_Password = item["password"].get<std::string>().c_str();
	infoitem.KeepAliveInterval = item["keepalive"].get<unsigned>();
	infoitem.ExpiryTime = item["expiry"].get<unsigned>();
	infoitem.nPort = item["port"].get<u_short>();
	if (needProt2)
	{
		infoitem.nPort2 = item["port2"].get<u_short>();
	}
	return infoitem;
}

int ParserProjectSetting(const CString & strConfigpath)
{
	if (strConfigpath.IsEmpty()) return -1;
	json json_info;
	std::ifstream(strConfigpath) >> json_info;

	//check empty
	assert(!json_info.empty());

	//读取SipCom的工作模式：0下级1上级2路由
	appConf.nSipComMode = json_info["app_work_mode"].get<unsigned>();
	//读取中文字符的编码方式
	appConf.strChsEncoding = json_info["app_encoding"].get<std::string>().c_str();

	// 读取日志配置
	auto jsmain = json_info["main"];

	auto jslog = jsmain["loginfo"];
	appConf.m_LogInfo.strRootDir = appConf.strModulePath + jslog["sipcom_root_dir"].get<std::string>().c_str();
	appConf.m_LogInfo.nLoglevel = jslog["level"].get<unsigned>();
	appConf.m_LogInfo.nLogfilesize = jslog["max_size"].get<unsigned>();
	appConf.m_LogInfo.nKeeptime = jslog["keeptime"].get<unsigned>();
	appConf.m_LogInfo.strCurDirName = jslog["cur_dir"].get<std::string>().c_str();
	appConf.m_LogInfo.strTodayDirName = jslog["today_dir"].get<std::string>().c_str();
	appConf.m_LogInfo.strHistoryDirName = jslog["history_dir"].get<std::string>().c_str();

	//读取设备配置环境信息
	auto jsdeviceinfo = jsmain["devices_config_files_info"];
	appConf.strDevConfDir = appConf.strModulePath + jsdeviceinfo["config_dir_name"].get<std::string>().c_str();
	appConf.strDevInfoConfPath = appConf.strDevConfDir + _T("\\") + jsdeviceinfo["hus_devices_config"].get<std::string>().c_str();

#ifdef DEBUG
	_tprintf("\r\n-----------------开始本地域平台信息读取-----------------\r\n");
#endif // DEBUG

	//本地平台信息
	auto jscur = jsmain["current_domian"];
	appConf.m_Current = read_domain_info(jscur,true);

#ifdef DEBUG
	_tprintf("\r\n-----------------开始上级域平台信息读取-----------------\r\n");
#endif // DEBUG
	// 上级域平台信息
	for (auto& item : json_info["upper_domian"])
	{
		auto infoitem = read_domain_info(item,false);
		appConf.m_UpperList.emplace_back(std::move(infoitem));
	}
#ifdef DEBUG
	_tprintf("\r\n-----------------开始下级域平台信息读取-----------------\r\n");
#endif // DEBUG
	//下级域平台信息
	for (auto& item : json_info["lower_domian"])
	{
		auto infoitem = read_domain_info(item,false);
		appConf.m_LowerList.emplace_back(std::move(infoitem));
	}
	//下载配置
	appConf.m_DownLoadConfig.n_Rate = jsmain["network"]["download_rate"].get<unsigned>();
	//防火墙IP
	appConf.m_Firewall.str_IP = jsmain["network"]["firewall_ip"].get<std::string>().c_str();
	appConf.m_Firewall.nPort = jsmain["network"]["firewall_port"].get<unsigned>();

	//进程间通信的配置
	auto ipcsm = jsmain["sharedmemory"];
	appConf.m_SharedMemory.Sipcom_to_App.str_Name = ipcsm["sipcom_to_app"].get<std::string>().c_str();
	appConf.m_SharedMemory.Sipcom_to_App.uint_Size = ipcsm["nsize"].get<unsigned>();
	appConf.m_SharedMemory.App_to_Sipcom.str_Name = ipcsm["app_to_sipcom"].get<std::string>().c_str();;
	appConf.m_SharedMemory.App_to_Sipcom.uint_Size = ipcsm["nsize"].get<unsigned>();

	return 0;
}

int ParserDeviceInfoSetting(const CString &  strDevInfoConfPath)
{
	if (strDevInfoConfPath.IsEmpty()) return -1;

	json json_info;
	std::ifstream(strDevInfoConfPath.GetString()) >> json_info;

	//check empty
	assert(!json_info.empty());

	//解析id序列的生成规律

	auto id_generator = json_info["id_generator_rules"];
	devInfoConf.m_GBIDCreatorInfo.b_number_only = id_generator["number_only"].get<bool>();
	devInfoConf.m_GBIDCreatorInfo.b_letter_only = id_generator["letter_ony"].get<bool>();
	devInfoConf.m_GBIDCreatorInfo.b_re_create = id_generator["re_create"].get<bool>();
	devInfoConf.m_GBIDCreatorInfo.str_local_civil_mask = id_generator["civil_mask"].get<std::string>();

	//检测为空
	for (auto &item : id_generator)
	{
		if (item.is_string())
		{
			if (item.get<std::string>().empty())
			{
				item = "0";
			}
		}
	}

	devInfoConf.m_GBIDCreatorInfo.str_last_alarm_id = id_generator["last_ipc_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_dvr_id = id_generator["last_dvr_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_camera_id = id_generator["last_camera_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_nvr_id = id_generator["last_nvr_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_alarm_id = id_generator["last_alarm_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_encord_id = id_generator["last_encord_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_decord_id = id_generator["last_decord_id"].get<std::string>();
	devInfoConf.m_GBIDCreatorInfo.str_last_decord_channel_id = id_generator["last_decord_channel_id"].get<std::string>();

	//解析目录查询时，将要推送的设备类别
	for (auto& item : json_info["catalog_push"])
	{
		devInfoConf.m_CatalogPush.emplace(item.get<std::string>(), item.get<std::string>().c_str());
	}

	//摄像机配置
	devInfoConf.m_CameraConfig.videoWidth_Base = 1920;
	devInfoConf.m_CameraConfig.videoHeight_Base = 1080;

	//解析设备基本信息
	auto deviceinfo_default = json_info["deviceinfo_default"];
	devInfoConf.m_DeviceBaseInfo.strName = deviceinfo_default["Name"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strModel = deviceinfo_default["Model"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strFirmware = deviceinfo_default["Firmware"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strMaxCam = deviceinfo_default["Maxcam"].get<unsigned>();
	devInfoConf.m_DeviceBaseInfo.strMaxAlarm = deviceinfo_default["Maxalarm"].get<unsigned>();
	devInfoConf.m_DeviceBaseInfo.strManufacture = deviceinfo_default["Manufacture"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strAddress = deviceinfo_default["Address"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strCivilCode = deviceinfo_default["CivilCode"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strOwner = deviceinfo_default["Owner"].get<std::string>().c_str();
	devInfoConf.m_DeviceBaseInfo.strParental = deviceinfo_default["Parental"].get<unsigned>();
	devInfoConf.m_DeviceBaseInfo.strSafetyWay = deviceinfo_default["SafetyWay"].get<unsigned>();
	devInfoConf.m_DeviceBaseInfo.strRegisterWay = deviceinfo_default["RegisterWay"].get<unsigned>();
	devInfoConf.m_DeviceBaseInfo.strSecrecy = deviceinfo_default["Secrecy"].get<unsigned>();

	//解析设备类型
	for (auto& item : json_info["device_typedef"])
	{
		auto node1 = item[0].get<std::string>();
		auto node2 = item[1].get<std::string>();
		transform(node1.begin(), node1.end(), node1.begin(), ::toupper);
		devInfoConf.m_DeviceTypeDef.emplace(node1, node2);
	}

	//解析虚拟目录
	for (auto& item : json_info["virtual_dir"])
	{
		auto node1 = ws_utf8_cvt_t().from_bytes(item[0].get<std::string>());
		auto node2 = item[1].get<std::string>();
		devInfoConf.m_DeviceVirtualDir.emplace(node1, node2);
	}

	//解析业务分组
	for (auto& item : json_info["business_group"])
	{
		auto node1 = ws_utf8_cvt_t().from_bytes(item[0].get<std::string>());
		auto node2 = item[1].get<std::string>();
		devInfoConf.m_BusinessGroup.emplace(node1, node2);
	}
	return 0;
}

int ServerConsoleMain()
{
	int nRetCode = 0;
	CWinThread	* p_mt;
	DWORD		dwWaitObject;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 更改错误代码以符合您的需要
			_tprintf(_T("错误: MFC 初始化失败\n"));
			nRetCode = 1;
		}
		else
		{
			// 初始化基本配置
			//appConf.strConfFileName = "ServerConsole.ini";
			appConf.strConfFileName = _T("Project.json");
			GetModuleFileName(nullptr, appConf.strModulePath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
			appConf.strModulePath.ReleaseBuffer();
			auto nPos = appConf.strModulePath.ReverseFind('\\');
			//进程模块路径
			appConf.strModulePath = appConf.strModulePath.Left(nPos + 1);
			//主配置文件路径
			appConf.strConfigPath = appConf.strModulePath + appConf.strConfFileName;

			//解析工程配置
			ParserProjectSetting(appConf.strConfigPath);
			//
			ParserDeviceInfoSetting(appConf.strDevInfoConfPath);

			// 建立主控线程
			CEvent	stopEvent;
			p_mt = AfxBeginThread(pfnMainThreadProc, &stopEvent);
			if (p_mt == nullptr)
			{
				_tprintf(_T("错误：创建主线程失败\n"));
				nRetCode = 1;
			}
			else
			{
#ifdef WIN_SERVICE
				MSG	msg;
				while (GetMessage(&msg, nullptr, 0, 0))
				{
					if (WM_CLOSE == msg.message)
					{
						stopEvent.SetEvent();
						Sleep(1000);
						break;
					}
				}
#else				// 等待主控线程退出
				dwWaitObject = WaitForSingleObject(p_mt->m_hThread, INFINITE);
				if (dwWaitObject == WAIT_FAILED)
				{
					//TODO....
				}
#endif
			}
		}
	}
	else
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("错误: GetModuleHandle 失败\n"));
		nRetCode = 1;
	}

	return nRetCode;
}

void WINAPI WinServiceMain()
{
	// Register the control request handler
	status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	//   assert(0);
	   //注册服务控制
	hServiceStatus = RegisterServiceCtrlHandler(szServiceName, WinServiceCtrl);
	if (hServiceStatus == nullptr)
	{
		WinServiceLogEvent(_T("Handler not installed"));
		return;
	}
	SetServiceStatus(hServiceStatus, &status);

	status.dwWin32ExitCode = S_OK;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;
	status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hServiceStatus, &status);
	dwThreadID = GetCurrentThreadId();

	ServerConsoleMain();

	status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(hServiceStatus, &status);
	OutputDebugString(_T("Service stopped"));
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
#ifdef WIN_SERVICE
	WinServiceInit();

	_tcsncpy_s(szServiceName, "Honeywell_GB28181_PLTD_Service", MAX_PATH);
	SERVICE_TABLE_ENTRY st[] =
	{
		{ szServiceName, reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(WinServiceMain) },
		{ NULL, NULL }
	};

	if (argv[1])
	{
		if (_stricmp(argv[1], "/install") == 0)
		{
			WinServiceInstall();
			return 0;
		}
		else if (_stricmp(argv[1], "/uninstall") == 0)
		{
			WinServiceUninstall();
			return 0;
		}
		else if (_stricmp(argv[1], "/config") == 0)
		{
			// TODO:
			return 0;
		}
	}

	if (!StartServiceCtrlDispatcher(st))
	{
		WinServiceLogEvent(_T("Register Service Main Function Error!"));
	}

	return 0/*nRetCode*/;
#else
	(argc, argv, envp);
	return ServerConsoleMain();
#endif
}