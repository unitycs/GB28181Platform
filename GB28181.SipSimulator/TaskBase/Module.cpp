#include "./../stdafx.h"
#include "Module.h"

// 模块主线程
UINT AFX_CDECL CModule::pfnMainCtrlProc(LPVOID lParam)
{
	auto			pModule = static_cast<CModule *>(lParam);
	auto			nThrds = pModule->m_vThreads.GetCount();
	auto			hEvents = static_cast<HANDLE *>(malloc(sizeof(HANDLE) * nThrds));
	DWORD			dWaitObject;
	DWORD			dWaitTm = MAKEWORD(1000, 0);

	// 初始化监听句柄数组
	for (auto i = 0; i < nThrds; i++)
	{
		hEvents[i] = static_cast<CWinThread *>(pModule->m_vThreads[i])->m_hThread;
	}

	// 进入监听循环
	while(!pModule->m_bIsExit)
	{
		// 监听主线程信号及模块处理
		dWaitObject = WaitForMultipleObjects(nThrds, hEvents, FALSE, WSA_INFINITE);

		// 等待失败，打印错误日志
		if (dWaitObject == WAIT_FAILED)
		{
			// TODO
			printf("等待等待事件失败，重新开始等待！\n");
			continue;
		}
		//TODO.....
		//有事件需要处理...

		// 线程退出
		pModule->m_objNotice.SetEvent();
		break;
	}

	return 0;
}

CModule::CModule(void) : m_bIsExit(false),
	m_nLogIdx(0),m_pMainThread(nullptr),m_pMainCtrlProc(pfnMainCtrlProc)
{
	m_hNotice = m_objNotice.m_hObject;
}


CModule::~CModule(void)
{
	CProcDesc	* pDesc;

	// 释放线程函数描述
	for (auto i = 0; i < static_cast<int>(m_vThreadProcs.GetCount()); i ++)
	{
		pDesc = reinterpret_cast<CProcDesc *>(m_vThreadProcs[i]);
		delete pDesc;
	}
}

// 启动所有线程
void CModule::Startup()
{
	// 启动所有工作线程
	for (auto i = 0; i < static_cast<int>(m_vThreadProcs.GetCount()); i ++)
	{
		auto pDesc = reinterpret_cast<CProcDesc *>(m_vThreadProcs[i]);
		CWinThread	* pThread;
		for (auto j = 0; j < pDesc->count; j ++)
		{
			// 启动线程
			pThread = AfxBeginThread(pDesc->pfn, pDesc->lParam,	0, 0, 0, nullptr);
			m_vThreads.Add(pThread);

		}
	}

	// 启动主控线程
	m_pMainThread = AfxBeginThread(m_pMainCtrlProc, static_cast<LPVOID>(this), 0, 0, 0, nullptr);
}

// 注册工作线程函数
void CModule::RegisterProc( THREAD_PROC pfn, LPVOID lParam, int count )
{
	CProcDesc	 * pDesc = new CProcDesc();

	pDesc->pfn = pfn;
	pDesc->lParam = lParam;
	pDesc->count =count;

	m_vThreadProcs.Add(reinterpret_cast<void *>(pDesc));
}

// 注册主控线程函数
void CModule::RegisterMainProc( THREAD_PROC pfn )
{
	m_pMainCtrlProc = pfn;
}
