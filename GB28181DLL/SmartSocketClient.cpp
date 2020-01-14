#include "StdAfx.h"
#include "SmartSocketClient.h"


CSmartSocketClient::CSmartSocketClient(void)
{
}

CSmartSocketClient::CSmartSocketClient(CUACTCP *pParent) : CSmartSocket(pParent)
{
}

CSmartSocketClient::~CSmartSocketClient(void)
{
}

UINT CSmartSocketClient::ThreadPro()
{
	int ret;				// 接收的状态
	int recved;			// 接受到的字符数
	char buf[LEN_RECEIVE];	// 临时空间
	fd_set *readfds = new fd_set;		// 读集合
	fd_set *writefds = new fd_set;		// 写集合
	fd_set *exceptfds = new fd_set;		// 带外数据集合
	timeval *timeout = new timeval;		// 超时时间

	// 初始化Select函数要用到的变量
	FD_ZERO(readfds);
	FD_ZERO(writefds);
	FD_ZERO(exceptfds);
	timeout->tv_sec = TIMEOUT_RECV;
	timeout->tv_usec = 0;

	int sendbuff= LEN_RECEIVE;
	int nRevOpt=sizeof(sendbuff);
	setsockopt( m_SocketSrvEnd,SOL_SOCKET,SO_RCVBUF,(char*)&buf,nRevOpt);
	struct sockaddr_storage sa;
	int slen;
	// 进入接收循环
	while(!m_bExit)
	{
		// 把该socket加入读集合
		if(!FD_ISSET(m_SocketSrvEnd, readfds))
			FD_SET(m_SocketSrvEnd, readfds);
		// 把该socket加入读集合
		if(!FD_ISSET(m_socExit, readfds))
			FD_SET(m_socExit, readfds);

		// 监控是否有数据
		ret = select(0, readfds, writefds, exceptfds, timeout);

		// 如果超时，则继续
		if (ret == 0)
			continue;

		// 如果网络错误，则判断是否已断开连接
		if (ret == SOCKET_ERROR)
		{
			if (m_bExit)
			{
				SAFE_DELETE(readfds);
				SAFE_DELETE(writefds)
				SAFE_DELETE(exceptfds)
				SAFE_DELETE(timeout)
			}
			else
			{
				//CString str;
				//str.Format(_T("通信过程中出错，错误号：%d"), WSAGetLastError());
				//::MessageBox(NULL, str, _T("错误"), MB_ICONSTOP | MB_OK);
				//g_objLog.LogoutDebug(k_LOG_DLL, "%s 通信过程中出错，错误号：%d\n", __FUNCTION__, WSAGetLastError());
			}
			continue;
		}

		// 如果有数据到达则收取数据
		if(FD_ISSET(m_SocketSrvEnd, readfds))
		{
			recved = recv(m_SocketSrvEnd, buf, LEN_RECEIVE, 0);
			// 如果接收正常，则把接收到的字符串加入缓冲区
			if (recved > 0)
			{
				//m_buffer.BufferIn(buf, recved);
				//if(0 == ProRawData(buf, recved))
					//g_objLog.LogoutDebug(k_LOG_DLL, "%s 接受到%d字节有效数据\n", __FUNCTION__, recved);
				
			}
			// 否则退出接收线程
			else
			{
				//续连
				int nRet;

				while(TRUE)
				{
					if(m_SocketSrvEnd != INVALID_SOCKET)
					{
						closesocket(m_SocketSrvEnd);
						m_SocketSrvEnd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					}
					nRet = connect(m_SocketSrvEnd, (sockaddr *)(&m_svraddr), sizeof(m_svraddr));
					if (nRet == SOCKET_ERROR)
					{
						//g_objLog.LogoutDebug(k_LOG_DLL, "%s 续连服务器中\n", __FUNCTION__);
						Sleep(1000);
					}
					else
						break;
				}
				
				if (nRet == SOCKET_ERROR)
				{
					//g_objLog.LogoutDebug(k_LOG_DLL, "%s 服务端续连未能成功\n", __FUNCTION__);
					closesocket(m_SocketSrvEnd);
					m_SocketSrvEnd = INVALID_SOCKET;
					break;
				}
				else
				{
					Login();
					//g_objLog.LogoutDebug(k_LOG_DLL, "%s 服务端续连成功\n", __FUNCTION__);
				}

			}
		
		}
		else if(FD_ISSET(m_socExit, readfds))
		{
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 退出socke\n", __FUNCTION__);
		}

		// 重置读集合
		FD_ZERO(readfds);
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 退出RecvThread\n", __FUNCTION__);
	SAFE_DELETE(readfds);
	SAFE_DELETE(writefds);
	SAFE_DELETE(exceptfds);
	SAFE_DELETE(timeout);
	
	//SetEvent(m_hExitEvent);

	return 0;
}