#pragma once

#ifdef WIN_SERVICE

void WINAPI WinServiceCtrl(DWORD dwOpcode);
void WINAPI WinServiceMainProc();
int  WinServiceMain(_TCHAR* argv[]);
int	 SIPConsoleMain();
#endif