// Main\MainThread.cpp : 实现文件
//
#include "stdafx.h"
#include "MainThread.h"
#include "SIPConsole.h"
#include "Log/Log.h"
#include "SipServiceCom/SIPService.h"

static bool InitConfig();

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID /*lParam*/)
{
	bool				ret;

	HANDLE				hEvents[1];
	DWORD				dWaitObject;
	DWORD				dWaitTm = MAKEWORD(1000, 0);

	// 模块主控线程


	// 初始化配置
	ret = InitConfig();
	if (ret == false)
	{
		return 999;
	}

	// 创建每个模块对象
	auto pSIPService = new CSIPService();

	auto pRouter = new CRouter();
	auto pLog = new CLog();
	
	// 添加日志模块
//	pLog->AddModule(ROUTER, pRouter->GetModuleID());
	pLog->AddModule(SIPSERVICE, pSIPService->GetModuleID());


	// 创建路由映射
//	pRouter->AddModule(SIPSERVICE, pSIPService);

	// 启动日志模块
	pLog->Init(appConf.m_LogInfo.strRootDir);
	pLog->Startup();

	// 初始化SDK模块读取设备信息
	pSIPService->Init();


	// 启动各个模块
	pSIPService->Startup();
	//CLog::Log(SIPSERVICE, LL_NORMAL, "dd.");

	hEvents[0] = pSIPService->m_hNotice;
	// 进入监听
	while (true)
	{
	
		// 监听全部模块
		dWaitObject = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, dWaitTm);

		// 处理模块消息
		// 模块内部应自行尝试恢复运行
		// 如一个模块退出，则认为错误不可恢复
		// 于是通知所有模块退出
		// 如果超时则继续等待
		if (dWaitObject == WAIT_TIMEOUT)
		{
			continue;
		}

		//if (dWaitObject == WAIT_FAILED)
		//{
		pLog->m_bIsExit = true;
		CLog::Log(SIPSERVICE, LL_NORMAL, "退出主控制线程.");
		break;
		//}
	}

	// 通知各个模块退出

	pSIPService->Cleanup();
	pLog->Cleanup();


	delete pSIPService;
	delete pRouter;
	delete pLog;
	return 0;
}

bool InitConfig()
{
	CString		sPath;

	// TODO

	return true;
}