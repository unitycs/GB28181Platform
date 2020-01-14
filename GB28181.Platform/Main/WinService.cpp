//定义全局函数变量
#include "StdAfx.h"
#include "WinService.h"

TCHAR					szServiceName[MAX_PATH];
BOOL					bInstall;
DWORD					dwThreadID;
SERVICE_STATUS_HANDLE	hServiceStatus;
SERVICE_STATUS			status;

void WinServiceInit()
{
	hServiceStatus = nullptr;
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	status.dwWin32ExitCode = 0;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;
}

void WINAPI WinServiceCtrl(DWORD dwOpcode)
{
	switch (dwOpcode)
	{
	case SERVICE_CONTROL_STOP:
		status.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(hServiceStatus, &status);
		PostThreadMessage(dwThreadID, WM_CLOSE, 0, 0);
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
		WinServiceLogEvent(_T("Bad service request"));
		OutputDebugString(_T("Bad service request"));
	}
}

BOOL WinServiceIsInstalled()
{
	BOOL bResult = FALSE;

	//打开服务控制管理器
	SC_HANDLE hSCM = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

	if (hSCM != nullptr)
	{
		//打开服务
		SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_QUERY_CONFIG);
		if (hService != nullptr)
		{
			bResult = TRUE;
			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCM);
	}
	return bResult;
}

BOOL WinServiceInstall()
{
	if (WinServiceIsInstalled())
		return TRUE;

	//打开服务控制管理器
	SC_HANDLE hSCM = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (hSCM == nullptr)
	{
		MessageBoxEx(nullptr, _T("Couldn't open service manager"), szServiceName, MB_OK, 0);
		//MessageBox(NULL, _T("Couldn't open service manager"), szServiceName, MB_OK);
		return FALSE;
	}

	// Get the executable file path
	TCHAR szFilePath[MAX_PATH];
	GetModuleFileName(nullptr, szFilePath, MAX_PATH);

	//创建服务
	SC_HANDLE hService = ::CreateService(hSCM, szServiceName, szServiceName,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		szFilePath, nullptr, nullptr, _T(""), nullptr, nullptr);

	if (hService == nullptr)
	{
		CloseServiceHandle(hSCM);
		MessageBoxEx(nullptr, _T("Couldn't create service"), szServiceName, MB_OK, 0);
		//MessageBox(NULL, _T("Couldn't create service"), szServiceName, MB_OK);
		return FALSE;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	return TRUE;
}

BOOL WinServiceUninstall()
{
	if (!WinServiceIsInstalled())
		return TRUE;

	SC_HANDLE hSCM = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

	if (hSCM == nullptr)
	{
		MessageBoxEx(nullptr, _T("Couldn't open service manager"), szServiceName, MB_OK, 0);
		//MessageBox(NULL, _T("Couldn't open service manager"), szServiceName, MB_OK);
		return FALSE;
	}

	SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_STOP | DELETE);

	if (hService == nullptr)
	{
		CloseServiceHandle(hSCM);
		MessageBoxEx(nullptr, _T("Couldn't open service"), szServiceName, MB_OK, 0);
		//MessageBox(NULL, _T("Couldn't open service"), szServiceName, MB_OK);
		return FALSE;
	}
	SERVICE_STATUS m_status;
	ControlService(hService, SERVICE_CONTROL_STOP, &m_status);

	//删除服务
	BOOL bDelete = ::DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);

	if (bDelete)
		return TRUE;

	WinServiceLogEvent(_T("Service could not be deleted"));
	return FALSE;
}

void WinServiceLogEvent(LPCTSTR pFormat, ...)
{
	TCHAR    chMsg[256];
	HANDLE  hEventSource;
	LPTSTR  lpszStrings[1];
	va_list pArg;

	va_start(pArg, pFormat);
	_vstprintf_s(chMsg, pFormat, pArg);
	va_end(pArg);

	lpszStrings[0] = chMsg;

	hEventSource = RegisterEventSource(nullptr, szServiceName);
	if (hEventSource != nullptr)
	{
		ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, nullptr, 1, 0, const_cast<LPCTSTR*>(&lpszStrings[0]), nullptr);
		DeregisterEventSource(hEventSource);
	}
}