// stdafx.cpp : 只包括标准包含文件的源文件
// GB28181DLL.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"
#include "tinyxml/tinyxml2.h"

HMODULE g_dllHmodule = 0;
bool GetLocalIp(char* localIp)
{
	bool nRet = false;
	auto pDocument = new tinyxml2::XMLDocument();
	string strXmlPath = GetCurrentMoudulePath() + "/GB28181Adapter.xml";
	if (pDocument)
		pDocument->LoadFile(strXmlPath.c_str());
	else
		goto Error;

	auto pRootElement = pDocument->RootElement();				//GB28181Adapter
	if (NULL == pRootElement)
		goto Error;
	auto  pLocalIpElement = pRootElement->FirstChildElement("localIp");	//localIp

	if (NULL == pLocalIpElement)
		goto Error;
	strcpy(localIp, pLocalIpElement->GetText());

	nRet = true;
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s GetLocalIp by GB28181Adapter.xml\n", __FUNCTION__);
Error:
	if (NULL != pDocument)
		delete pDocument;

	return nRet;
}

int GetPtzSpeed()
{
	int nRet = 0;
	auto  pDocument = new tinyxml2::XMLDocument();
	string strXmlPath = GetCurrentMoudulePath() + "\\devices\\GB28181\\GB28181Adapter.xml";
	if (pDocument)
		pDocument->LoadFile(strXmlPath.c_str());
	else
		goto Error;

	auto  pRootElement = pDocument->RootElement();				//GB28181Adapter
	if (NULL == pRootElement)
		goto Error;
	auto  pLocalIpElement = pRootElement->FirstChildElement("ptzSpeed");	//ptzSpeed

	if (NULL == pLocalIpElement)
		goto Error;

	nRet = atoi(pLocalIpElement->GetText());

	//g_objLog.LogoutInfo(k_LOG_DLL, "%s GetPtzSpeed by GB28181Adapter.xml nRet:%d \n", __FUNCTION__, nRet);
Error:
	if (NULL != pDocument)
		delete pDocument;
	if (nRet == 0)
		return 0xC8;
	else
		return nRet;
}

std::string GetCurrentMoudulePath()
{
	char fileName[256] = { 0 };
	//HMODULE hHmodule = _AtlBaseModule.GetModuleInstance();

	GetModuleFileNameA(/*hHmodule*/g_dllHmodule, fileName, MAX_PATH);
	CStringA temFileName = fileName;
	temFileName = temFileName.Mid(0, temFileName.ReverseFind('\\'));
	string strRet = temFileName.GetBuffer(0);
	return strRet;
}

// TODO: 在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用