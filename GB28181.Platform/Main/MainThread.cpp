// Main\MainThread.cpp : 实现文件
//
#include "stdafx.h"
#include "ServerConsole.h"
#include "SIPCom/SIPCom.h"
#include "SDKCom/SDKCom.h"
#include "RTSPCom/RTSPCom.h"
#include "DevInfo/DevInfo.h"
#include "GBAdaptorCom/GBAdaptorCom.h"

static bool InitConfig();

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID lParam)
{
	bool				ret;
	CEvent				*pEvent = reinterpret_cast<CEvent*>(lParam);
	HANDLE				hEvents[6 + 1];
	DWORD				dWaitObject;
	DWORD				dWaitTm = MAKEWORD(1000, 0);



	// 初始化配置
	ret = InitConfig();
	if (ret == false)
	{
		return 999;
	}

	// 创建每个模块对象
	auto pSIPCom = new CSIPCom();
	auto pRouter = new CRouter();
	auto pSDKCom = new CSDKCom();
	auto pRTSPCom = new CRTSPCom();
	auto pDevInfo = new CDevInfo();
	auto pGBAdaptorCom = new GBAdaptorCom();
	auto pDataStore = new CDataStore();
	auto pLog = new CLog();

	// 添加日志模块
	pLog->AddModule(ROUTER, pRouter->GetModuleID());
	pLog->AddModule(SDKCOM, pSDKCom->GetModuleID());
	pLog->AddModule(SIPCOM, pSIPCom->GetModuleID());
	pLog->AddModule(RTSPCOM, pRTSPCom->GetModuleID());
	pLog->AddModule(DEVINFO, pDevInfo->GetModuleID());
	pLog->AddModule(GBADAPTORCOM, pGBAdaptorCom->GetModuleID());
	pLog->AddModule(DATASTORE, pDataStore->GetModuleID());


	// 创建路由映射
	pRouter->AddModule(SDKCOM, pSDKCom);
	pRouter->AddModule(SIPCOM, pSIPCom);
	pRouter->AddModule(RTSPCOM, pRTSPCom);
	pRouter->AddModule(DEVINFO, pDevInfo);
	pRouter->AddModule(GBADAPTORCOM, pGBAdaptorCom);
	pRouter->AddModule(DATASTORE, pDataStore);

	// 启动日志模块
	pLog->Init(appConf.m_LogInfo.strRootDir);
	pLog->Startup();

	// 初始化SDK模块读取设备信息
	pSDKCom->Init();

	// 初始化其它模块
	pSIPCom->Init();
	pRTSPCom->Init();
	pDevInfo->Init();
	pGBAdaptorCom->Init();
	pDataStore->Init();

	// 启动各个模块
	pSDKCom->Startup();
	pSIPCom->Startup();
	pRTSPCom->Startup();
	pDevInfo->Startup();
	pGBAdaptorCom->Startup();
	pDataStore->Startup();

	hEvents[0] = pSDKCom->m_hNotice;
	hEvents[1] = pSIPCom->m_hNotice;
	hEvents[2] = pRTSPCom->m_hNotice;
	hEvents[3] = pDevInfo->m_hNotice;
	hEvents[4] = pGBAdaptorCom->m_hNotice;
	hEvents[5] = pDataStore->m_hNotice;
	hEvents[6] = pEvent->m_hObject;

	// 进入监听
	while (1)
	{
		// 监听全部模块
		dWaitObject = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, dWaitTm);
		if (dWaitObject == WAIT_OBJECT_0 + 6)
		{
			pSIPCom->StopSubprocess();
			break;
		}

		// 处理模块消息
		// 模块内部应自行尝试恢复运行
		// 如一个模块退出，则认为错误不可恢复
		// 于是通知所有模块退出
		else if (dWaitObject == WAIT_FAILED)
		{
			break;
		}
	}

	// 通知各个模块退出
	pSDKCom->Cleanup();
	pSIPCom->Cleanup();
	pRTSPCom->Cleanup();
	pDevInfo->Cleanup();
	pGBAdaptorCom->Cleanup();
	pDataStore->Cleanup();

	pLog->Cleanup();

	delete pSDKCom;
	delete pSIPCom;
	delete pRTSPCom;
	delete pDevInfo;
	delete pRouter;
	delete pGBAdaptorCom;
	delete pDataStore;
	delete pLog;
	return 0;
}

bool InitConfig()
{
	// TODO

	//

	return true;
}

void StartupSubProcess()
{
	STARTUPINFO			si;	//该结构用于指定新进程的主窗口特性
	PROCESS_INFORMATION pi;	//指定新进程的主窗口特性
	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	//用来创建一个新的进程和它的主线程，这个新进程运行指定的可执行文件
	CString strProcessFile = appConf.strModulePath + "GB28181.SipCom.exe";
	if (FALSE == CreateProcess(strProcessFile, nullptr, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
	{
		DWORD dwErr = GetLastError();
		CLog::Log(SIPCOM, LL_NORMAL, "SIPCom.exe进程启动失败 code:%d", dwErr);
	}
	CLog::Log(SIPCOM, LL_NORMAL, "创建SIPCom.exe进程");
	//CloseHandle 关闭一个内核对象。其中包括文件、文件映射、进程、线程、安全和同步对象等
	CloseHandle(pi.hThread);//关闭新进程
	CloseHandle(pi.hProcess);//关闭新线程
}