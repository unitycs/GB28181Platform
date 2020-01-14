#pragma once
#include "SIPCom/SipUnified.h"
// 录像文件信息
class CModMessage;

class VideoRecordItem : public CMemPoolUnit, public CRecordBase<VideoRecordItem>
{
public:
	// 设置录像类型
	void SetType(record_type_t eRecordType)
	{
		m_eRecordType = eRecordType;
	}

	// 设置录像地址
	void SetAddr(const char *pszRecordAddr)
	{
		Utils::StringCpy_s(m_szRecordAddr, PARAM_NAME_BUF_LEN, pszRecordAddr);
	}

	// 取得录像类型
	record_type_t GetType() const
	{
		return m_eRecordType;
	}

	// 取得录像地址
	const char *GetAddr() const
	{
		return m_szRecordAddr;
	}

	record_type_t	m_eRecordType;
	char		m_szRecordAddr[PARAM_NAME_BUF_LEN];
	char        fileName[PARAM_NAME_BUF_LEN];
	char        filePath[PARAM_NAME_BUF_LEN];
	char        fileSize[PARAM_NAME_BUF_LEN];
	char        recordId[PARAM_NAME_BUF_LEN];
};


class VideoRecordsSeachMgr : public DeviceBasicObject, public RecordQueryWoker<VideoRecordItem>
{
public:
	VideoRecordsSeachMgr() = default;
	~VideoRecordsSeachMgr() = default;
protected:
	// 开始时间-结束时间为key的录像map
	CString					m_strDeviceName;
	CList<CModMessage *> m_oUnifiedMsgList;
	CList<GUID>				m_oStreamerGUIDList;
	CList<GUID>				m_oNVRGUIDList;
	CCriticalSection		m_oLock;

public:
	// 设置查询条件
	void SetSearchMessage(CModMessage * pUnifiedMsg, const GUID &guidStreamer, const GUID &guidNVR);
	// 取得查询条件
	CModMessage *GetSearchMessage(GUID &guidStreamer, GUID &guidNVR);
	// 取得查询条件
	CModMessage *GetSearchMessage();
	// 删除首个查询信息
	void RemoveHeadSearchMessage();

	// 保存查询结果
	void InserRecord(VideoRecordItem *pRecord);
	// 返回值：剩余Info的数量
	int GetCatalogBodyContent(char *pszInfo, int nBufLen, const char *pstrSN, InfoContext_t *pContext);
	// 设置设备名
	void SetDeviceName(const char *pstrDeviceName);

	// 取得设备名
	CString &GetDeviceName();
};

//报警记录，查询到的报警记录和要查询的报警记录
class AlarmRecordItem :public DeviceAlarmInfo, public CRecordBase<AlarmRecordItem>
{
public:
	CString        m_strLongitude;
	CString        m_strLatitude;


	void FromDeviceAlarmInfo(DeviceAlarmInfo& alarminfo)
	{
		auto pthis = static_cast<DeviceAlarmInfo*>(this);
		*pthis = std::move(alarminfo);
		//this->m_strTime = alarminfo.m_strTime;
		//this->m_strPriority = alarminfo.m_strPriority;
		//this->m_nAlarmMethord = alarminfo.m_nAlarmMethord;
		//this->m_strAlarmStatus = alarminfo.m_strAlarmStatus;
		//this->m_nAlarmType = alarminfo.m_nAlarmType;
	}

};


//设备报警信息
class AlarmRecordsMgr : public DeviceBasicObject, public RecordQueryWoker<AlarmRecordItem>
{
public:
	AlarmRecordsMgr() {}
	~AlarmRecordsMgr() {}
protected:
	CString					m_strDeviceName;
	CList<CModMessage *>    m_oUnifiedMsgList;
	CCriticalSection		m_oLock;

public:
	CString                m_deviceId;

	// 设置查询条件
	void SetSearchMessage(CModMessage * pUnifiedMsg)
	{
		m_oLock.Lock();
		m_oUnifiedMsgList.AddTail(pUnifiedMsg);
		m_oLock.Unlock();
	}

	// 取得查询条件
	CModMessage *GetSearchMessage()
	{
		return m_oUnifiedMsgList.GetHead();
	}

	// 删除首个查询信息
	void RemoveHeadSearchMessage()
	{
		m_oLock.Lock();
		if (0 < m_oUnifiedMsgList.GetSize())
		{
			m_oUnifiedMsgList.RemoveHead();
		}
		m_oLock.Unlock();
	}

	// 保存查询结果
	void InserRecord(AlarmRecordItem *pRecord)
	{
		m_oRecordList.AddTail(pRecord);
	}

	// 返回值：剩余Info的数量
	int GetCatalogBodyContent(char *pszInfo, int nBufLen, const char *pstrSN, InfoContext_t *pContext)
	{
		int nRecordSum = m_oRecordList.GetSize();
		AlarmRecordItem	*pRecord = nullptr;
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
				if (ERROR_NOACCESS == m_oBodyBuilder.CreateAlarmRecordHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
					"OK", nRecordSum, nListSum))
					goto error;

				// 添加录像文件xml的文件尾
				if (ERROR_NOACCESS == m_oBodyBuilder.CompleteAlarmRecord(pszInfo + nCompleteLen, nBufLen - nCompleteLen))
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
			nResult = m_oBodyBuilder.CreateAlarmRecordHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
				"OK", nRecordSum, nListSum);
			if (ERROR_NOACCESS == nResult)
				goto error;

			nCountUsed += nCompleteLen;
			nCompleteLen = nBufLen - nCountUsed;

			for (int nItemIdx = 0; nItemIdx < MAX_LIST_LEN; nItemIdx++)
			{
				// 此参数暂时保留
				CString strFilePath;
				pRecord = m_oRecordList.GetNext(pContext->posRecordMap);

				// 添加录像文件
				nResult = m_oBodyBuilder.AddAlarmRecordBody(pszInfo + nCountUsed,
					nCompleteLen,
					m_strDeviceID.GetString(),
					pRecord->m_strPriority,
					std::to_string(pRecord->m_nAlarmMethord).c_str(),
					pRecord->m_strTime,
					pRecord->m_Description,
					pRecord->m_strLongitude,
					pRecord->m_strLatitude,
					std::to_string(pRecord->m_nAlarmType).c_str(),
					pRecord->m_strAlarmStatus
				);

				if (ERROR_NOACCESS == nResult)
					goto error;

				pRecord = nullptr;
				pContext->nRawDevSum--;
				nCountUsed += nCompleteLen;
				nCompleteLen = nBufLen - nCountUsed;

				// 最后一条记录
				if (NULL == pContext->posRecordMap)
				{
					if (ERROR_NOACCESS == m_oBodyBuilder.CompleteAlarmRecord(pszInfo + nCountUsed, nCompleteLen))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog fail \r\n", __FUNCTION__);

						goto error;
					}

					CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog nItemIdex = %d MAX_LIST_LEN = %d strlen(pszInfo) = %d  pszInfo[0] = %d %d pszInfo = %s last\r\n", __FUNCTION__, nItemIdx, MAX_LIST_LEN, strlen(pszInfo), pszInfo[0], pszInfo[1], pszInfo);
					return 0;
				}
			}

			// 添加录像文件xml的文件尾
			nResult = m_oBodyBuilder.CompleteAlarmRecord(pszInfo + nCountUsed, nCompleteLen);
			if (ERROR_NOACCESS == nResult)
				goto error;

			pContext->nSurplus--;
			return pContext->nSurplus;
		}

		return 0;
	error:

		m_oRecordList.RemoveAll();

		return -1;
	}

	// 设置设备名
	void SetDeviceName(const char *pstrDeviceName)
	{
		m_strDeviceName = pstrDeviceName;
	}

	// 取得设备名
	CString &GetDeviceName()
	{
		return m_strDeviceName;
	}
};