#include "stdafx.h"
#include <string>
#include "SmartSocket.h"
#include "UACTCP.h"

using namespace std;

CSmartSocket::CSmartSocket(void)
{
	m_bExit = false;
	m_hThread = NULL;
	m_SocketSrvEnd = INVALID_SOCKET;
	m_socExit = INVALID_SOCKET;

	pIncompleteData = NULL;
	nSurplus = 0;

	m_hMsgEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
}

CSmartSocket::CSmartSocket(CUACTCP *pParent)
{
	m_bExit = false;
	m_hThread = NULL;
	m_SocketSrvEnd = INVALID_SOCKET;
	m_socExit = INVALID_SOCKET;
	m_pParent = pParent;
	m_hMsgEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	pIncompleteData = NULL;
	nSurplus = 0;
	
}

CSmartSocket::~CSmartSocket(void)
{

	ExitRecv();
	Disconnect();
	//::WaitForSingleObject(m_hThread, TIMEOUT_SOCKET_MS);
	DeleteCriticalSection(&m_stRawCriSec);
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s ~CSmartSocket end\n", __FUNCTION__);

	pIncompleteData = NULL;
	nSurplus = 0;
	if(m_socExit != INVALID_SOCKET)
	{
		closesocket(m_socExit);
		m_socExit = INVALID_SOCKET;
	}
	if(m_SocketSrvEnd != INVALID_SOCKET)
	{
		closesocket(m_SocketSrvEnd);
		m_SocketSrvEnd = INVALID_SOCKET;
	}
}
int CSmartSocket::Login()
{
	if(m_pParent)
		return m_pParent->SendLogin();
	return 0;
}
SOCKET CSmartSocket::Init(int nLocalPort)
{
	InitializeCriticalSection(&m_stRawCriSec);

	int Ret;
	WSADATA wsaData;
	// 初始化WinSock
	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		char str[256];
		sprintf_s(str, "WinSock初始化出错，错误码为：%d。", Ret);
	
		MessageBoxA(NULL, str, "错误", MB_ICONSTOP | MB_OK);
	}

	// 判断是否已开启线程
	if (m_hThread)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 连接已经建立，重写初始化SOCKET。\n", __FUNCTION__);
		ExitRecv();
		Disconnect();
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 清空数据缓冲池。\n", __FUNCTION__);
	//m_buffer.ResetBuffer();
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 创建套接字。\n", __FUNCTION__);
	// 创建套接字
	m_SocketSrvEnd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == m_SocketSrvEnd)
		return m_SocketSrvEnd;

	m_socExit = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(INVALID_SOCKET == m_socExit)
		return m_socExit;

	if(INVALID_SOCKET)
	if(-1 != nLocalPort)
	{
		SOCKADDR_IN svraddr;
		memset(&svraddr, 0, sizeof(struct sockaddr_in));
		// 设置服务端的地址
		svraddr.sin_family = AF_INET;
		svraddr.sin_addr.S_un.S_addr = INADDR_ANY;
		svraddr.sin_port = htons(nLocalPort);

		if (0 != bind(m_SocketSrvEnd, (struct sockaddr *)&svraddr, sizeof(struct sockaddr_in)))
		{			
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 端口绑定失败\n", __FUNCTION__);
			return INVALID_SOCKET;
		}

		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 绑定本地端口：%d soket=%lu\n", __FUNCTION__, nLocalPort, m_SocketSrvEnd);
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 套接字创建完成 socket=%d。\n", __FUNCTION__, (long)m_SocketSrvEnd);
	return m_SocketSrvEnd;
}

UINT WINAPI CSmartSocket::RecvThread(LPVOID pParam)
{
	CSmartSocket *pSocket = (CSmartSocket *) pParam;	// 调用此线程的Socket对象的指针

	return pSocket->ThreadPro();
}

bool CSmartSocket::Connect(_tstring svrip, u_short port, bool bIsStart)
{
	int Ret = 0;

	// 设置服务端的地址
	m_svraddr.sin_family = AF_INET;
	string strIP = CLog::UnicodeToANSI(svrip.c_str());
	m_svraddr.sin_addr.S_un.S_addr = inet_addr(strIP.c_str());
	m_svraddr.sin_port = htons(port);

	// 连接到服务端
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 连接到%s:%d\n", __FUNCTION__, svrip.c_str(), port);
	Ret = connect(m_SocketSrvEnd, (sockaddr *)(&m_svraddr), sizeof(m_svraddr));
	if (Ret == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 服务端连接出错：%lu\n", __FUNCTION__, dwErr);
		closesocket(m_SocketSrvEnd);
		m_SocketSrvEnd = INVALID_SOCKET;
		return false;
	}

	if(bIsStart)
	{
		m_bExit = false;
		//ResetEvent(m_hExitEvent);

		// 启动接收线程
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, this, 0, &m_unThreadID);
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 启动接收线程\n", __FUNCTION__);
	}

	return true;
}

void CSmartSocket::Disconnect(void)
{
	if(m_SocketSrvEnd != INVALID_SOCKET)
	{	
		m_hThread = NULL;
		// 关闭SOCKET
		if(shutdown(m_SocketSrvEnd, SD_BOTH) != 0)
		{
			return;
		}
		//CLog::Out(__FUNCTION__,"shutdownTCP链路 socket=%d。", (long)m_SocketSrvEnd);
		closesocket(m_SocketSrvEnd);
		m_SocketSrvEnd = INVALID_SOCKET;
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP链路已关闭。\n", __FUNCTION__);
	}
}

bool CSmartSocket::SendData(const char* buf, u_int datalength)
{
	u_int msgsent;
	u_int msgleft = datalength;
	//要把接受数据的指针清零。
	pIncompleteData=NULL;
	while(msgleft > 0)
	{
		msgsent = send(m_SocketSrvEnd, buf, msgleft, 0);
		if (msgsent == SOCKET_ERROR)
		{
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 数据发送失败1\n", __FUNCTION__);
			return false;
		}
		else if (msgsent == 0)
		{
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 数据发送失败2\n", __FUNCTION__);
			return false;
		}
		msgleft -= msgsent;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 数据发送完成:%d\n", __FUNCTION__, datalength);
	return true;
}
bool CSmartSocket::SendString(const TCHAR* buf, u_int datalength)
{
	if(datalength != _tcslen(buf))
		return false;

	string strDataBuf = CLog::UnicodeToANSI(buf);
	return SendData(strDataBuf.c_str(), datalength);
}

int CSmartSocket::AddDataToList(raw_data_t* pData)
{
	EnterCriticalSection(&m_stRawCriSec);
	m_vecRawDataPkgList.push_back(pData);
	LeaveCriticalSection(&m_stRawCriSec);
	SetEvent(m_hMsgEvent);
	return 0;
}

int CSmartSocket::ProRawData(char *pBuf, int nBufSize)
{
//    static raw_data_t *pInComeRawData = NULL;
//	static int nSurplus = 0;

	int nDataSize;

	if(pIncompleteData)
	{
		int nExist = 0;
		memcpy(&nDataSize, pIncompleteData->pDataBuf, 4);
		nExist = nDataSize - nSurplus + 4;
		memcpy(&(pIncompleteData->pDataBuf[nExist]), pBuf, nBufSize);
		if('#' != pIncompleteData->pDataBuf[nDataSize+3])
		{
			//DeleteDataFromList(pInComeRawData);
			//delete pInComeRawData;
			nSurplus -= nBufSize;
			if(0 > nSurplus)
			{
				delete pIncompleteData;
				pIncompleteData = NULL;
			}
			return -1;
		}
		else
		{
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 添加消息到链表2。\n", __FUNCTION__);
			AddDataToList(pIncompleteData);
			pIncompleteData = NULL;
		}
	}

	memcpy(&nDataSize, pBuf, 4);
	if(50000 < nDataSize || 8 > nDataSize)
		return -1;

	int count=0;
	//i为遍历消息的条数
	int i=0;
	while(0 < nBufSize)
	{
		if (i>0)
		{
			nDataSize=0;
			memcpy(&nDataSize, pBuf+count, 4);
		}
		raw_data_t * pData = new raw_data_t(nDataSize+4);

		if(nDataSize+4 <= nBufSize)
		{
			memcpy(pData->pDataBuf, pBuf+count, nDataSize+4);
			pIncompleteData = NULL;
			nSurplus = 0;
		}
		else
		{
			memcpy(pData->pDataBuf, pBuf+count, nBufSize);
			pIncompleteData = pData;
			nSurplus = nDataSize+4-nBufSize;
		}

		if('#' == pData->pDataBuf[nDataSize+3])
		{
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s 添加消息到链表。\n", __FUNCTION__);
			AddDataToList(pData);
		}
		else
		{
			/*if(pInComeRawData == pData)
				pInComeRawData = NULL;

			delete pData;*/
			return -1;
		}
		i++;
		count+=(nDataSize+4);
		nBufSize -= (nDataSize+4);
	}
	return 0;
}

raw_data_t* CSmartSocket::DequeuePackage(BOOL bIsDelete)
{
	EnterCriticalSection(&m_stRawCriSec);
	while(0 < m_vecRawDataPkgList.size())
	{
		raw_data_t * pData = m_vecRawDataPkgList.front();
		if(TRUE == pData->bIsReaded)
		{
			if(pData)
			{
				delete pData;
				pData = NULL;
			}
			m_vecRawDataPkgList.erase(m_vecRawDataPkgList.begin());
			continue;
		}

		if(TRUE == bIsDelete)
			m_vecRawDataPkgList.erase(m_vecRawDataPkgList.begin());
		LeaveCriticalSection(&m_stRawCriSec);

		return pData;
	}

	LeaveCriticalSection(&m_stRawCriSec);
	ResetEvent(m_hMsgEvent);
	return NULL;
}

int CSmartSocket::DeleteDataFromList(raw_data_t *pData)
{
	EnterCriticalSection(&m_stRawCriSec);
	list<raw_data_t*>::iterator itr;

	for (itr = m_vecRawDataPkgList.begin(); itr != m_vecRawDataPkgList.end(); itr++)
	{
		if((*itr) == pData)
		{
			m_vecRawDataPkgList.erase(itr);
			LeaveCriticalSection(&m_stRawCriSec);
			return 0;
		}
	}
	LeaveCriticalSection(&m_stRawCriSec);

	return -1;
}

int CSmartSocket::WaitData(int nTimeOut)
{
	WaitForSingleObject(m_hMsgEvent, nTimeOut);
	return 0;
}

int CSmartSocket::ExitRecv()
{
	m_bExit = true;
	SOCKADDR_IN svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	svraddr.sin_port = htons(0);
	//“0”是不可达端口，icmp会返回一个假数据，如果recvfrom返回值是-1.
	if (m_socExit!=INVALID_SOCKET)
	{
		sendto(m_socExit, "0", 1, 0, (const sockaddr*)&svraddr,sizeof(svraddr));
	}
	if(m_hThread)
	{
		::WaitForSingleObject(m_hThread, INFINITE/*TIMEOUT_SOCKET_MS*/);
		m_hThread = NULL;
	}

	return 0;
}

std::string CSmartSocket::GetLocalIP()
{
	if(m_SocketSrvEnd != INVALID_SOCKET)
	{
		struct sockaddr_in sin;
		int len = sizeof(sin);
		if(getsockname(m_SocketSrvEnd, (struct sockaddr *)&sin, &len) == -1)
			return "";
		else
		{
			char fVal[128];
			u_long addrNBO = htonl(sin.sin_addr.s_addr); // make sure we have a value in a known byte order: big endian
			sprintf(fVal, "%u.%u.%u.%u", (addrNBO>>24)&0xFF, (addrNBO>>16)&0xFF, (addrNBO>>8)&0xFF, addrNBO&0xFF);
			return std::string(fVal);
		}
	}
	return "";
}
