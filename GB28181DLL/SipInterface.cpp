#include "StdAfx.h"
#include "SipInterface.h"
#include "tinyxml/tinyxml2.h"

CSipInterface::CSipInterface(void)
{
	m_bIsExit = FALSE;
	m_unSeq = 0;
	strcpy_s(m_strOwnID, "");
	m_fpDeviceStaustAlarm = NULL;
	m_fpPlayBackFinished = NULL;
	m_fpInviteResponsed = NULL;

	m_pStreamer = NULL;
	m_strRecordListXml = "";
	m_strDeviceStatusListXml = "";
	m_strDeviceInfoListXml = "";
	m_strCatalogInfoListXml = "";
	m_nRecordSum = 0;
	nTargetTime = 0;
	m_externalInterface = NULL;
}



BOOL CSipInterface::IsExit()
{
	return m_bIsExit;
}

void CSipInterface::ExitThread()
{
	m_bIsExit = TRUE;
}

DEVICE_PRESETQUERY_INFO_T* CSipInterface::ParseDevicePresetQuery(const char * pXml)
{
	lpDevicePresetQuery_info_t pPresetQueryInfo = new devicePresetQuery_info_t;
	auto myDocument = new  tinyxml2::XMLDocument();
	myDocument->Parse(pXml);  //提取xml字符串的声明部分
	auto pResponse = myDocument->FirstChildElement();//Response
	auto pChildNode = pResponse->FirstChildElement("CmdType");
	if (pChildNode)
	{
		pPresetQueryInfo->strCmdType = pChildNode->GetText();
	}
	pChildNode = pResponse->FirstChildElement("DeviceID");
	if (pChildNode)
	{
		pPresetQueryInfo->strDeviceID = pChildNode->GetText();
	}
	pChildNode = pResponse->FirstChildElement("PresetList");
	if (pChildNode)
	{
		int nNum = atoi(pChildNode->Attribute("Num"));
		pChildNode = pChildNode->FirstChildElement("Item");
		while (pChildNode)
		{
			devicePresetQuery_info_t::DEVICE_PRESET_INFO_T presetInfo = { 0 };
			auto pChildItem = pChildNode->FirstChildElement("PresetID");
			if (pChildItem)
				presetInfo.strPresetId = pChildItem->GetText();
			pChildItem = pChildNode->FirstChildElement("PresetName");
			if (pChildItem)
				presetInfo.strPresetName = pChildItem->GetText();
			pPresetQueryInfo->v_PresetInfo.push_back(presetInfo);
			pChildNode = pChildNode->FirstChildElement("Item");
		}
	}
	delete myDocument;
	return pPresetQueryInfo;
}

DEVICE_CONFIG_INFO_T* CSipInterface::ParseDeviceConfigDownload(const char * pXml)
{
	//auto pConfigInfo = new DEVICE_CONFIG_INFO_T;
	//auto myDocument = new tinyxml2::XMLDocument();
	//myDocument->Parse(pXml);  //提取xml字符串的声明部分
	//auto pResponse = myDocument->FirstChildElement();//Response

	//auto pChildNode = pResponse->FirstChildElement("BasicParam");
	//if (pChildNode)
	//{
	//	DEVICE_CONFIG_INFO_T::BASICPARAM* basicParam = new DEVICE_CONFIG_INFO_T::BASICPARAM;
	//	pConfigInfo->lpBasicParam = basicParam;
	//	pChildNode = pChildNode->FirstChildElement("Name");
	//	if (pChildNode)
	//	{
	//		basicParam->strName = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("DeviceID");
	//	if (pChildNode)
	//	{
	//		strcpy_s(basicParam->strDeviceID, 20, pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("SIPServerID");
	//	if (pChildNode)
	//	{
	//		strcpy_s(basicParam->strSIPServerId, 20, pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("SIPServerIP");
	//	if (pChildNode)
	//	{
	//		strcpy_s(basicParam->strSIPServerIp, 20, pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("SIPServerPort");
	//	if (pChildNode)
	//	{
	//		basicParam->nSIPServerPort = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("DomainName");
	//	if (pChildNode)
	//	{
	//		basicParam->strDomainName = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("Expiration");
	//	if (pChildNode)
	//	{
	//		basicParam->nExpiration = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("Password");
	//	if (pChildNode)
	//	{
	//		basicParam->strPassword = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("HeartBeatInterval");
	//	if (pChildNode)
	//	{
	//		basicParam->nHeartBeatInternal = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("HeartBeatCount");
	//	if (pChildNode)
	//	{
	//		basicParam->nHeartBeatCount = atoi(pChildNode->GetText());
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("VideoParamOpt");
	//if (pChildNode)
	//{
	//	DEVICE_CONFIG_INFO_T::VIDEO_PARAM_OPT_T* videoParam = new DEVICE_CONFIG_INFO_T::VIDEO_PARAM_OPT_T;
	//	pConfigInfo->lpVideoParamOpt = videoParam;
	//	pChildNode = pChildNode->FirstChildElement("VideoFormatOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->strVideoFormatOpt = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("ResolutionOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->strResolutionOpt = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("FrameRateOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->nFrameRateOpt = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("BitRateTypeOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->nBitRateTypeOpt = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("VideoBitRateOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->nVideoBitRateOpt = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("DownloadSpeedOpt");
	//	if (pChildNode)
	//	{
	//		videoParam->strDwonloadSpeedOpt = pChildNode->GetText();
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("VideoParamConfig");
	//if (pChildNode)
	//{
	//	pConfigInfo->nVideoParamConfigNum = atoi(pChildNode->Attribute("Num"));
	//	if (pConfigInfo)
	//	{
	//		DEVICE_CONFIG_INFO_T::VIDEO_PARAM_ATTRIBUTE_T* pVideoParamAttribute = new DEVICE_CONFIG_INFO_T::VIDEO_PARAM_ATTRIBUTE_T[pConfigInfo->nVideoParamConfigNum];
	//		pConfigInfo->lpVideoParamAttribute = pVideoParamAttribute;
	//		int nNum = 0;
	//		pChildNode = pChildNode->FirstChildElement("Item");
	//		while (pChildNode)
	//		{
	//			auto  pItemNode = pChildNode->FirstChildElement("StreamName");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strStreamName = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("VideoFormat");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strVideoFormat = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("Resolution");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strResolution = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("FrameRate");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strFrameRate = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("BitRateType");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strBitRateType = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("VideoBitRate");
	//			if (pItemNode)
	//			{
	//				pVideoParamAttribute[nNum].strVideoBitRate = pItemNode->GetText();
	//			}
	//			pChildNode = pChildNode->NextSiblingElement("Item");
	//			nNum++;
	//		}
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("AudioParamOpt");
	//if (pChildNode)
	//{
	//	auto *audioParam = new DEVICE_CONFIG_INFO_T::AUDIO_PARAM_OPT_T;
	//	pConfigInfo->lpAudioParamOpt = audioParam;
	//	pChildNode = pChildNode->FirstChildElement("AudioFormatOpt");
	//	if (pChildNode)
	//	{
	//		audioParam->strAudioParamOpt = pChildNode->GetText();
	//	}
	//	pChildNode = pChildNode->FirstChildElement("AudioBitRateOpt");
	//	if (pChildNode)
	//	{
	//		audioParam->nAudioBitRateOpt = atoi(pChildNode->GetText());
	//	}
	//	pChildNode = pChildNode->FirstChildElement("SamplingRateOpt");
	//	if (pChildNode)
	//	{
	//		audioParam->strSamplingRateOpt = pChildNode->GetText();
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("AudioParamConfig");
	//if (pChildNode)
	//{
	//	pConfigInfo->nAudioParamConfigNum = atoi(pChildNode->Attribute("Num"));
	//	if (pConfigInfo)
	//	{
	//		auto pAudioParamAttribute = new DEVICE_CONFIG_INFO_T::AUDIO_PARAM_ATTRIBUTE_T[pConfigInfo->nAudioParamConfigNum];
	//		pConfigInfo->lpAudioParamAttribute = pAudioParamAttribute;
	//		int nNum = 0;
	//		pChildNode = pChildNode->FirstChildElement("Item");
	//		while (pChildNode)
	//		{
	//			auto pItemNode = pChildNode->FirstChildElement("StreamName");
	//			if (pItemNode)
	//			{
	//				pAudioParamAttribute[nNum].strStreamName = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("AudioFormat");
	//			if (pItemNode)
	//			{
	//				pAudioParamAttribute[nNum].strAudioFormat = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("AudioBitRate");
	//			if (pItemNode)
	//			{
	//				pAudioParamAttribute[nNum].strAudioBitRate = pItemNode->GetText();
	//			}
	//			pItemNode = pChildNode->FirstChildElement("SamplingRate");
	//			if (pItemNode)
	//			{
	//				pAudioParamAttribute[nNum].strSamplingRate = pItemNode->GetText();
	//			}
	//			pChildNode = pChildNode->NextSiblingElement("Item");
	//			nNum++;
	//		}
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("SVACEncodeConfig");
	//if (pChildNode)
	//{
	//	//<!-- 感兴趣区域参数（可选）-->
	//	auto pItemNode = pChildNode->FirstChildElement("ROIParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("ROINumber");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.ROIParam.nROINumber = atoi(pItemChildNode->GetText());
	//			if (pConfigInfo->SVACEncodeConfig.ROIParam.nROINumber)
	//			{
	//				DEVICE_CONFIG_INFO_T::ROI_PARAM_ATTRIBUTE_T* roiParamAttribute = new DEVICE_CONFIG_INFO_T::ROI_PARAM_ATTRIBUTE_T[pConfigInfo->SVACEncodeConfig.ROIParam.nROINumber];
	//				pConfigInfo->SVACEncodeConfig.ROIParam.lpROIParamAttribute;
	//				int nNum = 0;
	//			auto pItem2Node = pChildNode->FirstChildElement("Item");
	//				while (pItemNode)
	//				{
	//					pItemChildNode = pItemNode->FirstChildElement("ROISeq");
	//					if (pItemNode)
	//					{
	//						roiParamAttribute[nNum].nROISeq = atoi(pItemChildNode->GetText());
	//					}
	//					pItemChildNode = pItemNode->FirstChildElement("TopLeft");
	//					if (pItemNode)
	//					{
	//						roiParamAttribute[nNum].nTopLeft = atoi(pItemChildNode->GetText());
	//					}
	//					pItemChildNode = pItemNode->FirstChildElement("BottomRight");
	//					if (pItemNode)
	//					{
	//						roiParamAttribute[nNum].nBottomRight = atoi(pItemChildNode->GetText());
	//					}
	//					pItemChildNode = pItemNode->FirstChildElement("ROIQP");
	//					if (pItemNode)
	//					{
	//						roiParamAttribute[nNum].nROIQP = atoi(pItemChildNode->GetText());
	//					}
	//					pItemNode = pChildNode->NextSiblingElement("Item");
	//					nNum++;
	//				}
	//				pItem2Node = pChildNode->FirstChildElement("BackGroundQP");
	//				if (pItem2Node)
	//				{
	//					pConfigInfo->SVACEncodeConfig.ROIParam.nBackGroundQP = atoi(pItemChildNode->GetText());
	//				}
	//				pItem2Node = pChildNode->FirstChildElement("BackGroundSkipFlag");
	//				if (pItem2Node)
	//				{
	//					pConfigInfo->SVACEncodeConfig.ROIParam.nBackGroundSkipFlag = atoi(pItemChildNode->GetText());
	//				}
	//			}
	//		}
	//	}
	//	//<!-- SVC参数（可选）-->
	//	pItemNode = pChildNode->FirstChildElement("SVCParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("SVCFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SVCParam.nSVCFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("SVCSTMMode");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SVCParam.nSVCSTMMode = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("SVCSpaceDomainMode");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SVCParam.nSVCSpaceDomainMode = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("SVCTimeDomainMode");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SVCParam.nSVCTimeDomainMode = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//	//<!--监控专用信息参数（可选）-->
	//	pItemNode = pChildNode->FirstChildElement("SurveillanceParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("TimeFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SurveillanceParam.nTimeFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("EventFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SurveillanceParam.nEventFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("AlertFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.SurveillanceParam.nAlertFlag = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//	//<!--加密与认证参数（可选）-->
	//	pItemNode = pChildNode->FirstChildElement("EncryptParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("EncryptionFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.EncryptParam.nEncryptionFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("AuthenticationFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.EncryptParam.nAuthenticationFlag = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//	//<!--音频参数（可选）-->
	//	pItemNode = pChildNode->FirstChildElement("AudioParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("AudioRecognitionFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACEncodeConfig.AudioParam.nAudioRecognitionFlag = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//}
	//pChildNode = pResponse->FirstChildElement("SVACDecodeConfig");
	//if (pChildNode)
	//{
	//	//<!-- SVC参数（可选）-->
	//	auto pItemNode = pChildNode->FirstChildElement("SVCParam");
	//	if (pItemNode)
	//	{
	//		auto pItemChildNode = pItemNode->FirstChildElement("SVCSTMMode");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACDecodeConfig.SVCParam.nSVCSTMMode = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//	//<!--监控专用信息参数（可选）-->
	//	auto pItemNode = pChildNode->FirstChildElement("SurveillanceParam");
	//	if (pItemNode)
	//	{
	//	auto pItemChildNode = pItemNode->FirstChildElement("TimeShowFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACDecodeConfig.SurveillanceParam.nTimeShowFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("EventShowFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACDecodeConfig.SurveillanceParam.nEventShowFlag = atoi(pItemChildNode->GetText());
	//		}
	//		pItemChildNode = pItemNode->FirstChildElement("AlerShowtFlag");
	//		if (pItemChildNode)
	//		{
	//			pConfigInfo->SVACDecodeConfig.SurveillanceParam.nAlertShowFlag = atoi(pItemChildNode->GetText());
	//		}
	//	}
	//}
	//delete myDocument;
	//return pConfigInfo;
	return nullptr;
}

int CSipInterface::ParseRealPlayUrl(const char * pXml, char* url)
{
	auto pPresetQueryInfo = new devicePresetQuery_info_t;
	auto myDocument = new tinyxml2::XMLDocument();
	myDocument->Parse(pXml);  //提取xml字符串的声明部分
	auto pResponse = myDocument->FirstChildElement();//Response
	auto pChildNode = pResponse->FirstChildElement("PlayUrl");
	if (pChildNode)
	{
		strcpy(url, pChildNode->GetText());
	}
	delete myDocument;
	return TRUE;
}

int CSipInterface::ParseDecoderStatusVideoDeviceID(const char * pXml, char* strVideoDeviceID)
{
	auto pPresetQueryInfo = new devicePresetQuery_info_t();
	auto myDocument = new tinyxml2::XMLDocument();
	myDocument->Parse(pXml);  //提取xml字符串的声明部分
	auto pResponse = myDocument->FirstChildElement();//Response
	auto pChildNode = pResponse->FirstChildElement("VideoDeviceID");
	if (pChildNode)
	{
		strcpy(strVideoDeviceID, pChildNode->GetText());
	}
	delete myDocument;
	return TRUE;
}

int CSipInterface::SetDeviceStatusCallBack(DeviceStaustAlarm fpDeviceStaustAlarm)
{
	m_fpDeviceStaustAlarm = fpDeviceStaustAlarm;
	return 0;
}

int CSipInterface::SetPlayBackFinishedCallBack(PlayBackFinished fpPlayBackFinished)
{
	m_fpPlayBackFinished = fpPlayBackFinished;
	return 0;
}

int CSipInterface::SetInviteResponseInfoCallback(InviteResponsed fpInviteResponsed) {
	m_fpInviteResponsed = fpInviteResponsed;
	return 0;
}
