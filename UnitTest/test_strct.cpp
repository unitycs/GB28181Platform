#include "catch.hpp"
#include <iostream>
#include <windows.h>
typedef struct VIDEO_PARAM_ATTRIBUTE_T
{
	~VIDEO_PARAM_ATTRIBUTE_T()
	{
		if (strStreamName) delete[]strStreamName;
		if (strVideoFormat) delete[]strVideoFormat;
		if (strResolution) delete[]strResolution;
		if (strFrameRate) delete[]strFrameRate;
		if (strBitRateType) delete[]strBitRateType;
		if (strVideoBitRate) delete[]strVideoBitRate;
	}
	int nStreamNameLen;			//length of StreamName
	char* strStreamName;		//SteamName
	int nVideoFormatLen;		//length of VideoFormat
	char* strVideoFormat;		//VideoFormat
	int nResolutionLen;			//length of Resolution
	char* strResolution;		//Resolution
	int nFrameRateLen;			//length of FrameRate
	char* strFrameRate;			//FrameRate
	int nBitRateTypeLen;		//length of BitRateType
	char* strBitRateType;		//BitRateType
	int nVideoBitRateLen = 0;		//length of VideoBitRate
	char* strVideoBitRate = nullptr;		//VideoBitRate
	char* strn[MAX_PATH] = { 0 };		//VideoBitRate
}videoParamAttribute_t, *lpVideoParamAttribute_t;


TEST_CASE
(
	"test makeword",
	"[makeword]"
)
{
	SECTION(" MAKEWORD(0,8)") {
		auto dWaitTm = MAKEWORD(0, 8);
		std::cout << dWaitTm << std::endl;

		REQUIRE(dWaitTm == 2048);
	}
	SECTION(" MAKEWORD(2000,0)") {
		auto dWaitTm = MAKEWORD(2000, 0);
		std::cout << dWaitTm << std::endl;

		REQUIRE(dWaitTm != 2000);
	}
}


