#include "StdAfx.h"
#include "MapStreamerSipID.h"


CMapStreamerSipID::CMapStreamerSipID(void)
{
	InitializeCriticalSectionAndSpinCount( &m_tCS, 4000 );
}

CMapStreamerSipID::~CMapStreamerSipID(void)
{
	DeleteCriticalSection( &m_tCS );
}

bool CMapStreamerSipID::GetPlayBackID(const void* pStreamer, int& nCid, int& nDid)
{
	nCid = -1;
	nDid = -1;
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr != m_MapStreamerID.end())
	{
		nCid = m_MapStreamerID[pStreamer].nCid;
		nDid = m_MapStreamerID[pStreamer].nDid;
		LeaveCriticalSection( &m_tCS );
		return true;
	}
	LeaveCriticalSection( &m_tCS );
	return false;
}

void CMapStreamerSipID::SetPlayBackID(const void* pStreamer, int nCid, int nDid)
{
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr == m_MapStreamerID.end())
	{
		SIPID sipID;
		sipID.nCid = nCid;
		sipID.nDid = nDid;
		sipID.m_nState = 0;
		sipID.fSpeed = 0;
		m_MapStreamerID.insert(MapID::value_type(pStreamer, sipID));
	}
	else
	{
		m_MapStreamerID[pStreamer].nCid = nCid;
		m_MapStreamerID[pStreamer].nDid = nDid;
	}
	LeaveCriticalSection( &m_tCS );
}

void CMapStreamerSipID::SetSpeed(const void* pStreamer, float fSpeed)
{
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr == m_MapStreamerID.end())
	{
		SIPID sipID;
		sipID.nCid = -1;
		sipID.nDid = -1;
		sipID.m_nState = 0;
		sipID.fSpeed = fSpeed;
		m_MapStreamerID.insert(MapID::value_type(pStreamer, sipID));
	}
	else
	{
		m_MapStreamerID[pStreamer].fSpeed = fSpeed;
	}
	LeaveCriticalSection( &m_tCS );
}

bool CMapStreamerSipID::GetSpeed(const void* pStreamer, float& fSpeed)
{
	fSpeed = 0;
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr == m_MapStreamerID.end())
	{
		fSpeed = m_MapStreamerID[pStreamer].fSpeed;
		LeaveCriticalSection( &m_tCS );
		return true;
	}
	LeaveCriticalSection( &m_tCS );
	return false;
}

void CMapStreamerSipID::SetPlayStatus(const void* pStreamer, int nStatus)
{
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr == m_MapStreamerID.end())
	{
		SIPID sipID;
		sipID.nCid = -1;
		sipID.nDid = -1;
		sipID.m_nState = nStatus;
		sipID.fSpeed = 0;
		m_MapStreamerID.insert(MapID::value_type(pStreamer, sipID));
	}
	else
	{
		m_MapStreamerID[pStreamer].m_nState = nStatus;
	}
	LeaveCriticalSection( &m_tCS );
}

bool CMapStreamerSipID::GetPlayStatus(const void* pStreamer, int& nStatus)
{
	nStatus = 0;
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr != m_MapStreamerID.end())
	{
		nStatus = m_MapStreamerID[pStreamer].m_nState;
		LeaveCriticalSection( &m_tCS );
		return true;
	}
	LeaveCriticalSection( &m_tCS );
	return false;
}



void CMapStreamerSipID::RemovePlayBackID(const void* pStreamer)
{
	EnterCriticalSection( &m_tCS );
	MapID::iterator itr = m_MapStreamerID.find(pStreamer);
	if(itr != m_MapStreamerID.end())
	{
		m_MapStreamerID.erase(itr);
	}
	LeaveCriticalSection( &m_tCS );
}

