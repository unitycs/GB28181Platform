#include "StdAfx.h"
#include "SipServiceRelay.h"
#include "Common/Utils.h"

CSipServiceRealy::CSipServiceRealy(SipRegMgr* pSipRegMgr) :
	m_pSipRegMgr(pSipRegMgr), m_pEvent(nullptr)
{
	//m_pRelayFunCall[EXOSIP_CALL_INVITE] = &route_call_conversion;

}

CSipServiceRealy::~CSipServiceRealy()
{
}

void CSipServiceRealy::Init_relay_context(eXosip_t *pContext)
{
	this->m_pSipContext = pContext;
	this->m_pRelayRouteTable = m_pSipRegMgr->GetRelayRoutings();
}


CSipDomain* CSipServiceRealy::get_dst_domian_by(const char* p_str_from_id)
{
	//校验路由关系
	auto  & routing = (*m_pRelayRouteTable);
	if (routing.count(p_str_from_id) == 0) return nullptr;
	//校验通过
	return routing[p_str_from_id];
}


int CSipServiceRealy::relay_message(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;


	//校验通过，创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_new_request_message_send(in_msg.p_cseq->number, in_msg.psz_body, in_msg.p_body->length);

	return nRet;

}


int CSipServiceRealy::relay_message_info(eXosip_event_t* p_Event)
{

	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);
	//校验通过
	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	//创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_request_message_info_send(p_Event->did, in_msg.psz_body, in_msg.p_body->length);

	return nRet;
}

int CSipServiceRealy::relay_message_new(eXosip_event_t* p_Event)
{

	if (MSG_IS_INFO(p_Event->request))
	{
		return relay_message_info(p_Event);
	}
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);
	//校验通过
	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	//创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_new_request_message_send(in_msg.p_cseq->number, in_msg.psz_body, in_msg.p_body->length);

	return nRet;

}

int CSipServiceRealy::relay_message_answered(eXosip_event_t* p_Event)
{

	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);
	//校验通过
	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	//创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_new_request_message_send(in_msg.p_cseq->number, in_msg.psz_body, in_msg.p_body->length);

	return nRet;

}


int CSipServiceRealy::relay_call_invite(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	//创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_call_invite_send(in_msg.psz_subject, in_msg.psz_body);

	return nRet;
}

int CSipServiceRealy::relay_call_ack(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_call_ack_send(p_Event->tid, p_Event->ack->status_code, in_msg.psz_body);

	return  nRet;


}

int CSipServiceRealy::relay_call_closed(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;

	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_call_closed_send(p_Event->cid, p_Event->did);

	return nRet;

}


int CSipServiceRealy::relay_subscription(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;
	//创建消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_new_subscription_query_send(in_msg.p_cseq->number, in_msg.psz_expires, in_msg.psz_body, in_msg.p_body->length);

	return nRet;
}

int CSipServiceRealy::relay_notify(eXosip_event_t* p_Event)
{
	//解析消息
	auto in_msg = SipMessage().parser_message(p_Event->request);

	//校验路由关系
	auto targetDomain = get_dst_domian_by(in_msg.psz_from_id);
	if (!targetDomain) return 0;
	//创建发送消息
	auto nRet = SipMessageMaker(m_pSipContext, m_pSipRegMgr)
		.init_from_to(in_msg.psz_req_uri_id, targetDomain)
		.replace_id_inbody(in_msg.psz_body)
		.build_insubscription_notify_send(p_Event->tid, p_Event->did, in_msg.p_cseq->number, in_msg.psz_body, in_msg.p_body->length);

	return nRet;
}

