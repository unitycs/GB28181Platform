#pragma once

#include <afxmt.h>

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID);


// 启动SIPCom子进程
void StartupSubProcess();