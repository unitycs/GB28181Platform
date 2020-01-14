#pragma once
#include "SipServiceCom/SipMsgMaker.hpp"
#include <tuple>

class CSipServiceRealy
{
public:

	CSipServiceRealy(SipRegMgr *pSipRegMgr);
	virtual ~CSipServiceRealy();

	int relay_message(eXosip_event_t* p_Event);

	int relay_message_info(eXosip_event_t * p_Event);

	int relay_message_new(eXosip_event_t * p_Event);

	int relay_message_answered(eXosip_event_t * p_Event);

	int relay_call_invite(eXosip_event_t * p_Event);

	int relay_call_ack(eXosip_event_t * p_Event);

	int relay_call_closed(eXosip_event_t * p_Event);

	int relay_subscription(eXosip_event_t* p_Event);

	int relay_notify(eXosip_event_t * p_Event);


	void Init_relay_context(eXosip_t *pContext);

	CSipDomain * get_dst_domian_by(const char * p_str_from_id);


private:

	//注册管理器
	SipRegMgr *m_pSipRegMgr;
	// 操作用的上下文件指针
	struct eXosip_t *m_pSipContext;
	// 监听到的事件
	eXosip_event_t *m_pEvent;
	DOMAINS_ROUTING_T * m_pRelayRouteTable;

	// 发送函数列表
	using SIP_Realy_FunCall_T = int (CSipServiceRealy::*)(eXosip_event_t*);

	SIP_Realy_FunCall_T m_pRelayFunCall[EXOSIP_EVENT_COUNT] = { nullptr };

};

