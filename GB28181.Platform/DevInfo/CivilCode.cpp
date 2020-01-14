#include "StdAfx.h"
#include "CivilCode.h"


CCivilCode::CCivilCode(void)
{
}


CCivilCode::~CCivilCode(void)
{
	POSITION		pos = m_oProvinceToCityMap.GetStartPosition();
	CString			strTmp;
	CList<CString, LPCSTR> *pTmp = nullptr;
	while(pos)
	{
		m_oProvinceToCityMap.GetNextAssoc(pos, strTmp, pTmp);
		delete pTmp;
	}

	pos = m_oCityToCountyMap.GetStartPosition();
	while(pos)
	{
		m_oCityToCountyMap.GetNextAssoc(pos, strTmp, pTmp);
		delete pTmp;
	}

	pos = m_oCountyToStationMap.GetStartPosition();
	while(pos)
	{
		m_oCountyToStationMap.GetNextAssoc(pos, strTmp, pTmp);
		delete pTmp;
	}
}


void CCivilCode::Init(const char *pszFileName)
{
	m_strFileName = pszFileName;
	FILE *pf = nullptr;
	fopen_s(&pf,pszFileName, "r");

	if(nullptr != pf/*TRUE == oFile.Open(pszFileName, CStdioFile::modeRead)*/)
	{
		CString		strData;
		PlaceInfo_t tPlayInfo;
		CString     strCode;
		char		szData[150];

		while(!feof(pf))
		{
			memset(szData, 0, sizeof(szData));
			if(0 == fread(szData, 132, 1, pf))
				continue;

			strData = szData;
			//OutputDebugString(szData);

			// 获取编码
			int nPos = strData.Find(",");
			if(8 != nPos)
			{
				MessageBoxEx(nullptr, "行政区划编码文件格式错误！", "错误", MB_OK,0);
				break;;
			}

			strCode = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);
			
			// 获取地名
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlayInfo.strName = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			if(strData.Left(1) == "\"")
				strData = strData.Right(strData.GetLength()-1);

			if(strData.Right(1) == "\"")
				strData = strData.Left(strData.GetLength()-1);

			// 获取经度
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlayInfo.strLongitude = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			// 获取纬度
			tPlayInfo.strLatitude = strData;

			AddCode(strCode, &tPlayInfo);
		}
		fclose(pf);
	}
}

// 根据行政区划编码获取地名
BOOL CCivilCode::GetPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo) const
{

	if(nullptr == pszCode || 8 < strlen(pszCode))
		return FALSE;

	CString strCode = pszCode;
	FormatLength(strCode, '0', 8);

	return TRUE == m_oPlaceInfoMap.Lookup(strCode, tPlaceInfo);	
}

// 添加新的行政区划编码和地域信息
int CCivilCode::AddCode(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude)
{
	if(nullptr == pszCode || nullptr == pszPlaceName || 8 < strlen(pszCode))
		return -1;

	CString		strCode = pszCode;
	PlaceInfo_t tPlaceInfo;
	FormatLength(strCode, '0', 8);

	if(pszPlaceName)
	{
		tPlaceInfo.strName = pszPlaceName;
		tPlaceInfo.strName.TrimRight();
	}

	if(pszLongitude)
		tPlaceInfo.strLongitude = pszLongitude;
	if(pszLatitude)
		tPlaceInfo.strLatitude = pszLatitude;

	CString strProvince = strCode.Left(2);
	CString strCity		= strCode.Left(4);
	CList<CString, LPCSTR> *pTmp = nullptr;
	if(FALSE == m_oProvinceToCityMap.Lookup(strProvince, pTmp))
	{
		pTmp = new CList<CString, LPCSTR>;
		m_oProvinceToCityMap.SetAt(strProvince, pTmp);
	}
	//pTmp->AddTail(strCity);
	InsertSort(pTmp, strCity);

	CString strCounty	= strCode.Left(6);

	if(FALSE == m_oCityToCountyMap.Lookup(strCity, pTmp))
	{
		pTmp = new CList<CString, LPCSTR>;
		m_oCityToCountyMap.SetAt(strCity, pTmp);
	}
	//pTmp->AddTail(strCounty);
	InsertSort(pTmp, strCounty);

	CString strStation	= strCode;

	if(FALSE == m_oCountyToStationMap.Lookup(strCounty, pTmp))
	{
		pTmp = new CList<CString, LPCSTR>;
		m_oCountyToStationMap.SetAt(strCounty, pTmp);
	}
	if(0 == pTmp->GetCount() || (0 < pTmp->GetCount() && pTmp->GetHead() != "00"))
		//pTmp->AddTail(strStation);
		InsertSort(pTmp, strStation);

	// 如果之前已存在，则覆盖
	m_oPlaceInfoMap.SetAt(pszCode, tPlaceInfo);

	return 0;
}

int CCivilCode::AddCode(const char *pszCode, const PlaceInfo_t *ptPlaceInfo)
{
	int nRet = -1;
	if(ptPlaceInfo)
		nRet =  AddCode(pszCode, ptPlaceInfo->strName, ptPlaceInfo->strLongitude, ptPlaceInfo->strLatitude);

	return nRet;
}

// 修正指定行政区划编码对应的地域信息
void CCivilCode::ModifyPlaceInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude)
{
	PlaceInfo_t tPlaceInfo;
	if(TRUE == m_oPlaceInfoMap.Lookup(pszCode, tPlaceInfo))
	{
		tPlaceInfo.strName		= pszPlaceName;
		if(nullptr != pszLongitude && 0 < strlen(pszLongitude))
			tPlaceInfo.strLongitude = pszLongitude;
		if(nullptr != pszLatitude && 0 < strlen(pszLatitude))
			tPlaceInfo.strLatitude	= pszLatitude;
		m_oPlaceInfoMap.SetAt(pszCode, tPlaceInfo);
	}

	if(0 == WriteInfo(pszCode, pszPlaceName, tPlaceInfo.strLongitude, tPlaceInfo.strLatitude))
		AddCode(pszCode, pszPlaceName, pszLongitude, pszLatitude);
}

// 写入修正后的地域信息
int CCivilCode::WriteInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude) const
{
	int nRet = 0;
	CString		strData;
	FILE *pf = nullptr;
	char		szData[150];
	int			nFlg = 0;

	fopen_s(&pf, m_strFileName, "r+");
	if(pf)
	{
		while(!feof(pf))
		{
			memset(szData, 0 ,sizeof(szData));
			if(0 == fread(szData, 132, 1, pf))
				continue;
			strData = szData;
			int nPos = strData.Find(",");
			if(8 > nPos)
			{	
				nRet = -1;
				break;
			}

			CString strCurCode = strData.Left(nPos);
			strCurCode.TrimRight();
			FormatLength(strCurCode, '0', strlen(pszCode));

			if(0 == strCurCode.Compare(pszCode))
			{
				nFlg = 1;
				break;
			}
			//strData.Empty();
		}

		// 格式化后写入
		CString		strOutput;
		PlaceInfo_t tTmp;

		strOutput = pszCode;
		int nLen = 8 - strOutput.GetLength();
		for(int i = 0; i < nLen; i++)
			strOutput += "0";
		if(pszPlaceName)
			tTmp.strName = pszPlaceName;
		if(pszLongitude)
			tTmp.strLongitude = pszLongitude;
		if(pszLatitude)
			tTmp.strLatitude = pszLatitude;

		FormatLength(tTmp.strName, ' ', 100);

		strOutput += ",";
		strOutput += tTmp.strName + ",";
		FormatLength(tTmp.strLongitude, '0', 10);
		strOutput += tTmp.strLongitude + ",";
		FormatLength(tTmp.strLatitude, '0', 10);
		strOutput += tTmp.strLatitude + "\n";

		// 修改
		if(1 == nFlg)
			fseek(pf, -strData.GetLength()-1, SEEK_CUR);
		// 追加
		else
		{
			// 回车被删除
			if(131 == strData.GetLength())
			{
				// 补回车符
				strOutput = "\n" + strOutput;
			}
		}

		fwrite(strOutput.GetString(), strOutput.GetLength(), 1, pf);
	}

	fclose(pf);
	return nRet;
}

void CCivilCode::FormatLength(CString &strData, char cCharset, int nDigit)
{
	if(strData.GetLength() < nDigit)
	{
		int nLen = nDigit - strData.GetLength();
		for(int i = 0; i < nLen; i++)
			strData += cCharset;
	}
	else if(strData.GetLength() > nDigit)
	{
		strData = strData.Left(nDigit);
	}
}

// 排序插入
void CCivilCode::InsertSort(CList<CString, LPCSTR> *pList, const char* pszData)
{
	if(!pList || !pszData)
		return;

	if(0 == pList->GetCount())
	{
		pList->AddTail(pszData);
	}
	else
	{
		CString		strTmp;
		POSITION pos = pList->GetHeadPosition();
		while(pos)
		{
			strTmp = pList->GetAt(pos);
			if(strTmp > pszData)
			{
				pList->InsertBefore(pos, pszData);
				break;
			}
			else if(strTmp == pszData)
			{
				// 如果之前已存在，则不做操作
				break;
			}
			pList->GetNext(pos);
		}
		if(!pos)
		{
			pList->AddTail(pszData);
		}
	}
}

void CCivilCode::CreateCodeFile(const char *pszFileName)
{
	CStdioFile	oFile;
	CString		strData;
	FILE *pfSimple = nullptr;
	fopen_s(&pfSimple, "C:\\honeywell\\aaaa.csv", "w");

	if(TRUE == oFile.Open(pszFileName, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeReadWrite))
	{
		//CString		strData;
		PlaceInfo_t tPlayInfo;
		CString     strCode;

		while(oFile.ReadString(strData))
		{
			// 获取编码
			int nPos = strData.Find(",");
			if(6 != nPos)
			{
				MessageBoxEx(nullptr, "行政区划编码文件格式错误！", "错误", MB_OK,0);
				break;;
			}
			strCode = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			// 获取地名
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlayInfo.strName = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			if(strData.Left(1) == "\"")
				strData = strData.Right(strData.GetLength()-1);

			if(strData.Right(1) == "\"")
				strData = strData.Left(strData.GetLength()-1);

			// 获取经度
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlayInfo.strLongitude = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			// 获取纬度
			tPlayInfo.strLatitude = strData;



			CString strOutput;
			strCode.TrimRight();
			strCode += "00,";
			strOutput += strCode;
			int nNameLen = tPlayInfo.strName.GetLength();
			for(int i = 0; i < 100 - nNameLen; i++)
				tPlayInfo.strName += " ";

			FormatLength(tPlayInfo.strLongitude, '0', 10);
			FormatLength(tPlayInfo.strLatitude, '0', 10);

			strOutput += tPlayInfo.strName + ",";
			strOutput += tPlayInfo.strLongitude + ",";
			strOutput += tPlayInfo.strLatitude + "\n";
			fwrite(strOutput.GetString(), strOutput.GetLength(), 1, pfSimple);
		}
		oFile.Close();
		fclose(pfSimple);
	}

}

// 读入指定区划编码的地名等信息
void CCivilCode::ReadPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo)
{
	CString strReadCode = pszCode;
	FormatLength(strReadCode, '0', 8);

	FILE *pf = nullptr;
	fopen_s(&pf, m_strFileName, "r");
	
	if(nullptr != pf)
	{
		CString		strData;
		//PlaceInfo_t tPlayInfo;
		CString     strCode;
		char		szData[150];

		while(!feof(pf))
		{
			memset(szData, 0, sizeof(szData));
			if(0 == fread(szData, 132, 1, pf))
				continue;

			strData = szData;
			//OutputDebugString(szData);

			// 获取编码
			int nPos = strData.Find(",");
			if(8 != nPos)
			{
				MessageBoxEx(nullptr, "行政区划编码文件格式错误！", "错误", MB_OK,0);
				break;;
			}

			strCode = strData.Left(nPos);
			if(strCode != strReadCode)
				continue;

			strData = strData.Right(strData.GetLength()-nPos-1);

			// 获取地名
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlaceInfo.strName = strData.Left(nPos);
			tPlaceInfo.strName.TrimRight();
			strData = strData.Right(strData.GetLength()-nPos-1);

			if(strData.Left(1) == "\"")
				strData = strData.Right(strData.GetLength()-1);

			if(strData.Right(1) == "\"")
				strData = strData.Left(strData.GetLength()-1);

			// 获取经度
			nPos = strData.Find(",");
			if(2 > nPos)
				continue;
			tPlaceInfo.strLongitude = strData.Left(nPos);
			strData = strData.Right(strData.GetLength()-nPos-1);

			// 获取纬度
			tPlaceInfo.strLatitude = strData;

			AddCode(strCode, &tPlaceInfo);

			break;
		}
		fclose(pf);
	}
}