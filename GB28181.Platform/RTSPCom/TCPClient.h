#pragma once

#include <vector>
#include "winsock2.h"


class CTCPClient
{
public:
	CTCPClient();
	virtual ~CTCPClient(void);

	// 初始化TCPSocket客户端
	virtual SOCKET Init();

	// 连接到远程服务端,默认连接到远端RTSP的554
	virtual bool Connect(const char *pszServerIP, u_short nServerPort = 554, int trytimes = 3);

	// 断开连接
	virtual void Disconnect(void);

	// 发送指定长度的数据
	virtual bool SendString(const char *buf, u_int datalength);

	// 发送指定长度的数据
	static bool SendData(SOCKET sock, const char *buf, u_int datalength);

	// 接受数据线程处理函数
	static UINT RecvThreadProc(LPVOID pParam);

	// 取得SOCKET
	SOCKET GetSocket() const;

protected:
	SOCKET			m_socket;
	SOCKADDR_IN		m_addrServer;
	CWinThread		*m_pThread;
	int				m_nIdx;
	INT64			m_nCallDialogID;
	CString			m_strRemoteIP;
	u_short         m_uRemotePort;
public:
	const int		TIMEOUT;
};

class CRTSPClient
{
public:
	typedef struct RTSPClientInfo
	{
		CString	strIP;
		CTCPClient *pTCPClient;
		RTSPClientInfo()
		{
			pTCPClient = nullptr;
		}
	}RTSPClientInfo_t;


	enum class NETWORK_EVENT : unsigned
	{
		RTSP_CLOSE,
		RTSP_READ,
		RTSP_ERROR,
		RTSP_DEFAULT
	};


	CRTSPClient(const char *pszRemoteIP)
	{
		sockCreateSucess = CreateTcpClient(pszRemoteIP);
		if (sockCreateSucess)
		{
			//注册感兴趣的事件
			// 创建事件对象，并关联到新的套节字
			//句柄对象，用完务必close，用完务必close.用完务必close.，重要的事情说三遍。
			networkEventHandle = WSACreateEvent();
			::WSAEventSelect(GetScoket(), networkEventHandle, FD_READ | FD_CLOSE);
		}
		else
		{
			//TCP 连接创建失败，需要在操作时候重新连接NVR
			CLog::Log(RTSPCOM, LL_NORMAL, "%s Create CRTSPClient Failed\r\n", __FUNCTION__);
		}

	}
	~CRTSPClient()
	{
		DestroyTcpClient();
		//销毁网络事件监听句柄
		if (networkEventHandle)
		{
			WSACloseEvent(networkEventHandle);
			networkEventHandle = nullptr;
		}
	}
	// 创建TCP客户端
	bool CreateTcpClient(const char *pszRemoteIP = nullptr)
	{
		if (!pszRemoteIP) return false;
		SOCKET socetClient = INVALID_SOCKET;
		m_oRTSPClient.strIP = pszRemoteIP;
		if (nullptr == m_oRTSPClient.pTCPClient)
		{
			m_oRTSPClient.pTCPClient = new CTCPClient();
			socetClient = m_oRTSPClient.pTCPClient->Init();
			if (INVALID_SOCKET == socetClient)
			{
				delete m_oRTSPClient.pTCPClient;
				m_oRTSPClient.pTCPClient = nullptr;
				return false;
			}
		}
		if (!m_oRTSPClient.pTCPClient->Connect(pszRemoteIP, 554))
		{
			delete m_oRTSPClient.pTCPClient;
			m_oRTSPClient.pTCPClient = nullptr;
			return false;
		}

		return true;

	}
	bool CanSend()
	{
		return m_oRTSPClient.pTCPClient != nullptr;
	}
	bool Reconnect()
	{
		//之前创建的链接未销毁，不用重连
		if (CanSend()) return true;

		auto b_ret = CreateTcpClient(m_oRTSPClient.strIP);
		if (b_ret)
		{
			//句柄重新重新绑定到新的socket
			::WSAEventSelect(GetScoket(), networkEventHandle, FD_READ | FD_CLOSE);
		}
		return b_ret;
	}

	// 销毁TCP客户端
	void DestroyTcpClient()
	{
		if (m_oRTSPClient.pTCPClient)
		{
			m_oRTSPClient.pTCPClient->Disconnect();
			delete m_oRTSPClient.pTCPClient;
			m_oRTSPClient.pTCPClient = nullptr;
		}

	}

	// 发送数据
	bool SendData(const char *pszData) const
	{
		return m_oRTSPClient.pTCPClient->SendString(pszData, strlen(pszData));
	}

	NETWORK_EVENT GetNetworkEvent(char *szBuf, size_t buffersize, int & nRecvlen)
	{
		WSANETWORKEVENTS tEvent;
		::WSAEnumNetworkEvents(GetScoket(), networkEventHandle, &tEvent);
		if (tEvent.lNetworkEvents & FD_READ)   // 处理FD_READ通知消息
		{
			if (tEvent.iErrorCode[FD_READ_BIT] == 0)
			{
				ASSERT(szBuf != nullptr  && buffersize > 0);

				auto nLen = RecvData(szBuf, buffersize);
				if (1 > nLen)
				{
					CLog::Log(RTSPCOM, LL_NORMAL, "RECV RTSP Message nlen = %d return -1\r\n", nLen);
					return NETWORK_EVENT::RTSP_DEFAULT;
				}
				nRecvlen = nLen;  //返回接受长度
				return NETWORK_EVENT::RTSP_READ;
			}

		}
		if (tEvent.lNetworkEvents & FD_CLOSE)
		{
			CLog::Log(RTSPCOM, LL_NORMAL, "%s close\r\n", __FUNCTION__);
			return NETWORK_EVENT::RTSP_CLOSE;
		}

		return NETWORK_EVENT::RTSP_DEFAULT;

	}

	// 接收数据
	int RecvData(char *pszBuf, int nBufLen) const
	{
		int recved = recv(m_oRTSPClient.pTCPClient->GetSocket(), pszBuf, nBufLen, 0);
		int err;
		if (-1 == recved)
		{
			err = WSAGetLastError();
			return err;
		}
		return recved;
	}

	// 取得socket对应的远端IP
	const char *GetClientIP() const
	{
		return m_oRTSPClient.strIP.GetString();
	}

	//录像业务的网络事件处理
	NETWORK_EVENT GetNetworkEvent()
	{
		//tcp链接已经销毁了
		if (m_oRTSPClient.pTCPClient != nullptr)
		{
			// 判断网络事件
			WSANETWORKEVENTS tEvent;
			::WSAEnumNetworkEvents(GetScoket(), networkEventHandle, &tEvent);

			if (tEvent.lNetworkEvents & FD_READ)   // 处理FD_READ通知消息
			{
				return NETWORK_EVENT::RTSP_READ;
			}
			else if (tEvent.lNetworkEvents & FD_CLOSE)
			{
				DestroyTcpClient();
				return NETWORK_EVENT::RTSP_CLOSE;
			}
		}
		return NETWORK_EVENT::RTSP_DEFAULT;
	}


	// 取得socket
	SOCKET GetScoket() const
	{
		return m_oRTSPClient.pTCPClient->GetSocket();
	}
	HANDLE GetNetworkEventHandle()
	{
		return networkEventHandle;
	}

private:
	RTSPClientInfo_t		m_oRTSPClient;
	bool                    sockCreateSucess;
	HANDLE networkEventHandle = nullptr;
};