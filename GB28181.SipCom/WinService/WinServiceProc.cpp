#include "StdAfx.h"
#ifdef WIN_SERVICE
#include "WinServiceProc.h"
#include "WinService.h"

WinServiceManager wsm;

void WINAPI WinServiceCtrl(DWORD dwOpcode) 
{ 
	
	switch (dwOpcode) 
	{ 
	case SERVICE_CONTROL_STOP: 
		wsm.status.dwCurrentState = SERVICE_STOP_PENDING; 
		SetServiceStatus( wsm.hServiceStatus, & wsm.status); 
		PostThreadMessage( wsm.dwThreadID, WM_CLOSE, 0, 0); 
		break; 
	case SERVICE_CONTROL_PAUSE: 
		break; 
	case SERVICE_CONTROL_CONTINUE: 
		break; 
	case SERVICE_CONTROL_INTERROGATE: 
		break; 
	case SERVICE_CONTROL_SHUTDOWN: 
		break; 
	default: 
		 wsm.WinServiceLogEvent(_T("Bad service request")); 
		OutputDebugString(_T("Bad service request")); 
	} 
} 

void WINAPI WinServiceMainProc() 
{
	
	// Register the control request handler 
	wsm.status.dwCurrentState = SERVICE_START_PENDING; 
	wsm.status.dwControlsAccepted = SERVICE_ACCEPT_STOP; 
 
	wsm.hServiceStatus = RegisterServiceCtrlHandler(wsm.szServiceName, WinServiceCtrl); 
	if (wsm.hServiceStatus == nullptr) 
	{ 
		wsm.WinServiceLogEvent(_T("Handler not installed")); 
		return; 
	} 
	SetServiceStatus(wsm.hServiceStatus, &wsm.status); 

	wsm.status.dwWin32ExitCode = S_OK; 
	wsm.status.dwCheckPoint = 0; 
	wsm.status.dwWaitHint = 0; 
	wsm.status.dwCurrentState = SERVICE_RUNNING; 
	SetServiceStatus(wsm.hServiceStatus, &wsm.status); 
	wsm.dwThreadID = GetCurrentThreadId(); 
	SIPConsoleMain();
	wsm.status.dwCurrentState = SERVICE_STOPPED; 
	SetServiceStatus(wsm.hServiceStatus, &wsm.status); 
	OutputDebugString(_T("Service stopped")); 
} 

int  WinServiceMain(_TCHAR* argv[])
{
	 wsm.SetServiceName(_T("Honeywell_GB28181_SIPCOM_Service"));
	 wsm.SetServiceMainProc(WinServiceMainProc);

	 wsm.WinServiceInit();
	if(argv[1])
	{
		if (_stricmp(argv[1], "/install") == 0) 
		{ 
			wsm.WinServiceInstall(); 
			return 0;
		} 
		 if (_stricmp(argv[1], "/uninstall") == 0) 
		{ 
			wsm.WinServiceUninstall(); 
			return 0;
		}
		 if(_stricmp(argv[1], "/config") == 0)
		{
			// TODO:
			return 0;
		}
	}

	if (!wsm.StartServiceDispatcher()) 
	{ 
		wsm.WinServiceLogEvent(_T("Register Service Main Function Error!")); 
	} 

	return 0/*nRetCode*/;

}
#endif
