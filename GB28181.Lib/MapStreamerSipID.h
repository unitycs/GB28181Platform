#pragma once

#include <map>

struct SIPID
{
	int nCid;
	int nDid;
	float fSpeed;
	int m_nState;
};

typedef std::map<const void*,SIPID> MapID;

class CMapStreamerSipID
{
public:
	CMapStreamerSipID(void);
	~CMapStreamerSipID(void);

	bool GetPlayBackID(const void* pStreamer, int& nCid, int& nDid);
	void SetPlayBackID(const void* pStreamer, int nCid, int nDid);

	void SetSpeed(const void* pStreamer, float fSpeed);
	bool GetSpeed(const void* pStreamer, float& fSpeed);

	void SetPlayStatus(const void* pStreamer, int nStatus);
	bool GetPlayStatus(const void* pStreamer, int& nStatus); 

	void RemovePlayBackID(const void* pStreamer);

private:
	MapID m_MapStreamerID;
	CRITICAL_SECTION m_tCS;
};

