#pragma once

typedef struct PlaceInfo{
	CString strName;				// 地名
	CString strLongitude;			// 经度
	CString strLatitude;			// 纬度
}PlaceInfo_t;

// 行政区划编码和地域信息的映射
typedef CMap<CString, LPCSTR, PlaceInfo_t, PlaceInfo_t&>	PlaceInfoMap;

// 省和市、市和区县、区县和基层上下级映射
typedef CMap<CString, LPCSTR, CList<CString, LPCSTR>*, CList<CString, LPCSTR>*>			GradeMap;
class CCivilCode
{
public:
	CCivilCode(void);
	~CCivilCode(void);

	void Init(const char *pszFileName);

	// 根据行政区划编码获取地名信息
	BOOL GetPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo) const;

	// 添加新的行政区划编码和地域信息
	int AddCode(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude);
	int AddCode(const char *pszCode, const PlaceInfo_t *ptPlaceInfo);

	// 排序插入
	static void InsertSort(CList<CString, LPCSTR> *pList, const char* pszData);

	// 修正指定行政区划编码对应的地域信息
	void ModifyPlaceInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude);

	// 写入地域信息
	int WriteInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude) const;

	// 读入指定区划编码的地名等信息
	void ReadPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo);
	
	// 创建行政区划编码文件
	static void CreateCodeFile(const char *pszFileName);

	GradeMap		m_oProvinceToCityMap;
	GradeMap		m_oCityToCountyMap;
	GradeMap		m_oCountyToStationMap;
	CString			m_strFileName;
protected:
	static void FormatLength(CString &strData,				// 待格式化的字符串
		char cCharset,							// 填充的字符
		int nDigit);							// 格式化后的长度

protected:
	PlaceInfoMap	m_oPlaceInfoMap;
};

