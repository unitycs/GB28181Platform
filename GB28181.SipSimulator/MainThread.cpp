// Main\MainThread.cpp : 实现文件
//

#include "stdafx.h"
#include "MainThread.h"
#include "Module.h"
#include "SipTask/InviteTask.h"
#include "LoginHandler.h"
#include "SipTask/QueryTask.h"

TASK_DESC	task_array[] = {
							{DEVICE_INFO_QUERY, "目录查询"},
							{PLAYBACK, "回放测试"},
							{PLAYBACK_CTRL, "回放控制测试"},
							{PLAY_VIDEO, "播放测试"}
							};

static TASKID InitTask();

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID lParam)
{
	TASKID				tid;
	CModule				* pModule;
	HANDLE				hEvents[1];
	DWORD				dWaitObject;

	//auto				bSet	= false;
	char				ip[32]	= "";
	int					port;
	CLoginHandler		loginHandler;
	SOCKADDR_IN			tSockAddr;
	SOCKET				sock;

	// 分配socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("无法分配socket，线程退出\n");
		return -1;
	}
	printf("本地IP：");
	scanf_s("%s", ip, 32);
	//strcpy_s(ip, 32, "10.10.124.174");
	printf("\n");
	printf("本地端口：50601");
	//scanf_s("%d", &port);
	port = 50601;
	printf("\n");

	// 设置本地IP和端口
	tSockAddr.sin_family = AF_INET;
	tSockAddr.sin_addr.S_un.S_addr = inet_addr(ip);
	tSockAddr.sin_port = htons(port);
	if(0 != bind(sock, reinterpret_cast<struct sockaddr*>(&tSockAddr), sizeof(tSockAddr)))
	{
		DWORD dwErr = WSAGetLastError();
		return dwErr;
	}
	//printf("等待客户端登陆……\n\n");
	//loginHandler.Init(sock);
	//loginHandler.WaitLogin(tSockAddr);
	//printf("客户端登陆成功\n\n");

	// 创建接收线程
	CTask::CreateRecvThread(sock);
	printf("接收线程启动\n\n");

	// 设置远程IP和端口
	tSockAddr.sin_family = AF_INET;
	tSockAddr.sin_addr.S_un.S_addr = inet_addr("10.10.124.75");
	tSockAddr.sin_port = htons(50601);

	// 进入测试任务循环
	while (1)
	{
		// 用户输入任务
		tid = InitTask();
		if (tid < DEVICE_INFO_QUERY)
		{
			// 用户选择退出
			return 0;
		}

		//// 如果没设置
		//if (!bSet && tid != SETIPPORT)
		//{
		//	printf("请先设置目标IP和端口！\n\n");
		//	continue;
		//}

		// 创建工作模块
		switch (tid)
		{
		case DEVICE_INFO_QUERY:
			pModule = new CQueryTask(sock, tSockAddr);
			break;
		case PLAYBACK:
			pModule = new CInviteTask(sock, tSockAddr);
			break;
		case PLAYBACK_CTRL:
			continue;
		case PLAY_VIDEO:
			pModule= new CInviteTask(sock,tSockAddr);
			break;
		//case SETIPPORT:
		//	bSet = true;
		//	printf("本地IP：");
		//	scanf_s("%s", ip);
		//	printf("\n");
		//	printf("本地端口：");
		//	scanf_s("%d", &port);
		//	printf("\n");
		//	break;
		default:
			return 0;
		}

		// 启动工作模块
		pModule->Init();
		pModule->Startup();

		// 进入监听
		hEvents[0] = pModule->m_hNotice;
		dWaitObject = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE);

		// 处理模块消息
		// 模块内部应自行尝试恢复运行
		// 如一个模块退出，则认为错误不可恢复
		// 于是通知所有模块退出
		if (dWaitObject == WAIT_FAILED)
		{
			break;
		}

		// 释放工作模块 
		pModule->Cleanup();
		delete pModule;
	}
	closesocket(sock);
	return 0;
}

TASKID InitTask()
{
	TASKID	n;

	// 显示菜单
	printf("----------------------------------------\n\n");
	printf("三方平台模拟器\n");
	printf("----------------------------------------\n\n");
	printf("请选择测试任务：\n\n");
	for (auto i = 0; i < ARRAYSIZE(task_array); i++)
	{
		printf("\t%d:\t%s\n", task_array[i].id, task_array[i].name);
	}
	//printf("\t9:\t设置本地IP和端口\n");
	printf("\t0:\t退出\n");

	// 读取输入
	printf("你的输入：");
	scanf_s("%d", &n);
	printf("\n");

	return n;
}
