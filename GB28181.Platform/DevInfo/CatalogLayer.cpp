#include "stdafx.h"
#include "CatalogLayer.h"
#include "./log/Log.h"
#include "Main/MainThread.h"
#include "Common/common.h"
#include "Main/UnifiedMessage.h"

CCatalogLayer::CCatalogLayer()
{
}

CCatalogLayer::~CCatalogLayer()
{
}

// 加入设备ID创建目录层级结构
int CCatalogLayer::AddDevice(const char *pszDeviceID)
{
	char szProvince[3];
	char szCity[5];
	char szDistrict[7];
	char szStation[9];
	CCatalogList *pList = nullptr;
	CCatalogList *pListTmp = nullptr;
	int nLayer = 0;
	int nGrade = 0;
	int nDeviceGrade = 0;

	m_oLock.Lock();
	// 设备已经添加
	if (TRUE == m_oDeviceQueue.Lookup(pszDeviceID, nGrade))
	{
		CLog::Log(DEVINFO, LL_NORMAL, "重复添加目录层级结构 设备ID;%s 设备级别:%d", pszDeviceID, nGrade);
		nLayer = -1;
		goto error;
	}

	Utils::StringCpy_s(szProvince, 3, pszDeviceID);
	Utils::StringCpy_s(szCity, 5, pszDeviceID);
	Utils::StringCpy_s(szDistrict, 7, pszDeviceID);
	Utils::StringCpy_s(szStation, 9, pszDeviceID);

	// 添加到设备队列
	m_oDeviceQueue.SetAt(pszDeviceID, nDeviceGrade);

	// 省
	if (FALSE == m_oProvinceCount.Lookup(szProvince, pList))
	{
		pList = new CCatalogList;
		m_oProvinceCount.SetAt(szProvince, pList);
		nLayer++;
	}
	pListTmp = pList;
	// 市
	if (FALSE == m_oCityCount.Lookup(szCity, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szCity, nLayer);
		m_oCityCount.SetAt(szCity, pList);
		nLayer++;
	}
	pListTmp = pList;
	// 区/县
	if (FALSE == m_oDistrictCount.Lookup(szDistrict, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szDistrict, nLayer);
		m_oDistrictCount.SetAt(szDistrict, pList);
		nLayer++;
	}
	pListTmp = pList;
	// 基层单位
	if (FALSE == m_oStationCount.Lookup(szStation, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szStation, nLayer);
		m_oStationCount.SetAt(szStation, pList);
		nLayer++;
	}
	pList->SetAt(pszDeviceID, nLayer);
error:
	m_oLock.Unlock();
	return nLayer;
}

// 删除设备及设备对应的目录层级结构
int CCatalogLayer::DelDevice(const char *pszDeviceID)
{
	char szProvince[3];
	char szCity[5];
	char szDistrict[7];
	char szStation[9];
	int nCount = 0;
	int nlayer = 0;
	CCatalogList *pList = nullptr;
	m_oLock.Lock();
	if (FALSE == m_oDeviceQueue.Lookup(pszDeviceID, nCount))
	{
		CLog::Log(DEVINFO, LL_NORMAL, "待删除的目录不存在 目录ID;%s", pszDeviceID);
		nlayer = -1;
		goto error;
	}

	m_oDeviceQueue.RemoveKey(pszDeviceID);

	Utils::StringCpy_s(szProvince, 3, pszDeviceID);
	Utils::StringCpy_s(szCity, 5, pszDeviceID);
	Utils::StringCpy_s(szDistrict, 7, pszDeviceID);
	Utils::StringCpy_s(szStation, 9, pszDeviceID);

	// 基层单位
	if (TRUE == m_oStationCount.Lookup(szStation, pList))
	{
		pList->RemoveKey(pszDeviceID);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oStationCount.RemoveKey(szStation);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -5;
		goto error;
	}

	// 区/县
	if (TRUE == m_oDistrictCount.Lookup(szDistrict, pList))
	{
		pList->RemoveKey(szStation);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oDistrictCount.RemoveKey(szDistrict);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -4;
		goto error;
	}

	// 市
	if (TRUE == m_oCityCount.Lookup(szCity, pList))
	{
		pList->RemoveKey(szDistrict);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oCityCount.RemoveKey(szCity);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -3;
		goto error;
	}

	// 省
	if (TRUE == m_oProvinceCount.Lookup(szProvince, pList))
	{
		pList->RemoveKey(szCity);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oProvinceCount.RemoveKey(szProvince);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -2;
		goto error;
	}
error:
	m_oLock.Unlock();
	return nlayer;
}

// 查询目录层级
int CCatalogLayer::FindDeviceLayer(const char *pszCivilCode, const char *pszGWID, CivilList &oCivilList)
{
	CString			strProvinceID;
	CCatalogList*	pTmp = nullptr;

	m_oLock.Lock();
	if (ID_LEN == strlen(pszCivilCode))
	{
		// 平台目录
		if (0 == _stricmp(pszGWID, pszCivilCode))
		{
			// 遍历所有省份
			POSITION pos = m_oProvinceCount.GetStartPos();
			while (pos)
			{
				m_oProvinceCount.GetNext(pos, strProvinceID, pTmp);
				oCivilList.AddTail(strProvinceID);
				ErgodicRead(strProvinceID, oCivilList);
			}
		}
		else
		{
			int nTmp;
			if (TRUE == m_oDeviceQueue.Lookup(pszCivilCode, nTmp))
				oCivilList.AddTail(pszCivilCode);
		}
	}
	else
	{
		//oCivilList.AddTail(pszCivilCode);
		ErgodicRead(pszCivilCode, oCivilList);
	}
	m_oLock.Unlock();

	return 0;
}

// 便利读取，结构为带有层级关系的列表
// 1.省ID
// 2.	市ID
// 3.     区县ID
// 4.		  派出所ID
// 5.			  设备ID
// 6.	  	      设备ID
// 7.		  派出所ID
// 8.			  设备ID
// 10.	  区县ID
void CCatalogLayer::ErgodicRead(const char *pszCivilCode, CivilList &oCivilList)
{
	CounterMap *pCivilMaps[4] = { &m_oProvinceCount, &m_oCityCount, &m_oDistrictCount, &m_oStationCount };
	int				nLayer = strlen(pszCivilCode) / 2 - 1;
	CCatalogList*	pList;
	CString			strDeviceID;

	if (0 > nLayer || 3 < nLayer)
		return;

	if (TRUE == pCivilMaps[nLayer]->Lookup(pszCivilCode, pList))
	{
		POSITION pos = pList->GetStartPosition();
		int nGrade = 0;
		CString	strRight2;
		while (pos)
		{
			pList->GetNextAssoc(pos, strDeviceID, nGrade);
			// 末尾是00的也算区号？
//			if(0 != strDeviceID.Right(2).Compare("00"))
			oCivilList.AddTail(strDeviceID);

			// 当前查询ID为设备ID，已到达最底层，递归结束
			if (3 > nLayer)
			{
				ErgodicRead(strDeviceID, oCivilList);
			}
		}
	}
}