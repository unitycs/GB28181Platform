#pragma once

#include "winsock.h"
#include "Log.h"
#include <string>
#include <list>
using namespace std;

typedef struct RAW_DATA_T
{
	RAW_DATA_T(int nSize)
	{
		pDataBuf = new char[nSize];
		bIsReaded = FALSE;
		nBufSize = nSize;
	}
	~RAW_DATA_T()
	{
		if(pDataBuf)
			delete []pDataBuf;
	}
	int GetBufSize(){return nBufSize;}
	int nBufSize;
	char *pDataBuf;
	BOOL bIsReaded;

}raw_data_t;

#define		LEN_RECEIVE		65536		// 每次接收的数据量
#define		TIMEOUT_RECV	5			// 接收超时的时间限
#define     TIMEOUT_SOCKET_MS			5000
#define		TIMEOUT_WAIT_DEGREE		60
#define		TIMEOUT_WAIT		50
#define SAFE_DELETE(p) { if(p) { delete (p); (p)=NULL; } }


class CUACTCP;
class CSmartSocket
{
public:

	CSmartSocket(void);
	CSmartSocket(CUACTCP *pParent);
	virtual ~CSmartSocket(void);
	SOCKET Init(int nLocalPort);
	bool Connect(_tstring svrip, u_short port, bool bIsStart = true);	// 连接到远程服务端
	void Disconnect(void);										// 从远程服务端断开
	bool SendData(const char *buf, u_int datalength);			// 发送指定长度的数据
	bool SendString(const TCHAR* buf, u_int datalength);

	raw_data_t* DequeuePackage(BOOL bIsDelete = FALSE);
	int DeleteDataFromList(raw_data_t *pData);
	int AddDataToList(raw_data_t* pData);
	int ProRawData(char *pBuf, int nBufSize);
	int WaitData(int nTimeOut);
	int ExitRecv();
	SOCKET GetSocket(){return m_SocketSrvEnd;}
	int Login();
	virtual UINT ThreadPro() = 0;

	std::string GetLocalIP();
protected:
	//HANDLE m_hExitEvent;
	HANDLE m_hMsgEvent;
	SOCKET m_SocketSrvEnd;										// 通信用的socket
	HANDLE m_hThread;
	volatile bool m_bExit;
	SOCKADDR_IN m_svraddr;
	UINT m_unThreadID;
	list<raw_data_t *> m_vecRawDataPkgList;
	CRITICAL_SECTION m_stRawCriSec;	
	SOCKET m_socExit;
	CUACTCP * m_pParent;

	raw_data_t *pIncompleteData;
	int nSurplus;
protected:
	static UINT WINAPI RecvThread(LPVOID pParam);
};
