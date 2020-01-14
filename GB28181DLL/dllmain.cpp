// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#define GB28181_LOG4_PROPERTIES ".\\devices\\GB28181\\GB28181.properties"
#define GB28181_LOG4_LOGGER "gb28181"

//CLog4cpp g_objLog;

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			//log4wr_init(GB28181_LOG4_LOGGER, GB28181_LOG4_PROPERTIES);
				/*	g_dllHmodule = hModule;
			string strXmlPath = GetCurrentMoudulePath() + "/GB28181.properties";

			char srcPath[MAX_PATH] = {0};
			::GetCurrentDirectoryA(MAX_PATH, srcPath);
			::SetCurrentDirectoryA(GetCurrentMoudulePath().c_str());
			log4wr_init(GB28181_LOG4_LOGGER, strXmlPath.c_str());
			::SetCurrentDirectoryA(srcPath);*/
			//g_objLog.LogoutDebug(k_LOG_DLL, "%s ----------------GB28181 init------version = %s build_time %s %s----------\r\n", __FUNCTION__,"1.0", __DATE__, __TIME__);
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

