#include "StdAfx.h"
#include "UnifiedMessage.h"

CModMessage::CModMessage(void)
	:m_eModAction(), m_tParam(nullptr)
{
	memset(szToDeviceID, 0, ID_BUF_LEN);
	memset(work_params.video_call.szSubjectID, 0, ID_BUF_LEN);
	memset(szFromDeviceID, 0, ID_BUF_LEN);
	memset(m_szNvrIP, 0, IP_BUF_LEN);

}

CModMessage::CModMessage(const CModMessage& msgSrc)
{
	this->Clone(&msgSrc);
}

void CModMessage::operator=(const CModMessage& msgSrc)
{
	this->Clone(&msgSrc);
}

void CModMessage::FromSipDataBlock(ipc_sip_block_t * p_sip_block)
{
	if (p_sip_block->tHeader.nExtBodySize > 0)
	{
		auto p_buffer = reinterpret_cast<char*>(p_sip_block) + sizeof(ipc_sip_block_t);
		strBody = p_buffer;
	}
	auto sizelenth = p_sip_block->tHeader.nPackSize - p_sip_block->tHeader.nExtBodySize;
	auto this_sip_block = static_cast<ipc_sip_block_t*>(this);
	memset(this_sip_block, 0, sizelenth);
	memcpy(this_sip_block, p_sip_block, sizelenth);


	/*	SetDeviceID(p_sip_block->szToDeviceID);
		SetRemoteID(p_sip_block->szRemoteID);

		if (p_sip_block->tHeader.nExtBodySize > 0)
		{
			auto p_buffer = reinterpret_cast<char*>(p_sip_block) + sizeof(ipc_sip_block_t);
			m_strBody = p_buffer;
		}
		if (p_sip_block->tHeader.eWorkType == work_kinds::video_call ||
			p_sip_block->tHeader.eWorkType == work_kinds::subscription
			)
		{
			SetCallDialogID(p_sip_block->nCallDialogID);
		}
		if (p_sip_block->tHeader.eWorkType == work_kinds::video_call)
		{
			SetSubjectID(p_sip_block->szTargetSubjectID);

			SetSubject(p_sip_block->szSubject);
			SetVideoCallSender(p_sip_block->work.video_call.invite.p_sender);
		}
	*/
}

void CModMessage::SetModAction(mod_op_t::u_op_type_t eOperateType)
{
	m_eModAction = eOperateType;
}

// 设置查询条件
void CModMessage::SetSearchType(const char *pszSearchType)
{
	Utils::StringCpy_s(work_params.query_record.szCmdType, DEV_NAME_BUF_LEN, pszSearchType);
}

// 设置控制命令
void CModMessage::SetCtrlCmd(const char *pszCtrlCmd)
{
	Utils::StringCpy_s(work_params.device_control.szCtrlCmd, PTZ_LBUF_LEN, pszCtrlCmd);
}

void CModMessage::SetSubjectID(const char *pszSubjectID)
{
	memcpy(work_params.video_call.szSubjectID, pszSubjectID, ID_BUF_LEN);
}

void CModMessage::SetDeviceID(const char *pszDeviceID)
{
	memcpy(szToDeviceID, pszDeviceID, ID_BUF_LEN);
}

void CModMessage::SetTID(int n_TID)
{
	//work.video_call.nTID = n_TID;
	this->nTID = n_TID;
}

void CModMessage::SetCallDialogID(INT64 n_CallDialogID)
{
	//work.video_call.nCallDialogID = n_CallDialogID;
	this->nCallDialogID = n_CallDialogID;
}

// 设置新数捿
void CModMessage::SetSearchData(void *pUpdateData)
{
	work_params.query_record.pData = pUpdateData;
}

// 设置播放数据
void CModMessage::SetPlayData(void *pPlayData)
{
	work_params.video_call.pPlayInfo = pPlayData;
}

// 设置通知内容
void CModMessage::SetNotifyData(void *pNotifyData, const char *pszSN)
{
	work_params.event_notify.pszNotifyContent = pNotifyData;
	if (pszSN)
	{
		Utils::StringCpy_s(work_params.event_notify.szSN, CSEQ_BUF_LEN, pszSN);
	}
}

// 设置SN
void CModMessage::SetQuerySN(const char *pszSN)
{
	memcpy(szSN, pszSN, CSEQ_BUF_LEN);
}

// 设置设备类型
void CModMessage::SetDeviceType(HUSDevice_T eDeviceType)
{
	this->eHusDevice = eDeviceType;
}

// 设置媒体接收端地址
void CModMessage::SetRecvAddress(const char *pszIP, u_short uPort)
{
	Utils::StringCpy_s(work_params.video_call.szRecvIP, IP_BUF_LEN, pszIP);
	work_params.video_call.nRecvPort[0] = uPort;
	work_params.video_call.nRecvPort[1] = uPort + 1;
}

// 设置文件查询的开始时闿
void CModMessage::SetSearchStartTime(const char *pszStartTime)
{
	Utils::StringCpy_s(work_params.query_record.szStartTime, TIME_BUF_LEN, pszStartTime);
}

// 设置文件查询的结束时闿
void CModMessage::SetSearchEndTime(const char *pszEndTime)
{
	Utils::StringCpy_s(work_params.query_record.szEndTime, TIME_BUF_LEN, pszEndTime);
}

// 设置录像文件查询的录像类垿
void CModMessage::SetRecordType(record_type_t eRecordType)
{
	work_params.query_record.eRecordType = eRecordType;
}

// 设置录像查询的地址
void CModMessage::SetRecordAddr(const char *pszRecordAddr)
{
	Utils::StringCpy_s(work_params.query_record.szAddr, PARAM_NAME_BUF_LEN, pszRecordAddr);
}

void CModMessage::SetRecLocation(const char *pszRecLocation)
{
	Utils::StringCpy_s(work_params.query_record.szRecLocation, PARAM_NAME_BUF_LEN, pszRecLocation);
}

void CModMessage::SetPlayGUID(const char *pszPlayDeviceGUID)
{
	Utils::StringCpy_s(szGUID, GUID_BUF_LEN, pszPlayDeviceGUID);
}

// 设置更新数据设备的GUID
void CModMessage::SetUpdataGUID(const char *pszDeviceGUID)
{
	Utils::StringCpy_s(szGUID, GUID_BUF_LEN, pszDeviceGUID);
}

// 设置更新类型
void CModMessage::SetNotifyUpdataType(event_notify_t::e_update_t eUpdateType)
{
	work_params.event_notify.e_update_type = eUpdateType;
}

// 设置NVR IP
void CModMessage::SetNVRIP(const char *pszNVRIP)
{
	Utils::StringCpy_s(m_szNvrIP, IP_BUF_LEN, pszNVRIP);
}

//设置subject
void CModMessage::SetSubject(const char *pszSubject)
{
	Utils::StringCpy_s(work_params.video_call.szSubjectContent, SUBJECT_BUF_LEN, pszSubject);
}

//取broadcastSrcId信息
void CModMessage::SetBroadcastSrcID(const char *broadcastSrcId)
{
	Utils::StringCpy_s(szFromDeviceID, ID_BUF_LEN, broadcastSrcId);
}

//取broadcastTargetId信息
void CModMessage::SetBroadcastTargetID(const char *broadcastTargetId)
{
	Utils::StringCpy_s(szToDeviceID, ID_BUF_LEN, broadcastTargetId);
}

void CModMessage::SetCallSender(void *pInviteSender)
{
	work_params.video_call.sender_parms.p_sender = pInviteSender;
}

void CModMessage::SetRecordID(const char *recordID)
{
	Utils::StringCpy_s(szGUID, GUID_LEN + 1, recordID);
}

const char *CModMessage::GetRecordID() const
{
	return szGUID;
}

//获取subject
const char *CModMessage::GetSubject() const
{
	return work_params.video_call.szSubjectContent;
}

//取broadcastSrcId信息
const char *CModMessage::GetBroadcastSrcID() const
{
	return szFromDeviceID;
}

//取broadcastTargetId信息
const char *CModMessage::GetBroadcastTargetID() const
{
	return szToDeviceID;
}

//获取pInviteSender
void *CModMessage::GetCallSender() const
{
	return work_params.video_call.sender_parms.p_sender;
}

// 取得操作类型
mod_op_t::u_op_type_t CModMessage::GetModAction() const
{
	return m_eModAction;
}

// 取得查询类型
char *CModMessage::GetQueryType()
{
	return work_params.query_record.szCmdType;
}

// 取得文件查询的开始时闿
char *CModMessage::GetSearchStartTime()
{
	return work_params.query_record.szStartTime;
}

// 取得文件查询的结束时闿
char *CModMessage::GetSearchEndTime()
{
	return work_params.query_record.szEndTime;
}

// 取得控制类型
char *CModMessage::GetCtrlCmd()
{
	return work_params.device_control.szCtrlCmd;
}

char *CModMessage::GetRemoteID()
{
	return szFromDeviceID;
}
// 取得设备GBID
char *CModMessage::GetDeviceID()
{
	if (strlen(work_params.video_call.szSubjectID) == ID_LEN && Utils::StringCmp_s(szToDeviceID, work_params.video_call.szSubjectID, ID_LEN) != 0)
	{
		return work_params.video_call.szSubjectID;
	}
	return szToDeviceID;
}
// 取得设备Subject GBID
char *CModMessage::GetSubjectID()
{
	return work_params.video_call.szSubjectID;
}

// 取得更新数据
void *CModMessage::GetSearchData() const
{
	return work_params.query_record.pData;
}

// 取得call-dialog ID
int CModMessage::GetDID() const
{
	return nCallDialogID & 0x00000000ffffffff;
}

// 取得call-dialog ID
INT64 CModMessage::GetCallDialogID() const
{
	return nCallDialogID;
}

// 取得播放GUID
char *CModMessage::GetPlayGUID()
{
	return szGUID;
}

// 取得更新数据设备的GUID
char *CModMessage::GetUpdataGUID()
{
	return szGUID;
}

// 设置更新类型
event_notify_t::e_update_t CModMessage::GetUpdataType() const
{
	return work_params.event_notify.e_update_type;
}

// 取得播放数据
void *CModMessage::GetPlayData() const
{
	return work_params.video_call.pPlayInfo;
}

// 取得通知内容
void *CModMessage::GetNotifyData(CString *pstrSNBuf)
{
	if (pstrSNBuf)
	{
		*pstrSNBuf = work_params.event_notify.szSN;
	}

	return work_params.event_notify.pszNotifyContent;
}

// 取得SN叿
char *CModMessage::GetQuerySN()
{
	return szSN;
}

// 取得设备类型
HUSDevice_T CModMessage::GetDeviceType() const
{
	return eHusDevice;
}

// 取得媒体接收端IP
char *CModMessage::GetRecvIP()
{
	return work_params.video_call.szRecvIP;
}
// 取得媒体接收端Port
int CModMessage::GetRecvPort()
{
	return work_params.video_call.nRecvPort[0];
}

int CModMessage::GetTID() const
{
	return this->nTID;
}

// 设置录像文件查询的录像类垿
const record_type_t CModMessage::GetRecordType() const
{
	return work_params.query_record.eRecordType;
}

// 取得录像地址
const char *CModMessage::GetRecordAddr() const
{
	return work_params.query_record.szAddr;
}

const char * CModMessage::GetRecLocation() const
{
	return work_params.query_record.szRecLocation;
}

// 取得NVR IP
char *CModMessage::GetNVRIP()
{
	return m_szNvrIP;
}

void CModMessage::SetSubResult(int nResult)
{
	work_params.subscription.n_sub_result = nResult;
}

int CModMessage::GetSubResult() const
{
	return work_params.subscription.n_sub_result;
}

const char * CModMessage::GetSubExpires() const
{
	return szExpiresReason;
}

void CModMessage::SetSubExpires(const char *pszExpires)
{
	Utils::StringCpy_s(szExpiresReason, EXPIRES_BUF_LEN, pszExpires);
}

void CModMessage::Free()
{
	auto pCmd = GetCmdParam();
	if (pCmd)
	{
		delete pCmd;
	}
	CMemPoolUnit::Free();
}


void CModMessage::SetSubType(const char *pszCmdType)
{
	Utils::StringCpy_s(work_params.subscription.sz_sub_type, DEV_NAME_BUF_LEN, pszCmdType);
}

const char * CModMessage::GetSubType() const
{
	return work_params.subscription.sz_sub_type;
}

void * CModMessage::GetCmdParam() const
{
	// return m_uOperation.tParam;
	return this->m_tParam;
}

void CModMessage::SetCmdParam(void *pParam)
{
	//  m_uOperation.tParam = pParam;
	this->m_tParam = pParam;
}

void CModMessage::Clone(const CModMessage* srcUnifiedMessage)
{
	if (nullptr == srcUnifiedMessage) return;

	// 拷贝操作，拷贝需要的信息
	SetModAction(srcUnifiedMessage->GetModAction());
	Utils::MemBlockCpy_s(&(work_params), &(srcUnifiedMessage->work_params), sizeof(work_detail_t));
	Utils::StringCpy_s(work_params.video_call.szSubjectID, ID_BUF_LEN, srcUnifiedMessage->work_params.video_call.szSubjectID);
	Utils::StringCpy_s(szToDeviceID, ID_BUF_LEN, srcUnifiedMessage->szToDeviceID);
	Utils::StringCpy_s(szFromDeviceID, ID_BUF_LEN, srcUnifiedMessage->szFromDeviceID);
	Utils::StringCpy_s(m_szNvrIP, ID_LEN, srcUnifiedMessage->m_szNvrIP);
}