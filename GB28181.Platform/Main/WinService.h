#pragma once
#include <winsvc.h>

void WinServiceInit(); 
BOOL WinServiceIsInstalled(); 
BOOL WinServiceInstall(); 
BOOL WinServiceUninstall(); 

void WinServiceLogEvent(LPCTSTR pszFormat, ...); 
//void WINAPI WinServiceMain(); 
void WINAPI WinServiceCtrl(DWORD dwOpcode); 

extern TCHAR szServiceName[MAX_PATH]; 
extern BOOL bInstall; 
extern SERVICE_STATUS_HANDLE hServiceStatus; 
extern SERVICE_STATUS status; 
extern DWORD dwThreadID; 