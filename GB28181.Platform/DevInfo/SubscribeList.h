#pragma once
#include "Memory/MapWithLock.h"

class CSubscribeList
{
public:
	typedef struct SubInfo {
		time_t	exp;
		int		did;
	}SubInfo_t;
	CSubscribeList(void);
	~CSubscribeList(void);
	void Init(const char *pszGWID);

	void Add(const char *pszDeviceID, int nExp, int nDID);
	int Find(const char *pszDeviceID, char *pzsSubscribeID, int &nDID);
	int Del(const char *pszDeviceID);
protected:
	CMapWithLock<CString, LPCSTR, SubInfo_t, SubInfo_t&> m_oSubscribeMap;
private:
	CString  m_strSysSubscribe;
	SubInfo_t m_strSysSubInfo;
};
