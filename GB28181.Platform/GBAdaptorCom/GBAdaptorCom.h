#pragma once
#include "Main/UnifiedMessage.h"

class CUASTCP;


class GBAdaptorCom : public CModuleWithIQ
{
public:
	GBAdaptorCom();

	virtual ~GBAdaptorCom() = default;

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("GBAdaptorCom");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};

	static UINT pfnUASPProc(LPVOID pParam);

	virtual void Init() override;
	virtual void Cleanup() override;

	int ProcMessageResponse(CModMessage * pUnifiedMsg);

	int ProcInviteResponse(CModMessage * pUnifiedMsg);

	int ProcCallMessage(CModMessage * pUnifiedMsg);

	int ProcCallByeResponse(CModMessage * pUnifiedMsg);

	int ProcSubscribeResponse(CModMessage * pUnifiedMsg);

	int ProcSubscribeNotify(CModMessage * pUnifiedMsg);

	int ProcRegisterResponse(CModMessage * pUnifiedMsg);


protected:
	void InitgGlobalConfigInfo(void);
	bool HandleMsg(CMemPoolUnit * pUnit) override;

	CAllocator<CModMessage> m_MemAllocator;

private:

	using FORWD_SIP_Call_T = int (GBAdaptorCom::*)(CModMessage * );

	FORWD_SIP_Call_T m_pFwdSipCallFun[ST_MAX_COUNT];

	APP_SETTING_T::PLT_CONFIG_INFO_T m_EndPoint;

	CUASTCP *pUASTCPServer;
	friend class CUASTCP;
};

