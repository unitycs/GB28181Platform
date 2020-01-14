// SIPCom.cpp : 定义控制台应用程序的入口点。
//
//头文件的顺序不要随更改,对C++而言，包含顺序很重要。
#include "stdafx.h"
#include "SIPConsole.h"
#include "Memory/SharedVarQueue.h"
#include "Main/MainThread.h"
#include <iostream>
#include "Common/common.h"
#include <fstream>
#ifdef WIN_SERVICE
// 隐藏窗口
#include "WinService/WinServiceProc.h"
#pragma   comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

APP_SETTING_T	appConf;


using namespace std;

// 程序唯一性检测
BOOL SipCom_Unique_MutexCheck()
{
	
	CreateMutex(nullptr, FALSE, _T("APP_UNIQUENESS_CHECK_SIPCOM.EXE"));

	// 检查错误代码
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{

		auto shareMemName = appConf.m_SharedMemory.App_to_Sipcom.str_Name;
		CSharedVarQueue oSharedReadQ(shareMemName);

		oSharedReadQ.Init(MB2B(DEFAULT_SHARED_MEMORY_SIZE_MB), false);

		// 通知同名线程退出
		ipc_sip_block_t *pMsgHeader;
		if (oSharedReadQ.PushAlloc(reinterpret_cast<void **>(&pMsgHeader), sizeof(ipc_sip_block_t)))
		{
			pMsgHeader->cmd_type  = ST_EXIT_PROCESS;
			oSharedReadQ.PushDone(pMsgHeader);
		}
		// 等待同名线程退出
		Sleep(8000);
	}

	return TRUE;
}

auto read_domain_info(json & item)
{
	APP_SETTING_T::PLT_CONFIG_INFO_T infoitem;

	infoitem.str_Name = item["name"].get<std::string>().c_str();
	infoitem.b_needAuth = item["needauth"].get<bool>();
	infoitem.str_ID = item["device_id"].get<std::string>().c_str();
	infoitem.str_IP = item["ip"].get<std::string>().c_str();
	auto username = item["username"].get<std::string>();
	infoitem.str_Username = username.c_str();
	if (username.empty())
	{
		infoitem.str_Username = infoitem.str_ID;
#ifdef DEBUG
		_tprintf("配置信息读取:\r\n SIP平台域：%s,发现其UserName为空,使用本地域ID.\r\n", infoitem.str_ID.GetString());
#endif // DEBUG
	}
	infoitem.str_Password = item["password"].get<std::string>().c_str();
	infoitem.KeepAliveInterval = item["keepalive"].get<unsigned>();
	infoitem.ExpiryTime = item["expiry"].get<unsigned>();
	infoitem.nPort = item["port"].get<u_short>();
//	infoitem.nPort2 = item["port2"].get<u_short>();  //用于与GB adaptor的二进制私有协议通信
	return infoitem;
}

//读取配置文件
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

	auto jsmain = json_info["main"];
	// 读取日志配置
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

	//本地平台信息
	auto jscur = jsmain["current_domian"];
	appConf.m_Current = read_domain_info(jscur);

#ifdef DEBUG
	_tprintf("\r\n-----------------开始上级域平台信息读取-----------------\r\n");
#endif // DEBUG
	// 上级域平台信息
	for (auto& item : json_info["upper_domian"])
	{
		auto infoitem = read_domain_info(item);
		appConf.m_UpperList.emplace_back(std::move(infoitem));
	}
#ifdef DEBUG
	_tprintf("\r\n-----------------开始下级域平台信息读取-----------------\r\n");
#endif // DEBUG
	//下级域平台信息
	for (auto& item : json_info["lower_domian"])
	{

		auto infoitem = read_domain_info(item);
		appConf.m_LowerList.emplace_back(std::move(infoitem));
	}
	//域间路由表,node1和node1是相互转发的，一般默认node1是上级域的路由节点
	for (auto& item : json_info["routing"])
	{
		auto node1 = item[0].get<std::string>();
		auto node2 = item[1].get<std::string>();
		appConf.m_Routing.emplace(node1, node2);
	}
	//进程间通信的配置
	auto ipcsm = jsmain["sharedmemory"];
	appConf.m_SharedMemory.Sipcom_to_App.str_Name = ipcsm["sipcom_to_app"].get<std::string>().c_str();
	appConf.m_SharedMemory.Sipcom_to_App.uint_Size = ipcsm["nsize"].get<unsigned>();
	appConf.m_SharedMemory.App_to_Sipcom.str_Name = ipcsm["app_to_sipcom"].get<std::string>().c_str();;
	appConf.m_SharedMemory.App_to_Sipcom.uint_Size = ipcsm["nsize"].get<unsigned>();

	//防火墙配置
	appConf.m_Firewall.str_IP = jsmain["network"]["firewall_ip"].get<std::string>().c_str();
	appConf.m_Firewall.nPort = jsmain["network"]["firewall_port"].get<unsigned>();
	return 0;
}

int SIPConsoleMain()
{
	int			nRetCode = 0;
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
			//appConf.strConfFileName = _T("serverconsole.ini");
			appConf.strConfFileName = _T("Project.json");
			GetModuleFileName(nullptr, appConf.strModulePath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
			appConf.strModulePath.ReleaseBuffer();
			auto nPos = appConf.strModulePath.ReverseFind('\\');
			//进程模块路径
			appConf.strModulePath = appConf.strModulePath.Left(nPos + 1);
			//主配置文件路径
			appConf.strConfigPath = appConf.strModulePath + appConf.strConfFileName;

			ParserProjectSetting(appConf.strConfigPath);

			SipCom_Unique_MutexCheck();

			// 建立主控线程
			p_mt = AfxBeginThread(pfnMainThreadProc, nullptr);
			if (p_mt == nullptr)
			{
				_tprintf(_T("错误：创建主线程失败\n"));
				nRetCode = 1;
			}
			else
			{
				// 等待主控线程退出
				dwWaitObject = WaitForSingleObject(p_mt->m_hThread, INFINITE);
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

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{

#ifdef WIN_SERVICE
	return WinServiceMain(argv);
#else
	(argc, argv, envp);
	return SIPConsoleMain();
#endif
}