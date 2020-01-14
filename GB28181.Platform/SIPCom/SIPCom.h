#pragma once
#include "SIPUnit.h"
#include "Module/ModuleWithIQ.h"
#include "Memory/SharedVarQueue.h"
#include "Main/UnifiedMessage.h"

class CSIPCom : public CModuleWithIQ
{
public:
	CSIPCom(void);
	virtual ~CSIPCom(void) = default;

	void Init(void) override;
	void Cleanup(void) override;

	const TCHAR * GetModuleID() override
	{
		static const TCHAR	m_szModule[20] = _T("SIPCOM");
		return reinterpret_cast<const TCHAR *>(m_szModule);
	};
	friend UINT AFX_CDECL pfnMainThreadProc(LPVOID lParam);
protected:
	// 报警应答超时时间
	const int ALARM_TIMEOUT;

	// 用于输出的共享内存
	CSharedVarQueue  *m_pShareWriteQ;

	// 用于输入的共享内存
	CSharedVarQueue  *m_pShareReadQ;

	// 消息数据内存的管理
	CMemPool m_oModuleMsgMemMgr;

	// Body数据缓存管理
	CMemPool m_oBodyBufMemMgr;

	// 三方平台对象
	CSIPUnit m_oSIPUnit;

	// 线程锁
	CCriticalSection m_oLock;

	// 终止子进程
	void StopSubprocess();

	// 处理共享内存数据
	static UINT pfnSharedQueueProc(LPVOID);

	// 处理模块消息
	bool HandleMsg(CMemPoolUnit * pUnit) override;

	// 读取配置文件
	int ReadConfig();

	// 定时事件包括心跳保活、重注册等
	int RegisterTimerProc();

	// 接受共享内存中的消息
	int HandleShared(void);

	//分发模块间消息
	int DispatchModMessage(CModMessage * p_mod_message = nullptr);

	//分发模块订阅消息
	int DispatchModSSMessage(CModMessage * p_mod_message = nullptr);

	// 发送数据到共享内存
	int SendShared(ipc_sip_block_t * pExPortHeader, const char *pszData, const int nSeq = 0);

	// 进行和远程平台之间的保活
	void Keepalive();

	// 注销
	void Unregister();

	// 订阅计时器
	int SubscribeTimerProc();

private:
	CString m_strRemotePltID;
};
