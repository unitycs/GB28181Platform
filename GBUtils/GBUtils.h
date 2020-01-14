// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the GBUTILS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// GBUTILS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#ifdef GBUTILS_EXPORTS
#define GBUTILS_API __declspec(dllexport)
#else
#define GBUTILS_API __declspec(dllimport)
#endif

#ifndef SAFE_DEL_PTR
#define  SAFE_DEL_PTR(ptr) if(ptr!=nullptr) delete ptr; ptr=NULL;
#endif // !SAFE_DEL_PTR

#include "Algorithm/des.h"
#include "Algorithm/base64.h"
#include "Algorithm/md5.h"
#include "Algorithm/des_more.h"
#include "Algorithm/Algorithm.h"
#include <string>

// This class is exported from the GBUtils.dll
class GBUTILS_API CGBUtils {
public:
	CGBUtils(void);
	// TODO: add your methods here.

	const static    GUID NewGUID(bool withBrakets = false);

	const static 	std::string GUIDToCString(const GUID & guid, bool withBrackets = true);

	const static    std::string NewGUIDStr(bool withBrakets = false);

	static void     StringCpy_s(char * pszDst, int nDstBufLen, const char * pszSrc);

	const static    std::string make_endpoint(const char * in_ip_addr, const char * serviceName, char * out_ip_adrr = nullptr, int buffer_lenth = 0);
};

extern GBUTILS_API int nGBUtils;

GBUTILS_API int fnGBUtils(void);