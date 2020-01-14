#include "stdafx.h"
#include "SDKInfoSearcherMgr.h"
#include "Main/UnifiedMessage.h"

int VideoRecordsSeachMgr::GetCatalogBodyContent(char *pszInfo, int nBufLen, const char *pstrSN, InfoContext_t *pContext)
{
	int nRecordSum = m_oRecordList.GetSize();
	VideoRecordItem	*pRecord = nullptr;
	if (NULL == pContext->posRecordMap)
	{
		pContext->posRecordMap = m_oRecordList.GetHeadPosition();
		pContext->nRawDevSum = nRecordSum;
		// 计算生成 xml文件的个数
		// 每10个记录生成一个xml
		pContext->nSurplus = nRecordSum / MAX_LIST_LEN;
		if (0 < (nRecordSum % MAX_LIST_LEN))
			pContext->nSurplus++;

		int nListSum = (0 == pContext->nSurplus ? nRecordSum : MAX_LIST_LEN);
		int nCompleteLen = nBufLen;

		// 没有录像，生成空文件
		if (0 == pContext->nSurplus)
		{
			// 创建录像文件列表xml的头
			if (ERROR_NOACCESS == m_oBodyBuilder.CreateRecordHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
				m_strDeviceName.GetString(), nRecordSum, nListSum))
				goto error;

			// 添加录像文件xml的文件尾
			if (ERROR_NOACCESS == m_oBodyBuilder.CompleteRecord(pszInfo + nCompleteLen, nBufLen - nCompleteLen))
				goto error;

			return 0;
		}
	}

	if (pContext->nSurplus)
	{
		int nListSum = 0;
		int nCountUsed = 0;
		int nCompleteLen = nBufLen;
		int nResult = 0;
		if (MAX_LIST_LEN < pContext->nRawDevSum)
			nListSum = MAX_LIST_LEN;
		else
			nListSum = pContext->nRawDevSum;

		// 创建录像文件列表xml的头
		nResult = m_oBodyBuilder.CreateRecordHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
			m_strDeviceName.GetString(), nRecordSum, nListSum);
		if (ERROR_NOACCESS == nResult)
			goto error;

		nCountUsed += nCompleteLen;
		nCompleteLen = nBufLen - nCountUsed;

		for (int nItemIdx = 0; nItemIdx < MAX_LIST_LEN; nItemIdx++)
		{
			// 此参数暂时保留
			CString strFilePath;
			pRecord = m_oRecordList.GetNext(pContext->posRecordMap);

			// 添加录像文件 //USE SHENTONG API
			nResult = m_oBodyBuilder.AddRecordBodyShengtong(pszInfo + nCountUsed,
				nCompleteLen,
				m_strDeviceID.GetString(),
				pRecord->fileName,
				pRecord->GetStartTime(nullptr),
				pRecord->GetEndTime(nullptr),
				pRecord->GetAddr(),
				pRecord->filePath,
				pRecord->fileSize,
				pRecord->recordId,
				pRecord->GetType(), 0);

			if (ERROR_NOACCESS == nResult)
				goto error;

			pRecord->Free();
			pRecord = nullptr;
			pContext->nRawDevSum--;
			nCountUsed += nCompleteLen;
			nCompleteLen = nBufLen - nCountUsed;

			// 最后一条记录
			if (NULL == pContext->posRecordMap)
			{
				m_oRecordList.RemoveAll();
				break;
			}
		}

		// 添加录像文件xml的文件尾
		nResult = m_oBodyBuilder.CompleteRecord(pszInfo + nCountUsed, nCompleteLen);
		if (ERROR_NOACCESS == nResult)
			goto error;

		pContext->nSurplus--;
		return pContext->nSurplus;
	}

	return 0;
error:
	if (pRecord)
		pRecord->Free();

	while (pContext->posRecordMap)
	{
		pRecord = m_oRecordList.GetNext(pContext->posRecordMap);
		pRecord->Free();
	}

	m_oRecordList.RemoveAll();

	return -1;
}



// 设置查询条件
void VideoRecordsSeachMgr::SetSearchMessage(CModMessage * pUnifiedMsg, const GUID &guidStreamer, const GUID &guidNVR)
{
	m_oLock.Lock();
	m_oUnifiedMsgList.AddTail(pUnifiedMsg);
	m_oStreamerGUIDList.AddTail(guidStreamer);
	m_oNVRGUIDList.AddTail(guidNVR);
	m_oLock.Unlock();
}

// 取得查询条件
CModMessage *VideoRecordsSeachMgr::GetSearchMessage(GUID &guidStreamer, GUID &guidNVR)
{
	CModMessage * pMsg = nullptr;
	m_oLock.Lock();
	if (0 < m_oUnifiedMsgList.GetSize())
	{
		pMsg = m_oUnifiedMsgList.GetHead();
		guidStreamer = m_oStreamerGUIDList.GetHead();
		guidNVR = m_oNVRGUIDList.GetHead();
	}
	m_oLock.Unlock();
	return pMsg;
}

// 取得查询条件
CModMessage * VideoRecordsSeachMgr::GetSearchMessage()
{
	CModMessage * pMsg = nullptr;
	m_oLock.Lock();
	if (0 < m_oUnifiedMsgList.GetSize())
	{
		pMsg = m_oUnifiedMsgList.GetHead();
	}
	m_oLock.Unlock();
	return pMsg;
}

// 删除首个查询信息
void VideoRecordsSeachMgr::RemoveHeadSearchMessage()
{
	m_oLock.Lock();
	if (0 < m_oUnifiedMsgList.GetSize())
	{
		m_oUnifiedMsgList.RemoveHead();
		m_oStreamerGUIDList.RemoveHead();
		m_oNVRGUIDList.RemoveHead();
	}
	m_oLock.Unlock();
}

// 保存查询结果
void VideoRecordsSeachMgr::InserRecord(VideoRecordItem *pRecord)
{
	m_oRecordList.AddTail(pRecord);
}


// 设置设备名
void VideoRecordsSeachMgr::SetDeviceName(const char *pstrDeviceName)
{
	m_strDeviceName = pstrDeviceName;
}

// 取得设备名
CString &VideoRecordsSeachMgr::GetDeviceName()
{
	return m_strDeviceName;
}

