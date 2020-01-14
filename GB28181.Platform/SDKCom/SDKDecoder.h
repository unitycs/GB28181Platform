#pragma once
#include "Main/UnifiedMessage.h"
#include "HUSSDKUnified.h"

class SDKDecoder
{
public:
	using XMLDocument = tinyxml2::XMLDocument;
	using XMLElement = tinyxml2::XMLElement;
	SDKDecoder(void)//: g_decoderInfo(nullptr), g_decoderCatalogInfo(nullptr), g_division_vidioIdInfo(nullptr)
	{};
	virtual ~SDKDecoder(void) {};
	//void SetInfomation(
	//		vector<decoderInfoStruc>* g_decoderInfo,
	//		vector<CCatalog>*         g_decoderCatalogInfo,
	//		map<string, string>* g_division_vidioIdInfo
	//	);
	bool getDecoderInfo();
	static bool modifyXmlDivisonInfo(std::string decoderGbId, int chn, int divisonCount);
	CatalogItem getSingleDecoderCatalogByGbId(string decoderGbId);
	void getDivisonCatalog(string& parentId, string& divisonName, string divisonGbId, string opType, string& status, int divisonNum, CatalogItem* pCatalog) const;
	bool modifDecoderDivisonInfo(string chnId, int divison, decoderInfoStruc& decoderInfo, vector<CatalogItem>& cataLogInfo);
	bool queryDecoderDivisonInfo(string divisionGbId, int divison, decoderInfoStruc& decoderInfo) const;
	bool queryDecoderInfo(string chnGbId, decoderInfoStruc& decoderInfo) const;
	void getChnCatalog(string& parentId, string& chnName, string chnGbId, string opType, CatalogItem* pCatalog, int chnNum) const;
	bool getSingleCataLogInfoByGbId(string& gbId, CatalogItem* pCatalog, string& opType);
	bool getAllDecoderChnAndDivisonCatalog(int* chnAndDivisonCount, vector<CatalogItem>& catalogInfo);
	static void packSingleDecoderChnOrDivisonInfo(char* pszInfo, int nBufLen, const char* pstrSN, string& deviceId, int recordCount, CatalogItem* pCatalog);
	void getOneDeocderAllCatalog(string& gbId, vector<CatalogItem>& catalogInfo);
	void getOneDecoderChnAllCatalog(string& gbId, vector<CatalogItem>& catalogInfo);
	void getOneDecoderChnOneDivisonCatalog(string& gbId, vector<CatalogItem>& catalogInfo);

	vector<decoderInfoStruc> g_decoderInfo;
	vector<CatalogItem>        g_decoderCatalogInfo;
	map<string, string> g_division_vidioIdInfo;
};
