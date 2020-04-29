#pragma once
#include <atlstr.h>
#include <string>

#ifdef UNICODE
#define _L(x) __L(x)
#define __L(x) L##x
#define _tstring	wstring
#else
#define _L(x) (x)
#define _tstring	string
#endif

#ifdef GB28181DLL_EXPORTS
#define GB28181DLL_API __declspec(dllexport)
#define EXPIMP_TEMPLATE
#else
#define GB28181DLL_API __declspec(dllimport)
#define EXPIMP_TEMPLATE extern
#endif


using namespace std;
class CLog
{
public:
	CLog(void);
	virtual ~CLog(void);

	static wstring ANSIToUnicode(const char* str);
	static string UnicodeToANSI(const WCHAR * wstrSrc);
};

