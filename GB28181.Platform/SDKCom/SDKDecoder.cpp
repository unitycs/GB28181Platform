#include "StdAfx.h"
#include "SDKDecoder.h"
#include "Log/Log.h"
#include "HUSSDKUnified.h"
#include "ServerConsole.h"

bool SDKDecoder::getDecoderInfo()
{
	CString decoderInfoFileName = appConf.strDevConfDir + "\\" + "decoderInfo.xml";

	CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s\r\n", __FUNCTION__, decoderInfoFileName);
	FILE*fp;
	fopen_s(&fp, decoderInfoFileName, "rb");
	if (NULL == fp)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s open fail\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	std::string xml;
	char buf[256];
	while (!feof(fp))
	{
		int len = fread(buf, 1, 255, fp);
		buf[len] = '\0';
		xml += buf;
	}
	fclose(fp);

	XMLDocument	xmlDoc;
	XMLElement	*cfgNode;
	XMLElement	*node;
	XMLElement	*child;
	//	TiXmlElement	*sibNode = nullptr;
	xmlDoc.Parse(xml.c_str());
	if (xmlDoc.Error())
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s xmlDoc.Error()\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	cfgNode = xmlDoc.FirstChildElement("config");
	if (cfgNode == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s xmlDoc.Error()\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	node = cfgNode->FirstChildElement("decoderInfo");
	if (node == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s no decoderInfo\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}
	child = node->FirstChildElement("decoder");

	while (child)
	{
		decoderInfoStruc decoder;
		decoder.gbId = child->Attribute("gbId");
		string gbPrefix = decoder.gbId.substr(0, 11);

		decoder.guid = child->Attribute("guid");
		decoder.chnIdStart = atoi(child->Attribute("chnIdStart"));
		int chnIdStrat = decoder.chnIdStart;

		decoder.screenIdStart = atoi(child->Attribute("screenIdStart"));
		int screenIdStart = decoder.screenIdStart;

		decoder.maxScreenCount = atoi(child->Attribute("maxScreenCount"));
		decoder.validFlag = atoi(child->Attribute("validFlag"));

		XMLElement	*parent = child;

		child = parent->FirstChildElement("chn");
		while (child)
		{
			decoderChnInfoStruc decoderChn;
			decoderChn.chnNum = atoi(child->Attribute("chnNum"));
			decoderChn.curScreenCount = atoi(child->Attribute("curScreenCount"));
			int chGbSubFix = chnIdStrat + decoderChn.chnNum;
			char tmpBuf[MAX_PATH] = { 0 };
			sprintf_s(tmpBuf, "%s0170%05d", gbPrefix.c_str(), chGbSubFix);
			decoderChn.chnGbId = tmpBuf;

			for (int i = 1; i <= decoderChn.curScreenCount; i++)
			{
				int screenGbSubfix = screenIdStart + decoderChn.chnNum * decoder.maxScreenCount + i - 1;
				sprintf_s(tmpBuf, "%s0210%05d", gbPrefix.c_str(), screenGbSubfix);
				screenInfoStruc screenInfo;
				screenInfo.screenGbId = tmpBuf;
				screenInfo.screenNum = i - 1;
				screenInfo.status = "ON";
				decoderChn.screenInfo.push_back(screenInfo);
			}

			for (int i = decoderChn.curScreenCount + 1; i <= decoder.maxScreenCount; i++)
			{
				int screenGbSubfix = screenIdStart + decoderChn.chnNum * decoder.maxScreenCount + i - 1;
				sprintf_s(tmpBuf, "%s0210%05d", gbPrefix.c_str(), screenGbSubfix);
				screenInfoStruc screenInfo;
				screenInfo.screenGbId = tmpBuf;
				screenInfo.screenNum = i - 1;
				screenInfo.status = "OFF";
				decoderChn.screenInfo.push_back(screenInfo);
			}

			decoder.decoderChnInfo.push_back(decoderChn);
			child = child->NextSiblingElement();
		}
		g_decoderInfo.push_back(decoder);
		child = parent->NextSiblingElement();
	}

	return true;
}

bool SDKDecoder::modifyXmlDivisonInfo(string decoderGbId, int chn, int divisonCount)
{
	CString decoderInfoFileName = appConf.strDevConfDir + "\\" + "decoderInfo.xml";

	CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s\r\n", __FUNCTION__, decoderInfoFileName);
	FILE*fp;
	fopen_s(&fp, decoderInfoFileName, "rb");
	if (NULL == fp)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s open fail\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	std::string xml;
	char buf[256];
	while (!feof(fp))
	{
		int len = fread(buf, 1, 255, fp);
		buf[len] = '\0';
		xml += buf;
	}
	fclose(fp);

	XMLDocument	xmlDoc;
	XMLElement	*cfgNode;
	XMLElement	*node;
	XMLElement	*child;
	//    TiXmlElement	*sibNode = NULL;
	xmlDoc.Parse(xml.c_str());
	if (xmlDoc.Error())
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s xmlDoc.Error()\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	cfgNode = xmlDoc.FirstChildElement("config");
	if (cfgNode == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s xmlDoc.Error()\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}

	node = cfgNode->FirstChildElement("decoderInfo");
	if (node == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s fileName = %s no decoderInfo\r\n", __FUNCTION__, decoderInfoFileName);
		return false;
	}
	child = node->FirstChildElement("decoder");

	while (child)
	{
		decoderInfoStruc decoder;
		decoder.gbId = child->Attribute("gbId");
		if (decoder.gbId != decoderGbId)
		{
			child = child->NextSiblingElement();
			continue;
		}

		string gbPrefix = decoder.gbId.substr(0, 11);

		decoder.guid = child->Attribute("guid");
		decoder.chnIdStart = atoi(child->Attribute("chnIdStart"));
		//    int chnIdStrat = decoder.chnIdStart;

		decoder.screenIdStart = atoi(child->Attribute("screenIdStart"));
		//        int screenIdStart = decoder.screenIdStart;

		decoder.maxScreenCount = atoi(child->Attribute("maxScreenCount"));
		decoder.validFlag = atoi(child->Attribute("validFlag"));

		XMLElement	*parent = child;

		child = parent->FirstChildElement("chn");
		while (child)
		{
			decoderChnInfoStruc decoderChn;
			decoderChn.chnNum = atoi(child->Attribute("chnNum"));
			if (decoderChn.chnNum != chn)
			{
				child = child->NextSiblingElement();
				continue;
			}
			child->SetAttribute("curScreenCount", divisonCount);
			return true;
		}
	}

	return true;
}

CatalogItem SDKDecoder::getSingleDecoderCatalogByGbId(string decoderGbId)
{
	CatalogItem catalog;
	CLog::Log(SDKCOM, LL_NORMAL, "%s gbId = %s \r\n", __FUNCTION__, decoderGbId.c_str());
	int count = g_decoderCatalogInfo.size();
	for (int i = 0; i < count; i++)
	{
		if (g_decoderCatalogInfo[i].GetDeviceID() == decoderGbId.c_str())
		{
			return g_decoderCatalogInfo[i];
		}
	}
	return catalog;
}

void SDKDecoder::getDivisonCatalog(string& parentId, string& divisonName, string divisonGbId, string opType, string& status, int divisonNum, CatalogItem *pCatalog) const
{
	pCatalog->SetParentID(parentId.c_str());
	pCatalog->SetDeviceID(divisonGbId.c_str());
	pCatalog->SetName(divisonName.c_str());
	pCatalog->SetOperateType(opType.c_str());
	pCatalog->SetParental("0");
	CString statusStr = status.c_str();

	pCatalog->SetStatus(statusStr);

	char tmpBuf[MAX_PATH] = { 0 };
	sprintf_s(tmpBuf, "%d", divisonNum);
	pCatalog->m_strChnNum = tmpBuf;
}

bool SDKDecoder::modifDecoderDivisonInfo(string chnId, int divison, decoderInfoStruc& decoderInfo, vector<CatalogItem>& cataLogInfo)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s chnId = %s divison = %d\r\n", __FUNCTION__, chnId.c_str(), divison);
	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	string opType = "MOD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		string gbPrefix = decoder.gbId.substr(0, 11);

		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];
			if (chnInfo.chnGbId == chnId)
			{
				chnInfo.screenInfo.clear();
				int oldScreenCount = chnInfo.curScreenCount;
				chnInfo.curScreenCount = divison;
				if (divison > oldScreenCount)
				{
					opType = "ADD";
				}

				int screenIdStart = decoder.screenIdStart;
				for (int k = 1; k <= chnInfo.curScreenCount; k++)
				{
					int screenGbSubfix = screenIdStart + chnInfo.chnNum * decoder.maxScreenCount + k - 1;
					sprintf_s(tmpBuf, "%s0210%05d", gbPrefix.c_str(), screenGbSubfix);
					screenInfoStruc screenInfo;
					screenInfo.screenGbId = tmpBuf;
					screenInfo.screenNum = k - 1;
					screenInfo.status = "ON";
					chnInfo.screenInfo.push_back(screenInfo);
				}

				for (int k = chnInfo.curScreenCount + 1; k <= decoder.maxScreenCount; k++)
				{
					int screenGbSubfix = screenIdStart + chnInfo.chnNum * decoder.maxScreenCount + k - 1;
					sprintf_s(tmpBuf, "%s0210%05d", gbPrefix.c_str(), screenGbSubfix);
					screenInfoStruc screenInfo;
					screenInfo.screenGbId = tmpBuf;
					screenInfo.screenNum = k - 1;
					screenInfo.status = "OFF";
					chnInfo.screenInfo.push_back(screenInfo);
				}

				decoder.decoderChnInfo[j] = chnInfo;
				modifyXmlDivisonInfo(decoder.gbId, chnInfo.chnNum, divison);
				decoderInfo.gbId = decoder.gbId;
				decoderInfo.decoderChnInfo.push_back(chnInfo);
				g_decoderInfo[i] = decoder;

				int beginIndex = oldScreenCount;
				int endIndex = divison;
				if (opType == "MOD")
				{
					beginIndex = divison;
					endIndex = oldScreenCount;
				}
				for (int m = beginIndex; m < endIndex; m++)
				{
					opType = "MOD";
					screenInfoStruc screenInfo = chnInfo.screenInfo[m];

					CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);

					sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), m);
					string name = tmpBuf;
					getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, &decoderCatalog);
					cataLogInfo.push_back(decoderCatalog);
				}

				CLog::Log(SDKCOM, LL_NORMAL, "%s chnId = %s divison = %d deocderGbId = %s \r\n", __FUNCTION__, chnId.c_str(), divison, decoderInfo.gbId.c_str());
				return true;
			}
		}
	}
	return true;
}

bool SDKDecoder::queryDecoderDivisonInfo(string divisionGbId, int divison, decoderInfoStruc& decoderInfo) const
{
	(divison);
	CLog::Log(SDKCOM, LL_NORMAL, "%s divisionGbId = %s \r\n", __FUNCTION__, divisionGbId.c_str());
	int count = g_decoderInfo.size();
	//    char tmpBuf[512] = {0};
	string opType = "MOD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		string gbPrefix = decoder.gbId.substr(0, 11);

		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];

			//                int screenIdStart = decoder.screenIdStart;
			for (int k = 1; k <= chnInfo.curScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[k - 1];
				if (screenInfo.screenGbId == divisionGbId)
				{
					decoderInfo = decoder;
					decoderInfo.decoderChnInfo.clear();
					chnInfo.screenInfo.clear();
					chnInfo.screenInfo.push_back(screenInfo);
					::CLog::Log(SDKCOM, LL_NORMAL, "%s divisionGbId = %s decoderGbId = %s chnNum = %d divisonNum = %d\r\n", __FUNCTION__, divisionGbId.c_str(), decoder.gbId.c_str(), chnInfo.chnNum, screenInfo.screenNum);
					decoderInfo.decoderChnInfo.push_back(chnInfo);
					return true;
				}
			}
		}
	}
	return true;
}

bool SDKDecoder::queryDecoderInfo(string chnGbId, decoderInfoStruc& decoderInfo) const
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s divisionGbId = %s \r\n", __FUNCTION__, chnGbId.c_str());
	int count = g_decoderInfo.size();
	//    char tmpBuf[512] = {0};
	string opType = "MOD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		string gbPrefix = decoder.gbId.substr(0, 11);

		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];

			if (chnInfo.chnGbId == chnGbId)
			{
				decoderInfo = decoder;
				decoderInfo.decoderChnInfo.clear();
				decoderInfo.decoderChnInfo.push_back(chnInfo);
			}
		}
	}
	return true;
}

void SDKDecoder::getChnCatalog(string& parentId, string& chnName, string chnGbId, string opType, CatalogItem *pCatalog, int chnNum) const
{
	(chnNum);
	pCatalog->SetParentID(parentId.c_str());
	pCatalog->SetDeviceID(chnGbId.c_str());
	pCatalog->SetName(chnName.c_str());
	pCatalog->SetParental("1");
	pCatalog->SetOperateType(opType.c_str());
	pCatalog->SetStatus("ON");
	char tmpBuf[MAX_PATH] = { 0 };
	//sprintf_s(tmpBuf, "%s", chnName);
	CString strChnNum(chnName.c_str());
	pCatalog->m_strChnNum = strChnNum;
}

bool SDKDecoder::getSingleCataLogInfoByGbId(string& gbId, CatalogItem *pCatalog, string& opType)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s gbId = %s \r\n", __FUNCTION__, gbId.c_str());
	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		string gbPrefix = decoder.gbId.substr(0, 11);

		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];
			if (chnInfo.chnGbId == gbId)
			{
				CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
				*pCatalog = decoderCatalog;

				sprintf_s(tmpBuf, "ch_%s_%d", chnInfo.chnGbId.c_str(), chnInfo.chnNum);
				string name = tmpBuf;
				getChnCatalog(decoder.gbId, name, chnInfo.chnGbId, opType, pCatalog, chnInfo.chnNum);
				return true;
			}

			for (int k = 1; k <= chnInfo.curScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[i];
				if (screenInfo.screenGbId == gbId)
				{
					CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
					*pCatalog = decoderCatalog;

					sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), k);
					string name = tmpBuf;
					getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, pCatalog);
					return true;
				}
			}
		}
	}
	return true;
}

bool SDKDecoder::getAllDecoderChnAndDivisonCatalog(int *chnAndDivisonCount, vector<CatalogItem>& catalogInfo)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s \r\n", __FUNCTION__);
	*chnAndDivisonCount = 0;
	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	string opType = "ADD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		string gbPrefix = decoder.gbId.substr(0, 11);

		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];
			CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
			sprintf_s(tmpBuf, "ch_%s_%d", chnInfo.chnGbId.c_str(), chnInfo.chnNum);
			string name = tmpBuf;
			getChnCatalog(decoder.gbId, name, chnInfo.chnGbId, opType, &decoderCatalog, chnInfo.chnNum);
			catalogInfo.push_back(decoderCatalog);
			(*chnAndDivisonCount)++;
			for (int k = 1; k <= decoder.maxScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[k - 1];

				decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);

				sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), k);
				name = tmpBuf;
				getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, &decoderCatalog);
				catalogInfo.push_back(decoderCatalog);
				(*chnAndDivisonCount)++;
			}
		}
	}

	CLog::Log(SDKCOM, LL_NORMAL, "%s chnAndDivisonCount = %d catalogCount = %d\r\n", __FUNCTION__, *chnAndDivisonCount, catalogInfo.size());
	return true;
}

void SDKDecoder::packSingleDecoderChnOrDivisonInfo(char *pszInfo, int nBufLen, const char *pstrSN, string& deviceId, int recordCount, CatalogItem *pCatalog)
{
	CString			m_strDeviceID = deviceId.c_str();
	CBodyBuilder	m_oBodyBuilder;
	int             nCompleteLen = nBufLen;
	int             nCountUsed = 0;
	m_oBodyBuilder.CreateCatalogHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(), recordCount, 1);
	nCountUsed += nCompleteLen;

	nCompleteLen = nBufLen - nCountUsed;
	//  pCatalog->SetStatus("ON");
	//   pCatalog->SetModAction("ADD");
	//   pCatalog->SetParentID(m_strDeviceID);

	m_oBodyBuilder.AddCatalogBody(pszInfo + nCountUsed, nCompleteLen, pCatalog);

	nCountUsed += nCompleteLen;
	nCompleteLen = nBufLen - nCountUsed;
	m_oBodyBuilder.CompleteCatalog(pszInfo + nCountUsed, nCompleteLen);

	CLog::Log(SDKCOM, LL_NORMAL, "%s pszInfo = %s\r\n", __FUNCTION__, pszInfo);
	return;
}

void SDKDecoder::getOneDeocderAllCatalog(string& gbId, vector<CatalogItem>& catalogInfo)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s \r\n", __FUNCTION__);

	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	string opType = "ADD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];
		if (decoder.gbId != gbId)
		{
			continue;
		}
		string gbPrefix = decoder.gbId.substr(0, 11);
		CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
		decoderCatalog.SetParental(appConf.m_UpperList[0].str_ID);
		catalogInfo.push_back(decoderCatalog);
		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];
			decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
			sprintf_s(tmpBuf, "ch_%s_%d", chnInfo.chnGbId.c_str(), chnInfo.chnNum);
			string name = tmpBuf;
			getChnCatalog(decoder.gbId, name, chnInfo.chnGbId, opType, &decoderCatalog, chnInfo.chnNum);
			catalogInfo.push_back(decoderCatalog);

			for (int k = 1; k <= decoder.maxScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[i];

				//          CCatalog decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);

				sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), k);
				name = tmpBuf;
				getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, &decoderCatalog);
				catalogInfo.push_back(decoderCatalog);
			}
		}

		return;
	}
}

void SDKDecoder::getOneDecoderChnAllCatalog(string& gbId, vector<CatalogItem>& catalogInfo)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s \r\n", __FUNCTION__);

	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	string opType = "ADD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];

		string gbPrefix = decoder.gbId.substr(0, 11);
		CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
		decoderCatalog.SetParental(appConf.m_UpperList[0].str_ID);
		catalogInfo.push_back(decoderCatalog);
		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];

			if (chnInfo.chnGbId != gbId)
			{
				continue;
			}

			decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
			sprintf_s(tmpBuf, "ch_%s_%d", chnInfo.chnGbId.c_str(), chnInfo.chnNum);
			string name = tmpBuf;
			getChnCatalog(decoder.gbId, name, chnInfo.chnGbId, opType, &decoderCatalog, chnInfo.chnNum);
			catalogInfo.push_back(decoderCatalog);

			for (int k = 1; k <= decoder.maxScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[i];

				//            CCatalog decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);

				sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), k);
				name = tmpBuf;
				getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, &decoderCatalog);
				catalogInfo.push_back(decoderCatalog);
			}
		}
	}
}

void SDKDecoder::getOneDecoderChnOneDivisonCatalog(string& gbId, vector<CatalogItem>& catalogInfo)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s \r\n", __FUNCTION__);

	int count = g_decoderInfo.size();
	char tmpBuf[MAX_PATH] = { 0 };
	string opType = "ADD";
	for (int i = 0; i < count; i++)
	{
		decoderInfoStruc decoder = g_decoderInfo[i];

		string gbPrefix = decoder.gbId.substr(0, 11);
		CatalogItem decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
		decoderCatalog.SetParental(appConf.m_UpperList[0].str_ID);
		catalogInfo.push_back(decoderCatalog);
		int decoderCount = decoder.decoderChnInfo.size();
		for (int j = 0; j < decoderCount; j++)
		{
			decoderChnInfoStruc chnInfo = decoder.decoderChnInfo[j];

			decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);
			sprintf_s(tmpBuf, "ch_%s_%d", chnInfo.chnGbId.c_str(), chnInfo.chnNum);
			string name = tmpBuf;
			getChnCatalog(decoder.gbId, name, chnInfo.chnGbId, opType, &decoderCatalog, chnInfo.chnNum);
			//      catalogInfo.push_back(decoderCatalog);

			for (int k = 1; k <= decoder.maxScreenCount; k++)
			{
				screenInfoStruc screenInfo = chnInfo.screenInfo[i];
				if (screenInfo.screenGbId != gbId)
				{
					continue;
				}

				catalogInfo.push_back(decoderCatalog);
				//   CCatalog decoderCatalog = getSingleDecoderCatalogByGbId(decoder.gbId);

				sprintf_s(tmpBuf, "divison_%s_%d", screenInfo.screenGbId.c_str(), k);
				name = tmpBuf;
				getDivisonCatalog(chnInfo.chnGbId, name, screenInfo.screenGbId, opType, screenInfo.status, screenInfo.screenNum, &decoderCatalog);
				catalogInfo.push_back(decoderCatalog);
			}
		}
	}
}