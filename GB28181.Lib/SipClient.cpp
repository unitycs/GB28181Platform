#include "StdAfx.h"
#include "SipClient.h"

#ifdef USESIPCLIENT
#include "RtspParser.h"
#include "MediaStreamer.h"
#include <atlcomtime.h>
/*
UINT WINAPI SipClientProc(LPVOID lpParameter)
{
    CSipClient *pSipClient = (CSipClient *)lpParameter;
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 等待消息……\n", __FUNCTION__);

    while(!pSipClient->IsExit())
    {
        int nMsgType = pSipClient->GetMsg();
        switch(nMsgType)
        {
        case LOGIN_CMD:
            pSipClient->OnRegister();
            break;
        case LOGINRES_CMD:
            pSipClient->OnRegisterRes();
            break;
        case LOGOUT_CMD:
            pSipClient->OnLogout();
            break;
        case INVITERES_CMD:
            pSipClient->OnInviteRes();
            break;
        case MESSAGE_CMD:
            pSipClient->OnMessage();
            break;
        case MESSAGERES_CMD:
            pSipClient->OnMessageRes();
        default:
            pSipClient->OnOther();
            break;
        }
    }

    return 0;
}*/

CSipClient::CSipClient(void)
{
    m_uas.SetUASGB28181CallBack(this);
}


CSipClient::~CSipClient(void)
{
}

int CSipClient::Init( const char *pstrSipId, const char *pstrSipIp, const char * pstrSipPort, const char * pstrPassword )
{
    m_uas.Init();
    return 0;
}

int CSipClient::SendInvite( int nOperate, int nProType, const char *pstrDeviceID, const char *pstrChannelID, const char *pstrRecvIP, int nRecvPort[2], void *pStreamer, ClientRealData fpClientHandleRealData, void **ppRTPTrans, char *ptmStart, char *ptmEnd )
{
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 开始媒体流协商\n", __FUNCTION__);
    lpInvite_info_t pInvietData  = new invite_info_t();
    pInvietData->unSeq = m_unSeq;
    pInvietData->nOperate = nOperate;
    strcpy_s(pInvietData->strDstID, pstrDeviceID);
    strcpy_s(pInvietData->strChannelID, pstrChannelID);
    pInvietData->nTransType = nProType;
    strcpy_s(pInvietData->strSrcIP, pstrRecvIP);
    pInvietData->nSrcPort[0] = nRecvPort[0];
    pInvietData->nSrcPort[1] = nRecvPort[1];
    strcpy_s(pInvietData->strStartTime, ptmStart);
    strcpy_s(pInvietData->strEndTime, ptmEnd);
    pInvietData->socUser = NULL;
    pInvietData->nCmdID = INVITE_CMD;
    m_uas.SendOppositeData(pInvietData);
    //delete pInvietData;
    //////////////////////////////////////////////////////////////////////////

    //添加到消息队列，等待前端返回相应
    lpInviteParam_t lMsg = m_objInviteList.NewWaitMsg();
    lMsg->nCmdID = INVITERES_CMD;
    lMsg->unSeq = m_unSeq;
    strcpy_s(lMsg->strDstID, pstrDeviceID);
    lMsg->pStreamer = pStreamer;
    lMsg->fpClientHandleRealData = fpClientHandleRealData;
    lMsg->nRecvPort[0] = nRecvPort[0];
    lMsg->nRecvPort[1] = nRecvPort[2]; 
    lMsg->ppRTPTrans = ppRTPTrans;
    lMsg->nOperate = nOperate;

    m_unSeq++;
    return 0;
}

int CSipClient::OnUASGB28181Data( lpMsgParam_t pMsgParam )
{
    switch(pMsgParam->nCmdID)
    {
    case INVITERES_CMD:
        OnInviteRes(pMsgParam);
        break;
    case NOTIFY_CMD:
        {
            switch(pMsgParam->nOperate)
            {
            case CATALOG_SUBSCRIBE_TYPE:
                OnCatalogSubscribeRes(pMsgParam);
                break;
            case SUBSCRIBE_ALARM_TYPE:
                OnAlarmSubscribeRes(pMsgParam);
                break;
            }
        }
        break;
    }
    switch(pMsgParam->nOperate)
    {
    case RECORD_INQUIRE_TYPE:
        OnRecordInfoInquiryRes(pMsgParam);
        break;
    case CATALOG_INQUIRE_TYPE:
        OnDeviceCatalogInquiryRes(pMsgParam);
        break;
    case PLAY_BACK_URL_TYPE:
        OnPlayBackUrlRes(pMsgParam);
        break;
    case STATUS_INQUIRE_TYPE:
        OnDeviceStatusInquiryRes(pMsgParam);
        break;
    case INFO_INQUIRE_TYPE:
        OnDeviceInfoInquiryRes(pMsgParam);
        break;
    }
    
    return 0;
}

int CSipClient::OnInviteRes( lpMsgParam_t m_pMsgParam )
{
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到invite respons消息 status %d, cid:%d, did:%d\n", __FUNCTION__, m_pMsgParam->nMsgStatus, m_pMsgParam->nCid, m_pMsgParam->nDid);

#ifdef UNICODE 
    _tstring strSdp = CLog::ANSIToUnicode(m_pMsgParam->pExtendData);
#else
    _tstring strSdp = m_pMsgParam->pExtendData;
#endif
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
    lpInviteParam_t pWaitMsg = m_objInviteList.GetWaitMsg(m_pMsgParam->nCmdID, m_pMsgParam->unSeq);
    if(pWaitMsg)
    {
        if(200 == m_pMsgParam->nMsgStatus)
        {
            g_objLog.LogoutDebug(k_LOG_DLL, "%s 创建媒体流,远程端口:%d\n", __FUNCTION__, nPort);
            if(0 > CreateMediaStreamer(0, pWaitMsg->nOperate, stSpdStack.connect_info.address, nPort, pWaitMsg))
            {
                g_objLog.LogoutDebug(k_LOG_DLL, "%s 创建媒体流失败 connect err pWaitMsg->nOperate = %d addr = %s port = %d\n", __FUNCTION__,pWaitMsg->nOperate, stSpdStack.connect_info.address.c_str(),nPort);
                return CREATE_MEDIASTREAMER_ERROR;
            }

            SetPlayBackID(pWaitMsg->ppRTPTrans, m_pMsgParam->nCid, m_pMsgParam->nDid);
        }
        //delete pWaitMsg;
    }
    SendCallAck(m_pMsgParam->nDid);
    return 0;
}

int CSipClient::SetPlayBackID( void **ppRTPTrans, int nCid, int nDid )
{
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        pRTP->SetPlayBackID(nCid, nDid);
        g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
    }

    return 0;
}

int CSipClient::GetPlayBackID( void **ppRTPTrans, int &nCid, int &nDid )
{
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        pRTP->GetPlayBackID(nCid, nDid);
        g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
    }
    return 0;
}

int CSipClient::SendCallAck( int nDID )
{
    m_uas.SendCallAck(nDID);
    g_objLog.LogoutDebug(k_LOG_DLL, "%s Send ACK did:%d\n", __FUNCTION__, nDID);
    m_unSeq++;
    return 0;
}

int CSipClient::SendBye( const char *pstrDeviceID, const char *pstrChannelID, void **ppRTPTrans, int nOption )
{
    int nMegSize = 4;
    //char szCatalogBuf[CTRL_MSG_SIZE];
    int nCmdID = BYE_CMD;
    int nCid = 0;
    int nDid = 0;
    GetPlayBackID(ppRTPTrans, nCid, nDid);


    //g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
    //if(-1 != nCid && -1 != nDid)
    //{
    //	CDataStream dataStream((BYTE*)szCatalogBuf, sizeof(szCatalogBuf));

    //	dataStream.InputData(4, &nCmdID, 4);					//command id
    //	dataStream.InputData(-1, &m_unSeq, 4);					//sequence
    //
    //	//dataStream.InputData(-1, &nStreamType, 4);				//Operate  :play, playback, download
    //	dataStream.InputData(-1, pstrDeviceID, ID_BUFFER_SIZE);	//Remove Device ID
    //	dataStream.InputData(-1, pstrChannelID, ID_BUFFER_SIZE);//Remove Device ID
    //	dataStream.InputData(-1, &nCid, 4);						//Call ID
    //	dataStream.InputData(-1, &nDid, 4);						//Call ID
    //	dataStream.InputData(-1, "#", 1);						//finish symbol


    //	g_objLog.LogoutDebug(k_LOG_DLL, "%s Size=%d, CmdID=%d, Seq=%d\n", __FUNCTION__, 
    //								nMegSize, nCmdID, m_unSeq);
    //	m_pSocket->SendData(szCatalogBuf, dataStream.GetLastPos());
    //}
    if(-1 != nCid && -1 != nDid)
    {
        SendBye(pstrDeviceID, pstrChannelID, nCid, nDid, nOption);
        SetPlayBackID(ppRTPTrans, -1, -1);
    }

    m_unSeq++;
    return StopStreamer(ppRTPTrans, nOption);
}

int CSipClient::SendBye( const char *pstrDeviceID, const char *pstrChannelID, int nCid, int nDid, int nOption )
{
    int nMegSize = 4;
    char szCatalogBuf[CTRL_MSG_SIZE];
    int nCmdID = BYE_CMD;

    g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
    if(-1 != nCid && -1 != nDid)
    {
        lpBye_info_t pByeInfo  = new bye_info_t();
        pByeInfo->unSeq = m_unSeq;
        strcpy_s(pByeInfo->strDstID, pstrDeviceID);
        strcpy_s(pByeInfo->strDstSubID, pstrChannelID);
        pByeInfo->nCid = nCid;
        pByeInfo->nDid = nDid;
        pByeInfo->socUser = NULL;
        pByeInfo->nCmdID = BYE_CMD;
        m_uas.SendOppositeData(pByeInfo);
        m_unSeq++;
    }else
        return -1;


    return 0;
}

int CSipClient::StopStreamer( void **ppRTPTrans, int nOption )
{
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 停止流接收\n", __FUNCTION__);
    if(*ppRTPTrans)
    {

        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);

        //文件下载时不马上关闭
        if(1000 == nOption && DOWNLOAD == pRTP->GetStreamerType())
        {
            g_objLog.LogoutDebug(k_LOG_DLL, "%s 不删除MediaStreamer\n", __FUNCTION__);
            return 1;
        }
        g_objLog.LogoutDebug(k_LOG_DLL, "%s 删除MediaStreamer\n", __FUNCTION__);
        delete pRTP;
        (*ppRTPTrans) = NULL;
    }
    return 0;
}

int CSipClient::ProcessAudio( void * pRTPTrans, BYTE * pBuffer, int nBufSize )
{
    if(pRTPTrans)
    {
        CMediaStreamer * pMediaStreamer = (CMediaStreamer *)pRTPTrans;
        pMediaStreamer->ProcessAudio(pBuffer, nBufSize);
        return 0;
    }
    return 0;
}

int CSipClient::CreateMediaStreamer( int nProtocolType, int nStreamerType, _tstring strRemoteIP, int nRemotePort, lpInviteParam_t pInviteInfo )
{
    CMediaStreamer * pMediaStreamer = new CMediaStreamer();
    return  pMediaStreamer->Init(nProtocolType, nStreamerType, strRemoteIP, nRemotePort, pInviteInfo);
}

const char * CSipClient::SendDeviceCatalogInquiry( const char *pstrDeviceID )
{
    m_strCatalogInfoListXml = "";
    
    //////////////////////////////////////////////////////////////////////////
    lpCatalogInquire_info_t pCatalogData  = new catalogInquire_info_t();
    pCatalogData->unSeq = m_unSeq;
    pCatalogData->nOperate = CATALOG_INQUIRE_TYPE;
    strcpy_s(pCatalogData->strDstID, pstrDeviceID);

    pCatalogData->socUser = NULL;
    pCatalogData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pCatalogData);
    //////////////////////////////////////////////////////////////////////////
    m_unSeq++;

    //等待5秒
    for(int i = 0; i < 50; i++)
    {
        if(0 < m_strCatalogInfoListXml.size())
        {
            return m_strCatalogInfoListXml.c_str();
        }
        Sleep(100);
    }
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 未查找到设备目录\n", __FUNCTION__);
    return 0;
}

const char * CSipClient::SendDeviceInfoInquiry( const char *pstrDeviceID )
{
    m_strDeviceInfoListXml = "";
    lpInfoInquire_info_t pInquireData  = new infoInquire_info_t();
    pInquireData->unSeq = m_unSeq;
    pInquireData->nOperate = INFO_INQUIRE_TYPE;
    strcpy_s(pInquireData->strDstID, pstrDeviceID);
    pInquireData->socUser = NULL;
    pInquireData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pInquireData);
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
    return 0;
}

int CSipClient::SendAnswered( int nStatus )
{
    return 0;
}

int CSipClient::SendDeviceControl( const char *pstrDeviceID, const char *pstrChannelID, BYTE bCmdType
    , BYTE cData1, BYTE cData2, BYTE cData3, BYTE cZoom /*= DRAG_ZOOM_NONE*/
    , int nLength /*= 0*/, int nWidth /*= 0*/, int nMidPointX /*= 0*/, int nMidPointY /*= 0*/, int nLengthX /*= 0*/, int nLengthY /*= 0*/ )
{
    lpPTZCtrl_info_t pCtrlData  = new ptzCtrl_info_t();
    pCtrlData->unSeq = m_unSeq;                                    //sequence
    pCtrlData->nOperate = DEVICE_CTRL_TYPE;                         //Operate
    pCtrlData->cCmdType = bCmdType;                                 //Command type
    strcpy_s(pCtrlData->strDstID, pstrDeviceID);		//Remove Device ID
    strcpy_s(pCtrlData->strDstSubID, pstrChannelID);	//Remove Device ID
    pCtrlData->cData1 = cData1;				//cData1
    pCtrlData->cData2= cData2;				//VerticalPace
    pCtrlData->cData3= cData3;				//ZoomPace
    //Add by Rongqian Soong 20150723 -S
    pCtrlData->cZoom = cZoom;				//Zoom
    pCtrlData->nLength = nLength;			//Length
    pCtrlData->nWidth = nWidth;				//Width
    pCtrlData->nMidPointX = nMidPointX;			//MidPointX
    pCtrlData->nMidPointY = nMidPointY;			//MidPointY
    pCtrlData->nLengthX = nLengthX;			//LengthX
    pCtrlData->nLengthY = nLengthY;			//LengthY
    //Add by Rongqian Soong 20150723 -E		
    pCtrlData->socUser = NULL;
    pCtrlData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pCtrlData);
    m_unSeq++;
    return 0;
}

int CSipClient::SendRemoteStartup( const char *pstrDeviceID )
{
    lpRemotStartup_info_t pRemoteStartupData  = new remoteStartup_info_t();
    pRemoteStartupData->unSeq = m_unSeq;				//sequence
    pRemoteStartupData->nOperate = REMOTE_STARTUP_TYPE;			//Operate
    strcpy_s(pRemoteStartupData->strDstID, pstrDeviceID);	//Remove Device ID

    pRemoteStartupData->socUser = NULL;
    pRemoteStartupData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pRemoteStartupData);
    m_fpDeviceStaustAlarm(CLog::ANSIToUnicode(pstrDeviceID).c_str(), _T(""), AT_DEVICE_STATUS, AC_DEVICE_STATUS_OFFLINE);
    m_unSeq++;
    return 0;
}

const char * CSipClient::SendDeviceStatusInquiry( const char *pstrDeviceID )
{
    m_strDeviceStatusListXml = "";
    lpStatusInquire_info_t pInquireData  = new statusInquire_info_t();
    pInquireData->unSeq = m_unSeq;			//sequence
    pInquireData->nOperate = STATUS_INQUIRE_TYPE;		//Operate
    strcpy_s(pInquireData->strDstID, pstrDeviceID);	//Remove Device ID

    pInquireData->socUser = NULL;
    pInquireData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pInquireData);
    m_unSeq++;
    //等待5秒
    for(int i = 0; i < 50; i++)
    {
        if(0 < m_strDeviceStatusListXml.size())
            return m_strDeviceStatusListXml.c_str();
        Sleep(100);
    }
    return 0;
}

int CSipClient::SendEventSubscribe( const char *pstrDeviceID, int nStratPriority, int nEndPriority, int nMethod, char* pstrStartTime, char* pstrEndTime )
{
    lpEventSubscribe_info_t pInquireData  = new eventSubscribe_info_t();
    pInquireData->unSeq = m_unSeq;					//sequence
    pInquireData->nOperate = SEND_EVENT_SUBSCRIBE_TYPE;				//Operate	
    pInquireData->nStartPriority = nStratPriority;			//StratPriority
    pInquireData->nEndPriority = nEndPriority;			//EndPriority
    pInquireData->nMethod = nMethod;					//Method
    strcpy_s(pInquireData->strStartTime, pstrStartTime);	//StartTime	
    strcpy_s(pInquireData->strEndTime, pstrEndTime);		//EndTime

    //memcpy_s(&pInquireData->socUser, sizeof(pInquireData->socUser), &(pData[nPos+1] = ;
    pInquireData->socUser = NULL;
    pInquireData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pInquireData);
    m_unSeq++;
    return 0;
}

int CSipClient::SendCatalogSubscribe( const char *pstrDeviceID, const char *pstrChannelID, int nExpires, char* pstrStartTime, char* pstrEndTime )
{
    lpCatalogSubscribe_info_t pCatalogData  = new catalogSubscribe_info_t();
    pCatalogData->unSeq = m_unSeq;
    pCatalogData->nOperate = CATALOG_SUBSCRIBE_TYPE;
    strcpy_s(pCatalogData->strDstID, pstrDeviceID);	//Remote Device ID
    strcpy_s(pCatalogData->strDstSubID, pstrChannelID);	//Device ID
    strcpy_s(pCatalogData->strStartTime, pstrStartTime);
    strcpy_s(pCatalogData->strEndTime, pstrEndTime);
    pCatalogData->nExpires = nExpires;
    pCatalogData->socUser = NULL;
    pCatalogData->nCmdID = SUBSCRIBE_CMD;
    m_uas.SendOppositeData(pCatalogData);
    m_unSeq++;
    return 0;
}

int CSipClient::SendRecord( const char *pstrDeviceID, const char *pstrChannelID, int nActive )
{
    lpSetRecord_info_t pStartRecordData  = new setRecord_info_t();
    pStartRecordData->unSeq = m_unSeq;			//sequence
    pStartRecordData->nOperate = SET_RECORD_TYPE;		//Operate
    pStartRecordData->nActive = nActive;			//0 start , 1 stop
    strcpy_s(pStartRecordData->strDstID, pstrDeviceID);	//device id
    strcpy_s(pStartRecordData->strDstSubID, pstrChannelID);	//device id
    pStartRecordData->socUser = NULL;
    pStartRecordData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pStartRecordData);
    m_unSeq++;
    return 0;
}

int CSipClient::SendGuard( const char *pstrDeviceID, const char *pstrChannelID, int nActive )
{
    lpSetGuard_info_t pStartGuardData  = new setGuard_info_t();
    pStartGuardData->unSeq = m_unSeq;				//sequence
    pStartGuardData->nOperate = SET_GUARD_TYPE;			//Operate
    pStartGuardData->nActive = nActive;			//0 start , 1 stop
    strcpy_s(pStartGuardData->strDstID, pstrDeviceID);		//device id
    strcpy_s(pStartGuardData->strDstSubID, pstrChannelID);	//device id
    pStartGuardData->socUser = NULL;
    pStartGuardData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pStartGuardData);
    m_unSeq++;
    return 0;
}

int CSipClient::SendPlayBackCtrl( const char *pstrDeviceID, const char *pstrChannelID, void **ppRTPTrans, int nOperate, float fValue )
{
    int nMegSize = 4;
    char szCatalogBuf[CTRL_MSG_SIZE];
    int nCmdID = INFO_CMD;
    int nCid = 0;
    int nDid = 0;
    float fSpeed = 1.0;
    GetPlayBackID(ppRTPTrans, nCid, nDid);
    if(SPEED_TYPE == nOperate)
        SetSpeed(ppRTPTrans, fValue);
    else
        fSpeed = GetSpeed(ppRTPTrans);

    g_objLog.LogoutDebug(k_LOG_DLL, "%s cid:%d, did:%d\n", __FUNCTION__, nCid, nDid);
    lpPlayBackCtrl_info_t pPlayBackCtrlData  = new playBackCtrl_info_t();
    pPlayBackCtrlData->unSeq = m_unSeq;				//sequence
    pPlayBackCtrlData->nOperate = INFO_CMD;			//Operate
    strcpy_s(pPlayBackCtrlData->strDstID, pstrDeviceID);	//Remove Device ID
    pPlayBackCtrlData->nDid = nDid;				
    pPlayBackCtrlData->fValue = fValue;
    pPlayBackCtrlData->fValue2 = fSpeed;			
    pPlayBackCtrlData->socUser = NULL;
    pPlayBackCtrlData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pPlayBackCtrlData);
    if(PAUSE_TYPE == nOperate)
        SetPlayStatus(ppRTPTrans, nOperate);
    else
        SetPlayStatus(ppRTPTrans, nOperate);
    m_unSeq++;
    return 0;
}

int CSipClient::SetSpeed( void **ppRTPTrans, float fSpeed )
{
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        pRTP->SetSpeed(fSpeed);
        g_objLog.LogoutDebug(k_LOG_DLL, "%s Set Speed:%f\n", __FUNCTION__, fSpeed);
    }

    return 0;
}

int CSipClient::GetPlayStatus( void **ppRTPTrans )
{
    int nPlayStatus  = 0;
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        nPlayStatus = pRTP->GetPlayStatus();
        g_objLog.LogoutDebug(k_LOG_DLL, "%s Get nPlayStatus:%d\n", __FUNCTION__, nPlayStatus);
    }

    return nPlayStatus;
}

int CSipClient::SetPlayStatus( void **ppRTPTrans, int nPlayStatus )
{
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        pRTP->SetPlayStatus(nPlayStatus);
        g_objLog.LogoutDebug(k_LOG_DLL, "%s Set PlayStatus:%d\n", __FUNCTION__, nPlayStatus);
    }

    return 0;
}

float CSipClient::GetSpeed( void **ppRTPTrans )
{
    float fSpeed  = 0.0;
    if(*ppRTPTrans)
    {
        CMediaStreamer* pRTP = static_cast<CMediaStreamer *>(*ppRTPTrans);
        fSpeed = pRTP->GetSpeed();
        g_objLog.LogoutDebug(k_LOG_DLL, "%s Get Speed:%f\n", __FUNCTION__, fSpeed);
    }

    return fSpeed;
}

const char* CSipClient::SendRecordInfoInquiry( const char *pstrDeviceID, const char *pstrChannelID, int nType
    ,const char* strSTime, const char* strETime, const char* pstrFilePath /*= NULL*/, const char* pstrAddress /*= NULL*/
    , BYTE bSecrecy /*= 0*/, const char* pstrRecorderID /*= NULL*/, BYTE IndistinctQuery /*= 0*/ )
{
    //return SendPlayBackUrl(pstrDeviceID, pstrChannelID, pstrSTime, strETime,"");
    //SendCatalogSubscribe("34020000001180000002", "34020000001180000002", (char*)strSTime, (char*)strETime);
    //SendAlarmSubScribe("34020000001180000002", "34020000001180000002", 90, 1, 4, 0, (char*)strSTime, (char*)strETime);
    //SendAlarmQuery("34020000001180000002", "34020000001180000002", 1, 4, 0, (char*)strSTime, (char*)strETime,"告警设备类型");
    //return "";
    m_strRecordListXml = "";
    m_nRecordSum = 0;
    lpRecordInquire_info_t pInquireData  = new recordInquire_info_t();
    pInquireData->unSeq = m_unSeq;			//sequence
    pInquireData->nOperate = RECORD_INQUIRE_TYPE;		//Operate
    strcpy_s(pInquireData->strDstID, pstrDeviceID);	//device id
    strcpy_s(pInquireData->strDstSubID, pstrChannelID);	//device id
    pInquireData->nActive  = nType;
    strcpy_s(pInquireData->strStartTime, strSTime);
    strcpy_s(pInquireData->strEndTime, strETime);

    int len = 0; 
    if(pstrFilePath) len = strlen(pstrFilePath) + 1;
    else len = 0;
    pInquireData->nFilePathLen = len;
    if ( pInquireData->nFilePathLen > 0)
    {
        pInquireData->strFilePathExt = new char[pInquireData->nFilePathLen];
        strcpy_s(pInquireData->strFilePathExt, pInquireData->nFilePathLen, pstrFilePath);
    }

    if(pstrAddress) len = strlen(pstrAddress) + 1;
    else len = 0;
    pInquireData->nAddressLen = len;
    if ( pInquireData->nAddressLen > 0)
    {
        pInquireData->strAddress = new char[pInquireData->nAddressLen];
        strcpy_s(pInquireData->strAddress, pInquireData->nAddressLen, pstrAddress);
    }
    pInquireData->nSecrecy = bSecrecy;
    if(pstrRecorderID) len = strlen(pstrRecorderID) + 1;
    else len = 0;
    pInquireData->nRecordIDLen = len;
    if ( pInquireData->nRecordIDLen > 0)
    {
        pInquireData->strRecordID = new char[pInquireData->nRecordIDLen];
        strcpy_s(pInquireData->strRecordID, pInquireData->nRecordIDLen, pstrRecorderID);
    }
    pInquireData->cIndistinctQuery = IndistinctQuery;
    pInquireData->socUser = NULL;
    pInquireData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pInquireData);
    m_unSeq++;
    //等待5秒
    for(int i = 0; i < 50; i++)
    {
        g_objLog.LogoutDebug(k_LOG_DLL, "%s m_nRecordSum%d\n", __FUNCTION__, m_nRecordSum);
        if(XML_LEN == m_nRecordSum)
            return m_strRecordListXml.c_str();
        Sleep(100);
    }
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 请求超时\n", __FUNCTION__);
    return 0;
}

int CSipClient::SendDecorder( const char* pstrDecorderID, const char* pstrCameraID, int nOption )
{
    m_unSeq++;
    int nRet = 0;
    if(nOption == 0)
    {
        nRet = m_uas.PlayStart((char*)pstrCameraID, (char*)pstrDecorderID);
        if(nRet != 0)
        {
            return nRet;
        }
    }
    else
    {
        nRet = m_uas.PlayStop((char*)pstrCameraID, (char*)pstrDecorderID);
        if(nRet != 0)
        {
            return nRet;
        }
    }
    return 0;
}

int CSipClient::SendResetAlarm( const char *pstrDeviceID, const char *pstrChannelID )
{
    lpAlarm_info_t pAlarmInfo = new alarm_info_t(0);
    strcpy_s(pAlarmInfo->strDstID, pstrDeviceID);
    strcpy_s(pAlarmInfo->strDstSubID, pstrChannelID);
    m_uas.SendResetAlarm(pAlarmInfo);
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 发送Reset Alarm ID:%s\n", __FUNCTION__, pAlarmInfo->strDstSubID);
    m_unSeq++;
    return 0;
}

int CSipClient::GetOwnerType()
{
    return strcmp("EC", m_strOwnID);
}

int CSipClient::SendDeviceConfig( const char *pstrDeviceID, deviceConfig_info_t* pDeviceConfig_ )
{
    lpsipUASDeviceConfig_info_t pDeviceConfig  = new sipUASdeviceConfig_info_t();
    pDeviceConfig->unSeq = m_unSeq;				//sequence
    pDeviceConfig->nOperate = DEVICE_CONFIG_TYPE;			//Operate
    strcpy_s(pDeviceConfig->strDstID, pstrDeviceID);		//Remove Device ID
    pDeviceConfig->cBasicParam = pDeviceConfig_->cBasicParam;
    if ( pDeviceConfig->cBasicParam )
    {
        pDeviceConfig->nNameLen = pDeviceConfig_->lpBasicParam->strName.length()+1;
        pDeviceConfig->strName = new char[pDeviceConfig->nNameLen];
        strcpy_s(pDeviceConfig->strName, pDeviceConfig->nNameLen, pDeviceConfig_->lpBasicParam->strName.c_str());
        strcpy_s(pDeviceConfig->strNewDeviceId, pDeviceConfig_->lpBasicParam->strDeviceID);
        strcpy_s(pDeviceConfig->strSIPServerId, pDeviceConfig_->lpBasicParam->strSIPServerId);
        strcpy_s(pDeviceConfig->strSIPServerIp, pDeviceConfig_->lpBasicParam->strSIPServerIp);
        pDeviceConfig->nSIPServerPort = pDeviceConfig_->lpBasicParam->nSIPServerPort;
        pDeviceConfig->nDomainNameLen = pDeviceConfig_->lpBasicParam->strDomainName.length()+1;
        pDeviceConfig->strDomainName = new char[pDeviceConfig->nDomainNameLen];
        strcpy_s(pDeviceConfig->strDomainName, pDeviceConfig->nDomainNameLen, pDeviceConfig_->lpBasicParam->strDomainName.c_str());
        pDeviceConfig->nExpiration = pDeviceConfig_->lpBasicParam->nExpiration;
        pDeviceConfig->nPasswordLen = pDeviceConfig_->lpBasicParam->nPasswordLen;
        pDeviceConfig->strPassword = new char[pDeviceConfig->nPasswordLen];
        strcpy_s(pDeviceConfig->strPassword, pDeviceConfig->nPasswordLen, pDeviceConfig_->lpBasicParam->strPassword.c_str());
        pDeviceConfig->nHeartBeatInternal = pDeviceConfig_->lpBasicParam->nHeartBeatInternal;
        pDeviceConfig->nHeartBeatCount = pDeviceConfig_->lpBasicParam->nHeartBeatCount;
        pDeviceConfig->nVideoParamConfigNum = pDeviceConfig_->nVideoParamConfigNum;
        if ( pDeviceConfig->nVideoParamConfigNum > 0 )
        {
            pDeviceConfig->lpVideoParamAttribute = new sipUASdeviceConfig_info_t::videoParamAttribute_t[pDeviceConfig->nVideoParamConfigNum];
            for (int i = 0; i < pDeviceConfig->nVideoParamConfigNum; ++i)
            {
                pDeviceConfig->lpVideoParamAttribute[i].nStreamNameLen = pDeviceConfig_->lpVideoParamAttribute[i].strStreamName.length() + 1;
                pDeviceConfig->lpVideoParamAttribute[i].strStreamName = new char[pDeviceConfig->lpVideoParamAttribute[i].nStreamNameLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strStreamName, pDeviceConfig->lpVideoParamAttribute[i].nStreamNameLen, pDeviceConfig_->lpVideoParamAttribute[i].strStreamName.c_str());

                pDeviceConfig->lpVideoParamAttribute[i].nVideoFormatLen = pDeviceConfig_->lpVideoParamAttribute[i].strVideoFormat.length() + 1;
                pDeviceConfig->lpVideoParamAttribute[i].strVideoFormat = new char[pDeviceConfig->lpVideoParamAttribute[i].nVideoFormatLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strVideoFormat, pDeviceConfig->lpVideoParamAttribute[i].nVideoFormatLen, pDeviceConfig_->lpVideoParamAttribute[i].strVideoFormat.c_str());

                pDeviceConfig->lpVideoParamAttribute[i].nResolutionLen = pDeviceConfig_->lpVideoParamAttribute[i].strResolution.length() + 1;
                pDeviceConfig->lpVideoParamAttribute[i].strResolution = new char[pDeviceConfig->lpVideoParamAttribute[i].nResolutionLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strResolution, pDeviceConfig->lpVideoParamAttribute[i].nResolutionLen, pDeviceConfig_->lpVideoParamAttribute[i].strResolution.c_str());

                pDeviceConfig->lpVideoParamAttribute[i].nFrameRateLen = pDeviceConfig_->lpVideoParamAttribute[i].strFrameRate.length() + 1;
                pDeviceConfig->lpVideoParamAttribute[i].strFrameRate = new char[pDeviceConfig->lpVideoParamAttribute[i].nFrameRateLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strFrameRate, pDeviceConfig->lpVideoParamAttribute[i].nFrameRateLen, pDeviceConfig_->lpVideoParamAttribute[i].strFrameRate.c_str());

                pDeviceConfig->lpVideoParamAttribute[i].nBitRateTypeLen = pDeviceConfig_->lpVideoParamAttribute[i].strBitRateType.length() + 1;
                pDeviceConfig->lpVideoParamAttribute[i].strBitRateType = new char[pDeviceConfig->lpVideoParamAttribute[i].nBitRateTypeLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strBitRateType, pDeviceConfig->lpVideoParamAttribute[i].nBitRateTypeLen, pDeviceConfig_->lpVideoParamAttribute[i].strBitRateType.c_str());

                pDeviceConfig->lpVideoParamAttribute[i].nVideoBitRateLen = pDeviceConfig_->lpVideoParamAttribute[i].strVideoBitRate.length() + 1;;
                pDeviceConfig->lpVideoParamAttribute[i].strVideoBitRate = new char[pDeviceConfig->lpVideoParamAttribute[i].nVideoBitRateLen];
                strcpy_s(pDeviceConfig->lpVideoParamAttribute[i].strVideoBitRate, pDeviceConfig->lpVideoParamAttribute[i].nVideoBitRateLen, pDeviceConfig_->lpVideoParamAttribute[i].strVideoBitRate.c_str());
            }
        }
        pDeviceConfig->nAudioParamConfigNum = pDeviceConfig_->nAudioParamConfigNum;
        if (pDeviceConfig->nAudioParamConfigNum > 0)
        {
            pDeviceConfig->lpAudioParamAttribute = new sipUASdeviceConfig_info_t::audioParamAttribute_t[pDeviceConfig->nAudioParamConfigNum];
            for (int i = 0; i < pDeviceConfig->nAudioParamConfigNum; ++i)
            {
                pDeviceConfig->lpAudioParamAttribute[i].nStreamNameLen = pDeviceConfig_->lpAudioParamAttribute[i].strStreamName.length()+1;
                pDeviceConfig->lpAudioParamAttribute[i].strStreamName = new char[pDeviceConfig->lpAudioParamAttribute[i].nStreamNameLen];
                strcpy_s(pDeviceConfig->lpAudioParamAttribute[i].strStreamName, pDeviceConfig->lpAudioParamAttribute[i].nStreamNameLen, pDeviceConfig_->lpAudioParamAttribute[i].strStreamName.c_str());

                pDeviceConfig->lpAudioParamAttribute[i].nAudioFormatLen = pDeviceConfig_->lpAudioParamAttribute[i].strAudioFormat.length() + 1;
                pDeviceConfig->lpAudioParamAttribute[i].strAudioFormat = new char[pDeviceConfig->lpAudioParamAttribute[i].nAudioFormatLen];
                strcpy_s(pDeviceConfig->lpAudioParamAttribute[i].strAudioFormat, pDeviceConfig->lpAudioParamAttribute[i].nAudioFormatLen, pDeviceConfig_->lpAudioParamAttribute[i].strAudioFormat.c_str());

                pDeviceConfig->lpAudioParamAttribute[i].nAudioBitRateLen = pDeviceConfig_->lpAudioParamAttribute[i].strAudioBitRate.length() + 1;
                pDeviceConfig->lpAudioParamAttribute[i].strAudioBitRate = new char[pDeviceConfig->lpAudioParamAttribute[i].nAudioBitRateLen];
                strcpy_s(pDeviceConfig->lpAudioParamAttribute[i].strAudioBitRate, pDeviceConfig->lpAudioParamAttribute[i].nAudioBitRateLen, pDeviceConfig_->lpAudioParamAttribute[i].strAudioBitRate.c_str());

                pDeviceConfig->lpAudioParamAttribute[i].nSamplingRateLen = pDeviceConfig_->lpAudioParamAttribute[i].strSamplingRate.length() + 1;
                pDeviceConfig->lpAudioParamAttribute[i].strSamplingRate = new char[pDeviceConfig->lpAudioParamAttribute[i].nSamplingRateLen];
                strcpy_s(pDeviceConfig->lpAudioParamAttribute[i].strSamplingRate, pDeviceConfig->lpAudioParamAttribute[i].nSamplingRateLen, pDeviceConfig_->lpAudioParamAttribute[i].strSamplingRate.c_str());
            }
        }
        pDeviceConfig->SVACEncodeConfig.cFlag = pDeviceConfig_->SVACEncodeConfig.cFlag;
        if (pDeviceConfig->SVACEncodeConfig.cFlag)
        {
            pDeviceConfig->SVACEncodeConfig.ROIParam.cFlag = pDeviceConfig_->SVACEncodeConfig.ROIParam.cFlag;
            if (pDeviceConfig->SVACEncodeConfig.ROIParam.cFlag)
            {
                pDeviceConfig->SVACEncodeConfig.ROIParam.nROIFlag = pDeviceConfig_->SVACEncodeConfig.ROIParam.nROIFlag;
                pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber = pDeviceConfig_->SVACEncodeConfig.ROIParam.nROINumber;
                if (pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber > 0 )
                {
                    pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute = new struct sipUASdeviceConfig_info_t::ROI_PARAM_ATTRIBUTE_T[pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber];
                    for (int i = 0; i < pDeviceConfig->SVACEncodeConfig.ROIParam.nROINumber; ++i)
                    {
                        pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROISeq = pDeviceConfig_->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROISeq;
                        pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nTopLeft = pDeviceConfig_->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nTopLeft;
                        pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nBottomRight = pDeviceConfig_->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nBottomRight;
                        pDeviceConfig->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROIQP = pDeviceConfig_->SVACEncodeConfig.ROIParam.lpROIParamAttribute[i].nROIQP;
                    }
                }
                pDeviceConfig->SVACEncodeConfig.ROIParam.nBackGroundQP = pDeviceConfig_->SVACEncodeConfig.ROIParam.nBackGroundQP;
                pDeviceConfig->SVACEncodeConfig.ROIParam.nBackGroundSkipFlag = pDeviceConfig_->SVACEncodeConfig.ROIParam.nBackGroundSkipFlag;
            }
            pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag = pDeviceConfig_->SVACEncodeConfig.SVCParam.cFlag;
            if (pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag)
            {
                pDeviceConfig->SVACEncodeConfig.SVCParam.cFlag = pDeviceConfig_->SVACEncodeConfig.SVCParam.cFlag;
                pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCFlag = pDeviceConfig_->SVACEncodeConfig.SVCParam.nSVCFlag;
                pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCSTMMode = pDeviceConfig_->SVACEncodeConfig.SVCParam.nSVCSTMMode;
                pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCSpaceDomainMode = pDeviceConfig_->SVACEncodeConfig.SVCParam.nSVCSpaceDomainMode;
                pDeviceConfig->SVACEncodeConfig.SVCParam.nSVCTimeDomainMode = pDeviceConfig_->SVACEncodeConfig.SVCParam.nSVCTimeDomainMode;
            }
            pDeviceConfig->SVACEncodeConfig.SurveillanceParam.cFlag = pDeviceConfig_->SVACEncodeConfig.SurveillanceParam.cFlag;
            if (pDeviceConfig->SVACEncodeConfig.SurveillanceParam.cFlag)
            {
                pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nTimeFlag = pDeviceConfig_->SVACEncodeConfig.SurveillanceParam.nTimeFlag;
                pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nEventFlag = pDeviceConfig_->SVACEncodeConfig.SurveillanceParam.nEventFlag;
                pDeviceConfig->SVACEncodeConfig.SurveillanceParam.nAlertFlag = pDeviceConfig_->SVACEncodeConfig.SurveillanceParam.nAlertFlag;
            }
            pDeviceConfig->SVACEncodeConfig.EncryptParam.cFlag = pDeviceConfig_->SVACEncodeConfig.EncryptParam.cFlag;
            if (pDeviceConfig->SVACEncodeConfig.EncryptParam.cFlag)
            {
                pDeviceConfig->SVACEncodeConfig.EncryptParam.nEncryptionFlag = pDeviceConfig_->SVACEncodeConfig.EncryptParam.nEncryptionFlag;
                pDeviceConfig->SVACEncodeConfig.EncryptParam.nAuthenticationFlag = pDeviceConfig_->SVACEncodeConfig.EncryptParam.nAuthenticationFlag;
            }
            pDeviceConfig->SVACEncodeConfig.AudioParam.cFlag = pDeviceConfig->SVACEncodeConfig.AudioParam.cFlag;
            if (pDeviceConfig->SVACEncodeConfig.AudioParam.cFlag)
            {
                pDeviceConfig->SVACEncodeConfig.AudioParam.nAudioRecognitionFlag = pDeviceConfig_->SVACEncodeConfig.AudioParam.nAudioRecognitionFlag;
            }
        }
        pDeviceConfig->SVACDecodeConfig.cFlag = pDeviceConfig_->SVACDecodeConfig.cFlag;
        if (pDeviceConfig->SVACDecodeConfig.cFlag)
        {
            pDeviceConfig->SVACDecodeConfig.SVCParam.cFlag = pDeviceConfig_->SVACDecodeConfig.SVCParam.cFlag;
            if (pDeviceConfig->SVACDecodeConfig.SVCParam.cFlag)
            {
                pDeviceConfig->SVACDecodeConfig.SVCParam.nSVCSTMMode = pDeviceConfig_->SVACDecodeConfig.SVCParam.nSVCSTMMode;
            }
            pDeviceConfig->SVACDecodeConfig.SurveillanceParam.cFlag = pDeviceConfig_->SVACDecodeConfig.SurveillanceParam.cFlag;
            if (pDeviceConfig->SVACDecodeConfig.SurveillanceParam.cFlag)
            {
                pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nTimeShowFlag = pDeviceConfig_->SVACDecodeConfig.SurveillanceParam.nTimeShowFlag;
                pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nEventShowFlag = pDeviceConfig_->SVACDecodeConfig.SurveillanceParam.nEventShowFlag;
                pDeviceConfig->SVACDecodeConfig.SurveillanceParam.nAlertShowFlag = pDeviceConfig_->SVACDecodeConfig.SurveillanceParam.nAlertShowFlag;
            }
        }
    }

    pDeviceConfig->socUser = NULL;
    pDeviceConfig->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pDeviceConfig);
    m_unSeq++;
    return 0;
}

DEVICE_PRESETQUERY_INFO_T* CSipClient::SendDevicePersetQuery( const char *pstrDeviceID )
{
    lpPresetQuery_info_t pPresetQuery  = new presetQuery_info_t();
    pPresetQuery->unSeq = m_unSeq;				//sequence
    pPresetQuery->nOperate = PRESET_QUERY_TYPE;			//Operate
    strcpy_s(pPresetQuery->strDstID, pstrDeviceID);		//Remove Device ID	

    pPresetQuery->socUser = NULL;
    pPresetQuery->nCmdID = MESSAGE_CMD;
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
    return 0;
}

DEVICE_CONFIG_INFO_T* CSipClient::SendConfigDownload( const char *pstrDeviceID,const char *strConfigType )
{
    lpConfigDownliad_info_t pConfigDownload  = new configDownload_info_t();
    pConfigDownload->unSeq = m_unSeq;				//sequence
    pConfigDownload->nOperate = CONFIG_DOWNLOAD_TYPE;			//Operate
    strcpy_s(pConfigDownload->strDstID, pstrDeviceID);		//Remove Device ID
    pConfigDownload->nConfigTypeLen = strlen(strConfigType);	
    pConfigDownload->strConfigType = new char[pConfigDownload->nConfigTypeLen];
    strcpy_s(pConfigDownload->strConfigType, pConfigDownload->nConfigTypeLen, strConfigType);

    pConfigDownload->socUser = NULL;
    pConfigDownload->nCmdID = MESSAGE_CMD;

    m_uas.SendOppositeData(pConfigDownload);
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
    return 0;
}

const char * CSipClient::SendRealPlayUrl( const char *pstrDeviceID, const char *pstrChannelID )
{
    m_strRealPlayUrlXml = "";
    lpRealPlayUrl_info_t pRealPlayUrlData  = new realPlayUrl_info_t();
    pRealPlayUrlData->unSeq = m_unSeq;
    pRealPlayUrlData->nOperate = REAL_PLAY_URL_TYPE;
    strcpy_s(pRealPlayUrlData->strDstID, pstrDeviceID);
    strcpy_s(pRealPlayUrlData->strDecoderChannelID, pstrChannelID);

    pRealPlayUrlData->socUser = NULL;
    pRealPlayUrlData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pRealPlayUrlData);
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
    return 0;
}

const char * CSipClient::SendPlayBackUrl( const char *pstrDeviceID, const char *pstrChannelID, const char* strSTime, const char* strETime, const char* pstrLocation )
{
    m_strPlayBackUrlXml = "";

    lpPlayBackUrl_info_t pPlayBackUrl  = new playBackUrl_info_t();

    pPlayBackUrl->unSeq = m_unSeq;
    pPlayBackUrl->nOperate = PLAY_BACK_URL_TYPE;
    strcpy_s(pPlayBackUrl->strDstID, pstrDeviceID);	//Remote Device ID
    strcpy_s(pPlayBackUrl->strDstSubID, pstrChannelID);	//Device ID
    strcpy_s(pPlayBackUrl->strStartTime, strSTime);
    strcpy_s(pPlayBackUrl->strEndTime, strETime);
    pPlayBackUrl->nRecLocationLen = strlen(pstrLocation) + 1;
    pPlayBackUrl->strRecLocation = new char[pPlayBackUrl->nRecLocationLen];
    strcpy_s(pPlayBackUrl->strRecLocation, pPlayBackUrl->nRecLocationLen, pstrLocation);

    pPlayBackUrl->socUser = NULL;
    pPlayBackUrl->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pPlayBackUrl);
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
    return 0;
}

const char * CSipClient::SendDecoderStatus( const char *pstrDeviceID, const char *pstrChannelID )
{
    m_strDecoderStatusXml = "";
    lpDecoderStatus_info_t pDecoderStatus  = new decoderStatus_info_t();
    pDecoderStatus->unSeq = m_unSeq;
    pDecoderStatus->nOperate = DECODER_STATUS_TYPE;
    strcpy_s(pDecoderStatus->strDstID, pstrDeviceID);	//Remote Device ID
    strcpy_s(pDecoderStatus->strDstSubID, pstrChannelID);	//Device ID
    pDecoderStatus->socUser = NULL;
    pDecoderStatus->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pDecoderStatus);

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
    return 0;
}

int CSipClient::SendAlarmSubScribe( const char *pstrDeviceID, const char *pstrChannelID, int nExpires, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime )
{
    lpAlarmSubscribe_info_t pAlarmData  = new alarmSubscribe_info_t();
    pAlarmData->unSeq = m_unSeq;
    pAlarmData->nOperate = SUBSCRIBE_ALARM_TYPE;
    strcpy_s(pAlarmData->strDstID, pstrDeviceID);	//Remote Device ID
    strcpy_s(pAlarmData->strDstSubID, pstrChannelID);	//Device ID
    pAlarmData->nExpires = nExpires;
    pAlarmData->nStartAlarmPriority = nStartAlarmPriority;
    pAlarmData->nEndAlarmPriority = nEndAlarmPriority;
    pAlarmData->nAlarmMethod = nAlarmMethod;
    strcpy_s(pAlarmData->strStartTime, pstrSTime);
    strcpy_s(pAlarmData->strEndTime, pstrETime);

    pAlarmData->socUser = NULL;
    pAlarmData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pAlarmData);
    m_unSeq++;
    return 0;
}

const char * CSipClient::SendAlarmQuery( const char *pstrDeviceID, const char *pstrChannelID, int nStartAlarmPriority, int nEndAlarmPriority, int nAlarmMethod, const char* pstrSTime,const char* pstrETime,const char* pstrAlarmType )
{
    lpAlarmQuery_info_t pAlarmData = new alarmQuery_info_t();
    pAlarmData->unSeq = m_unSeq;
    pAlarmData->nOperate = ALARM_QUERY_TYPE;
    strcpy_s(pAlarmData->strDstID, pstrDeviceID);
    strcpy_s(pAlarmData->strDstSubID, pstrChannelID);
    pAlarmData->nStartAlarmPriority = nStartAlarmPriority;
    pAlarmData->nEndAlarmPriority = nEndAlarmPriority;
    pAlarmData->nAlarmMethod = nAlarmMethod;
    strcpy_s(pAlarmData->strStartTime, pstrSTime);
    strcpy_s(pAlarmData->strEndTime, pstrETime);
    if(pstrAlarmType)
        pAlarmData->nAlarmTypeLen = strlen(pstrAlarmType) + 1;
    else
        pAlarmData->nAlarmTypeLen = 0;
    if ( pAlarmData->nAlarmTypeLen > 0 )
    {
        pAlarmData->strAlarmType = new char[pAlarmData->nAlarmTypeLen];
        strcpy_s(pAlarmData->strAlarmType, pAlarmData->nAlarmTypeLen, pstrAlarmType);
    }

    pAlarmData->socUser = NULL;
    pAlarmData->nCmdID = MESSAGE_CMD;
    m_uas.SendOppositeData(pAlarmData);

    m_unSeq++;
	//等待5秒
	for(int i = 0; i < 50; i++)
	{
		if(0 < m_strDecoderStatusXml.size())
			return m_strAlarmQueryXml.c_str();
		Sleep(100);
	}
	log4wr_log(LOG4WR_LOG_DEBUG, "%s 请求超时\n", __FUNCTION__);
    return 0;
}

int CSipClient::OnRecordInfoInquiryRes( lpMsgParam_t m_pMsgParam )
{
    //g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到RecordInfo respons消息 nOperate:%d, nExterndSize:%d strExterndData:%s\n", __FUNCTION__, nOperate, nExterndSize,
    //	CLog::ANSIToUnicode(strExterndData).c_str());

    //string strFileInfo = strExterndData;

    //	char strFileInfo[] = "<?xml version=\"1.0\"?>\r\n"
    //"<Response>\r\n"
    //"<CmdType>RecordInfo</CmdType>\r\n"
    //"<SN>17430</SN>\r\n"
    //"<DeviceID>64010000001310000001</DeviceID>\r\n"
    //"<Name>Camera1</Name>\r\n"
    //"<SumNum>100</SumNum>\r\n"
    //"<RecordList Num=2>\r\n"
    //"<Item>\r\n"
    //"<DeviceID>64010000001310000001</DeviceID>\r\n"
    //"<Name>Camera1</Name>\r\n"
    //"<FilePath>64010000002100000001</FilePath>\r\n"
    //"<Address>Address 1</Address>\r\n"
    //"<StartTime>2010-11-12T10:10:00</StartTime>\r\n"
    //"<EndTime>2010-11-12T10:20:00</EndTime>\r\n"
    //"<Secrecy>0</Secrecy>\r\n"
    //"<Type>time</Type>\r\n"
    //"<RecorderID>64010000003000000001</RecorderID>\r\n"
    //"</Item>\r\n"
    //"<Item>\r\n"
    //"<DeviceID>64010000001310000001</DeviceID>\r\n"
    //"<Name>Camera1</Name>\r\n"
    //"<FilePath>64010000002100000001</ FilePath >\r\n"
    //"<Address>Address 1</Address>\r\n"
    //"<StartTime>2010-11-12T10:20:00</StartTime>\r\n"
    //"<EndTime>2010-11-12T10:30:00</EndTime>\r\n"
    //"<Secrecy>0</Secrecy>\r\n"
    //"<Type>time</Type>\r\n"
    //"<RecorderID>64010000003000000001</RecorderID>\r\n"
    //"</Item>\r\n"
    //"</RecordList>\r\n"
    //"</Response>\r\n";
    string strRecordInfo = m_pMsgParam->pExtendData;

    int nRecordSum = 0;
    string strSumNum = GetStringBetween(strRecordInfo, "<SumNum>", "</SumNum>", FALSE);
    if(!strSumNum.empty())
    {
        nRecordSum = atoi(strSumNum.c_str());
    }

    g_objLog.LogoutDebug(k_LOG_DLL, "%s SumNum:%d\n", __FUNCTION__, nRecordSum);
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


        m_strRecordListXml += strRecordInfo;
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
        m_strRecordListXml += strRecordInfo;
        m_nRecordSum = XML_LEN;
        return 0;
    }
}

int CSipClient::OnDeviceCatalogInquiryRes( lpMsgParam_t m_pMsgParam )
{
    m_strCatalogInfoListXml += m_pMsgParam->pExtendData;

    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DeviceCatalogInquiry respons消息 \n", __FUNCTION__);
    return 0;
}

int CSipClient::OnPlayBackUrlRes( lpMsgParam_t m_pMsgParam )
{
    m_strPlayBackUrlXml += m_pMsgParam->pExtendData;
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到PlayBackUrl respons消息 \nstrExterndData: %s\n", __FUNCTION__, m_strRealPlayUrlXml.c_str());
    return 0;
}

int CSipClient::OnCatalogSubscribeRes( lpMsgParam_t m_pMsgParam )
{
    //todo
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到CatalogSubscribe respons消息 \nstrExterndData: %s\n", __FUNCTION__, m_pMsgParam->pExtendData);
    return 0;
}

int CSipClient::OnAlarmSubscribeRes( lpMsgParam_t m_pMsgParam )
{
    //todo
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到AlarmSubscribe respons消息 \nstrExterndData: %s\n", __FUNCTION__, m_pMsgParam->pExtendData);
    return 0;
}

int CSipClient::OnDeviceStatusInquiryRes( lpMsgParam_t m_pMsgParam )
{
    m_strDeviceStatusListXml = m_pMsgParam->pExtendData;
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DeviceStatus respons消息 \nstrExterndData %s\n", __FUNCTION__, m_strDeviceStatusListXml.c_str());
    return 0;
}

int CSipClient::OnDeviceInfoInquiryRes( lpMsgParam_t m_pMsgParam )
{
    m_strDeviceInfoListXml = m_pMsgParam->pExtendData;
    g_objLog.LogoutDebug(k_LOG_DLL, "%s 收到DeviceStatus respons消息 \nstrExterndData %s\n", __FUNCTION__, m_strDeviceInfoListXml.c_str());
    return 0;
}

int CSipClient::SendLogin(const char* pstrLocalSipID, const char* pstrLocalIp, const char* pstrLocalPort, const char* pstrPltSipID, const char* pstrPltIp, const char* pstrPltPort, const char* pstrPltPassword)
{
    m_strSipClientID = pstrLocalSipID;
    m_uas.SetPltInfo(pstrLocalSipID, pstrLocalIp, pstrLocalPort, pstrPltSipID, pstrPltIp, pstrPltPort,pstrPltPassword);
    m_uas.SendRegister();
    return 0;
}

int CSipClient::SendLogout()
{
    m_uas.SendLogout();
    return 0;
}

#endif