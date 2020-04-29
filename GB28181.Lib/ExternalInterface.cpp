#include "StdAfx.h"
#include <atlcomtime.h>
#include "ExternalInterface.h"
#include "log.h"
#include "resource.h"
#include "commctrl.h"
#include "tinyxml/tinyxml2.h"
//#include "UACTCP.h"
//#include "SipClient.h"

const char *g_pInfo = NULL;
const char *g_pStatus = NULL;
const char *g_pCatalog = NULL;
const char *g_pPresetQuery = NULL;
const char *g_pDeviceConfigDownload = NULL;
HWND g_hwndListInfo = 0;
HWND g_hwndListAlarm = 0;
HWND g_hwndEditCatalog = 0;

string GetStringBetween(string &strData, char *strFront, char *strBack, BOOL bIsInclude)
{

	string strTmp;

	int nPos_L = strData.find(strFront);
	int nPos_R = strData.find(strBack);
	if (0 > nPos_L || 0 > nPos_R || nPos_L >= nPos_R)
		return strTmp;

	if (false == bIsInclude)
		strTmp = strData.substr(nPos_L + strlen(strFront), nPos_R - nPos_L - strlen(strBack) + 1);
	else
		strTmp = strData.substr(nPos_L, nPos_R - nPos_L + strlen(strBack));

	return strTmp;

}
int get_free_port(const char *local_ip, int nPorts[2])
{
	int nSocket = -1;
	int nLen = 0;
	int nPortCount = 0;
	int nIdx = 6500;
	for(int nIdx = 6500; nIdx < 65535; nIdx++)
	{
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(nIdx);
//		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_addr.s_addr = inet_addr(local_ip);
	
		nSocket = socket(PF_INET, SOCK_DGRAM, 0);

		if(nSocket < 0){
			//printf("socket() error:%s\n", strerror(errno));
			return -1;
		}
	
		if(bind(nSocket, (struct sockaddr *)&sin, sizeof(sin)) != 0)
		{
			closesocket(nSocket);
			//下次从偶数开始
			if(0 == nIdx % 2)
				nIdx++;

			continue;
		}

		nLen = sizeof(sin);
		if(getsockname(nSocket, (struct sockaddr *)&sin, &nLen) != 0)
		{
			printf("getsockname() error:%s\n", strerror(errno));
			closesocket(nSocket);
			return -1;
		}

		if(nSocket != -1)
		{
			nPorts[nPortCount] = nIdx;
			closesocket(nSocket);
			nPortCount++;
		}

		if(2 <= nPortCount)
			return 0;
	}

	return -1;
}

CExternalInterface::CExternalInterface(void)
{
	m_pCUATcp = NULL;
}

CExternalInterface::~CExternalInterface(void)
{

	if(m_pCUATcp)
	{
		//m_pCUATcp->ExitThread();
		m_pCUATcp->UnInit();
		delete m_pCUATcp;
		m_pCUATcp = NULL;
	}
}

int CExternalInterface::Init(const char *pstrUACID, const char *pstrIP, const char *pstrPort)
{
	//ShowResult(STATUS_INQUIRE_TYPE);
	//g_objLog.LogoutInfo(k_LOG_DLL, "%s 初始化TCP服务\n", __FUNCTION__);
#ifndef USESIPCLIENT
	//m_pCUATcp = new CUACTCP();
#else
	m_pCUATcp = new CSipClient();
#endif
	
	if(NULL == m_pCUATcp)
		return INIT_FAILD_ERROR;
	m_pCUATcp->SetExternalInterfaceCallback(this);
	int nRet = m_pCUATcp->Init(pstrUACID, pstrIP, pstrPort);
//commennt by eamon chen
//   SipClientLogin("34010000002000000001", "192.168.1.98", "15060", "34020000001180000002", "192.168.1.100", "5060", "12345678");
	if(0 > nRet)
	{
		delete m_pCUATcp;
		m_pCUATcp = NULL;
		return nRet;
	}
	else
		return 0;

}

int CExternalInterface::Play(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, const TCHAR *pstrMediaID, int nStreamerType, 
	void *pStreamer, time_t startTime, time_t endTime, const char* pLocalIP, int nRecvPort[2], int nMediaTransferMode)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}



	char tmStart[TIME_LEN+1] = "0";
	char tmEnd[TIME_LEN+1] = "0";
	//先读取XML
	char szLocalIP[MAX_PATH] = {0};
	if(!GetLocalIp(szLocalIP))
	{
		char szName[MAX_PATH];
		gethostname(szName, MAX_PATH);
		hostent *stHost = gethostbyname(szName);

		in_addr addr;
		memcpy(&addr.S_un.S_addr,stHost->h_addr_list[0],stHost->h_length);
		char *szTemLocalIP = inet_ntoa(addr);
		strcpy(szLocalIP,szTemLocalIP);
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 本机没配置XML,自动获取本地IP:%s\n", __FUNCTION__,szTemLocalIP);
	}

	int nLocalPorts[2] = {0, 0};
	if(0 != get_free_port(szLocalIP, nLocalPorts))
	{
		return GET_FREEPORT_ERROR;
	}
	//m_pCUATcp->SetRealDataCallBack(this, fpClientRealDataCallback);
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 结束时间:%lu\n", __FUNCTION__, endTime);
	if(0 != startTime && 0 != endTime)
	{
		sprintf(tmStart, "%ld", startTime+ 28800);
		sprintf(tmEnd, "%ld", endTime+ 28800);
	}

	int nRet = m_pCUATcp->SendInvite(nStreamerType,			//real stream
									  nMediaTransferMode,   // UDP_TRANS or TCP_TRANS
									  CLog::UnicodeToANSI(pstrDVRID).c_str(),
									  CLog::UnicodeToANSI(pstrChannelID).c_str(),
									  pLocalIP,
									  nRecvPort,
									  pStreamer,
									  tmStart, tmEnd);

	if(0 > nRet)
		return nRet;

	return 0;
}

int CExternalInterface::Speed(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, float fValue)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s\n", __FUNCTION__, pstrDVRID);
	return m_pCUATcp->SendPlayBackCtrl(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), pStreamer, SPEED_TYPE, fValue);
}

int CExternalInterface::Pause(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, int nPos)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s\n", __FUNCTION__, pstrDVRID);
	return m_pCUATcp->SendPlayBackCtrl(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), pStreamer, PAUSE_TYPE, nPos);
}

int CExternalInterface::Resume(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, int nPos)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s\n", __FUNCTION__, pstrDVRID);
	return m_pCUATcp->SendPlayBackCtrl(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), pStreamer, PLAY_TYPE, nPos);
}

int CExternalInterface::Seek(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void* pStreamer, UINT unValue)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	unValue -= 28800;
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s, to:lu\n", __FUNCTION__, pstrDVRID, unValue);
	this->Pause(pstrDVRID, pstrChannelID, pStreamer, 0);
	return m_pCUATcp->SendPlayBackCtrl(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), pStreamer, PLAY_TYPE, unValue);
}

int CExternalInterface::Stop(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID, void * pStreamer, int nOption)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s, pstrChannelID:%s\n", __FUNCTION__, pstrDVRID, pstrChannelID);

	return m_pCUATcp->SendBye(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), pStreamer, nOption);
}

int CExternalInterface::GetStreamerID(void* pStreamer, int &nCid, int &nDid)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	m_pCUATcp->GetPlayBackID(pStreamer, nCid, nDid);

	return 0;
}

int CExternalInterface::SetDeviceStatusCallBack(DeviceStaustAlarm fpDeviceStaustAlarm)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	return m_pCUATcp->SetDeviceStatusCallBack(fpDeviceStaustAlarm);
}

int CExternalInterface::SetPlayBackFinishedCallBack(PlayBackFinished fpPlayBackFinished)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	return m_pCUATcp->SetPlayBackFinishedCallBack(fpPlayBackFinished);
}

int CExternalInterface::SetInviteResponseInfoCallback(InviteResponsed fpInviteResponsed) {
	if (NULL == m_pCUATcp) {
		//g_objLog.LogoutInfo(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	return m_pCUATcp->SetInviteResponseInfoCallback(fpInviteResponsed);
}

int CExternalInterface::SetGuard(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, BYTE bCmdType)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
#ifdef UNICODE
	return m_pCUATcp->SendGuard(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), bCmdType);
#else
	return m_pCUATcp->SendGuard(pstrDeviceID, pstrChannelID, bCmdType);
#endif

}

int CExternalInterface::SetPTZControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, BYTE bCmdType,int nValue)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s bCmdType:%lu\n", __FUNCTION__, bCmdType);
	BYTE bData1 = 0;
	BYTE bData2 = 0;
	BYTE bData3 = 0;
	int ptzValue= int(((nValue)*255/10));
	static BYTE ptzSpeed =ptzValue /*GetPtzSpeed()*/; //不使用GB28181Adapter.xml的速度了
	
	short panSpeed =(static_cast<unsigned int>(nValue) & 0xFFFF0000)>> 16;
	short tiltSpeed=(static_cast<unsigned int>(nValue) &0xFFFF)>>0;
	int gbPanSpeed=abs(int(((panSpeed)*255/10)));
	int gbTiltSpeed=abs(int(((tiltSpeed)*255/10)));

	switch(bCmdType)
	{
	case 1:
		bCmdType = LEFT_DIR;
		bData1 = ptzSpeed;
		break;
	case 2:
		bCmdType = RIGHT_DIR;
		bData1 = ptzSpeed;
		break;
	case 3:
		bCmdType = UP_DIR;
		bData2 = ptzSpeed;
		break;
	case 4:
		bCmdType = DOWN_DIR;
		bData2 = ptzSpeed;
		break;
	case 5:
		bCmdType = FORWARD_DIR;
		bData3 = 1;
		break;
	case 6:
		bCmdType = BACK_DIR;
		bData3 = 1;
		break;
	case 7:
		//0/正(向上)
		if (panSpeed==0 &&tiltSpeed>0)
		{
			bCmdType = UP_DIR;
			bData2 = gbTiltSpeed;
		}
		//0/负(向下)
		if (panSpeed==0 &&tiltSpeed<0)
		{
			bCmdType = DOWN_DIR;
			bData2 = gbTiltSpeed;
		}
		//正/0(向右)
		if (panSpeed>0&&tiltSpeed==0)
		{
			bCmdType = RIGHT_DIR;
			bData1 = gbPanSpeed;
		}
		//负/0(向左)
		if (panSpeed<0&&tiltSpeed==0)
		{
			bCmdType = LEFT_DIR;
			bData1 = gbPanSpeed;
		}
		//正/正(上右)
		if (panSpeed>0&&tiltSpeed>0)
		{
			bCmdType=UPPERRIGHT_DIR;
			bData1 = gbPanSpeed;
			bData2 = gbTiltSpeed;
		}
		//正/负(下右)
		if (panSpeed>0&&tiltSpeed<0)
		{
			bCmdType=LOWERRIGHT_DIR;
			bData1 = gbPanSpeed;
			bData2 = gbTiltSpeed;
		}
		//负/正(左上)
		if (panSpeed<0 &&tiltSpeed>0)
		{
			bCmdType=UPPERLEFT_DIR;
			bData1 = gbPanSpeed;
			bData2 = gbTiltSpeed;
		}
		//负/负(左下)
		if (panSpeed<0 &&tiltSpeed<0)
		{
			bCmdType=LOWERLEFT_DIR;
			bData1 = gbPanSpeed;
			bData2 = gbTiltSpeed;
		}
		//0/0(停止)
		if (panSpeed==0 &&tiltSpeed==0)
		{
			bCmdType = 0;
			bData2 = 0;
		}
		break;
	case 48:
		bCmdType = NEAR_DIR;
		bData1 = 127;
		break;
	case 49:
		bCmdType = FAR_DIR;
		bData1 = 127;
		break;
	case 64:
		bCmdType = ZOOMIN_DIR;
		bData2 = 127;
		break;
	case 65:
		bCmdType = ZOOMOUT_DIR;
		bData2 = 127;
		break;
	case 80:
		bCmdType = PRECALL_CTRL;
		bData2 = nValue;
		break;
	case 81:
		bCmdType = PRESET_CTRL;
		bData2 = nValue;
		break;
	case 82:
		bCmdType = PREDEL_CTRL;
		bData2 = nValue;
		break;
	case 153:
		bCmdType = 0;
		bData2 = 0;
		break;
	default:
		break;
	}
	
#ifdef UNICODE
	return m_pCUATcp->SendDeviceControl(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), bCmdType, bData1, bData2, bData3);
#else
	return m_pCUATcp->SendDeviceControl(pstrDeviceID, pstrChannelID, bCmdType, bData1, bData2, bData3);
#endif
}

int CExternalInterface::SetScanControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nSectionID, int nValue1, int nValue2)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	BYTE bData1 = nValue1;
	BYTE bData2 = 0;
	BYTE bData3 = 0;
	BYTE bCmdType;

	switch(nSectionID)
	{
	case 201:
		bCmdType = SCANRANGE_CTRL;
		bData2 = 1;
		break;
	case 202:
		bCmdType = SCANRANGE_CTRL;
		bData2 = 2;
		break;
	case 203:
		bCmdType = SCANRANGE_CTRL;
		bData2 = 127;
		break;
	case 204:
		bCmdType = SCANSPEED_CTRL;
		bData2 = (nValue2 / 9 * 0xFFF) & 0xFF;
		bData3 = ((nValue2 / 9 * 0xFFF) & 0xF00) >> 4;
		break;
	default:
		break;
	}

#ifdef UNICODE
	return m_pCUATcp->SendDeviceControl(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), bCmdType, bData1, bData2, bData3);
#else
	return m_pCUATcp->SendDeviceControl(pstrDeviceID, pstrChannelID, bCmdType, bData1, bData2, bData3);
#endif
}

int CExternalInterface::SetCruiseControl(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nSectionID, int nValue1, int nValue2)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	BYTE bData1 = 0;
	BYTE bData2 = 0;
	BYTE bData3 = 0;
	BYTE bCmdType = 0;

	switch(nSectionID)
	{
	case 207:
		bCmdType = CRUADD_CTRL;
		bData1 = nValue1;
		bData2 = nValue2;
		break;
	case 208:
		bCmdType = CRUDEL_CTRL;
		bData1 = nValue1;
		bData2 = nValue2;
		break;
	case 209:
		bCmdType = CRUSPEED_CTRL;
		bData2 = (nValue2 / 9 * 0xFFF) & 0xFF;
		bData3 = ((nValue2 / 9 * 0xFFF) & 0xF00) >> 4;
		break;
	case 210:
		bCmdType = CRUTIME_CTRL;
		bData2 = (nValue2 / 9 * 0xFFF) & 0xFF;
		bData3 = ((nValue2 / 9 * 0xFFF) & 0xF00) >> 4;
		break;
	case 211:
		bCmdType = CRUSTART_CTRL;
		bData1 = nValue1;
		bData2 = nValue2; 
		break;
	default:
		break;
	}

#ifdef UNICODE
	return m_pCUATcp->SendDeviceControl(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), bCmdType, bData1, bData2, bData3);
#else
	return m_pCUATcp->SendDeviceControl(pstrDeviceID, pstrChannelID, bCmdType, bData1, bData2, bData3);
#endif
}

const char* CExternalInterface::SearchRecord(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nType, DATE tmStart, DATE tmEnd,
				const char* pstrFilePath /*= NULL*/, const char* pstrAddress /*= NULL*/, 
				BYTE bSecrecy /*= 0*/, const char* pstrRecorderID /*= NULL*/, BYTE IndistinctQuery/* = 0*/)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return NULL;
	}
	//cy 2016.3.26
	//客户端传过来nType多加了1,所以减去1
	nType =nType-1; // default for "all"
	if (nType<0)
	{
		nType=0;// default for "all"
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	COleDateTime datTimeStart(tmStart);
	int nSYear = datTimeStart.GetYear();
	int nSMonth = datTimeStart.GetMonth();
	int nSDay = datTimeStart.GetDay();
	int nSHour = datTimeStart.GetHour();
	int nSMinute = datTimeStart.GetMinute();
	int nSSecond = datTimeStart.GetSecond();

	COleDateTime datTimeEnd(tmEnd);
	int nEYear = datTimeEnd.GetYear();
	int nEMonth = datTimeEnd.GetMonth();
	int nEDay = datTimeEnd.GetDay();
	int nEHour = datTimeEnd.GetHour();
	int nEMinute = datTimeEnd.GetMinute();
	int nESecond = datTimeEnd.GetSecond();

	char strSTime[TIME_LEN+1];
	char strETime[TIME_LEN+1];

	if(3000 < nSYear || 12 < nSMonth || 31 < nSDay || 24 < nSHour || 60 < nSMinute || 60 < nSSecond)
		return NULL;
	if(3000 < nEYear || 12 < nEMonth || 31 < nEDay || 24 < nEHour || 60 < nEMinute || 60 < nESecond)
		return NULL;


	sprintf_s(strSTime,"%d-%02d-%02dT%02d:%02d:%02d", nSYear, nSMonth, nSDay, nSHour, nSMinute, nSSecond);
	sprintf_s(strETime,"%d-%02d-%02dT%02d:%02d:%02d", nEYear, nEMonth, nEDay, nEHour, nEMinute, nESecond);
	if(19 < strlen(strSTime) || 19 < strlen(strETime))
		return NULL;
#ifdef UNICODE
	return m_pCUATcp->SendRecordInfoInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), nType,
						 strSTime,
						 strETime);
#else
	return m_pCUATcp->SendRecordInfoInquiry(pstrDeviceID, pstrChannelID, nType,
		strSTime,
		strETime);
#endif

}


int CExternalInterface::SearchDeviceInfo(const TCHAR *pstrDeviceID)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceInfoInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	pInfo = m_pCUATcp->SendDeviceInfoInquiry(pstrDeviceID);
#endif
	if(pInfo)
	{
		g_pInfo = pInfo;
		ShowResult(MSG_INFO_INQUIRE);
	}

	return 0;
}

int CExternalInterface::SearchDeviceStatus(const TCHAR *pstrDeviceID)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceStatusInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	m_pCUATcp->SendDeviceStatusInquiry(pstrDeviceID);
#endif

	if(pInfo)
	{
		g_pStatus = pInfo;
		ShowResult(MSG_STATUS_INQUIRE);
	}
	return 0;
}

const char * CExternalInterface::GetDeviceStatus(const TCHAR *pstrDeviceID)
{
//	static string strStatus;
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		strStatus = "";
		return strStatus.c_str();
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceStatusInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	m_pCUATcp->SendDeviceStatusInquiry(pstrDeviceID);
#endif

	if(pInfo)
	{
		strStatus = pInfo;
	}
	else 
		strStatus = "";
	return strStatus.c_str();
}

int CExternalInterface::RemoteStartup(const TCHAR *pstrDeviceID)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

#ifdef UNICODE
	return m_pCUATcp->SendRemoteStartup(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	return m_pCUATcp->SendRemoteStartup(pstrDeviceID);
#endif
}

int CExternalInterface::SetRecord(const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nActive)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
#ifdef UNICODE
	return m_pCUATcp->SendRecord(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), nActive);
#else
	return m_pCUATcp->SendRecord(pstrDeviceID, pstrChannelID, nActive);
#endif
}

int CExternalInterface::SearchCatalog(const TCHAR *pstrDeviceID)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry(pstrDeviceID);
#endif

	if(pInfo)
	{
		g_pCatalog = pInfo;
		ShowResult(MSG_CATALOG_INQUIRE);
	}

	return 0;
}

const char * CExternalInterface::GetCatalog(const TCHAR *pstrDeviceID)
{
//	static string strCatalog;
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		strCatalog = "";
		return strCatalog.c_str();
	}

	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry(CLog::UnicodeToANSI(pstrDeviceID).c_str());
#else
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry(pstrDeviceID);
#endif
	if(pInfo)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s catalog size:%d\n", __FUNCTION__, strlen(pInfo));
		strCatalog = pInfo;
	}
	else
		strCatalog = "";

	return strCatalog.c_str();
}

int CExternalInterface::SetDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOperate)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}


#ifdef UNICODE
	return m_pCUATcp->SendDecorder(CLog::UnicodeToANSI(pstrDecorderID).c_str(), CLog::UnicodeToANSI(pstrCameraID).c_str(), nOperate);
#else
	return m_pCUATcp->SendDecorder(pstrDecorderID, pstrCameraID, nOperate);
#endif
}

int CExternalInterface::ExtractXML(const char * pXml, char *pKey, char *pElement1L, char *Element1R, char *Element2L, char *Element2R, DATE &tmStart, DATE &tmEnd)
{
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s start\n", __FUNCTION__);
	tmStart = 0.0;
	tmEnd = 0.0;
	string strFileInfo = pXml;
	int nPos = strFileInfo.find(pKey);
	if(string::npos == nPos)
		return RECORD_INFO_ERROR;
	
	strFileInfo = strFileInfo.substr(nPos, strFileInfo.length()-nPos);

	string strStartTime = GetStringBetween(strFileInfo, pElement1L, Element1R, FALSE);
	if(strStartTime.empty())
		return RECORD_INFO_ERROR;

	string strEndTime = GetStringBetween(strFileInfo, Element2L, Element2R, FALSE);
	if(strEndTime.empty())
		return RECORD_INFO_ERROR;

	char *pStartTime = new char[strStartTime.length()+1];
	strcpy_s(pStartTime, strStartTime.length()+1, strStartTime.c_str());
	for(auto i = 0U; i < strStartTime.length(); i++)
	{
		if('T' == pStartTime[i])
		{
			pStartTime[i] = ' ';
			break;
		}
	}

	char *pEndTime = new char[strEndTime.length()+1];
	strcpy_s(pEndTime, strEndTime.length()+1, strEndTime.c_str());
	for(auto i = 0U; i < strEndTime.length(); i++)
	{
		if('T' == pEndTime[i])
		{
			pEndTime[i] = ' ';
			break;
		}
	}

	COleDateTime datTimeStart;
	datTimeStart.ParseDateTime(CString(pStartTime));
	tmStart = datTimeStart;
	COleDateTime datTimeEnd;
	datTimeEnd.ParseDateTime(CString(pEndTime));
	tmEnd = datTimeEnd;

	if(pStartTime)
		delete []pStartTime;
	if(pEndTime)
		delete []pEndTime;

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s end\n", __FUNCTION__);
	return nPos+4;
}

int CExternalInterface::ExtractXML(const char * pXml, char *pKey, char *pElementL, char *ElementR, TCHAR *strValue, int nStrBufLen)
{
	strValue[0] = '\0';
	string strFileInfo = pXml;
	int nPos = strFileInfo.find(pKey);
	if(string::npos == nPos)
		return RECORD_INFO_ERROR;
	
	strFileInfo = strFileInfo.substr(nPos, strFileInfo.length()-nPos);

	string strElementVaule = GetStringBetween(strFileInfo, pElementL, ElementR, FALSE);
	if(strElementVaule.empty())
		return nPos+4;
#ifdef UNICODE
	if(nStrBufLen < strlen(strElementVaule.c_str()))
		_tcscpy(strValue, CLog::ANSIToUnicode(strElementVaule.c_str()).c_str());
	else
		_tcsncpy(strValue, CLog::ANSIToUnicode(strElementVaule.c_str()).c_str(), nStrBufLen-1);
#else
	if(nStrBufLen < strlen(strElementVaule.c_str()))
		strcpy(strValue, strElementVaule.c_str());
	else
		strcpy(strValue, strElementVaule.c_str(), nStrBufLen-1);
#endif

	return nPos+4;
}

int CExternalInterface::ExtractXML(const char * pXml, char *pKey, TCHAR *strValue, int nStrBufLen)
{
	strValue[0] = '\0';
	string strFileInfo = pXml;
	string strNum;
	int nPos = strFileInfo.find(pKey);
	if(string::npos == nPos)
		return RECORD_INFO_ERROR;
	
	strFileInfo = strFileInfo.substr(nPos, strFileInfo.length()-nPos);
	int nLPos = strFileInfo.find("=");
	int nRPos = strFileInfo.find(">");
	if(string::npos != nLPos && string::npos != nRPos && nLPos < nRPos)
		strNum = strFileInfo.substr(nLPos+1, nRPos-nLPos-1);

#ifdef UNICODE
	if(nStrBufLen < strlen(strNum.c_str()))
		_tcscpy(strValue, CLog::ANSIToUnicode(strNum.c_str()).c_str());
	else
		_tcsncpy(strValue, CLog::ANSIToUnicode(strNum.c_str()).c_str(), nStrBufLen-1);
#else
	if(nStrBufLen < strlen(strNum.c_str()))
		strcpy(strValue, strNum.c_str());
	else
		strcpy(strValue, strNum.c_str(), nStrBufLen-1);
#endif

	return nPos+4;
}

int CExternalInterface::ExtractDeviceID(const char * pXml, TCHAR *strValue, int nStrBufLen)
{
	strValue[0] = '\0';
	string strFileInfo = pXml;

	string strElementVaule = GetStringBetween(strFileInfo, "<DeviceID>", "</DeviceID>", FALSE);
	if(strElementVaule.empty())
		return -1;
#ifdef UNICODE
	if(nStrBufLen < strlen(strElementVaule.c_str()))
		_tcscpy(strValue, CLog::ANSIToUnicode(strElementVaule.c_str()).c_str());
	else
		_tcsncpy(strValue, CLog::ANSIToUnicode(strElementVaule.c_str()).c_str(), nStrBufLen-1);
#else
	if(nStrBufLen < strlen(strElementVaule.c_str()))
		strcpy(strValue, strElementVaule.c_str());
	else
		strcpy(strValue, strElementVaule.c_str(), nStrBufLen-1);
#endif
	int nPos = strFileInfo.find("</DeviceID>");
	return nPos+strlen("</DeviceID>");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	switch(msg) 
	{ 
	case WM_DESTROY:
		DestroyWindow(hwnd); 
		PostQuitMessage(0); 
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam); 
	}
	return 0; 
} 

int CExternalInterface::ShowResult(int nShowDataType)
{
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 显示\n", __FUNCTION__);
	TCHAR strFileName[MAX_PATH];
	GetModuleFileName(NULL, strFileName, MAX_PATH);
	_tstring strExeName = strFileName;
	int nPos = strExeName.rfind(_T("\\"));
	strExeName = strExeName.substr(nPos+1, strExeName.length()-nPos-1);
	HINSTANCE hInst = GetModuleHandle(strExeName.c_str());

	WNDCLASSEX wcex; 
	HWND hwnd; 
	MSG msg; 
 
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style = CS_HREDRAW | CS_VREDRAW; 
	wcex.cbClsExtra = 0; 
	wcex.cbWndExtra = 0; 
	wcex.hInstance = hInst; 
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); 
	wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION); 
	wcex.lpfnWndProc = (WNDPROC)WndProc; 
	wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION); 
	wcex.lpszClassName = _T("show"); 
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wcex.lpszMenuName = NULL; 
	RegisterClassEx(&wcex); 

	int nHeight = 200;
	int nWidth = 800;
	_tstring strWinCap;
	if(MSG_INFO_INQUIRE == nShowDataType)
	{
		strWinCap = _T("设备信息");
	}
	else if(MSG_STATUS_INQUIRE == nShowDataType)
	{
		strWinCap = _T("设备状态");
	}
	// PWolf: Add 2013.06.18
	else if(MSG_CATALOG_INQUIRE == nShowDataType)
	{
		strWinCap = _T("设备目录");
	}
	else if(MSG_ALARM_QUERY == nShowDataType)
	{
		strWinCap = _T("设备告警");
	}
	// PWolf: Add End
	HWND hWnd = FindWindow(_T("show"), strWinCap.c_str());
	if(hWnd)
		PostMessage(hWnd, WM_DESTROY, 0, 0);

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("show"), strWinCap.c_str(), WS_OVERLAPPEDWINDOW, 
		100, 100, nWidth, 440, NULL, 
		NULL, hInst, NULL); 


	g_hwndListInfo = CreateWindow(_T("SysListView32"), 0, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
			0, 10, nWidth, nHeight, hwnd,
			0,hInst,0);


	if(MSG_INFO_INQUIRE == nShowDataType)
	{
		ShowDeviceInfo(g_hwndListInfo, g_pInfo);
	}
	else if(MSG_STATUS_INQUIRE == nShowDataType)
	{
		g_hwndListAlarm = CreateWindow(_T("SysListView32"), 0, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
			0, nHeight+20, nWidth, nHeight, hwnd,
			0,hInst,0);

		ShowDeviceStatus(g_hwndListInfo,  g_hwndListAlarm, g_pStatus);
	}
	// PWolf: Add 2013.06.18
	else if(MSG_CATALOG_INQUIRE == nShowDataType)
	{
		int nCount = ShowDeviceCatalog(g_hwndListInfo, g_pCatalog);
		int nNeedCount = 0;
		if( nCount > 7 && nCount < 18)
			nNeedCount = nCount + 1;
		else if(nCount >= 20)
			nNeedCount = 20;
		else
			nNeedCount = 7;
		MoveWindow(g_hwndListInfo,0,0,nWidth,20 * nNeedCount, true);
	}
	else if(MSG_ALARM_QUERY == nShowDataType)
	{
		int nCount = ShowAlaramQuery(g_hwndListInfo, g_pInfo);
		int nNeedCount = 0;
		if( nCount > 7 && nCount < 18)
			nNeedCount = nCount + 1;
		else if(nCount >= 20)
			nNeedCount = 20;
		else
			nNeedCount = 7;
		MoveWindow(g_hwndListInfo,0,0,nWidth,20 * nNeedCount, true);
	}
	// PWolf: Add End

	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd); 

	while(GetMessage(&msg, NULL, 0, 0)) 
	{ 
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	} 

	return 0;
}

int CExternalInterface::ShowDeviceInfo(HWND hWndInfo, const char * pXML)
{
	//char aaa[] = 
	//	"<?xml version=\"1.0\"?>\r\n"
	//	"<Response>\r\n"
	//	"<CmdType>DeviceInfo</CmdType>\r\n"
	//	"<SN>17430</SN>\r\n"
	//	"<DeviceID>64010000001110000001</DeviceID>\r\n"
	//	"<Result>OK</Result>\r\n"
	//	"<DeviceType>DVR</DeviceType>\r\n"
	//	"<Manufacturer>Tiandy</Manufacturer>\r\n"
	//	"<Model>TC-2808AN-HD</Model>\r\n"
	//	"<Firmware>V2.1, build 091111</Firmware>\r\n"
	//	"<MaxCamera>8</MaxCamera>\r\n"
	//	"<MaxAlarm>16</MaxAlarm>\r\n"
	//	"</Response>\r\n";
	//pXML = aaa;

	if(pXML && 0 < strlen(pXML))
	{
		LVCOLUMN ColInfo = {0};

		ColInfo.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		ColInfo.iSubItem = 0;
		ColInfo.fmt = LVCFMT_CENTER;
		ColInfo.cx = 100;
		ColInfo.cchTextMax = 50;
		ColInfo.pszText = _T("设备ID");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(0), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备类型");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(1), LPARAM(&ColInfo));
		ColInfo.pszText = _T("制造厂商");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(2), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备型号");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(3), LPARAM(&ColInfo));
		ColInfo.pszText = _T("固件版本");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(4), LPARAM(&ColInfo));
		ColInfo.pszText = _T("最大视频通道数");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(5), LPARAM(&ColInfo));
		ColInfo.pszText = _T("最大报警通道数");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(6), LPARAM(&ColInfo));

		//取得XML中的设备信息
		TCHAR strDeviceID[MAX_PATH];
		TCHAR strDeviecType[MAX_PATH];
		TCHAR strManufacturer[MAX_PATH];
		TCHAR strModel[MAX_PATH];
		TCHAR strFirmware[MAX_PATH];
		TCHAR strMaxCamera[MAX_PATH];
		TCHAR strMaxAlarm[MAX_PATH];
		ExtractXML(pXML+4, "DeviceInfo", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<DeviceType>", "</DeviceType>", strDeviecType, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<Manufacturer>", "</Manufacturer>", strManufacturer, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<Model>", "</Model>", strModel, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<Firmware>", "</Firmware>", strFirmware, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<MaxCamera>", "</MaxCamera>", strMaxCamera, MAX_PATH);
		ExtractXML(pXML+4, "DeviceInfo", "<MaxAlarm>", "</MaxAlarm>", strMaxAlarm, MAX_PATH);

		LVITEM item;
		item.mask = LVIF_TEXT;
		item.pszText = strDeviceID;
		item.iItem = 0;
		item.iSubItem = 0;
		::SendMessage(hWndInfo, LVM_INSERTITEM, 0, LPARAM(&item));

		item.pszText = strDeviecType;
		item.iSubItem = 1;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strManufacturer;
		item.iSubItem = 2;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strModel;
		item.iSubItem = 3;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strFirmware;
		item.iSubItem = 4;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strMaxCamera;
		item.iSubItem = 5;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strMaxAlarm;
		item.iSubItem = 6;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));
		return 0;
	}

	return -1;
}

int CExternalInterface::ShowDeviceStatus(HWND hWndInfo,  HWND hWndAlarm, const char * pXML)
{
//	char aaa[] = 
//"<?xml version=\"1.0\"?>\r\n"
//"<Response>\r\n"
//"<CmdType>DeviceStatus</CmdType>\r\n"
//"<SN>3</SN>\r\n"
//"<DeviceID>34020000001110000001</DeviceID>\r\n"
//"<Result>OK</Result>\r\n"
//"<Online>ONLINE</Online>\r\n"
//"<Status>OK</Status>\r\n"
//"<DeviceTime>2013-05-15T10:27:41</DeviceTime>\r\n"
//"<Alarmstatus Num=\"1\">\r\n"
//"<Item>\r\n"
//"<DeviceID>34020000001340000001</DeviceID>\r\n"
//"<DutyStatus>OFFDUTY</DutyStatus>\r\n"
//"</Item>\r\n"
//"</Alarmstatus>\r\n"
//"</Response>\r\n";
//	pXML = aaa;

	if(pXML && 0 < strlen(pXML))
	{
		LVCOLUMN ColInfo = {0};

		ColInfo.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		ColInfo.iSubItem = 0;
		ColInfo.fmt = LVCFMT_CENTER;
		ColInfo.cx = 100;
		ColInfo.cchTextMax = 50;
		ColInfo.pszText = _T("设备ID");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(0), LPARAM(&ColInfo));
		ColInfo.pszText = _T("在线状态");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(1), LPARAM(&ColInfo));
		ColInfo.pszText = _T("运行状态");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(2), LPARAM(&ColInfo));
		ColInfo.pszText = _T("编码状态");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(3), LPARAM(&ColInfo));
		ColInfo.pszText = _T("录像状态");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(4), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备时间");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(5), LPARAM(&ColInfo));
		ColInfo.pszText = _T("报警通道数");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(6), LPARAM(&ColInfo));

		ColInfo.cx = 150;
		ColInfo.pszText = _T("报警设备ID");
		SendMessage(hWndAlarm, LVM_INSERTCOLUMN, WPARAM(0), LPARAM(&ColInfo));
		ColInfo.pszText = _T("报警状态");
		SendMessage(hWndAlarm, LVM_INSERTCOLUMN, WPARAM(1), LPARAM(&ColInfo));

		//取得XML中的设备信息
		TCHAR strDeviceID[MAX_PATH];
		TCHAR strOnline[MAX_PATH];
		TCHAR strStatus[MAX_PATH];
		TCHAR strEncode[MAX_PATH];
		TCHAR strRecord[MAX_PATH];
		TCHAR strTime[MAX_PATH];
		TCHAR strAlarmstatus[MAX_PATH];
		TCHAR strDutyStatus[MAX_PATH];
		ExtractXML(pXML+4, "DeviceStatus", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
		ExtractXML(pXML+4, "DeviceStatus", "<Online>", "</Online>", strOnline, MAX_PATH);
		ExtractXML(pXML+4, "DeviceStatus", "<Status>", "</Status>", strStatus, MAX_PATH);
		ExtractXML(pXML+4, "DeviceStatus", "<Encode>", "</Encode>", strEncode, MAX_PATH);
		ExtractXML(pXML+4, "DeviceStatus", "<Record>", "</Record>", strRecord, MAX_PATH);
		ExtractXML(pXML+4, "DeviceStatus", "<DeviceTime>", "</DeviceTime>", strTime, MAX_PATH);
		int nPos = ExtractXML(pXML+4, "Alarmstatus Num",  strAlarmstatus, MAX_PATH);

		//g_objLog.LogoutDebug(k_LOG_DLL, "%s 设备ID:%s, 在线状态:%s, 运行状态:%s, 编码状态:%s, 录像状态:%s, 设备时间:%s\n", __FUNCTION__, strDeviceID, strOnline, strStatus, strEncode, strRecord, strTime);

		LVITEM item;
		item.mask = LVIF_TEXT;
		item.pszText = strDeviceID;
		item.iItem = 0;
		item.iSubItem = 0;
		::SendMessage(hWndInfo, LVM_INSERTITEM, 0, LPARAM(&item));

		item.pszText = strOnline;
		item.iSubItem = 1;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strStatus;
		item.iSubItem = 2;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strEncode;
		item.iSubItem = 3;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strRecord;
		item.iSubItem = 4;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strTime;
		item.iSubItem = 5;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		item.pszText = strAlarmstatus;
		item.iSubItem = 6;
		::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));
		if(0 > nPos)
			return -1;

		int nItem = 0;
		//number check
		for(int i = 0; i < MAX_PATH && 0 != strAlarmstatus[i]; i++)
		{
			if('0' > strAlarmstatus[i] && '9' < strAlarmstatus[i])
				return -1;
		}

		int nAlarmSum = _ttoi(strAlarmstatus);
		const char *pCurrent = pXML+nPos;
		ExtractXML(pCurrent, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
		nPos = ExtractXML(pCurrent, "<Item>", "<DutyStatus>", "</DutyStatus>", strDutyStatus, MAX_PATH);
		while(-1 < nPos )
		{
			pCurrent += nPos;
			item.mask = LVIF_TEXT;
			item.pszText = strDeviceID;
			item.iItem = nItem;
			item.iSubItem = 0;
			::SendMessage(hWndAlarm, LVM_INSERTITEM, 0, LPARAM(&item));

			item.pszText = strDutyStatus;
			item.iSubItem = 1;
			::SendMessage(hWndAlarm, LVM_SETITEM, 0, LPARAM(&item));
			nItem++;

			ExtractXML(pCurrent, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
			nPos = ExtractXML(pCurrent, "<Item>", "<DutyStatus>", "</DutyStatus>", strDutyStatus, MAX_PATH);
		}

		return 0;
	}

	return -1;
}

// PWolf: Add 2013.06.18
int CExternalInterface::ShowDeviceCatalog(HWND hWndInfo, const char * pXML)
{
	if(pXML && 0 < strlen(pXML))
	{
		LVCOLUMN ColInfo = {0};

		ColInfo.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		ColInfo.iSubItem = 0;
		ColInfo.fmt = LVCFMT_CENTER;
		ColInfo.cx = 150;
		ColInfo.cchTextMax = 50;
		ColInfo.pszText = _T("设备ID");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(0), LPARAM(&ColInfo));
		ColInfo.cx = 100;
		ColInfo.pszText = _T("设备名称");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(1), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备厂商");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(2), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备类型");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(3), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备归属");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(4), LPARAM(&ColInfo));
		ColInfo.pszText = _T("行政区域");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(5), LPARAM(&ColInfo));
		ColInfo.pszText = _T("安装地址");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(6), LPARAM(&ColInfo));
		ColInfo.pszText = _T("父设备");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(7), LPARAM(&ColInfo));
		ColInfo.pszText = _T("信令安全模式");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(8), LPARAM(&ColInfo));
		ColInfo.pszText = _T("注册方式");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(9), LPARAM(&ColInfo));
		ColInfo.pszText = _T("保密属性");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(10), LPARAM(&ColInfo));
		ColInfo.pszText = _T("设备状态");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(11), LPARAM(&ColInfo));

		int						nOffset;
		int						nCatalogCnt = 0;
		const char*				pCurPos = pXML;
		TCHAR					strSumNum[MAX_PATH];

		nOffset = ExtractXML(pCurPos, "Catalog", "<SumNum>", "</SumNum>", strSumNum, MAX_PATH);
		pCurPos += nOffset;
		nCatalogCnt = _ttoi(strSumNum);
		
		int							nCurItemIdx = 0;

		for(int nCurCnt = 0; nCurCnt < nCatalogCnt; nCurCnt++)
		{
			TCHAR				strDeviceID[MAX_PATH];
			TCHAR				strName[MAX_PATH];
			TCHAR				strManufacturer[MAX_PATH];
			TCHAR				strModel[MAX_PATH];
			TCHAR				strOwner[MAX_PATH];
			TCHAR				strCivilCode[MAX_PATH];
			TCHAR				strAddress[MAX_PATH];
			TCHAR				strParental[MAX_PATH];
			TCHAR				strSafetyWay[MAX_PATH];
			TCHAR				strRegisterWay[MAX_PATH];
			TCHAR				strSecrecy[MAX_PATH];
			TCHAR				strStatus[MAX_PATH];

			ExtractXML(pCurPos, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Name>", "</Name>", strName, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Manufacturer>", "</Manufacturer>", strManufacturer, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Model>", "</Model>", strModel, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Owner>", "</Owner>", strOwner, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<CivilCode>", "</CivilCode>", strCivilCode, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Address>", "</Address>", strAddress, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Parental>", "</Parental>", strParental, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<SafetyWay>", "</SafetyWay>", strSafetyWay, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<RegisterWay>", "</RegisterWay>", strRegisterWay, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Secrecy>", "</Secrecy>", strSecrecy, MAX_PATH);
			nOffset = ExtractXML(pCurPos, "<Item>", "<Status>", "</Status>", strStatus, MAX_PATH);
			pCurPos += nOffset;
		
			LVITEM						item;

			item.mask = LVIF_TEXT;

			// strDeviceID
			item.pszText = strDeviceID;
			item.iItem = nCurItemIdx;
			item.iSubItem = 0;
			::SendMessage(hWndInfo, LVM_INSERTITEM, 0, LPARAM(&item));

			nCurItemIdx++;

			// strName
			item.pszText = strName;
			item.iSubItem = 1;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strManufacturer
			item.pszText = strManufacturer;
			item.iSubItem = 2;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strModel
			item.pszText = strModel;
			item.iSubItem = 3;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strOwner
			item.pszText = strOwner;
			item.iSubItem = 4;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strCivilCode
			item.pszText = strCivilCode;
			item.iSubItem = 5;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strAddress
			item.pszText = strAddress;
			item.iSubItem = 6;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strParental
			item.pszText = strParental;
			item.iSubItem = 7;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strSafetyWay
			item.pszText = strSafetyWay;
			item.iSubItem = 8;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strRegisterWay
			item.pszText = strRegisterWay;
			item.iSubItem = 9;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strSecrecy
			item.pszText = strSecrecy;
			item.iSubItem = 10;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strStatus
			item.pszText = strStatus;
			item.iSubItem = 11;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));
		}
		return nCatalogCnt;
	}

	return -1;
}

const char* CExternalInterface::SearchPlatformCatalog(void)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return NULL;
	}

	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry("");
#else
	pInfo = m_pCUATcp->SendDeviceCatalogInquiry(pstrDeviceID);
#endif
	if(pInfo)
	{
		g_pCatalog = pInfo;
		// PWolf: Noused ShowResult(CATALOG_INQUIRE_TYPE);
	}
	return g_pCatalog;
}
// PWolf: Add End


int CExternalInterface::SetPlayStatus(void* pStreamer, int nPlayStatus)
{
	if(pStreamer)
	{
		//control audio whether play or not, remove to husavlib
	}

	return 0;
}

int CExternalInterface::ResetAlarm(const TCHAR *pstrDVRID, const TCHAR *pstrChannelID)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	//g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDVRID:%s, pstrChannelID:%s\n", __FUNCTION__, pstrDVRID, pstrChannelID);

	return m_pCUATcp->SendResetAlarm(CLog::UnicodeToANSI(pstrDVRID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str());
}

int CExternalInterface::GetOwnerType()
{
	if(m_pCUATcp)
		return m_pCUATcp->GetOwnerType();
	else
		return -1;
}

float CExternalInterface::GetSpeed(void* pStreamer)
{
	if(m_pCUATcp)
		return m_pCUATcp->GetSpeed(pStreamer);
	else
		return 0;
}

int CExternalInterface::GetPlayStatus(void* pStreamer)
{
	if(m_pCUATcp)
		return m_pCUATcp->GetPlayStatus(pStreamer);
	else
		return 0;
}


int CExternalInterface::SetDeviceConfig( const TCHAR *pstrDeviceID, deviceConfig_info_t* pDeviceConfig )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}
	return m_pCUATcp->SendDeviceConfig(CLog::UnicodeToANSI(pstrDeviceID).c_str(), pDeviceConfig);
}

int CExternalInterface::DevicePersetQuery( const TCHAR *pstrDeviceID )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
//        strDeviceConfigDownload = "";
//        return strDeviceConfigDownload.c_str();
	}

	DEVICE_PRESETQUERY_INFO_T* pInfo = NULL;
	pInfo = m_pCUATcp->SendDevicePersetQuery(CLog::UnicodeToANSI(pstrDeviceID).c_str());
	if(pInfo)
	{
		//TODO
	}

	//return strDeviceConfigDownload.c_str();
	return -1;
}

int CExternalInterface::ConfigDownload( const TCHAR *pstrDeviceID, const TCHAR *strConfigType )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
	}

	DEVICE_CONFIG_INFO_T* pInfo = NULL;
	pInfo = m_pCUATcp->SendConfigDownload(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(strConfigType).c_str());
	if(pInfo)
	{
		//todo
	}
	return -1;
}

const char * CExternalInterface::RealPlayUrl(const TCHAR *pstrChannelID, const int nWndNum, const TCHAR * pstrDeviceId)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		//        strDeviceConfigDownload = "";
		//        return strDeviceConfigDownload.c_str();
	}
	const char *pInfo = NULL;
	pInfo = m_pCUATcp->SendRealPlayUrl(CLog::UnicodeToANSI(pstrChannelID).c_str(),nWndNum, CLog::UnicodeToANSI(pstrDeviceId).c_str());
	if(pInfo)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s RealPlayUrl URL:%s\n", __FUNCTION__, pInfo);
		return string(pInfo).c_str();
	}else
		return "";
	
}

const char * CExternalInterface::PlayBackUrl( const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, DATE tmStart, DATE tmEnd,const TCHAR *pstrLocation )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return NULL;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s PlayBackUrl DID:%s\n", __FUNCTION__, pstrDeviceID);
	COleDateTime datTimeStart(tmStart);
	int nSYear = datTimeStart.GetYear();
	int nSMonth = datTimeStart.GetMonth();
	int nSDay = datTimeStart.GetDay();
	int nSHour = datTimeStart.GetHour();
	int nSMinute = datTimeStart.GetMinute();
	int nSSecond = datTimeStart.GetSecond();

	COleDateTime datTimeEnd(tmEnd);
	int nEYear = datTimeEnd.GetYear();
	int nEMonth = datTimeEnd.GetMonth();
	int nEDay = datTimeEnd.GetDay();
	int nEHour = datTimeEnd.GetHour();
	int nEMinute = datTimeEnd.GetMinute();
	int nESecond = datTimeEnd.GetSecond();

	char strSTime[TIME_LEN+1];
	char strETime[TIME_LEN+1];

	if(3000 < nSYear || 12 < nSMonth || 31 < nSDay || 24 < nSHour || 60 < nSMinute || 60 < nSSecond)
		return NULL;
	if(3000 < nEYear || 12 < nEMonth || 31 < nEDay || 24 < nEHour || 60 < nEMinute || 60 < nESecond)
		return NULL;


	sprintf_s(strSTime,"%d-%02d-%02dT%02d:%02d:%02d", nSYear, nSMonth, nSDay, nSHour, nSMinute, nSSecond);
	sprintf_s(strETime,"%d-%02d-%02dT%02d:%02d:%02d", nEYear, nEMonth, nEDay, nEHour, nEMinute, nESecond);
	if(19 < strlen(strSTime) || 19 < strlen(strETime))
		return NULL;

	const char *pInfo = NULL;
#ifdef UNICODE
	pInfo = m_pCUATcp->SendPlayBackUrl(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(),
		strSTime, strETime, CLog::UnicodeToANSI(pstrLocation).c_str());
#else
	pInfo = m_pCUATcp->SendPlayBackUrl(pstrDeviceID, pstrChannelID,
		strSTime, strETime, pstrLocation);
#endif
	if(pInfo)
	{
		char url[256] = {0};
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s PlayBackUrl URL:%s\n", __FUNCTION__, pInfo);
	}

	return string(pInfo).c_str();
}

const char * CExternalInterface::DecoderStatus( const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID )
{
	strDecoderStatus = "";
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		//        strDeviceConfigDownload = "";
		//        return strDeviceConfigDownload.c_str();
	}
	const char *pInfo = NULL;
	pInfo = m_pCUATcp->SendDecoderStatus(CLog::UnicodeToANSI(pstrDeviceID).c_str(),CLog::UnicodeToANSI(pstrChannelID).c_str());
	if(pInfo)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s RealPlayUrl VideoDeviceID:%s\n", __FUNCTION__, pInfo);
		strDecoderStatus = pInfo;
		//todo
	}
	else
		strDecoderStatus = "";

	return strDecoderStatus.c_str();
}

int CExternalInterface::SipClientLogin( const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword )
{
	m_pCUATcp->SendLogin(pstrLocalSipID, pstrLocalIp, pstrLocalPort, pstrPltSipID, pstrPltIp, pstrPltPort, pstrPltPassword);
	return 0;
}

int CExternalInterface::SipClientLogout()
{
	m_pCUATcp->SendLogout();
	return 0;
}

int CExternalInterface::SendChangePassword( const char* pstrOldPassword, const char* pstrNewPassword )
{
	m_pCUATcp->SendChangePassword(pstrOldPassword, pstrNewPassword);
	return 0;
}

int CExternalInterface::SendCatalogSubscribe( const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime )
{
	return m_pCUATcp->SendCatalogSubscribe(pstrDeviceID, pstrChannelID, nExpires, pstrStartTime, pstrEndTime);
}

int CExternalInterface::SendAlarmSubScribe( const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime )
{
	return m_pCUATcp->SendAlarmSubScribe(pstrDeviceID, pstrChannelID,nExpires,nStartAlarmPriority, nEndAlarmPriority, nAlarmMethod, pstrSTime, pstrETime);
}

int CExternalInterface::SendAlarmQuery( const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const TCHAR* pstrSTime,const TCHAR* pstrETime,const TCHAR* pstrAlarmType )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return -1;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	const char *pInfo = NULL;
	pInfo = m_pCUATcp->SendAlarmQuery(CLog::UnicodeToANSI(pstrDeviceID).c_str(), CLog::UnicodeToANSI(pstrChannelID).c_str(), nStartAlarmPriority, nEndAlarmPriority, nAlarmMethod, CLog::UnicodeToANSI(pstrSTime).c_str(), CLog::UnicodeToANSI(pstrETime).c_str(),CLog::UnicodeToANSI(pstrAlarmType).c_str());
	if(pInfo)
	{
		g_pInfo = pInfo;
		ShowResult(MSG_ALARM_QUERY);
		return 1;
	}

	return 0;
}

int CExternalInterface::ShowAlaramQuery( HWND hWndInfo, const char * pXML )
{
	if(pXML && 0 < strlen(pXML))
	{
		LVCOLUMN ColInfo = {0};

		ColInfo.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		ColInfo.iSubItem = 0;
		ColInfo.fmt = LVCFMT_CENTER;
		ColInfo.cx = 150;
		ColInfo.cchTextMax = 50;
		ColInfo.pszText = _T("平台ID");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(0), LPARAM(&ColInfo));
		ColInfo.cx = 155;
		ColInfo.pszText = _T("设备ID");
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(1), LPARAM(&ColInfo));
		ColInfo.pszText = _T("Priority");
		ColInfo.cx = 55;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(2), LPARAM(&ColInfo));
		ColInfo.pszText = _T("Method");
		ColInfo.cx = 55;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(3), LPARAM(&ColInfo));
		ColInfo.pszText = _T("AlarmTime");
		ColInfo.cx = 140;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(4), LPARAM(&ColInfo));
		ColInfo.pszText = _T("AlarmDescription");
		ColInfo.cx = 100;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(5), LPARAM(&ColInfo));
		ColInfo.pszText = _T("Longitude");
		ColInfo.cx = 60;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(6), LPARAM(&ColInfo));
		ColInfo.pszText = _T("Latitude");
		ColInfo.cx = 60;
		SendMessage(hWndInfo, LVM_INSERTCOLUMN, WPARAM(7), LPARAM(&ColInfo));

		int						nOffset;
		int						nCatalogCnt = 0;
		const char*				pCurPos = pXML;
		TCHAR					strSumNum[MAX_PATH];
		TCHAR                   strPlatID[MAX_PATH];

		nOffset = ExtractXML(pCurPos, "Response", "<DeviceID>", "</DeviceID>", strPlatID, MAX_PATH);
		pCurPos += nOffset;
		nOffset = ExtractXML(pCurPos, "Alarm", "<SumNum>", "</SumNum>", strSumNum, MAX_PATH);
		pCurPos += nOffset;
		nCatalogCnt = _ttoi(strSumNum);

		int							nCurItemIdx = 0;

		for(int nCurCnt = 0; nCurCnt < nCatalogCnt; nCurCnt++)
		{
			
			TCHAR				strDeviceID[MAX_PATH];
			TCHAR				strAlarmPriority[MAX_PATH];
			TCHAR				strAlarmMethod[MAX_PATH];
			TCHAR				strAlarmTime[MAX_PATH];
			TCHAR				strAlarmDescription[MAX_PATH];
			TCHAR				strLongitude[MAX_PATH];
			TCHAR				strLatitude[MAX_PATH];

			ExtractXML(pCurPos, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<AlarmPriority>", "</AlarmPriority>", strAlarmPriority, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<AlarmMethod>", "</AlarmMethod>", strAlarmMethod, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<AlarmTime>", "</AlarmTime>", strAlarmTime, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<AlarmDescription>", "</AlarmDescription>", strAlarmDescription, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Longitude>", "</Longitude>", strLongitude, MAX_PATH);
			ExtractXML(pCurPos, "<Item>", "<Latitude>", "</Latitude>", strLatitude, MAX_PATH);
			pCurPos += nOffset;

			LVITEM						item;

			item.mask = LVIF_TEXT;

			// strPlatID
			item.pszText = strPlatID;
			item.iItem = nCurItemIdx;
			item.iSubItem = 0;
			::SendMessage(hWndInfo, LVM_INSERTITEM, 0, LPARAM(&item));

			nCurItemIdx++;

			// strDeviceID
			item.pszText = strDeviceID;
			item.iSubItem = 1;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strAlarmPriority
			item.pszText = strAlarmPriority;
			item.iSubItem = 2;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strAlarmMethod
			item.pszText = strAlarmMethod;
			item.iSubItem = 3;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strAlarmTime
			item.pszText = strAlarmTime;
			item.iSubItem = 4;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strAlarmDescription
			item.pszText = strAlarmDescription;
			item.iSubItem = 5;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strLongitude
			item.pszText = strLongitude;
			item.iSubItem = 6;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

			// strLatitude
			item.pszText = strLatitude;
			item.iSubItem = 7;
			::SendMessage(hWndInfo, LVM_SETITEM, 0, LPARAM(&item));

		}
		return nCatalogCnt;
	}

	return -1;
}

bool CExternalInterface::GetLocalIP(char* pReceiveIP, int nLen)
{
	if(m_pCUATcp)
	{
		std::string strIP = m_pCUATcp->GetLocalIP();
		if(strIP.length() < nLen && strIP.length() != 0)
		{
			memcpy(pReceiveIP, strIP.c_str(), strIP.length());
			return true;
		}
	}
	return false;
}
int CExternalInterface::StopPlayUrl(const TCHAR *pstrChannelID, const int nWndNum)
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return 0;
	}
	int nRet = 0;
#ifdef UNICODE
	nRet = m_pCUATcp->SendStopPlayUrl(CLog::UnicodeToANSI(pstrChannelID).c_str(), nWndNum);
#else
	nRet = m_pCUATcp->SendStopPlayUrl(pstrDeviceID, nWndNum);
#endif
	return nRet;
}

int CExternalInterface::DecoderDivision( const TCHAR *pstrDeviceID, const TCHAR *pstrChannelID, int nDivision )
{
	if(NULL == m_pCUATcp)
	{
		//g_objLog.LogoutDebug(k_LOG_DLL, "%s TCP服务未初始化\n", __FUNCTION__);
		return 0;
	}
	int nRet = 0;
#ifdef UNICODE
	nRet = m_pCUATcp->SendDecoderDivision(CLog::UnicodeToANSI(pstrDeviceID).c_str(),CLog::UnicodeToANSI(pstrChannelID).c_str(), nDivision);
#else
	nRet = m_pCUATcp->SendDecoderDivision(pstrDeviceID,pstrChannelID,nDivision);
#endif
	return nRet;
}



