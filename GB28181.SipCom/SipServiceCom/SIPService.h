#pragma once
#include "SipRoute/SipServiceRelay.h"
#include "Common/ControllableThread.h"
#include "Module/ModuleWithIQ.h"
#include "Memory/SharedVarQueue.h"
class CSIPService : public CModuleWithIQ
{
public:
	CSIPService(void);
	virtual ~CSIPService(void) = default;


	void Init() override;
	void Cleanup() override;

	bool HandleMsg(CMemPoolUnit * pUnit) override;

	const char * GetModuleID() override
	{
		static const char	m_szModule[20] = "SIPService";
		return reinterpret_cast<const char *>(m_szModule);
	};

	// 向平台注册
	int Register(sip_packet_t * pSipPacket);

	// 向平台发送心跳
	int KeepAlive(sip_packet_t * pSipPacket);

	// 从平台注销
	int Unregister(sip_packet_t * pSipPacket);

	// 发送SIP消息
	int SendSIP(sip_packet_t * pSipPacket);

	int  SendAnsweredByCode(int tid, int nCode, char * strContentType = nullptr, const char* strbody = nullptr, size_t lenth = 0) const;

	// 接收SIP消息
	static UINT pfnSIPProc(LPVOID pParam);

	// 把消息推入缓存队列
	static UINT pfnSharedProc(LPVOID pParam);

	// 把消息推送到共享内存
	int SendShared(ipc_sip_block_t * pSipPacket, const char * pszBody = nullptr, const unsigned nBodySize = 0) const;

protected:

	// 读取配置文件
	void InitgGlobalConfigInfo(void);

	// 设置CSeq
	int SetCSeq(osip_message_t * pMessage, const char *pszCSeq);

	// 接收SIP消息
	int HandleSIP(void);

	// 把消息推入缓存队列
	int HandleShared(void);

	// 发送会话内请求（INFO）
	int SendCallInfoRequest(sip_packet_t * pSipPacket);

	int SendCallRequest(sip_packet_t * pSipPacket);

	// 发送Invite的应答指令
	int SendInviteCallAnswer(sip_packet_t * pSipPacket);

	// 发送message
	int SendRequestMessage(sip_packet_t * pSipPacket);

	int SendRequestNotifyMessage(sip_packet_t * pSipPacket);

	// 发送broadcast invite
	int SendInviteBroadCast(sip_packet_t * pSipPacket);

	// 发送broadcast ack
	int SendAckBroadCast(sip_packet_t * pSipPacket);

	// 发送subscribe应答
	int SendSubscribeAnswer(sip_packet_t * pSipPacket);

	// 发送subscribe 终止Notify消息
	int SendSubScribeTerminateNotify(sip_packet_t * pSipPacket);

	int SendSubScribeNotify(sip_packet_t * pSipPacket);

	int SendDataToShared(SipMessage &sip_msg, ipc_sip_block_t & transfer_block);

	int SendCallBye(sip_packet_t * pSipPacket);

	int SendDeviceControl(sip_packet_t * pSipPacket);
	int SendRecordInfoInquiry(sip_packet_t * pRecordData);
	int SendMessages(const char *pstrMsgType, const char *pstrTo, const char *pstrFrom,
		const char *pstrBody, int nBodyLen, const char *pstrBodyType,
		string &strCallID);

	// 处理Message请求
	int HandleMessage(SipMessage &sip_msg, ipc_sip_block_t& transfer_block);

	// for platform
	// 处理Invite消息
	int HandleInvite(SipMessage &sip_msg);

	// 处理ACK消息
	int HandleACK(SipMessage &sip_msg);

	//处理invite 的应答
	int HandleInviteResponse(SipMessage &sip_msg);

	// 处理注册成功消息
	int HandleRegSuccess(SipMessage &sip_msg);

	// 处理注册失败消息
	int HandleRegFail(SipMessage &sip_msg);

	// 处理INFO消息
	int HandleInfo(SipMessage &sip_msg);

	// 处理播放停止消息
	int HandleBye(SipMessage &sip_msg);

	// 处理订阅消息
	int HandleSubscribe(SipMessage &sip_msg, ipc_sip_block_t & transfer_block);

private:
	// 用于线程通知的信号量
	CEvent m_objNotice;

	// 操作用的上下文件指针
	struct eXosip_t *m_pContext_eXosip;

	// 监听到的事件
	eXosip_event_t *m_pEvent;

	// 发送函数列表
	using SIP_FunCall_T = int (CSIPService::*)(sip_packet_t *);

	SIP_FunCall_T m_pSendFun[ST_MAX_COUNT];


	// 共享内存大小
	int m_nSharedMemSize;

	// 用于输出的共享内存
	CSharedVarQueue  *m_pSharedWriteQ;

	// 用于输入的共享内存
	CSharedVarQueue	 *m_pSharedReadQ;

	// 模块路径
	CString strModulePath;

	// 防火墙IP
	CString m_strFirewallIP;

	// 防火墙端口
	int m_nFirewallPort;

	CALLID_SENDER_MAP  m_callIdSenderMap;

	//0下级，1上级，2路由
	int m_nAppWorkMode;

	//CIDMap m_mapID;

	SipRegMgr m_RegManager;

	std::unique_ptr<CSipServiceRealy> m_pSipRelayWorker;

	//解析message消息体中的指令
	body_cmd_t ParseMessageBody(SipMessage &sip_msg, ipc_sip_block_t* p_sip_block = nullptr);

	void fill_sip_block_from_event(ipc_sip_block_t & transfer_block, eXosip_event_t * pEvent);

	INT64 GetCallDialogID(eXosip_event_t * pEvent);

	unordered_map<int, int> M_UniqueIDToCallID;

	unordered_map<int, int> M_SeqIDToCallID;

	UINT m_unSN;
	const char* drag_zoom_string(BYTE type)
	{
		static const char* STRING_MAP[] =
		{
			NULL,
			"DragZoomIn",
			"DragZoomOut"
		};
		return STRING_MAP[type];
	}

};
