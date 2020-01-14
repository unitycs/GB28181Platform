#pragma once
#include <afxmt.h>

class CModule : public CObject
{
public:
	CModule(void);
	~CModule(void);

	virtual void Init() = 0;
	virtual void Cleanup() = 0;

	virtual void Startup();

	virtual const char * GetModuleID()
	{
		static const char	m_szModule[20] = "MODULE";
		return reinterpret_cast<const char *>(m_szModule);
	};

	HANDLE	m_hNotice;
	bool	m_bIsExit;			// 通知模块下属线程退出标志位
protected:
	typedef UINT (*THREAD_PROC)(LPVOID);
	class CProcDesc
	{
	public:
		THREAD_PROC		pfn;
		LPVOID			lParam;
		int				count;
	} ;

	CEvent			m_objNotice;		// 用于通知模块总控线程模块处理退出
	int				m_nLogIdx;			// 模块日志号
	CWinThread		* m_pMainThread;	// 模块主控线程对象
	THREAD_PROC		m_pMainCtrlProc;	// 模块主控线程函数指针
	CPtrArray		m_vThreads;			// 存储所有工作线程的对象指针
	CPtrArray		m_vThreadProcs;		// 存储所有工作线程的执行函数信息

	// 注册工作线程函数
	void RegisterProc(THREAD_PROC pfn, LPVOID lParam, int count);

	// 注册主控线程函数
	void RegisterMainProc(THREAD_PROC pfn);

	// 模块主控线程模板
	static UINT AFX_CDECL pfnMainCtrlProc(LPVOID lParam);
};