// SipSimulator.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "MainThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

// 唯一的应用程序对象

//CWinApp theApp;



int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	auto nRetCode = 0;
	CWinThread	* p_mt;
	DWORD		dwWaitObject;
	WSADATA		wsaData;

	auto hModule = ::GetModuleHandle(nullptr);

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
			// 初始化winsock2.2
			WSAStartup(MAKEWORD(2,2), &wsaData);

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
			dwWaitObject=  WaitForSingleObject(p_mt->m_hThread, INFINITE);
			if(dwWaitObject== WAIT_FAILED)
			{
				//TODO...

			}
			//Exit App...
			WSACleanup();
			
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
