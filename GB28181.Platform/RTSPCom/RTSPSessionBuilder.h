#pragma once
#include "Main/UnifiedMessage.h"

enum MediaAction_T
{
	mt_real = 0,
	mt_replay,
	mt_download,
	mt_talk,
	mt_broadcast
};

enum TransProto_T
{
	tt_udp = 0,
	tt_tcp
};

enum MediaData_T
{
	md_video = 0,
	md_audio
};

enum RTSPStatus
{
	rs_null = 0,
	rs_options,
	rs_describe,
	rs_setup,
	rs_play,
	rs_playing,
	rs_pause,
	rs_record,
	rs_recording,
	rs_maxstatus,
	rs_third_200_ret
};

class CRTSPSession;
class CRTSPSessionBuilder
{
public:
	CRTSPSessionBuilder(void);
	~CRTSPSessionBuilder(void) = default;

	// 创建会话对象
	CRTSPSession* CreateLocalCall(CModMessage * pUnifiedMsg);

	CRTSPSession * Create(CModMessage * pUnifiedMsg);


	CRTSPSession* CreateThirdCall(CModMessage * pUnifiedMsg);

	CRTSPSession* CreateBroadcast(CModMessage * pUnifiedMsg);

	// 销毁会话对象
	void Destroy(CRTSPSession *&pSession);

	CModMessage * GetUnifiedMsg();
private:
	void GenerateSSRC(CRTSPSession *pSession);

	void CreateRealRTSPSession(CModMessage * pUnifiedMsg, CRTSPSession ** ppSession = nullptr);
	static DWORD m_dwSSRCBase;
	CModMessage * pRelatedMsg;

protected:
	// Session 对象管理器
	CMemPool m_oSessionMemMgr;
	CString strGateWayId;


};
