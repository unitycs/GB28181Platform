#include "stdafx.h"
#include "Log.h"

wstring CLog::ANSIToUnicode(const char* str)
{
	int  len = 0;
	len = strlen(str);
	int  unicodeLen = ::MultiByteToWideChar( CP_ACP,
											0,
											str,
											-1,
											NULL,
											0 );  
	wchar_t *  pUnicode;  
	pUnicode = new  wchar_t[unicodeLen+1];  
	memset(pUnicode,0,(unicodeLen+1)*sizeof(wchar_t));  
	::MultiByteToWideChar( CP_ACP,
							0,
							str,
							-1,
							(LPWSTR)pUnicode,
							unicodeLen );  
	wstring  rt;  
	rt = (wchar_t*)pUnicode;
	delete  []pUnicode; 

	return  rt;  
}

string CLog::UnicodeToANSI(const WCHAR * wstrSrc)
{
	CHAR*     pElementText;
	int    iTextLen;
	string strTmp;
	// wide char to multi char
	iTextLen = WideCharToMultiByte(CP_ACP, 0, wstrSrc, -1, NULL, 0, NULL, NULL);

	pElementText = new char[iTextLen + 1];
	memset(( void* )pElementText, 0, (iTextLen + 1));

	if(0 == WideCharToMultiByte(CP_ACP, 0, wstrSrc, -1, pElementText, iTextLen, NULL, NULL))
	{
		delete[] pElementText;
		return strTmp;
	}
	strTmp = pElementText;
	delete[] pElementText;

	return strTmp;
}

CLog::CLog(void)
{
}


CLog::~CLog(void)
{
}

