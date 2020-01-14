// GBUtils.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "GBUtils.h"
#include <combaseapi.h>
#include "Algorithm/Algorithm.h"

// This is an example of an exported variable
GBUTILS_API int nGBUtils = 0;

// This is an example of an exported function.
GBUTILS_API int fnGBUtils(void)
{
	return 42;
}


// This is the constructor of a class that has been exported.
// see GBUtils.h for the class definition
CGBUtils::CGBUtils()
{

	return;
}

const GUID CGBUtils::NewGUID(bool withBrakets)
{
	GUID newGUID = { 0 };
	CoCreateGuid(&newGUID);
	return	newGUID;
}


const std::string CGBUtils::GUIDToCString(const GUID &guid, bool withBrackets)
{
	if (guid == GUID_NULL)
		return{};

	std::string strGUID_Head = withBrackets ? "{" : "";
	std::string strGUID_Tail = withBrackets ? "}" : "";

	char szGuidBuf[128] = { 0 };

	sprintf_s(szGuidBuf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
		guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return  strGUID_Head + szGuidBuf + strGUID_Tail;
}


const std::string CGBUtils::NewGUIDStr(bool withBrakets)
{

	return GUIDToCString(NewGUID(), false);

}

// ×Ö·û´®¿½»ß
void  CGBUtils::StringCpy_s(char *pszDst, int nDstBufLen, const char *pszSrc)
{
	if (pszSrc)
	{
		int nSrcLen = strlen(pszSrc);
		if (nDstBufLen > nSrcLen)
		{
			memcpy(pszDst, pszSrc, nSrcLen + 1);
		}
		else
		{
			memcpy(pszDst, pszSrc, nDstBufLen - 1);
			pszDst[nDstBufLen - 1] = '\0';
		}
	}
}


const std::string CGBUtils::make_endpoint(const char* in_ip_addr, const char * serviceName, char * out_ip_adrr, int buffer_lenth)
{

	if (in_ip_addr && serviceName)
	{
		std::string service_end_point_url = "http://";
		service_end_point_url = service_end_point_url + in_ip_addr + R"(/HUSSite/Serviceasmx/)" + serviceName + R"(.asmx)";

		if (out_ip_adrr != nullptr && buffer_lenth > 0)
		{
			StringCpy_s(out_ip_adrr, buffer_lenth, service_end_point_url.c_str());
		}
		return service_end_point_url;
	}

	return nullptr;
}
