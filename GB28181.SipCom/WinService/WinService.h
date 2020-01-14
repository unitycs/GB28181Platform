#pragma once


#ifdef WIN_SERVICE
#include <winsvc.h>
class WinServiceManager{
public:
	WinServiceManager():
	bInstall(FALSE),
	hServiceStatus(nullptr),
	dwThreadID(0),
	serviceMainProc(nullptr)
	{};
	WinServiceManager(TCHAR* inszServiceName,HANDLE mainProcHander):
	bInstall(FALSE),
	hServiceStatus(nullptr),
	dwThreadID(0),
	serviceMainProc(mainProcHander)
	{
		_tcsncpy_s(szServiceName, inszServiceName, MAX_PATH);
	};

	~WinServiceManager(){};
	void WinServiceInit(); 
	BOOL WinServiceIsInstalled(); 
	BOOL WinServiceInstall(); 
	BOOL WinServiceUninstall(); 
	void WinServiceLogEvent(LPCTSTR pszFormat, ...); 
	BOOL StartServiceDispatcher();
	//void WINAPI WinServiceMain(); 
	//static void WINAPI WinServiceCtrl(DWORD dwOpcode); 
	void SetServiceName(TCHAR* inszServiceName);
	void SetServiceMainProc(HANDLE mainProcHander);
//	typedef int (*ConsoleMainCB)();
//	ConsoleMainCB consoleMainPtr;
//	void SetServiceMainProcCallBack(ConsoleMainCB mainProcCallBackHander);
	
	TCHAR						szServiceName[MAX_PATH]; 
	BOOL						bInstall; 
	SERVICE_STATUS_HANDLE		hServiceStatus; 
	SERVICE_STATUS				status; 
	DWORD						dwThreadID; 
	HANDLE						serviceMainProc;
};

#endif