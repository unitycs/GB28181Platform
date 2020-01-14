#pragma once
class CTask
{
public:
	CTask(SOCKET sock, SOCKADDR_IN tSockAddr);
	virtual ~CTask(void);

	void InitParam();

	static void CreateRecvThread(SOCKET sock);
	static UINT AFX_CDECL pfnRecvProc( LPVOID lParam );
	static UINT AFX_CDECL pfnSendProc( LPVOID lParam );
	static void ExitRecvProc();
protected:
	int		nConc;			// 并发线程数
	int		nMaxPerSec;		// 单线程每秒最大发送登录/注销次数（可能无法达到）
	int		nMaxCount;		// 总共发送多少次完整的登录/注销请求
	CString	m_strHead;		// SIP包头格式化串
	CString	m_strBody;		// SIP包体格式化串

	SOCKET			m_sock;
	SOCKADDR_IN		m_tSockAddr;
	static bool		m_bIsExit;
};

