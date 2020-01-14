#include "StdAfx.h"
#include "IDMap.h"
#include "SipServiceCom/typedef.h"

CIDMap::CIDMap(void)
{
}

CIDMap::~CIDMap(void)
{
	for(auto itr = m_listIDMap.begin(); itr != m_listIDMap.end(); )
	{
		delete (*itr);
		itr = m_listIDMap.erase(itr);
	}
}

int CIDMap::GetFromCallID(int nToCID, int nToDID, std::string &strFromCallID)
{
	for(auto itr = m_listIDMap.begin(); itr != m_listIDMap.end(); ++itr)
	{
		if((*itr)->nToCID == nToCID && (*itr)->nToDID == nToDID)
		{
			strFromCallID = (*itr)->strFromCallID;
			return 0;
		}
	}
	return NOT_FOUND_FROMID_ERROR;
}

int CIDMap::GetToID(std::string strFromCallID, int &nToCID, int &nToDID)
{
	for(auto itr = m_listIDMap.begin(); itr != m_listIDMap.end(); ++itr)
	{
		if((*itr)->strFromCallID == strFromCallID)
		{
			nToCID = (*itr)->nToCID;
			nToDID = (*itr)->nToDID;
			return 0;
		}
	}
	return NOT_FOUND_TOID_ERROR;
}

id_map_t * CIDMap::GetMapByFromCallID(std::string strFromCallID)
{
	for(auto itr = m_listIDMap.begin(); itr != m_listIDMap.end(); ++itr)
	{
		if((*itr)->strFromCallID == strFromCallID)
		{
			return (*itr);
		}
	}

	return nullptr;
}

int CIDMap::CreateIDMap(std::string strFromCallID, int nToCID, int nToDID)
{
	id_map_t * pMap;
	pMap = new id_map_t();
	if(NULL == pMap)
		return MEMERY_MALLC_ERROR;

	pMap->strFromCallID = strFromCallID;
	pMap->nToCID = nToCID;
	pMap->nToDID = nToDID;

	m_listIDMap.push_back(pMap);
	return 0;
}

int CIDMap::DeleteMapByFromCallID(std::string strFromCallID)
{
	for(auto itr = m_listIDMap.begin(); itr != m_listIDMap.end(); ++itr)
	{
		if((*itr)->strFromCallID == strFromCallID)
		{
			delete (*itr);
			m_listIDMap.erase(itr);
			return 0;
		}
	}

	return NOT_FOUND_MAP_ERROR;
}

// PWolf: Add 2013.06.11
int CIDMap::AddPlayInfo(id_map_paly_info_t &descPlayInfo)
{
	id_map_paly_info_t * pMap;

	pMap = new id_map_paly_info_t();
	if(NULL == pMap)
		return MEMERY_MALLC_ERROR;

	pMap->strCameraID = descPlayInfo.strCameraID;
	pMap->strDecoderID = descPlayInfo.strDecoderID;
	pMap->nCameraCID = descPlayInfo.nCameraCID;
	pMap->nCameraDID = descPlayInfo.nCameraDID;
	pMap->nDecoderCID = descPlayInfo.nDecoderCID;
	pMap->nDecoderDID = descPlayInfo.nDecoderDID;

	m_ListPlayInfoMap.push_back(pMap);
	return 0;
}

int CIDMap::GetPlayInfo(id_map_paly_info_t &descPlayInfo)
{
	for(auto itr = m_ListPlayInfoMap.begin(); itr != m_ListPlayInfoMap.end(); ++itr)
	{

		if( ((*itr)->strCameraID == descPlayInfo.strCameraID) && ((*itr)->strDecoderID == descPlayInfo.strDecoderID) )
		{
			descPlayInfo.nCameraCID = (*itr)->nCameraCID;
			descPlayInfo.nCameraDID = (*itr)->nCameraDID;
			descPlayInfo.nDecoderCID = (*itr)->nDecoderCID;
			descPlayInfo.nDecoderDID = (*itr)->nDecoderDID;
			return 0;
		}
	}
	return NOT_FOUND_TOID_ERROR;
}

int CIDMap::DelPlayInfo(id_map_paly_info_t &descPlayInfo)
{
	for(auto itr = m_ListPlayInfoMap.begin(); itr != m_ListPlayInfoMap.end(); ++itr)
	{
		if( ((*itr)->strCameraID == descPlayInfo.strCameraID) && ((*itr)->strDecoderID == descPlayInfo.strDecoderID) )
		{
			descPlayInfo.nCameraCID = (*itr)->nCameraCID;
			descPlayInfo.nCameraDID = (*itr)->nCameraDID;
			descPlayInfo.nDecoderCID = (*itr)->nDecoderCID;
			descPlayInfo.nDecoderDID = (*itr)->nDecoderDID;
			delete (*itr);
			m_ListPlayInfoMap.erase(itr);
			return 0;
		}
	}

	return NOT_FOUND_MAP_ERROR;
}
// PWolf: Add End

// PWolf: Add 2013.06.17
int CIDMap::GetPlayInfoByCameraID(id_map_paly_info_t &descPlayInfo)
{
	for(auto itr = m_ListPlayInfoMap.begin(); itr != m_ListPlayInfoMap.end(); ++itr)
	{

		if( (*itr)->strCameraID == descPlayInfo.strCameraID )
		{
			descPlayInfo.strCameraID = (*itr)->strCameraID;
			descPlayInfo.strDecoderID = (*itr)->strDecoderID;
			descPlayInfo.nCameraCID = (*itr)->nCameraCID;
			descPlayInfo.nCameraDID = (*itr)->nCameraDID;
			descPlayInfo.nDecoderCID = (*itr)->nDecoderCID;
			descPlayInfo.nDecoderDID = (*itr)->nDecoderDID;
			return 0;
		}
	}
	return NOT_FOUND_TOID_ERROR;
}

int CIDMap::GetPlayInfoByDecoderID(id_map_paly_info_t &descPlayInfo)
{
	for(auto itr = m_ListPlayInfoMap.begin(); itr != m_ListPlayInfoMap.end(); ++itr)
	{

		if( (*itr)->strDecoderID == descPlayInfo.strDecoderID )
		{
			descPlayInfo.strDecoderID = (*itr)->strDecoderID;
			descPlayInfo.strCameraID = (*itr)->strCameraID;
			descPlayInfo.nCameraCID = (*itr)->nCameraCID;
			descPlayInfo.nCameraDID = (*itr)->nCameraDID;
			descPlayInfo.nDecoderCID = (*itr)->nDecoderCID;
			descPlayInfo.nDecoderDID = (*itr)->nDecoderDID;
			return 0;
		}
	}
	return NOT_FOUND_TOID_ERROR;
}

int CIDMap::DelPlayInfoByDecoderID(id_map_paly_info_t &descPlayInfo)
{
	for(auto itr = m_ListPlayInfoMap.begin(); itr != m_ListPlayInfoMap.end(); ++itr)
	{
		if( (*itr)->strDecoderID == descPlayInfo.strDecoderID )
		{
			descPlayInfo.strCameraID = (*itr)->strCameraID;
			descPlayInfo.nCameraCID = (*itr)->nCameraCID;
			descPlayInfo.nCameraDID = (*itr)->nCameraDID;
			descPlayInfo.nDecoderCID = (*itr)->nDecoderCID;
			descPlayInfo.nDecoderDID = (*itr)->nDecoderDID;
			delete (*itr);
			m_ListPlayInfoMap.erase(itr);
			return 0;
		}
	}

	return NOT_FOUND_MAP_ERROR;
}
// PWolf: Add End