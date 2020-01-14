#pragma once
#include "SIPCom/SipUnified.h"
#include "SIPCom/CatalogConfig.h"

// 全部的Catalog对象集合
class CatalogCollections : public DeviceBasicObject
{
public:
	CatalogCollections() {}
	~CatalogCollections()
	{
		Clear();
	}

	// 添加设备目录
	void AddCatalog(const char *pszDeviceID, CatalogItem catalog)
	{
		if (nullptr == pszDeviceID || 0 == strlen(pszDeviceID)) return;

		CatalogItem * catalogValue = new CatalogItem;
		*catalogValue = catalog;
		m_catalogMap.SetAt(pszDeviceID, catalogValue);
		CLog::Log(DEVINFO, LL_NORMAL, " %s pszDeviceId = %s deviceId = %s name = %s\r\n", __FUNCTION__, pszDeviceID, catalogValue->GetDeviceID(), catalogValue->GetName());
	}

	CatalogItem* GetCatalogByGUID(const char * pszGUID)  const
	{
		CatalogItem* pCatalog;
		if (TRUE == m_catalogMap.Lookup(pszGUID, pCatalog))
			return pCatalog;

		return nullptr;
	}
	CatalogItem* RemoveCatalog(const char * pszGUID)
	{
		CatalogItem* pCatalog = GetCatalogByGUID(pszGUID);
		m_catalogMap.RemoveKey(pszGUID);

		return pCatalog;
	}

	int GetRecordCount() const
	{
		int nRecordSum = m_catalogMap.GetSize();
		return nRecordSum;
	}

	// return all channel object
	CMap<CString, LPCSTR, CatalogItem*, CatalogItem*>	*GetAllChannel()
	{
		return &m_catalogMap;
	}

	// 删除全部catalog
	void Clear()
	{
		CString strDeviceID;
		CatalogItem *pCatalog = nullptr;
		POSITION pos = m_catalogMap.GetStartPosition();
		while (pos)
		{
			m_catalogMap.GetNextAssoc(pos, strDeviceID, pCatalog);
			if (pCatalog)
			{
				delete pCatalog;
				pCatalog = nullptr;
			}
		}
		m_catalogMap.RemoveAll();
	}

	CatalogItem * RemoveFirst()
	{
		CString strChannelID;
		CMap<CString, LPCSTR, CatalogItem*, CatalogItem*>::CPair *pAir = m_catalogMap.PGetFirstAssoc();
		if (NULL == pAir)
			return nullptr;

		CatalogItem* pCatalog = pAir->value;
		m_catalogMap.RemoveKey(pAir->key);
		return pCatalog;
	}

	CatalogItem* GetNextCatalog(InfoContext_t *pContext) const
	{
		CString strKey;
		CatalogItem	*pCatalog = nullptr;
		int nRecordSum = m_catalogMap.GetSize();

		if (nullptr == pContext->posRecordMap)
		{
			pContext->posRecordMap = m_catalogMap.GetStartPosition();
		}

		if (nRecordSum > pContext->nRawDevSum)
		{
			m_catalogMap.GetNextAssoc(pContext->posRecordMap, strKey, pCatalog);
			pContext->nRawDevSum++;
		}
		return pCatalog;
	}

	// 返回值：剩余Info的数量
	int GetCatalogBodyContent(char *pszInfo, int nBufLen, const char *pstrSN, InfoContext_t *pContext)
	{
		int nRecordSum = m_catalogMap.GetSize();
		if (NULL == pContext->posRecordMap)
		{
			pContext->nRawDevSum = nRecordSum;
			pContext->posRecordMap = m_catalogMap.GetStartPosition();
			// 计算生成 xml文件的个数
			// 每10个记录生成一个xml
			pContext->nSurplus = nRecordSum / MAX_LIST_LEN;
			if (0 < (nRecordSum % MAX_LIST_LEN))
				pContext->nSurplus++;
		}

		CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus);

		// 还有剩余的数据未处理
		if ((0 < nRecordSum && 0 < pContext->nSurplus) || 0 == nRecordSum)
		{
			int nListSum = 0;
			int nCountUsed = 0;
			int nCompleteLen = nBufLen;
			if (MAX_LIST_LEN < pContext->nRawDevSum)
				nListSum = MAX_LIST_LEN;
			else
				nListSum = pContext->nRawDevSum;
			// 创建设备信息文件列表xml的头
			if (ERROR_NOACCESS == m_oBodyBuilder.CreateCatalogHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
				nRecordSum + needAddRecordCount, nListSum))
				goto error;

			for (int nItemIdx = 0; nItemIdx < MAX_LIST_LEN; nItemIdx++)
			{
				nCountUsed += nCompleteLen;
				nCompleteLen = nBufLen - nCountUsed;
				CString strKey;
				CString strFilePath;
				CString strAddress;
				CatalogItem	*pCatalog;

				CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d nCompleteLen = %d posRecordMap = %08x\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus, nCompleteLen, pContext->posRecordMap);

				if (NULL != pContext->posRecordMap)
				{
					m_catalogMap.GetNextAssoc(pContext->posRecordMap, strKey, pCatalog);

					CLog::Log(DEVINFO, LL_NORMAL, " %s pszDeviceId = %s deviceId = %s name = %s\r\n", __FUNCTION__, strKey, pCatalog->GetDeviceID(), pCatalog->GetName());
					// 添加目录信息
					//     pCatalog->SetStatus("OK");

					pCatalog->SetStatus("ON");
					pCatalog->SetOperateType("ADD");

					auto  tmpstr = pCatalog->GetParentID();
					if (tmpstr.IsEmpty() || tmpstr.GetLength() != 20)
					{
						pCatalog->SetParentID(m_strDeviceID);
						pCatalog->m_strChnNum = "0";
					}

					if (ERROR_NOACCESS == m_oBodyBuilder.AddCatalogBody(pszInfo + nCountUsed,
						nCompleteLen, pCatalog))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  AddCatalogBody failure\r\n", __FUNCTION__);

						goto error;
					}

					CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d nCompleteLen = %d nCountUsed = %d posRecordMap = %08x\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus, nCompleteLen, pContext->posRecordMap, nCountUsed);
				}
				else
				{
					nCompleteLen = 0;
				}

				if (NULL == pContext->posRecordMap)
				{
					nCountUsed += nCompleteLen;
					nCompleteLen = nBufLen - nCountUsed;
					// 添加录像文件xml的文件尾
					//		if(ERROR_NOACCESS == m_oBodyBuilder.CompleteRecord(pszInfo+nCountUsed, nCompleteLen))
					//			goto error;
					if (ERROR_NOACCESS == m_oBodyBuilder.CompleteCatalog(pszInfo + nCountUsed, nCompleteLen))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog fail \r\n", __FUNCTION__);

						goto error;
					}
					//     fwrite(pszInfo, 1, strlen(pszInfo), g_testWritelog);

					CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog nItemIdex = %d MAX_LIST_LEN = %d strlen(pszInfo) = %d  pszInfo[0] = %d %d pszInfo = %s last\r\n", __FUNCTION__, nItemIdx, MAX_LIST_LEN, strlen(pszInfo), pszInfo[0], pszInfo[1], pszInfo);
					return 0;
				}
				if (nItemIdx == MAX_LIST_LEN - 1)
				{
					nCountUsed += nCompleteLen;
					nCompleteLen = nBufLen - nCountUsed;
					// 添加录像文件xml的文件尾
					//		if(ERROR_NOACCESS == m_oBodyBuilder.CompleteRecord(pszInfo+nCountUsed, nCompleteLen))
					//			goto error;
					if (ERROR_NOACCESS == m_oBodyBuilder.CompleteCatalog(pszInfo + nCountUsed, nCompleteLen))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog fail \r\n", __FUNCTION__);

						goto error;
					}
					//        fwrite(pszInfo, 1, strlen(pszInfo), g_testWritelog);

					CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog nItemIdex = %d MAX_LIST_LEN = %d strlen(pszInfo) = %d pszInfo = %s\r\n", __FUNCTION__, nItemIdx, MAX_LIST_LEN, strlen(pszInfo), pszInfo);
				}
				pContext->nRawDevSum--;
			}

			pContext->nSurplus--;
			return pContext->nSurplus;
		}
	error:
		return 0;
	}

	int GetNotifyCatalogInfo(char *pszInfo, int nBufLen, const char *pstrSN, InfoContext_t *pContext, char *type) const
	{
		int nRecordSum = m_catalogMap.GetSize();
		if (NULL == pContext->posRecordMap)
		{
			pContext->nRawDevSum = nRecordSum;
			pContext->posRecordMap = m_catalogMap.GetStartPosition();
			// 计算生成 xml文件的个数
			// 每10个记录生成一个xml
			pContext->nSurplus = nRecordSum / MAX_LIST_LEN;
			if (0 < (nRecordSum % MAX_LIST_LEN))
				pContext->nSurplus++;
		}

		CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus);

		// 还有剩余的数据未处理
		if ((0 < nRecordSum && 0 < pContext->nSurplus) || 0 == nRecordSum)
		{
			int nListSum = 0;
			int nCountUsed = 0;
			int nCompleteLen = nBufLen;
			if (MAX_LIST_LEN < pContext->nRawDevSum)
				nListSum = MAX_LIST_LEN;
			else
				nListSum = pContext->nRawDevSum;
			// 创建录像文件列表xml的头
			if (ERROR_NOACCESS == m_oBodyBuilder.CreateNotifyCatalogHead(pszInfo, nCompleteLen, pstrSN, m_strDeviceID.GetString(),
				nRecordSum, nListSum))
				goto error;

			for (int nItemIdx = 0; nItemIdx < MAX_LIST_LEN; nItemIdx++)
			{
				nCountUsed += nCompleteLen;
				nCompleteLen = nBufLen - nCountUsed;
				CString strKey;
				CString strFilePath;
				CString strAddress;
				CatalogItem	*pCatalog;

				CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d nCompleteLen = %d posRecordMap = %08x\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus, nCompleteLen, pContext->posRecordMap);

				if (NULL != pContext->posRecordMap)
				{
					m_catalogMap.GetNextAssoc(pContext->posRecordMap, strKey, pCatalog);

					// 添加目录信息
					std::string status(pCatalog->GetStatus());
					if (status.size() == 0)
					{
						pCatalog->SetStatus("ON");
					}
					//     pCatalog->SetModAction("ADD");
					pCatalog->SetOperateType(type);
					std::string parentId(pCatalog->GetParentID());
					if (parentId.size() == 0)
					{
						pCatalog->SetParentID(m_strDeviceID);
					}
					if (ERROR_NOACCESS == m_oBodyBuilder.AddNotifyCatalogBody(pszInfo + nCountUsed,
						nCompleteLen, pCatalog))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  AddCatalogBody failure\r\n", __FUNCTION__);

						goto error;
					}

					CLog::Log(DEVINFO, LL_NORMAL, " %s buflen = %d nRecordSum = %d nSurplus = %d nCompleteLen = %d nCountUsed = %d posRecordMap = %08x\r\n", __FUNCTION__, nBufLen, nRecordSum, pContext->nSurplus, nCompleteLen, pContext->posRecordMap, nCountUsed);
				}
				else
				{
					nCompleteLen = 0;
				}

				if (NULL == pContext->posRecordMap)
				{
					nCountUsed += nCompleteLen;
					nCompleteLen = nBufLen - nCountUsed;
					// 添加录像文件xml的文件尾
					//		if(ERROR_NOACCESS == m_oBodyBuilder.CompleteRecord(pszInfo+nCountUsed, nCompleteLen))
					//			goto error;
					if (ERROR_NOACCESS == m_oBodyBuilder.CompleteNotifyCatalog(pszInfo + nCountUsed, nCompleteLen))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteNotifyCatalog fail \r\n", __FUNCTION__);

						goto error;
					}
					return 0;
				}

				if (nItemIdx == MAX_LIST_LEN - 1)
				{
					nCountUsed += nCompleteLen;
					nCompleteLen = nBufLen - nCountUsed;
					// 添加录像文件xml的文件尾
					//		if(ERROR_NOACCESS == m_oBodyBuilder.CompleteRecord(pszInfo+nCountUsed, nCompleteLen))
					//			goto error;
					if (ERROR_NOACCESS == m_oBodyBuilder.CompleteNotifyCatalog(pszInfo + nCountUsed, nCompleteLen))
					{
						CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog fail \r\n", __FUNCTION__);

						goto error;
					}
					//        fwrite(pszInfo, 1, strlen(pszInfo), g_testWritelog);

					CLog::Log(DEVINFO, LL_NORMAL, " %s  CompleteCatalog nItemIdex = %d MAX_LIST_LEN = %d strlen(pszInfo) = %d pszInfo = %s\r\n", __FUNCTION__, nItemIdx, MAX_LIST_LEN, strlen(pszInfo), pszInfo);
				}
				pContext->nRawDevSum--;
			}

			pContext->nSurplus--;
			return pContext->nSurplus;
		}

	error:
		return 0;
	}
private:
	// 设备全部的catalog
	CMap<CString, LPCSTR, CatalogItem*, CatalogItem*>	m_catalogMap;
};
