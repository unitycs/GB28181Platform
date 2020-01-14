#include <afxmt.h>
#include "ModuleWithIQ.h"
#include "MemPool.h"
#include "UnifiedMessage.h"
#include "MapWithLock.h"


class CRegManagerCom : public CModuleWithIQ
{
public:
	CRegManagerCom(void);
	virtual ~CRegManagerCom(void);

	virtual void Init(void);
	virtual void Cleanup(void);

	virtual inline const char * GetModuleID()
	{
		static const char	m_szModule[20] = "RTSPCOM";
		return reinterpret_cast<const char *>(m_szModule);
	};

public:
    void ProcessSIP(CUnifiedMessage * pMsg);

protected:
	

protected:	

	// 处理模块消息
	virtual bool HandleMsg(CMemPoolUnit * pUnit);
};
