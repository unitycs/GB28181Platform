#pragma once
#include "module.h"
#include "./../TaskBase/Task.h"

class CInviteTask :
	public CModule, public CTask
{
public:
	CInviteTask(SOCKET sock, SOCKADDR_IN tSockAddr);
	~CInviteTask(void);

	virtual void Init() override;
	virtual void Cleanup() override;

//protected:
	//static char		invite_req[4096];
	//static char		invite_ack[4096];
	//static char		bye_req[4096];

//	static UINT AFX_CDECL pfnLogProc(LPVOID lParam);
};
