#include "StdAfx.h"
#include "UACTCP.h"

#include "SdpParser.h"
#include <atlcomtime.h>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

extern const char *g_pCatalog;
using namespace std;
//#include <commctrl.h>

string GetStringBetween(string &strData, char *strFront, char *strBack, BOOL bIsInclude)
{
	string strTmp;

	int nPos_L = strData.find(strFront);
	int nPos_R = strData.find(strBack);
	if(0 >= nPos_L || 0 > nPos_R || nPos_L >= nPos_R)
		return strTmp;

	if(FALSE == bIsInclude)
		strTmp = strData.substr(nPos_L+strlen(strFront), nPos_R - nPos_L - strlen(strBack)+1);
	else
		strTmp = strData.substr(nPos_L, nPos_R - nPos_L + strlen(strBack));

	//去两边空格
	string::size_type loc = 0;
	loc = strTmp.find(' ');
	while (0 == loc)
	{
		strTmp.erase(loc, 1);
		loc = strTmp.find(' ',loc);
	}

	loc = strTmp.rfind(' ');
	while (0 == loc)
	{
		strTmp.erase(loc, 1);
		loc = strTmp.rfind(' ',loc);
	}

	return strTmp;
}

UINT WINAPI UACTCPProc(LPVOID lpParameter)
{
	CUACTCP *pUACTCP = (CUACTCP *)lpParameter;
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 等待消息……\n", __FUNCTION__);

	while(!pUACTCP->IsExit())
	{
		int nMsgType = pUACTCP->GetMsg();
		switch(nMsgType)
		{
		case LOGIN_CMD:
			pUACTCP->OnRegister();
			break;
		case LOGINRES_CMD:
			pUACTCP->OnRegisterRes();
			break;
		case LOGOUT_CMD:
			pUACTCP->OnLogout();
			break;
		case INVITERES_CMD:
			pUACTCP->OnInviteRes();
			break;
		case MESSAGE_CMD:
			pUACTCP->OnMessage();
			break;
		case MESSAGERES_CMD:
			pUACTCP->OnMessageRes();
			break;
		case NOTIFY_CMD:
			pUACTCP->OnNotify();
			break;
		default:
			pUACTCP->OnOther();
			break;
		}
	}

	return 0;
}

CUACTCP::CUACTCP(void)
{
	m_pSocket = NULL;
	m_pData = NULL;
	
	m_hMsgThread = NULL;
}

CUACTCP::~CUACTCP(void)
{
	g_objLog.LogoutDebug(k_LOG_DLL, "%s ~CUACTCP start\n", __FUNCTION__);

	if(NULL != m_hMsgThread)
	{
		WaitForSingleObject(m_hMsgThread, INFINITE/*1000*/);
		CloseHandle(m_hMsgThread);
		m_hMsgThread = NULL;
	}

	if(m_pSocket)
	{
		delete m_pSocket;
		m_pSocket = NULL;
	}
	
	g_objLog.LogoutDebug(k_LOG_DLL, "%s ~CUACTCP end\n", __FUNCTION__);
}

// 初始化对象
int CUACTCP::Init(const char *pstrUACID, const char *pstrIP, const char *pstrPort, const char * pstrPassword /*= NULL*/)
{
	g_objLog.LogoutInfo(k_LOG_DLL, "%s 创建Soket……\n", __FUNCTION__);
	if (m_pSocket!=NULL)
	{
		delete m_pSocket;
	}
	m_pSocket = new CSmartSocketClient(this);
	if(m_pSocket)
	{
		//char strHostName[MAX_PATH];
		//int nErr;
		//struct hostent* pHostent;
		//if(SOCKET_ERROR == gethostname(strHostName, MAX_PATH))
		//{
		//	nErr = WSAGetLastError();
		//}
		//pHostent = gethostbyname(strHostName);
		_tstring strIP = CLog::ANSIToUnicode(pstrIP);
		_tstring strPort = CLog::ANSIToUnicode(pstrPort);
		int nPort = -1;
		int nLen = strlen(pstrPort);
		if(6 > nLen)
		{
			int i;
			for(i = 0; i < nLen; i++)
			{
				if(48 > pstrPort[i] || 57 < pstrPort[i])
					break;
			}
			if(i == nLen)
				nPort = atoi(pstrPort);
			else
			{
				g_objLog.LogoutDebug(k_LOG_DLL, "%s 端口号异常 PORT:%s\n", __FUNCTION__, strPort.c_str());
				delete m_pSocket;
				m_pSocket = NULL;
				return -1;
			}
		}
		else
		{
			g_objLog.LogoutDebug(k_LOG_DLL, "%s 端口号异常 PORT:%s\n", __FUNCTION__, strPort.c_str());
			delete m_pSocket;
			m_pSocket = NULL;
			return -1;
		}
		if(INVALID_SOCKET == m_pSocket->Init(-1))
		{
			g_objLog.LogoutDebug(k_LOG_DLL, "%s Soket初始化失败 IP:%s  PORT:%d\n", __FUNCTION__, strIP.c_str(), nPort);
			delete m_pSocket;
			m_pSocket = NULL;
			return -1;
		}
		if(false == m_pSocket->Connect(strIP.c_str(),	//remote Sever IP 
												nPort			//remotr Sever Port
												))
		{
			g_objLog.LogoutDebug(k_LOG_DLL, "%s Soket初始化失败 IP:%s  PORT:%d\n", __FUNCTION__, strIP.c_str(), nPort);
			delete m_pSocket;
			m_pSocket = NULL;
			return -1;
		}
	}
	else
		g_objLog.LogoutDebug(k_LOG_DLL, "%s Soket创建失败\n", __FUNCTION__);

	strcpy_s(m_strOwnID, pstrUACID);

	//SendLogin();
	//ReadDeviceList();
	UINT unThreadID;
	m_hMsgThread = (HANDLE)_beginthreadex(NULL, 0, UACTCPProc, this, 0, &unThreadID);
	if(!m_hMsgThread)
	{
		delete m_pSocket;
		m_pSocket = NULL;
		return THREAD_CREATE_ERROR;
	}
	return 0;
}

int CUACTCP::GetOwnerType()
{
	return strcmp("EC", m_strOwnID);
}

int CUACTCP::GetMsg()
{
	m_pSocket->WaitData(100);
	HandleTimer();

	m_pData = m_pSocket->GetFirstDataFromList();
	
	if(m_pData)
	{
		g_objLog.LogoutDebug(k_LOG_DLL, "%s 取得数据\n", __FUNCTION__);
		if(m_pData)
		{
			int nMsgType = GetMsgType(m_pData->pDataBuf);
			g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到GetWay端消息，类型:%0x\n", __FUNCTION__, nMsgType);
			m_pData->bIsReaded = TRUE;
			return nMsgType;
		}
		else
			return UNDEFINED_CMD;
	}
	else
	{
		return UNDEFINED_CMD;
	}
}

int CUACTCP::OnMessage()
{
	if(m_pData)
	{
		int nOperate = GetMsgOperate(m_pData->pDataBuf);
		switch(nOperate)
		{
		case ALARM_NOTIFY_TYPE:
			OnAlarm();
			break;
		case FILE_TOEND_TYPE:
			OnPlayBackFinished();
			break;
		default:
			break;
		}

		g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到message消息 nOperate:%d\n", __FUNCTION__, nOperate);
		return 0;
	}
	return DATA_EMPTY_ERROR;
}

int CUACTCP::OnMessageRes()
{
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 处理msgRes\n", __FUNCTION__);
	if(m_pData)
	{
		int nStatus = 0;
		int nOperate = 0;

		nOperate = GetMsgOperate(m_pData->pDataBuf);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s nOperate:%d\n", __FUNCTION__, nOperate);
		switch(nOperate)
		{
		case CATALOG_INQUIRE_RES_TYPE:
			OnDeviceCatalogInquiryRes();
			break;
		case RECORD_INQUIRE_RES_TYPE:
			OnRecordInfoInquiryRes();
			break;
		case STATUS_INQUIRE_RES_TYPE:
			OnDeviceStatusInquiryRes();
			break;
		case INFO_INQUIRE_RES_TYPE:
			OnDeviceInfoInquiryRes();
			break;
		case SET_GUARD_RES_TYPE:
			OnSetGuardRes();
			break;
		case SET_RECORD_RES_TYPE:
			OnSetRecordRes();
			break;
		case FILE_TOEND_TYPE:
			OnPlayBackFinished();
			break;
		case PRESET_QUERY_TYPE:
			OnDevicePersetQueryRes();
			break;
		case CONFIG_DOWNLOAD_TYPE:
			OnConfigDownloadRes();
			break;
		case REAL_PLAY_URL_TYPE:
			OnRealPlayUrlRes();
			break;
		case PLAY_BACK_URL_TYPE:
			OnPlayBackUrlRes();
			break;
		case ALARM_QUERY_RES_TYPE:
			OnAlarmQueryRes();
			break;
		default:
			break;
		}

		nStatus = GetMsgStatus(m_pData->pDataBuf);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到message respons消息 类型:%d, 状态:%d\n", __FUNCTION__, nOperate, nStatus);

		return 0;
	}
	return DATA_EMPTY_ERROR;
}

int CUACTCP::OnInviteRes()
{
	if(m_pData)
	{
		int nStatus = 0;
		int nCmdID = 0;
		UINT unSeq = 0;
		int nCid = 0;
		int nDid = 0;
		int nDataSize = 0;
		char strData[XML_LEN];

		CDataStream dataStream((BYTE*)m_pData->pDataBuf);
		dataStream.OutputData(4, &nCmdID, 4);					//command id
		dataStream.OutputData(-1, &unSeq, 4);					//sequence
		dataStream.OutputData(-1, &nCid, 4);
		dataStream.OutputData(-1, &nDid, 4);
		dataStream.OutputData(-1, &nStatus, 4);					//status
		dataStream.OutputData(-1, &nDataSize, 4);					
		if(0 < nDataSize)
			dataStream.OutputData(-1, strData, nDataSize);					

		g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到invite respons消息 status %d, cid:%d, did:%d\n", __FUNCTION__, nStatus, nCid, nDid);

		_tstring strSdp = CLog::ANSIToUnicode(strData);
		SdpParser oSdpParser(strSdp);
		SDP_STACK  stSpdStack = oSdpParser.decode_sdp();
		MEDIA_INFO stMediaVideoInfo;
		MEDIA_INFO stMediaAudioInfo;

		std::list<MEDIA_INFO>::const_iterator it,begin,end;
		begin = stSpdStack.media_list.begin();
		end = stSpdStack.media_list.end();
		for (it = begin ; it != end ; it++)
		{
			if(!_tcsicmp((*it).media.c_str(), _T("video")))
				stMediaVideoInfo = (*it);
			if(!_tcsicmp((*it).media.c_str(), _T("audio")))
				stMediaAudioInfo = (*it);
		}
		
		int nPort = 0;
		nPort = _ttoi(stMediaVideoInfo.port.c_str());
		lpInviteParam_t pWaitMsg = m_objInviteList.GetWaitMsg(nCmdID, unSeq);
		if(pWaitMsg)
		{
			if(200 == nStatus)
			{
				void* pStreamer = pWaitMsg->pStreamer;
				m_mapStreamerID.SetPlayBackID(pStreamer, nCid, nDid);

				g_objLog.LogoutDebug(k_LOG_DLL, "%s 创建媒体流,远程端口:%d\n", __FUNCTION__, nPort);
				
			}

			if (m_fpInviteResponsed) {
				(*m_fpInviteResponsed)(strData, pWaitMsg->pStreamer);
			}

			delete pWaitMsg;
		}
		SendCallAck(nDid);
		return 0;
	}
	return DATA_EMPTY_ERROR;
}

int CUACTCP::OnRegister()
{
	int nMsgType = 0;
	if(m_pData)
	{
		int nLoginSum = 0;
		int nDataSize = 0;
		memcpy(&nDataSize, m_pData->pDataBuf, 4);
		g_objLog.LogoutInfo(k_LOG_DLL, "%s 下级注册成功后发CatalogQuery\n", __FUNCTION__);
		if(17 < nDataSize)
		{
			//取得已登录设备数
			memcpy(&nLoginSum, m_pData->pDataBuf+16, 4);

			for(int i = 0; i < nLoginSum; i++)
			{
				//发目录查询
				SendDeviceCatalogInquiry(m_pData->pDataBuf+20+i*(ID_LEN+1+4));
			}
		}
	}
	
	return OnHandleRegister(AC_DEVICE_STATUS_ONLINE);
}

int keepAliveCount=0;//Adapter和上级网关的keepalive次数
int CUACTCP::OnRegisterRes()
{
	keepAliveCount=0;
	return OnHandleRegister(AC_DEVICE_STATUS_NONE);
}

int CUACTCP::OnLogout()
{
	g_objLog.LogoutInfo(k_LOG_DLL, "%s m_listDowmPLDDevices的Size:%d\n", __FUNCTION__,m_listDowmPLDDevices.size() );
	for (  list<string>::iterator it = m_listDowmPLDDevices.begin(); it != m_listDowmPLDDevices.end(); ++it )
	{
		//把所有的下级站点的设备全部设置为离线
		m_fpDeviceStaustAlarm(CLog::ANSIToUnicode((*it).c_str()).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);
	} 
	keepAliveCount=0;
	return OnHandleRegister(AC_DEVICE_STATUS_OFFLINE);
}

int CUACTCP::OnHandleRegister(int nStatusType)
{
	int nMsgType = 0;
	if(m_pData)
	{
		int nLoginSum = 0;
		int nDataSize = 0;

		memcpy(&nDataSize, m_pData->pDataBuf, 4);
		int nStatus = 0;
		memcpy(&nStatus, m_pData->pDataBuf+12, 4);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到注册应答消息 状态： %d\n", __FUNCTION__, nStatus);

		if(17 < nDataSize && NULL != m_fpDeviceStaustAlarm)
		{
			//取得已登录设备数
			memcpy(&nLoginSum, m_pData->pDataBuf+16, 4);
			
			for(int i = 0; i < nLoginSum; i++)
			{
				if(AC_DEVICE_STATUS_NONE == nStatusType)
					memcpy(&nMsgType, m_pData->pDataBuf+20+i*(ID_LEN+1+4)+(ID_LEN+1), 4);
				else
					nMsgType = nStatusType;
#ifdef UNICODE
				
				if(AC_DEVICE_STATUS_ONLINE == nMsgType)
					g_objLog.LogoutDebug(k_LOG_DLL, "%s 设备%s在线\n", __FUNCTION__, m_pData->pDataBuf+20+i*(ID_LEN+1+4));
				else
					g_objLog.LogoutDebug(k_LOG_DLL, "%s 设备%s不在线\n", __FUNCTION__, m_pData->pDataBuf+20+i*(ID_LEN+1+4));

				m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(m_pData->pDataBuf+20+i*(ID_LEN+1+4)).c_str(), _T(""), AT_DEVICE_STATUS, nMsgType);
#else
				m_DeviceStaustAlarm(pData->pDataBuf+20+i*(ID_LEN+1), "", AT_DEVICE_STATUS, AC_DEVICE_STATUS_ONLINE);
#endif
			}
		}

		//delete pData;
		return nLoginSum;
	}
	return DATA_EMPTY_ERROR;	
}

int CUACTCP::OnDeviceCatalogInquiryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE*3]={0};
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strCatalogInfoListXml += strExterndData;

	g_objLog.LogoutInfo(k_LOG_DLL, "%s 收到DeviceStatus respons消息 nOperate:%d, nExterndSize:%d \n strExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strCatalogInfoListXml.c_str());

	int					nOffset;
	int					nCatalogCnt = 0;
	const char*			pCurPos = strExterndData;
	TCHAR				strSumNum[MAX_PATH];

	nOffset = externalInterface.ExtractXML(pCurPos, "Catalog", "<SumNum>", "</SumNum>", strSumNum, MAX_PATH);
	pCurPos += nOffset;
	nCatalogCnt = _ttoi(strSumNum);

	int	nCurItemIdx = 0;

	for(int nCurCnt = 0; nCurCnt < nCatalogCnt; nCurCnt++)
	{
		TCHAR		strDeviceID[MAX_PATH];
		TCHAR		strEvent[MAX_PATH];
		externalInterface.ExtractXML(pCurPos, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
		int iLen = WideCharToMultiByte(CP_ACP, 0,strDeviceID, -1, NULL, 0, NULL, NULL);
		char* chRtn =new char[iLen*sizeof(char)];
		WideCharToMultiByte(CP_ACP, 0, strDeviceID, -1, chRtn, iLen, NULL, NULL);
		std::string deviceID(chRtn);
		if (deviceID=="")
		{
			delete[] chRtn;
			break;
		}
		nOffset =externalInterface.ExtractXML(pCurPos, "<Item>", "<Status>", "</Status>", strEvent, MAX_PATH);
		pCurPos += nOffset;

		int iLenEvent = WideCharToMultiByte(CP_ACP, 0,strEvent, -1, NULL, 0, NULL, NULL);
		char* chRtnEvent =new char[iLenEvent*sizeof(char)];
		WideCharToMultiByte(CP_ACP, 0, strEvent, -1, chRtnEvent, iLenEvent, NULL, NULL);
		std::string stringEvent(chRtnEvent);

		//g_objLog.LogoutInfo(k_LOG_DLL, "%s 设备ID:%s,状态:%s\n", __FUNCTION__, deviceID,stringEvent);
		transform(stringEvent.begin(),stringEvent.end(),stringEvent.begin(),tolower);
		 list<string>::iterator it =find( m_listDowmPLDDevices.begin(), m_listDowmPLDDevices.end(), deviceID );
		 //判断该deviceID是否在该集合中
		if (it == m_listDowmPLDDevices.end()&&stringEvent!="")
		{
			m_listDowmPLDDevices.push_back(deviceID);
		}
		g_objLog.LogoutInfo(k_LOG_DLL, "%s m_listDowmPLDDevices的Size:%d\n", __FUNCTION__,m_listDowmPLDDevices.size() );
		if (stringEvent=="off")
		{
			//g_objLog.LogoutInfo(k_LOG_DLL, "%s off :deviceID %s\n", __FUNCTION__,deviceID);
			m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(deviceID.c_str()).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);
		}
		else if (stringEvent =="on")
		{
			//g_objLog.LogoutInfo(k_LOG_DLL, "%s on: deviceID %s\n", __FUNCTION__,deviceID);
			m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(deviceID.c_str()).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_ONLINE);
		}
		delete[] chRtnEvent;
		delete[] chRtn;
	}
	return 0;
}

int CUACTCP::OnDeviceStatusInquiryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strDeviceStatusListXml = strExterndData;
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DeviceStatus respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strDeviceStatusListXml.c_str());

	return 0;
}

int CUACTCP::OnDeviceInfoInquiryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strDeviceInfoListXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DeviceStatus respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, strExterndData);

	return 0;
}

int addCount=0;
string	 sn="";
int CUACTCP::OnRecordInfoInquiryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];

	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到RecordInfo respons消息 nOperate:%d, nExterndSize:%d strExterndData:%s\n", __FUNCTION__, nOperate, nExterndSize,
	//	CLog::ANSIToUnicode(strExterndData).c_str());

	string strRecordInfo = strExterndData;

	int nRecordSum = 0;
	string strSumNum = GetStringBetween(strRecordInfo, "<SumNum>", "</SumNum>", FALSE);
	if(!strSumNum.empty())
	{
		nRecordSum = atoi(strSumNum.c_str());
	}

	string strSN = GetStringBetween(strRecordInfo, "<SN>", "</SN>", FALSE);
	if (sn=="")
	{
		sn	=strSN;
	}
	else
	{
		if (sn!=strSN)
		{
			addCount=0;
			sn	=strSN;
		}
	}

	g_objLog.LogoutInfo(k_LOG_DLL, "%s SumNum:%d\n", __FUNCTION__, nRecordSum);
	if(MAX_RECORDLIST_SIZE < nRecordSum)
	{
		int nPos = strRecordInfo.find("RecordList");
		if(string::npos == nPos)
			return RECORD_INFO_ERROR;

		string strRecordSubInfo = strRecordInfo.substr(nPos, strRecordInfo.length()-nPos);
		string trNum;
		int nLPos = strRecordSubInfo.find("=");
		int nRPos = strRecordSubInfo.find(">");
		if(string::npos != nLPos && string::npos != nRPos && nLPos < nRPos)
			trNum = strRecordSubInfo.substr(nLPos+1, nRPos-nLPos-1);

		if(!trNum.empty())
		{
			if('\"' == trNum.front())
				trNum.erase(trNum.begin());
		}
		if(!trNum.empty())
		{
			if('\"' == trNum.back())
				trNum.erase(trNum.end());
		}
		int nNum = 0;
		if(!trNum.empty())
			nNum = atoi(trNum.c_str());

		m_nRecordSum += nNum;
		g_objLog.LogoutDebug(k_LOG_DLL, "%s m_nRecordSum%d, Record list:%d\n", __FUNCTION__, m_nRecordSum, nNum);


		m_strRecordListXml += strExterndData;
		g_objLog.LogoutDebug(k_LOG_DLL, "%s m_pRecordListXml list:%s\n", __FUNCTION__, m_strRecordListXml.c_str());
		
		if(nRecordSum == m_nRecordSum)
		{
			m_nRecordSum = XML_LEN;
		}
		return 0;
	}
	else
	{
		g_objLog.LogoutDebug(k_LOG_DLL, "%s nRecordSum:%d\n", __FUNCTION__, nRecordSum);
		m_strRecordListXml += strExterndData;

		int nPos = strRecordInfo.find("RecordList");
		if(string::npos == nPos)
			return RECORD_INFO_ERROR;

		string strRecordSubInfo = strRecordInfo.substr(nPos, strRecordInfo.length()-nPos);
		string trNum;
		int nLPos = strRecordSubInfo.find("=");
		int nRPos = strRecordSubInfo.find(">");
		if(string::npos != nLPos && string::npos != nRPos && nLPos < nRPos)
			trNum = strRecordSubInfo.substr(nLPos+1, nRPos-nLPos-1);

		if(!trNum.empty())
		{
			if('\"' == trNum.front())
				trNum.erase(trNum.begin());
		}
		if(!trNum.empty())
		{
			if('\"' == trNum.back())
				trNum.erase(trNum.end());
		}
		int nNum = 0;
		if(!trNum.empty())
			nNum = atoi(trNum.c_str());
		addCount+=nNum;
		if (addCount==nRecordSum || nRecordSum==0)
		{
			m_nRecordSum = XML_LEN;
			addCount=0;
		}
		return 0;
	}
}

int CUACTCP::OnSetRecordRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);					//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到StartRecord respons消息 \nstrExterndData %s\n", __FUNCTION__, strExterndData);
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到StartRecord respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, strExterndData);

	return 0;
}

int CUACTCP::OnPlayBackFinished()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nCid = 0;
	int nDid = 0;
	int nStatus = 0;
	char strDeviceID[ID_LEN+1];
	char strSubDeviceID[ID_LEN+1];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(12, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);			
	dataStream.OutputData(-1, strDeviceID, ID_LEN+1);	
	dataStream.OutputData(-1, strSubDeviceID, ID_LEN+1);	
	dataStream.OutputData(-1, &nCid, 4);						
	dataStream.OutputData(-1, &nDid, 4);						


	//g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到StartRecord respons消息 \nstrExterndData %s\n", __FUNCTION__, strExterndData);
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到录像回放结束消息 DeviceID:%s, SubDeviceID:%s, nCid:%d, nDid%d, 发送BYE\n", __FUNCTION__, strDeviceID, strSubDeviceID, nCid, nDid);
	SendBye(strDeviceID, strSubDeviceID, nCid, nDid, 0);

	return 0;
}

int CUACTCP::OnSetGuardRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);					//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到StartGuard respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, strExterndData);

	//m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(m_pData->pDataBuf+20+i*(ID_LEN+1+4)).c_str(), AT_DEVICE_STATUS, nMsgType);

	return 0;
}
int CUACTCP::SendLogin()
{
	//3次没有响应的话代表上级网关退出
	if (keepAliveCount>2)
	{
		OnLogout();
	}
	int nMegSize = 4;
	int nDataSize = 0;
	int nCmdID = LOGIN_CMD;
	char szLoginBuf[LOGIN_MSG_SIZE];
	CDataStream dataStream((BYTE*)szLoginBuf, LOGIN_MSG_SIZE);

	if(0 == strcmp("EC", m_strOwnID))
	{
		dataStream.InputData(4, &nCmdID, 4);					//command id
		dataStream.InputData(-1, &m_unSeq, 4);					//sequence
		dataStream.InputData(-1, m_strOwnID, ID_BUFFER_SIZE);	//own id
		dataStream.InputData(-1, "#", 1);						//finish symbol
		keepAliveCount++;
		m_pSocket->SendData(szLoginBuf, dataStream.GetLastPos());
		g_objLog.LogoutDebug(k_LOG_DLL, "%s EC longin, data size:%d\n", __FUNCTION__, nMegSize);

		return 0;
	}

	//g_objLog.LogoutDebug(k_LOG_DLL, "%s IS NOT EC longin\n", __FUNCTION__);
	return 0;
}

int CUACTCP::SendLogin( const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword )
{
	m_strSipClientID = pstrLocalSipID;

	char szSipClientLogin[150];
	int nCmdID = PRIVATE_CMD;
	int nOperate = SIP_CLIENT_LOGIN_TYPE;
	CDataStream dataStream((BYTE*)szSipClientLogin, 150);
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrLocalSipID, ID_LEN+1);
	dataStream.InputData(-1, pstrLocalIp, IP_LEN+1);
	dataStream.InputData(-1, pstrLocalPort, PORT_LEN + 1);
	dataStream.InputData(-1, pstrPltSipID, ID_LEN+1);
	dataStream.InputData(-1, pstrPltIp, IP_LEN+1);
	dataStream.InputData(-1, pstrPltPort, PORT_LEN + 1);
	dataStream.InputData(-1, pstrPltPassword, PASS_LEN + 1);
	dataStream.InputData(-1, "#", 1);					//finish symbol
	m_pSocket->SendData(szSipClientLogin, dataStream.GetLastPos());
	return NULL;
}

//int CUACTCP::SendDecorder(const TCHAR *pstrDecorderID, const TCHAR *pstrCameraID, int nOption)
int CUACTCP::SendDecorder(const char* pstrDecorderID, const char* pstrCameraID, int nOption)
{
	// PWolf: Add 2013.06.17
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 发送解码器实时点播(DECODER_ID[%s] CAMERA_ID[%s] OPER_TYPE[%d])\n", __FUNCTION__, pstrDecorderID, pstrCameraID, nOption);

	char szBuf[INVITE_MSG_SIZE];
	int nCmdID = DECODERPLAY_CMD;
	CDataStream dataStream((BYTE*)szBuf, INVITE_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, pstrDecorderID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, pstrCameraID, ID_BUFFER_SIZE);	//Media ID
	dataStream.InputData(-1, &nOption, 4);					//media transmission type
	
	dataStream.InputData(-1, "#", 1);						//finish symbolthis

	if(!m_pSocket->SendData(szBuf, dataStream.GetLastPos()))
	{
		g_objLog.LogoutDebug(k_LOG_DLL, "%s 媒体流协商失败\n", __FUNCTION__);
		return -1;
	}

	m_unSeq++;

	// PWolf: Add End
	return 0;
}

int CUACTCP::SendInvite(int nOperate, int nProType, const char *pstrDeviceID, 
						const char *pstrChannelID, const char *pstrRecvIP, int nRecvPort[2], 
						void *pStreamer, char *ptmStart, char *ptmEnd)
{

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 开始媒体流协商\n", __FUNCTION__);

	char szInviteBuf[INVITE_MSG_SIZE];
	int nCmdID = INVITE_CMD;
	CDataStream dataStream((BYTE*)szInviteBuf, INVITE_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);						//command id
	dataStream.InputData(-1, &m_unSeq, 4);						//sequence
	dataStream.InputData(-1, &nOperate, 4);						//nOperate
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);		//Remove Device ID
	dataStream.InputData(-1, pstrChannelID, ID_BUFFER_SIZE);	//Media ID
	dataStream.InputData(-1, &nProType, 4);						//media transmission type
	dataStream.InputData(-1, pstrRecvIP, IP_BUFFER_SIZE);		//Recv IP
	dataStream.InputData(-1, &(nRecvPort[0]), 4);				//Recv Port
	dataStream.InputData(-1, &(nRecvPort[1]), 4);				//Recv Port
	//dataStream.InputData(-1, &(nRecvPort[2]), 4);				//Recv Port
	//dataStream.InputData(-1, &(nRecvPort[3]), 4);				//Recv Port
	dataStream.InputData(-1, ptmStart, TIME_LEN+1);			
	dataStream.InputData(-1, ptmEnd, TIME_LEN+1);			

	dataStream.InputData(-1, "#", 1);						//finish symbolthis

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d, DeviceID=%s, MediaID=%s\n", __FUNCTION__, 
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate, 
								pstrDeviceID, pstrChannelID);

	if(!m_pSocket->SendData(szInviteBuf, dataStream.GetLastPos()))
	{
		g_objLog.LogoutDebug(k_LOG_DLL, "%s 媒体流协商失败\n", __FUNCTION__);
		return -1;
	}

	//添加到消息队列，等待前端返回相应
	lpInviteParam_t lMsg = m_objInviteList.NewWaitMsg();
	lMsg->nCmdID = INVITERES_CMD;
	lMsg->unSeq = m_unSeq;
	strcpy_s(lMsg->strDstID, pstrDeviceID);
	lMsg->pStreamer = pStreamer;
	lMsg->nRecvPort[0] = nRecvPort[0];
	lMsg->nRecvPort[1] = nRecvPort[1]; 
	lMsg->nOperate = nOperate;

	m_unSeq++;
	return 0;
}

int CUACTCP::SendAnswered(int nStatus)
{
	return 0;
}

int CUACTCP::OnOther()
{
	//recv_data_t *pData = m_pSocket->GetFirstDataFromList(TRUE);
	if(m_pData)
	{
		//delete pData;
		return 0;
	}

	return DATA_EMPTY_ERROR;
}

int CUACTCP::OnDevicePersetQueryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strDevicePersetQueryXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DevicePersetQuery respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strCatalogInfoListXml.c_str());
	return 0;
}

int CUACTCP::OnConfigDownloadRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strDeviceConfigDownloadXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到ConfigDownload respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strCatalogInfoListXml.c_str());
	return 0;
}

int CUACTCP::OnRealPlayUrlRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strRealPlayUrlXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到RealPlayUrl respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strRealPlayUrlXml.c_str());
	return 0;
}

int CUACTCP::OnPlayBackUrlRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strPlayBackUrlXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到PlayBackUrl respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strRealPlayUrlXml.c_str());
	return 0;
}

int CUACTCP::OnDecoderStatusRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strDecoderStatusXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DecoderStatus respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strRealPlayUrlXml.c_str());
	return 0;
}

int CUACTCP::SendDeviceControl(const char *pstrDeviceID, const char *pstrChannelID, BYTE bCmdType, BYTE bData1, BYTE bData2, BYTE bData3, BYTE cZoom, int nLength, int nWidth, int nMidPointX, int nMidPointY, int nLengthX, int nLengthY)
{
	char szDeviceCtrlBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = DEVICE_CTRL_TYPE;
	CDataStream dataStream((BYTE*)szDeviceCtrlBuf, CTRL_MSG_SIZE);
	g_objLog.LogoutDebug(k_LOG_DLL, "%s bCmdType:%lu\n", __FUNCTION__, bCmdType);
	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);					//Operate : device control, search file 
	dataStream.InputData(-1, &bCmdType, 1);					//Cmd type 0x00-0x8a
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, pstrChannelID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, &bData1, 1);					//LevelPace
	dataStream.InputData(-1, &bData2, 1);					//VerticalPace
	dataStream.InputData(-1, &bData3, 1);					//ZoomPace
	// add cy 2015.7.24
	dataStream.InputData(-1, &cZoom, 1);                     //ZoomType DRAG_ZOOM_NONE 0 DRAG_ZOOM_IN 1 DRAG_ZOOM_OUT 2 DRAG_ZOOM_NULL 3
	dataStream.InputData(-1, &nLength, 4);
	dataStream.InputData(-1, &nWidth, 4);
	dataStream.InputData(-1, &nMidPointX, 4);
	dataStream.InputData(-1, &nMidPointY, 4);
	dataStream.InputData(-1, &nLengthX, 4);
	dataStream.InputData(-1, &nLengthY, 4);
	dataStream.InputData(-1, "#", 1);						//finish symbol


	g_objLog.LogoutDebug(k_LOG_DLL, "%s pstrDeviceID=%s, pstrChannelID=%s\n", __FUNCTION__, 
							pstrDeviceID, pstrChannelID);
	m_pSocket->SendData(szDeviceCtrlBuf, dataStream.GetLastPos());

	m_unSeq++;
	return 0;
}

int CUACTCP::SendRemoteStartup(const char *pstrDeviceID)
{
	g_objLog.LogoutDebug(k_LOG_DLL, "%s SendRemoteStartup\n", __FUNCTION__);
	char szDeviceCtrlBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = REMOTE_STARTUP_TYPE;
	CDataStream dataStream((BYTE*)szDeviceCtrlBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);					//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, "#", 1);						//finish symbol

	if(true == m_pSocket->SendData(szDeviceCtrlBuf, dataStream.GetLastPos()))
	{
		m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(pstrDeviceID).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);
	}

	m_unSeq++;
	return 0;
}

const char* CUACTCP::SendDeviceCatalogInquiry(const char *pstrDeviceID)
{
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = CATALOG_INQUIRE_TYPE;
	m_strCatalogInfoListXml = "";
	CDataStream dataStream((BYTE*)szCatalogBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	//device id
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__,
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());
	m_unSeq++;

	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strCatalogInfoListXml.size())
		{
			Sleep(2000);
			return m_strCatalogInfoListXml.c_str();
		}
		Sleep(100);
	}
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 未查找到设备目录\n", __FUNCTION__);
	return 0;
}

const char * CUACTCP::SendDeviceInfoInquiry(const char *pstrDeviceID)
{
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = INFO_INQUIRE_TYPE;
	m_strDeviceInfoListXml = "";
	CDataStream dataStream((BYTE*)szCatalogBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);			
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());
	m_unSeq++;

	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDeviceInfoListXml.size())
		{
			return m_strDeviceInfoListXml.c_str();
		}
		Sleep(100);
	}

	return NULL;
}

const char * CUACTCP::SendDeviceStatusInquiry(const char *pstrDeviceID)
{
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = STATUS_INQUIRE_TYPE;
	m_strDeviceStatusListXml = "";
	CDataStream dataStream((BYTE*)szCatalogBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());

	m_unSeq++;
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDeviceStatusListXml.size())
			return m_strDeviceStatusListXml.c_str();
		Sleep(100);
	}

	return NULL;
}

int CUACTCP::SendRecord(const char *pstrDeviceID, const char *pstrChannelID, int nActive)
{
	char strStartRecordBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = SET_RECORD_TYPE;
	//int nActive = 0;    //start record
	CDataStream dataStream((BYTE*)strStartRecordBuf, sizeof(strStartRecordBuf));

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, &nActive, 4);				//0 start , 1 stop
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	//device id
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	//device id
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__,
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(strStartRecordBuf, dataStream.GetLastPos());
	m_unSeq++;
	return 0;
}

int CUACTCP::SendGuard(const char *pstrDeviceID, const char *pstrChannelID, int nActive)
{
	char strStartRecordBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = SET_GUARD_TYPE;

	CDataStream dataStream((BYTE*)strStartRecordBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, &nActive, 4);				//0 start , 1 stop
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	//device id
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	//device id
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(strStartRecordBuf, dataStream.GetLastPos());
	m_unSeq++;
	return 0;
}

int CUACTCP::SendEventSubscribe(const char *pstrDeviceID, int nStratPriority, int nEndPriority, int nMethod, char* pstrStartTime, char* pstrEndTime)
{
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = SEND_EVENT_SUBSCRIBE;
	CDataStream dataStream((BYTE*)szCatalogBuf, CTRL_MSG_SIZE);
	
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);			//Operate : device control, search file 
	dataStream.InputData(-1, &nStratPriority, 4);			
	dataStream.InputData(-1, &nEndPriority, 4);			
	dataStream.InputData(-1, &nMethod, 4);	
	dataStream.InputData(-1, pstrStartTime, TIME_LEN+1);
	dataStream.InputData(-1, pstrEndTime, TIME_LEN+1);
	dataStream.InputData(-1, "#", 1);					//finish symbol


	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());
	m_unSeq++;
	return 0;
}

int CUACTCP::SendCatalogSubscribe(const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime)
{
	int nMegSize = 4;
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = SUBSCRIBE_CMD;
	int nOperate = CATALOG_SUBSCRIBE_TYPE;
	CDataStream dataStream((BYTE*)szCatalogBuf, CTRL_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);			//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);
	dataStream.InputData(-1, &nExpires, 4);
	dataStream.InputData(-1, pstrStartTime, TIME_LEN+1);
	dataStream.InputData(-1, pstrEndTime, TIME_LEN+1);
	dataStream.InputData(-1, "#", 1);					//finish symbol


	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
								nMegSize, nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());
	m_unSeq++;
	return 0;
}

int CUACTCP::SendBye(const char *pstrDeviceID, const char *pstrChannelID, void *pStreamer, int nOption)
{
	int nMegSize = 4;
	int nCmdID = BYE_CMD;
	int nCid = -1;
	int nDid = -1;
	m_mapStreamerID.GetPlayBackID(pStreamer, nCid, nDid);

	if(-1 != nCid && -1 != nDid)
	{
		SendBye(pstrDeviceID, pstrChannelID, nCid, nDid, nOption);
		m_mapStreamerID.RemovePlayBackID(pStreamer);
	}

	m_unSeq++;
	return 0;
}

int CUACTCP::SendBye(const char *pstrDeviceID, const char *pstrChannelID, int nCid, int nDid, int nOption)
{
	int nMegSize = 4;
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = BYE_CMD;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
	if(-1 != nCid && -1 != nDid)
	{
		CDataStream dataStream((BYTE*)szCatalogBuf, sizeof(szCatalogBuf));

		dataStream.InputData(4, &nCmdID, 4);					//command id
		dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	
		//dataStream.InputData(-1, &nStreamType, 4);				//Operate  :play, playback, download
		dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
		dataStream.InputData(-1, pstrChannelID, ID_BUFFER_SIZE);//Remove Device ID
		dataStream.InputData(-1, &nCid, 4);						//Call ID
		dataStream.InputData(-1, &nDid, 4);						//Call ID
		dataStream.InputData(-1, "#", 1);						//finish symbol


		g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d\n", __FUNCTION__, 
									nMegSize, nCmdID, m_unSeq);
		if(true == m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos()))
			m_unSeq++;
		else 
			return -1;
	}

	return 0;
}

//nOperate: 0 play,  1 pause,  2 speed
int CUACTCP::SendPlayBackCtrl(const char *pstrDeviceID, const char *pstrChannelID, void* pStreamer, const int nOperate, float fValue)
{
	int nMegSize = 4;
	char szCatalogBuf[CTRL_MSG_SIZE];
	int nCmdID = INFO_CMD;
	int nCid = 0;
	int nDid = 0;
	float fSpeed = 1.0;
	m_mapStreamerID.GetPlayBackID(pStreamer, nCid, nDid);
	if(SPEED_TYPE == nOperate)
		SetSpeed(pStreamer, fValue);
	else
		fSpeed = GetSpeed(pStreamer);
	g_objLog.LogoutInfo(k_LOG_DLL, "%s fValue:%f\n", __FUNCTION__, fValue);
	g_objLog.LogoutInfo(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
	CDataStream dataStream((BYTE*)szCatalogBuf, sizeof(szCatalogBuf));

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);					//Operate
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, &nDid, 4);						//Dialog ID
	dataStream.InputData(-1, &fValue, 4);					
	dataStream.InputData(-1, &fSpeed, 4);	
	dataStream.InputData(-1, "#", 1);						//finish symbol


	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d\n", __FUNCTION__, 
								nMegSize, nCmdID, m_unSeq);
	if(m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos()))
	{
		if(PAUSE_TYPE == nOperate)
			SetPlayStatus(pStreamer, nOperate);
		else
			SetPlayStatus(pStreamer, nOperate);
	}
	m_unSeq++;

	return 0;
}

int CUACTCP::SetPlayBackID(void* pStreamer,  int nCid, int nDid)
{
	if(pStreamer)
	{
		m_mapStreamerID.SetPlayBackID(pStreamer, nCid, nDid);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
	}

	return 0;
}

int CUACTCP::GetPlayBackID(void* pStreamer, int &nCid, int &nDid)
{
	if(pStreamer)
	{
		m_mapStreamerID.GetPlayBackID(pStreamer, nCid, nDid);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
	}

	return 0;
}

int CUACTCP::SetSpeed(void* pStreamer, float fSpeed)
{
	if(pStreamer)
	{
		m_mapStreamerID.SetSpeed(pStreamer, fSpeed);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s Set Speed:%f\n", __FUNCTION__, fSpeed);
	}

	return 0;
}

int CUACTCP::GetPlayStatus(void* pStreamer)
{
	int nPlayStatus  = 0;
	if(pStreamer)
	{
		m_mapStreamerID.GetPlayStatus(pStreamer, nPlayStatus);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s Get nPlayStatus:%d\n", __FUNCTION__, nPlayStatus);
	}

	return nPlayStatus;
}

int CUACTCP::SetPlayStatus(void* pStreamer, int nPlayStatus)
{
	if(pStreamer)
	{
		m_mapStreamerID.SetPlayStatus(pStreamer, nPlayStatus);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s Set PlayStatus:%d\n", __FUNCTION__, nPlayStatus);
	}

	return 0;
}
float CUACTCP::GetSpeed(void* pStreamer)
{
	float fSpeed  = 0.0;
	if(pStreamer)
	{
		m_mapStreamerID.GetSpeed(pStreamer, fSpeed);
		g_objLog.LogoutDebug(k_LOG_DLL, "%s Get Speed:%f\n", __FUNCTION__, fSpeed);
	}

	return fSpeed;
}

const char* CUACTCP::SendRecordInfoInquiry(const char *pstrDeviceID,  const char *pstrChannelID, int nType, 
						 const char* strSTime,
						 const char* strETime, const char* pstrFilePath /*= 0*/, const char* pstrAddress /*= 0*/, BYTE bSecrecy /*= 0*/, const char* pstrRecorderID /*= NULL*/, BYTE IndistinctQuery /*= 0*/)
{
	//return SendPlayBackUrl(pstrDeviceID, pstrChannelID, pstrSTime, strETime,"");
	//SendCatalogSubscribe("00151000000403000001", "00151001031103000001", 3600, (char*)strSTime, (char*)strETime);
	//SendAlarmSubScribe("00151000000403000001", "00151001031103000001", 90, 1, 4, 0, (char*)strSTime, (char*)strETime);
	//SendAlarmQuery("34020000001180000002", "34020000001180000002", 1, 4, 0, (char*)strSTime, (char*)strETime,"告警设备类型");
	//return "";
	g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	char strSearchBuf[SEARCH_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = RECORD_INQUIRE_TYPE;
	m_strRecordListXml = "";
	m_nRecordSum = 0;

	CDataStream dataStream((BYTE*)strSearchBuf, sizeof(strSearchBuf));

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	//device id
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	//device id
	dataStream.InputData(-1, &nType, 4);
	dataStream.InputData(-1, strSTime, TIME_LEN+1);	
	dataStream.InputData(-1, strETime, TIME_LEN+1);
	//如果为默认NULL,则不传主体,只传长度NULL
	int len = 0; 
	if(pstrFilePath) len = strlen(pstrFilePath) + 1;
	else len = 0;
	dataStream.InputData(-1,&len,4);
	if(len) dataStream.InputData(-1,pstrFilePath,len);

	len = 0; 
	if(pstrAddress) len = strlen(pstrAddress) + 1;
	else len = 0;
	dataStream.InputData(-1,&len,4);
	if(len) dataStream.InputData(-1,pstrAddress,len);

	dataStream.InputData(-1,&bSecrecy,1);

	len = 0; 
	if(pstrRecorderID) len = strlen(pstrRecorderID) + 1;
	else len = 0;
	dataStream.InputData(-1,&len,4);
	if(len) dataStream.InputData(-1,pstrRecorderID,len);

	dataStream.InputData(-1,&IndistinctQuery,1);        //是否模糊查询 
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__,
								dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(strSearchBuf, dataStream.GetLastPos());
	m_unSeq++;
	//等待10秒
	for(int i = 0; i < 100; i++)
	{
		if(XML_LEN == m_nRecordSum)
			return m_strRecordListXml.c_str();
		Sleep(100);
	}
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 请求超时\n", __FUNCTION__);
	return NULL;
}

int CUACTCP::OnAlarm(void)
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nOperate = 0;
	int nSize = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	char strAlarmPriority[PRIORITY_LEN+1];
	char strAlarmMethod[METHOD_LEN+1];
	char strAlarmTime[TIME_LEN+1];
	char strSrcID[ID_LEN+1];
	char strSrcAlarmID[ID_LEN+1];
	char strSrcAlarmType[TYPE_LEN+1];

	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);					
	//dataStream.OutputData(-1, strAlarmPriority, 2);		
	//dataStream.OutputData(-1, strAlarmMethod, 2);
	//dataStream.OutputData(-1, strAlarmTime, TIME_LEN+1);
	dataStream.OutputData(-1, strSrcID, ID_LEN+1);
	dataStream.OutputData(-1, &nSize, 4);
	if(0 < nSize)
		dataStream.OutputData(-1, strExterndData, nSize);
	int nMsgType = 0;

	ParseAlarmData(strExterndData, strSrcAlarmID,  strAlarmPriority, strAlarmTime, strAlarmMethod,strSrcAlarmType);
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到Alaer respons消息 \n报警ID:%s 级别:%s 时间:%s 方法:%s\n", __FUNCTION__, 
		strSrcAlarmID, 
		strAlarmPriority, 
		strAlarmTime, 
		strAlarmMethod);
	
	int nPriority = atoi(strAlarmPriority);
	int nMethod = atoi(strAlarmMethod);
	
	if(1 > nPriority || 4 < nPriority)
		nPriority = 5;

	if(1 > nMethod || 7 < nMethod)
		nMethod = 7;
	if(!m_fpDeviceStaustAlarm)
	{
		g_objLog.LogoutDebug(k_LOG_DLL, "%s m_fpDeviceStaustAlarm = NULL\n", __FUNCTION__);
		return -1;
	}
#ifdef UNICODE
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 报警设备ID:%s\n", __FUNCTION__, strSrcID);
	m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(strSrcID).c_str(), CLog::ANSIToUnicode(strSrcAlarmID).c_str(), nPriority, nMethod);
#else
	m_DeviceStaustAlarm(strSrcID, strSrcAlarmID, nPriority, nMethod);
#endif

	//char strRes[RES_MSG_SIZE];
	//CDataStream dataRes((BYTE*)strRes, sizeof(strRes));
	//nCmdID = MESSAGERES_CMD;
	//nStatus = 200;
	//dataRes.InputData(4, &nCmdID, 4);						//command id
	//dataRes.InputData(-1, &unSeq, 4);						//sequence
	//dataRes.InputData(-1, &nStatus, 4);						//status
	//dataRes.InputData(-1, "#", 1);							//finish symbol

	//m_pSocket->SendData(strRes, dataRes.GetLastPos());
	return 0;
}

int	CUACTCP::ParseAlarmData(char *pData, char *strSrcID,  char *strAlarmPriority, char *strAlarmTime, char *strAlarmMethod, char * strAlarmType)
{
	string strDeviceID;
	string strPriority;
	string strTime;
	string strMethod;
	string strType;
	string strBody = pData;

	strDeviceID = GetStringBetween(strBody, "<DeviceID>", "</DeviceID>", FALSE);
	if(ID_LEN != strDeviceID.length())
		strSrcID[0] = 0;
	else
		strncpy_s(strSrcID, ID_LEN+1, strDeviceID.c_str(), ID_LEN);

	strPriority = GetStringBetween(strBody, "<AlarmPriority>", "</AlarmPriority>", FALSE);
	if(PRIORITY_LEN != strPriority.length())
		strAlarmPriority[0] = 0;
	else
		strncpy_s(strAlarmPriority, PRIORITY_LEN+1, strPriority.c_str(), PRIORITY_LEN);

	strTime = GetStringBetween(strBody, "<AlarmTime>", "</AlarmTime>", FALSE);
	if(14 > strTime.length() || 19 < strTime.length())
		strAlarmTime[0] = 0;
	else
		strncpy_s(strAlarmTime, TIME_LEN+1, strTime.c_str(), TIME_LEN);

	strMethod = GetStringBetween(strBody, "<AlarmMethod>", "</AlarmMethod>", FALSE);
	if(1 > strMethod.length() || METHOD_LEN < strMethod.length())
		strAlarmMethod[0] = 0;
	else
		strcpy_s(strAlarmMethod, METHOD_LEN+1, strMethod.c_str());

	strType = GetStringBetween(strBody, "<AlarmType>", "</AlarmType>", FALSE);
	if(1 > strType.length())
		strAlarmType[0] = 0;
	else
		strcpy_s(strAlarmType, METHOD_LEN+1, strType.c_str());
	return 0;
}

int CUACTCP::GetMsgType(char *pMsgData)
{
	int nMsgType = 0;
	memcpy(&nMsgType, pMsgData+4, 4);
	return nMsgType;
}

int CUACTCP::GetMsgOperate(char *pMsgData)
{
	int nOperate = 0;
	memcpy(&nOperate, pMsgData+16, 4);
	return nOperate;
}

int CUACTCP::GetMsgStatus(char *pMsgData)
{
	int nStatus = 0;
	memcpy(&nStatus, m_pData->pDataBuf+12, 4);
	return nStatus;
}

int CUACTCP::SendCallAck(int nDID)
{
	int nMegSize = 4;
	int nDataSize = 0;
	int nCmdID = CALLACK_CMD;
	char szCallAckBuf[ACK_MSG_SIZE];
	CDataStream dataStream((BYTE*)szCallAckBuf, LOGIN_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nDID, 4);						//did
	dataStream.InputData(-1, "#", 1);						//finish symbol

	m_pSocket->SendData(szCallAckBuf, dataStream.GetLastPos());
	g_objLog.LogoutDebug(k_LOG_DLL, "%s Send ACK did:%d\n", __FUNCTION__, nDID);
	m_unSeq++;
	return 0;
}

int CUACTCP::HandleTimer()
{
//	static INT64 nTargetTime = 0;

	time_t timeCurrent;
	time(&timeCurrent);
	if(0 == nTargetTime)
	{
		nTargetTime = timeCurrent+UPDATE_STATUS_TIME;
		return 0;
	}

	if(nTargetTime < timeCurrent)
	{
		//更新状态
		SendLogin();
		nTargetTime = timeCurrent+UPDATE_STATUS_TIME;
	}
	return 0;
}

int CUACTCP::SendResetAlarm(const char *pstrDeviceID, const char *pstrChannelID)
{
	int nMegSize = 4;
	int nDataSize = 0;
	int nCmdID = RESETALARM_CMD;
	char szResetAlarmBuf[ACK_MSG_SIZE];
	CDataStream dataStream((BYTE*)szResetAlarmBuf, LOGIN_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);		//device id
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);		//channel id
	dataStream.InputData(-1, "#", 1);						//finish symbol

	m_pSocket->SendData(szResetAlarmBuf, dataStream.GetLastPos());
	g_objLog.LogoutDebug(k_LOG_DLL, "%s Send ResetAlarm ID:%s\n", __FUNCTION__, pstrChannelID);
	m_unSeq++;
	return 0;
}

int CUACTCP::SendDeviceConfig(const char *pstrDeviceID, deviceConfig_info_t* pDeviceConfig )
{
	int nDataSize = 0;
	int nCmdID = MESSAGE_CMD;
	int nOperate = DEVICE_CONFIG_TYPE;
	char* szDeviceConfig = new char[1024];
	CDataStream dataStream((BYTE*)szDeviceConfig, LOGIN_MSG_SIZE);

	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : set device config
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);		//device id
	dataStream.InputData(-1,&pDeviceConfig->cBasicParam, 1);
	int len = 0;
	if(pDeviceConfig->cBasicParam)
	{
		len = pDeviceConfig->lpBasicParam->strName.length()+1;
		dataStream.InputData(-1,&len, 4);
		dataStream.InputData(-1,pDeviceConfig->lpBasicParam->strName.c_str(),len);
		dataStream.InputData(-1, pDeviceConfig->lpBasicParam->strDeviceID, ID_LEN+1);		//device id
		dataStream.InputData(-1, pDeviceConfig->lpBasicParam->strSIPServerId, ID_LEN+1);
		dataStream.InputData(-1, pDeviceConfig->lpBasicParam->strSIPServerIp, IP_LEN+1);
		dataStream.InputData(-1, &pDeviceConfig->lpBasicParam->nSIPServerPort, 4);

		len = pDeviceConfig->lpBasicParam->strDomainName.length()+1;
		dataStream.InputData(-1,&len, 4);
		dataStream.InputData(-1, pDeviceConfig->lpBasicParam->strDomainName.c_str(), len);
		dataStream.InputData(-1, &pDeviceConfig->lpBasicParam->nExpiration, 4);

		len = pDeviceConfig->lpBasicParam->strPassword.length()+1;
		dataStream.InputData(-1,&len, 4);
		dataStream.InputData(-1, pDeviceConfig->lpBasicParam->strPassword.c_str(), len);

		dataStream.InputData(-1, &pDeviceConfig->lpBasicParam->nHeartBeatInternal, 4);
		dataStream.InputData(-1, &pDeviceConfig->lpBasicParam->nHeartBeatCount, 4);

		dataStream.InputData(-1, &pDeviceConfig->nVideoParamConfigNum, 4);
		if(pDeviceConfig->nVideoParamConfigNum > 0)
		{
			for(int i = 0; i < pDeviceConfig->nVideoParamConfigNum; i++)
			{
				len = pDeviceConfig->lpVideoParamAttribute[i].strStreamName.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strStreamName.c_str(), len);

				len = pDeviceConfig->lpVideoParamAttribute[i].strVideoFormat.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strVideoFormat.c_str(), len);

				len = pDeviceConfig->lpVideoParamAttribute[i].strResolution.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strResolution.c_str(), len);

				len = pDeviceConfig->lpVideoParamAttribute[i].strFrameRate.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strFrameRate.c_str(), len);

				len = pDeviceConfig->lpVideoParamAttribute[i].strBitRateType.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strBitRateType.c_str(), len);

				len = pDeviceConfig->lpVideoParamAttribute[i].strVideoBitRate.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpVideoParamAttribute[i].strVideoBitRate.c_str(), len);

			}
		}
		dataStream.InputData(-1, &(pDeviceConfig->nAudioParamConfigNum), 4);
		if (pDeviceConfig->nAudioParamConfigNum > 0)
		{
			for (int i = 0; i < pDeviceConfig->nAudioParamConfigNum; ++i)
			{
				len = pDeviceConfig->lpAudioParamAttribute[i].strStreamName.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpAudioParamAttribute[i].strStreamName.c_str(), len);

				len = pDeviceConfig->lpAudioParamAttribute[i].strAudioFormat.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpAudioParamAttribute[i].strAudioFormat.c_str(), len);

				len = pDeviceConfig->lpAudioParamAttribute[i].strAudioBitRate.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpAudioParamAttribute[i].strAudioBitRate.c_str(), len);

				len = pDeviceConfig->lpAudioParamAttribute[i].strSamplingRate.length()+1;
				dataStream.InputData(-1,&len, 4);
				dataStream.InputData(-1, pDeviceConfig->lpAudioParamAttribute[i].strSamplingRate.c_str(), len);
			}
		}
		dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.cFlag), 1);
		if (pDeviceConfig->SVACEncodeConfig.cFlag)
		{
			dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.cFlag), 1);
			if (pDeviceConfig->SVACEncodeConfig.ROIParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.nROIFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber), 4);
				if (pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber > 0 )
				{
					for (int i = 0; i < pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber; ++i)
					{
						dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROISeq), 4);
						dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nTopLeft), 4);
						dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nBottomRight), 4);
						dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROIQP), 4);
					}
				}
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.nBackGroundQP), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.ROIParam.nBackGroundSkipFlag), 4);
			}
			dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag), 1);
			if (pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCSTMMode), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCSpaceDomainMode), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCTimeDomainMode), 4);
			}
			dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SurveillanceParam.cFlag), 1);
			if (pDeviceConfig->SVACEncodeConfig.SurveillanceParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nTimeFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nEventFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nAlertFlag), 4);
			}
			dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.EncryptParam.cFlag), 1);
			if (pDeviceConfig->SVACEncodeConfig.EncryptParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.EncryptParam.nEncryptionFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.EncryptParam.nAuthenticationFlag), 4);
			}
			dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.AudioParam.cFlag), 1);
			if (pDeviceConfig->SVACEncodeConfig.AudioParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACEncodeConfig.AudioParam.nAudioRecognitionFlag), 4);
			}
		}
		dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.cFlag), 1);
		if (pDeviceConfig->SVACDecodeConfig.cFlag)
		{
			dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SVCParam.cFlag), 1);
			if (pDeviceConfig->SVACDecodeConfig.SVCParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SVCParam.nSVCSTMMode), 4);
			}
			dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SurveillanceParam.cFlag), 1);
			if (pDeviceConfig->SVACDecodeConfig.SurveillanceParam.cFlag)
			{
				dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nTimeShowFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nEventShowFlag), 4);
				dataStream.InputData(-1, &(pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nAlertShowFlag), 4);
			}
		}
	}
	dataStream.InputData(-1, "#", 1);						//finish symbol

	m_pSocket->SendData(szDeviceConfig, dataStream.GetLastPos());
	g_objLog.LogoutDebug(k_LOG_DLL, "%s Send DeviceConfig device:%s\n", __FUNCTION__, pstrDeviceID);
	delete [] szDeviceConfig;
	m_unSeq++;
	return 0;
}

lpDevicePresetQuery_info_t CUACTCP::SendDevicePersetQuery(const char *pstrDeviceID)
{
	char szDevicePersetQueryBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = PRESET_QUERY_TYPE;
	CDataStream dataStream((BYTE*)szDevicePersetQueryBuf, CTRL_MSG_SIZE);
	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);					//Operate : device perset query
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, "#", 1);						//finish symbol
	g_objLog.LogoutDebug(k_LOG_DLL, "%s Send Device Perset Query pstrDeviceID=%s\n", __FUNCTION__, 
		pstrDeviceID);
	m_pSocket->SendData(szDevicePersetQueryBuf, dataStream.GetLastPos());
	m_unSeq++;
	//需要解析
	//TODO
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDevicePersetQueryXml.size())
		{
			return ParseDevicePresetQuery(m_strDevicePersetQueryXml.c_str());
		}
		Sleep(100);
	}
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 未查找到设备目录\n", __FUNCTION__);
	return NULL;
}

lpDeviceConfig_info_t CUACTCP::SendConfigDownload( const char *pstrDeviceID,const char *strConfigType )
{
	if(!pstrDeviceID || !strConfigType)
		return NULL;
	char szDeviceConfigDownloadBuf[CTRL_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = CONFIG_DOWNLOAD_TYPE;
	int nConfigTypeLen = sizeof(strConfigType);
	CDataStream dataStream((BYTE*)szDeviceConfigDownloadBuf, CTRL_MSG_SIZE);
	dataStream.InputData(4, &nCmdID, 4);					//command id
	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
	dataStream.InputData(-1, &nOperate, 4);					//Operate : device perset query
	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
	dataStream.InputData(-1, &(nConfigTypeLen), 4);	
	dataStream.InputData(-1, strConfigType, nConfigTypeLen);

	dataStream.InputData(-1, "#", 1);						//finish symbol
	g_objLog.LogoutDebug(k_LOG_DLL, "%s Send Config Download pstrDeviceID=%s strConfigType=%s\n", __FUNCTION__, 
		pstrDeviceID, strConfigType);
	m_pSocket->SendData(szDeviceConfigDownloadBuf, dataStream.GetLastPos());
	m_unSeq++;
	//需要解析
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDeviceConfigDownloadXml.size())
		{
			return ParseDeviceConfigDownload(m_strDeviceConfigDownloadXml.c_str());
		}
		Sleep(100);
	}
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 未查找到设备目录\n", __FUNCTION__);


	return NULL;
}

const char * CUACTCP::SendRealPlayUrl( const char *pstrChannelID, const int nWndNum, const char * pstrDeviceId )
{
	char szRealPlayUrlBuf[56];
	int nCmdID = MESSAGE_CMD;
	int nOperate = REAL_PLAY_URL_TYPE;
	CDataStream dataStream((BYTE*)szRealPlayUrlBuf, 56);
	m_strRealPlayUrlXml = "";
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	
	dataStream.InputData(-1, &nWndNum, 4);
	dataStream.InputData(-1,pstrDeviceId, ID_LEN+1);	
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
		dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szRealPlayUrlBuf, dataStream.GetLastPos());

	m_unSeq++;
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strRealPlayUrlXml.size())
		{
			char url[MAX_PATH] = {0};
			ParseRealPlayUrl(m_strRealPlayUrlXml.c_str(),url);
			m_strRealPlayUrlXml = url;
			return m_strRealPlayUrlXml.c_str();
		}
		Sleep(100);
	}

	return NULL;
}

const char * CUACTCP::SendPlayBackUrl(const char *pstrDeviceID, const char *pstrChannelID, const char* strSTime, const char* strETime, const char* pstrLocation)//A.11
{
	g_objLog.LogoutDebug(k_LOG_DLL, "%s DID:%s\n", __FUNCTION__, pstrDeviceID);
	char strSearchBuf[SEARCH_MSG_SIZE];
	int nCmdID = MESSAGE_CMD;
	int nOperate = PLAY_BACK_URL_TYPE;
	m_strPlayBackUrlXml = "";
	m_nRecordSum = 0;

	if(19 < strlen(strSTime) || 19 < strlen(strETime))
		return NULL;

	CDataStream dataStream((BYTE*)strSearchBuf, sizeof(strSearchBuf));

	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	//device id
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	//device id
	dataStream.InputData(-1, strSTime, TIME_LEN+1);	
	dataStream.InputData(-1, strETime, TIME_LEN+1);	
	int locationLen = strlen(pstrLocation)+1;
	dataStream.InputData(-1,&locationLen,4);
	dataStream.InputData(-1, pstrLocation, locationLen);	
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__,
		dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(strSearchBuf, dataStream.GetLastPos());
	m_unSeq++;

	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strPlayBackUrlXml.size())
		{
			char url[MAX_PATH] = {0};
			ParseRealPlayUrl(m_strPlayBackUrlXml.c_str(),url);
			m_strPlayBackUrlXml = url;
			return m_strPlayBackUrlXml.c_str();
		}
		Sleep(100);
	}

	
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 请求超时\n", __FUNCTION__);
	return NULL;
}

const char * CUACTCP::SendDecoderStatus( const char *pstrDeviceID, const char *pstrChannelID )
{
	char szDecoderStatusBuf[56];
	int nCmdID = MESSAGE_CMD;
	int nOperate = DECODER_STATUS_TYPE;
	CDataStream dataStream((BYTE*)szDecoderStatusBuf, 56);
	m_strDecoderStatusXml = "";
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
		dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szDecoderStatusBuf, dataStream.GetLastPos());

	m_unSeq++;
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDecoderStatusXml.size())
		{
			char deviceId[32] = {0};
			ParseDecoderStatusVideoDeviceID(m_strDecoderStatusXml.c_str(),deviceId);
			m_strDecoderStatusXml = deviceId;
			return m_strDecoderStatusXml.c_str();
		}
		Sleep(100);
	}

	return NULL;
}

int CUACTCP::SendAlarmSubScribe( const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime )
{
	char szDecoderStatusBuf[256];
	int nCmdID = SUBSCRIBE_CMD;
	int nOperate = SUBSCRIBE_ALARM_TYPE;
	CDataStream dataStream((BYTE*)szDecoderStatusBuf, 256);
	m_strDecoderStatusXml = "";
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	
	dataStream.InputData(-1, &nExpires, 4);
	dataStream.InputData(-1, &nStartAlarmPriority, 4);
	dataStream.InputData(-1, &nEndAlarmPriority, 4);
	dataStream.InputData(-1, &nAlarmMethod, 4);
	dataStream.InputData(-1, pstrSTime, TIME_LEN+1);
	dataStream.InputData(-1, pstrETime, TIME_LEN+1);
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
		dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szDecoderStatusBuf, dataStream.GetLastPos());

	m_unSeq++;

	return NULL;
}

const char * CUACTCP::SendAlarmQuery( const char *pstrDeviceID, const char *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime,const char* pstrAlarmType )
{
	char szDecoderStatusBuf[256];
	int nCmdID = SUBSCRIBE_CMD;
	int nOperate = ALARM_QUERY_TYPE;
	CDataStream dataStream((BYTE*)szDecoderStatusBuf, 256);
	m_strAlarmQueryXml = "";
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);	
	dataStream.InputData(-1, &nStartAlarmPriority, 4);
	dataStream.InputData(-1, &nEndAlarmPriority, 4);
	dataStream.InputData(-1, &nAlarmMethod, 4);
	dataStream.InputData(-1, pstrSTime, TIME_LEN+1);
	dataStream.InputData(-1, pstrETime, TIME_LEN+1);
	int alarmTypeLen = strlen(pstrAlarmType) + 1;
	dataStream.InputData(-1, &alarmTypeLen, 4);
	if(alarmTypeLen)
		dataStream.InputData(-1,pstrAlarmType, alarmTypeLen);
	dataStream.InputData(-1, "#", 1);					//finish symbol

	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d, Operate=%d\n", __FUNCTION__, 
		dataStream.GetLastPos(), nCmdID, m_unSeq, nOperate);
	m_pSocket->SendData(szDecoderStatusBuf, dataStream.GetLastPos());

	m_unSeq++;
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strAlarmQueryXml.size())
			return m_strAlarmQueryXml.c_str();
		Sleep(100);
	}
	g_objLog.LogoutDebug(k_LOG_DLL, "%s 请求超时\n", __FUNCTION__);

	return "";
}

int CUACTCP::SendLogout()
{
	char szSipClientLogout[128];
	int nCmdID = PRIVATE_CMD;
	int nOperate = SIP_CLIENT_LOGOUT_TYPE;
	CDataStream dataStream((BYTE*)szSipClientLogout, 128);
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, m_strSipClientID.c_str(), ID_LEN+1);
	dataStream.InputData(-1, "#", 1);					//finish symbol
	m_pSocket->SendData(szSipClientLogout, dataStream.GetLastPos());
	return NULL;
}

int CUACTCP::SendChangePassword( const char* pstrOldPassword, const char* pstrNewPassword )
{
	char szSipChangePassword[256];
	int nCmdID = PRIVATE_CMD;
	int nOperate = SIP_CLIENT_CHANGEPASSWORD_TYPE;
	CDataStream dataStream((BYTE*)szSipChangePassword, 256);
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, m_strSipClientID.c_str(), ID_LEN+1);
	dataStream.InputData(-1, pstrOldPassword, PASS_LEN+1);
	dataStream.InputData(-1, pstrNewPassword, PASS_LEN+1);
	dataStream.InputData(-1, "#", 1);					//finish symbol
	m_pSocket->SendData(szSipChangePassword, dataStream.GetLastPos());
	return NULL;
}

int CUACTCP::OnNotify()
{
	if(m_pData)
	{
		int nOperate = GetMsgOperate(m_pData->pDataBuf);
		switch(nOperate)
		{
		case CATALOG_SUBSCRIBE_TYPE:
			OnCatalogNotify();
			break;
		default:
			break;
		}

		g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到Notify消息 nOperate:%d\n", __FUNCTION__, nOperate);

		return 0;
	}
	return DATA_EMPTY_ERROR;
}

int CUACTCP::OnCatalogNotify()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nOperate = 0;
	int nSize = 0;
	char strExterndData[EXTERND_DATA_SIZE];

	/*
	dataStream.InputData(4, &nCmdID, 4);									//command id
	dataStream.InputData(-1, &pNotifyData->unSeq, 4);						//sequence
	dataStream.InputData(-1, &pNotifyData->nMsgStatus, 4);
	dataStream.InputData(-1, &nOperate, 4);									//sequence
	dataStream.InputData(-1, &nExterndSize, 4);								
	dataStream.InputData(-1, pNotifyData->pExtendData, nExterndSize);
	*/
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);					
	dataStream.OutputData(-1, &nSize, 4);
	if(0 < nSize)
		dataStream.OutputData(-1, strExterndData, nSize);

	g_pCatalog = strExterndData;
//	m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(pstrDeviceID).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);

	int						nOffset;
	int						nCatalogCnt = 0;
	const char*				pCurPos = g_pCatalog;
	TCHAR					strSumNum[MAX_PATH];

	nOffset = externalInterface.ExtractXML(pCurPos, "Catalog", "<SumNum>", "</SumNum>", strSumNum, MAX_PATH);
	pCurPos += nOffset;
	nCatalogCnt = _ttoi(strSumNum);

	int	nCurItemIdx = 0;

	for(int nCurCnt = 0; nCurCnt < nCatalogCnt; nCurCnt++)
	{
		TCHAR		strDeviceID[MAX_PATH];
		TCHAR		strEvent[MAX_PATH];
		externalInterface.ExtractXML(pCurPos, "<Item>", "<DeviceID>", "</DeviceID>", strDeviceID, MAX_PATH);
		int iLen = WideCharToMultiByte(CP_ACP, 0,strDeviceID, -1, NULL, 0, NULL, NULL);
		char* chRtn =new char[iLen*sizeof(char)];
		WideCharToMultiByte(CP_ACP, 0, strDeviceID, -1, chRtn, iLen, NULL, NULL);
		std::string deviceID(chRtn);
		if (deviceID=="")
		{
			delete[] chRtn;
			break;
		}
		nOffset =externalInterface.ExtractXML(pCurPos, "<Item>", "<Event>", "</Event>", strEvent, MAX_PATH);
		pCurPos += nOffset;
	
		

		int iLenEvent = WideCharToMultiByte(CP_ACP, 0,strEvent, -1, NULL, 0, NULL, NULL);
		char* chRtnEvent =new char[iLenEvent*sizeof(char)];
		WideCharToMultiByte(CP_ACP, 0, strEvent, -1, chRtnEvent, iLenEvent, NULL, NULL);
		std::string stringEvent(chRtnEvent);

		g_objLog.LogoutInfo(k_LOG_DLL, "%s 设备ID:%s,状态:%s\n", __FUNCTION__, deviceID,stringEvent);
		transform(stringEvent.begin(),stringEvent.end(),stringEvent.begin(),tolower);
		if (stringEvent=="off")
		{
			m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(deviceID.c_str()).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);
		}
		else
		{
			m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(deviceID.c_str()).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_ONLINE);
		
		}
		delete[] chRtnEvent;
		delete[] chRtn;
	}
	//m_externalInterface->ShowResult(CATALOG_INQUIRE_TYPE);
	return 0;
}

int CUACTCP::OnAlarmQueryRes()
{
	int nCmdID = 0;
	UINT unSeq = 0;
	int nStatus = 0;
	int nExterndSize = 0;
	int nOperate = 0;
	char strExterndData[EXTERND_DATA_SIZE];
	CDataStream dataStream((BYTE*)m_pData->pDataBuf);
	dataStream.OutputData(4, &nCmdID, 4);						//command id
	dataStream.OutputData(-1, &unSeq, 4);						//sequence
	dataStream.OutputData(-1, &nStatus, 4);						//status
	dataStream.OutputData(-1, &nOperate, 4);									//sequence
	dataStream.OutputData(-1, &nExterndSize, 4);				//exdentSize
	dataStream.OutputData(-1, strExterndData, nExterndSize);	//exdentSize

	m_strAlarmQueryXml += strExterndData;

	g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到AlarmQuery respons消息 nOperate:%d, nExterndSize:%d \nstrExterndData %s\n", __FUNCTION__, nOperate, nExterndSize, m_strRealPlayUrlXml.c_str());
	return 0;
}

int CUACTCP::UnInit()
{
	ExitThread();
	//if(m_pSocket)
	//{
	//	delete m_pSocket;
	//	m_pSocket = NULL;
	//}
	return 0;
}

std::string CUACTCP::GetLocalIP()
{
	if(m_pSocket)
	{
		return m_pSocket->GetLocalIP();
	}
	return "";
}
int CUACTCP::SendStopPlayUrl(const char *pstrChannelID, const int nWndNum)
{
	char szStopPlayBuf[128];
	int nCmdID = MESSAGE_CMD;
	int nOperate = STOP_PLAY_URL_TYPE;
	CDataStream dataStream((BYTE*)szStopPlayBuf, 128);
	m_strDecoderStatusXml = "";
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);
	dataStream.InputData(-1, &nWndNum, 4);
	dataStream.InputData(-1, "#", 1);					//finish symbol
	m_pSocket->SendData(szStopPlayBuf, dataStream.GetLastPos());
	return 0;
}

int CUACTCP::SendDecoderDivision( const char *pstrDeviceID, const char *pstrChannelID, int nDivision )
{
	char szDecoderDivisionBuf[128];
	int nCmdID = MESSAGE_CMD;
	int nOperate = DECODER_DIVISION_TYPE;
	CDataStream dataStream((BYTE*)szDecoderDivisionBuf, 128);
	dataStream.InputData(4, &nCmdID, 4);				//command id
	dataStream.InputData(-1, &m_unSeq, 4);				//sequence
	dataStream.InputData(-1, &nOperate, 4);				//Operate : device control, search file 
	dataStream.InputData(-1, pstrDeviceID, ID_LEN+1);	
	dataStream.InputData(-1, pstrChannelID, ID_LEN+1);
	dataStream.InputData(-1, &nDivision, 4);
	dataStream.InputData(-1, "#", 1);					//finish symbol
	m_pSocket->SendData(szDecoderDivisionBuf, dataStream.GetLastPos());
	return 0;
}


