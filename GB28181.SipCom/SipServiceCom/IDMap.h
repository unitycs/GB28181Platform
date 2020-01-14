#pragma once
#include <string>
#include <list>

//id map
// PWolf: Add 2013.06.11
typedef struct ID_MAP_PLAY_INFO_T
{
	ID_MAP_PLAY_INFO_T()
	{
		strCameraID = "";
		strDecoderID = "";
		nCameraCID = -1;
		nCameraDID = -1;
		nDecoderCID = -1;
		nDecoderDID = -1;
	}
	std::string strCameraID;
	std::string strDecoderID;
	int nCameraCID;
	int nCameraDID;
	int nDecoderCID;
	int nDecoderDID;
}id_map_paly_info_t;

// PWolf: Add End

typedef struct ID_MAP_T
{
	ID_MAP_T()
	{
		nToCID = -1;
		nToDID = -1;
	}
	std::string strFromCallID;
	int nToCID;
	int nToDID;
	// PWolf: Add 2013.06.11
	id_map_paly_info_t	descPlayInfo;
	// PWolf: Add End
}id_map_t;

class CIDMap
{
public:
	CIDMap(void);
	virtual ~CIDMap(void);

	int GetFromCallID(int nToCID, int nToDID, std::string &strFromCallID);
	int GetToID(std::string strFromCallID, int &nToCID, int &nToDID);
	id_map_t *GetMapByFromCallID(std::string strFromCallID);
	int CreateIDMap(std::string strFromCallID, int nToCID, int nToDID);
	int DeleteMapByFromCallID(std::string strFromCallID);

	// PWolf: Add 2013.06.11
	int AddPlayInfo(id_map_paly_info_t &descPlayInfo);
	int GetPlayInfo(id_map_paly_info_t &descPlayInfo);
	int DelPlayInfo(id_map_paly_info_t &descPlayInfo);
	// PWolf: Add End

	// PWolf: Add 2013.06.17
	int GetPlayInfoByCameraID(id_map_paly_info_t &descPlayInfo);
	int GetPlayInfoByDecoderID(id_map_paly_info_t &descPlayInfo);
	int DelPlayInfoByDecoderID(id_map_paly_info_t &descPlayInfo);
	// PWolf: Add End
protected:
	std::list<id_map_t*> m_listIDMap;
	// PWolf: Add 2013.06.11
	std::list<id_map_paly_info_t*> m_ListPlayInfoMap;
	// PWolf: Add End
};

