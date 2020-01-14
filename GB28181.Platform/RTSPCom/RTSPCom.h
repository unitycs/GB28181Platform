#pragma once
#include "RTSPTransSessionMgr.h"
#include "RTSPTransSessionMgrRec.h"
#include "RTSPTransSessionMgrThird.h"
#include "RTSPTransSessionMgrBroadcast.h"

class CRTSPCom : public CModuleWithIQ
{
public:

	CRTSPCom(void) :m_oResponseMemMgr(sizeof(CBigFile), MEMPOOL_BLOCK_SUM) {};
	virtual ~CRTSPCom(void) = default;

	void Init(void) override;
	void Cleanup(void) override;

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("RTSPCOM");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};

	void ProcessVideoSession(CModMessage * pMsg);

	void ProcessRecordSession(CModMessage * pMsg);

	void StartPlaySession(CModMessage * pUnifiedMsg, mod_op_t::ot_rtsp eOperateType);
protected:
	using  RTSP_CIDSESSION_MGR_T = std::unordered_map<INT64, CRTSPTransMgrBase*>;

	// 保存结果xml的内存管理器
	CMemPool	                  m_oResponseMemMgr;

	RTSP_CIDSESSION_MGR_T	      m_oMapCidSessionMgr; //用以快速区分 broadcast/thirdcall/local
	// 视频会话管理器
	CRTSPTransSessionMgr	      m_oSessionMgrLocal;
	// 录像会话管理器
	CRTSPTransSessionRecMgr		  m_oRecordMgr;

	CRTSPTransSessionMgrThird     m_oThirdCallSessionMgr;

	CRTSPTransSessionMgrBroadcast m_oBroadCastCallSessionMgr;

	// 处理模块消息
	virtual bool HandleMsg(CMemPoolUnit * pUnit) override;

private:
	bool SearchGUIDForPlay(CModMessage * pUnifiedMsg, mod_op_t::ot_rtsp eOperateType);
	BOOL ChannelLookup(const char * pszKey, DeviceObject & husdeviceInfo);
};
