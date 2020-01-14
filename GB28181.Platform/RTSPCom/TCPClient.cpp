#include "stdafx.h"
#include "TCPClient.h"
#include <string>
#include "Log/Log.h"
#include <WS2tcpip.h>

CTCPClient::CTCPClient()
	: m_socket(INVALID_SOCKET),
	m_pThread(nullptr), m_nIdx(0), m_nCallDialogID(0),
	TIMEOUT(5000)
{
	// 此段程序用在应用程序初始化处
	int Ret;
	WSADATA wsaData;
	// 初始化WinSock
	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		CLog::Log(RTSPCOM, LL_NORMAL, "WinSock初始化出错，错误码为：%d。", Ret);
	}
}

CTCPClient::~CTCPClient(void)
{
	this->Disconnect();
	if (m_pThread)
		WaitForSingleObject(m_pThread->m_hThread, TIMEOUT);
}

SOCKET CTCPClient::Init()
{
	// 判断是否已开启线程
	if (m_pThread)
	{
		Disconnect();
		::WaitForSingleObject(m_pThread->m_hThread, TIMEOUT);
	}
	// 创建套接字
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 指定不为0的端口号
	if (m_socket != INVALID_SOCKET)
	{
		return m_socket;
	}
	return INVALID_SOCKET;
}

bool CTCPClient::Connect(const char *pszServerIP, u_short nServerPort, int trytimes)
{
	// 设置服务端的地址
	ULONG addr;
	m_addrServer.sin_family = AF_INET;
	inet_pton(m_addrServer.sin_family, pszServerIP, &addr);
	m_addrServer.sin_addr.S_un.S_addr = addr;
	m_addrServer.sin_port = htons(nServerPort);
	m_strRemoteIP = pszServerIP;
	m_uRemotePort = nServerPort;
	// 连接到服务端
	int conect_count = 1;
	while (trytimes-- > 0)
	{
		if (0 == connect(m_socket, reinterpret_cast<sockaddr *>(&m_addrServer), sizeof(m_addrServer)))
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "第%d 次,与RTSP服务端连接成功.", conect_count);
			return true;
		}
		DWORD dwErr = GetLastError();
		CLog::Log(RTSPCOM, LL_NORMAL, "第%d 次,与RTSP服务端连接出错 ,error code: %d .", conect_count++, dwErr);
	}

	return false;
}

void CTCPClient::Disconnect(void)
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		// 关闭SOCKET
		if (shutdown(m_socket, SD_BOTH) != 0)
		{
			TRACE("终止通信出错!\n");
			return;
		}
		m_socket = INVALID_SOCKET;
	}
}

bool CTCPClient::SendString(const char* buf, u_int datalength)
{
	return SendData(m_socket, buf, datalength);
}

bool CTCPClient::SendData(SOCKET sock, const char* buf, u_int datalength)
{
	u_int msgsent;
	u_int msgleft = datalength;
	while (msgleft > 0)
	{
		msgsent = send(sock, buf, msgleft, 0);
		if (msgsent == SOCKET_ERROR)
		{
			TRACE("发送数据出错");
			return false;
		}
		else if (msgsent == 0)
		{
			TRACE("连接已断开");
			return false;
		}
		msgleft -= msgsent;
	}

	return true;
}

UINT CTCPClient::RecvThreadProc(LPVOID pParam)
{
	auto retv = reinterpret_cast<LONG_PTR>(pParam);
	auto strv = std::to_string(retv);
	strv += ":Fuction Call to this address have no implementation!";
	std::exception newexception(strv.c_str());
	throw newexception;
}

// 取得SOCKET
SOCKET CTCPClient::GetSocket() const
{
	return m_socket;
}