#pragma once
#include "RegMgrCom/UASSipRegMgr.h"
#include "Common/Utils.h"
#include "SDPParser/SDPParser.h"
#include "tinyxml/tinyxml2.h"

//进入本域的sip message
class SipMessage
{
public:
	SipMessage()
	{
		p_uri = nullptr;
		p_from_url = nullptr;
		p_to = nullptr;
		p_from = nullptr;
		p_cseq = nullptr;
		p_call_id = nullptr;
		p_header = nullptr;
		p_expires = nullptr;
		p_body = nullptr;


		psz_req_uri_id = nullptr;
		psz_cseq = nullptr;
		psz_from_id = nullptr;
		psz_to_id = nullptr;
		psz_call_id = nullptr;
		psz_subject = nullptr;
		psz_expires = nullptr;
		psz_body = nullptr;


	};
	virtual ~SipMessage() = default;

	osip_uri_t* p_uri;
	osip_uri_t* p_from_url;
	osip_to_t* p_to;
	osip_from_t* p_from;
	osip_cseq_t* p_cseq;
	osip_call_id_t* p_call_id;
	osip_header_t* p_header;
	osip_body_t* p_body;
	osip_header_t* p_expires;

	char* psz_req_uri_id;
	char* psz_cseq;
	char* psz_from_id;
	char* psz_to_id;
	char* psz_call_id;
	char* psz_subject;
	char* psz_expires;
	char* psz_body;

	//get some key fields
	inline SipMessage & parser_message(osip_message_t* p_osip_msg)
	{
		//获取顶部的uri
		p_uri = osip_message_get_uri(p_osip_msg);

		if (p_uri)
		{
			psz_req_uri_id = p_uri->username;
		}

		p_cseq = osip_message_get_cseq(p_osip_msg);

		if (p_cseq)
		{
			psz_cseq = p_cseq->number;
		}
		//获取from
		auto pFrom = osip_message_get_from(p_osip_msg);
		p_from_url = osip_from_get_url(pFrom);
		if (p_from_url)
		{
			psz_from_id = p_from_url->username;
		}

		//获取to
		p_to = osip_message_get_to(p_osip_msg);
		if (p_to&& p_to->url)
		{
			psz_to_id = p_to->url->username;
		}
		p_cseq = osip_message_get_cseq(p_osip_msg);

		//获取video call-id
		p_call_id = osip_message_get_call_id(p_osip_msg);

		if (p_call_id)
		{
			psz_call_id = p_call_id->number;
		}
		//获取subject
		osip_message_get_subject(p_osip_msg, 0, &p_header);

		if (p_header)
		{
			psz_subject = p_header->hvalue;
		}

		osip_message_header_get_byname(p_osip_msg, "Expires", 0, &p_expires);
		if (p_expires)
		{
			psz_expires = p_expires->hvalue;
		}

		//获取SDPbody	
		osip_message_get_body(p_osip_msg, 0, &p_body);

		if (p_body)
		{
			psz_body = p_body->body;
		}
		return (*this);
	}


private:
	using XMLDocument = tinyxml2::XMLDocument;

	inline bool check_body_cmd_type(XMLDocument& xmldoc, const char** ppsz_cmd_type)
	{
		if (!psz_body) return false;
		xmldoc.Parse(psz_body);
		if (xmldoc.Error())
		{
			return false;
		}
		auto xml_root_node = xmldoc.RootElement();
		auto psz_cmdtype = xml_root_node->FirstChildElement("CmdType")->GetText();
		if (!psz_cmdtype) return false;

		*ppsz_cmd_type = psz_cmdtype;
		return true;

	}
	inline  auto parser_cmdtype_in_query_body(const char* str_cmdtype)
	{
		auto result_cmd_type = body_cmd_t::query_t::default_none;
		//查询<query>
		if (Utils::findstr(str_cmdtype, "Catalog"))
		{
			result_cmd_type = body_cmd_t::query_t::query_catalog;
		}
		else if (Utils::findstr(str_cmdtype, "AlarmInfo"))
		{
			result_cmd_type = body_cmd_t::query_t::query_alarm;
		}
		else if (Utils::findstr(str_cmdtype, "RecordInfo"))
		{
			result_cmd_type = body_cmd_t::query_t::query_record_info;
		}
		else if (Utils::findstr(str_cmdtype, "DecoderInfo"))
		{
			result_cmd_type = body_cmd_t::query_t::query_decorder_division;
		}
		else if (Utils::findstr(str_cmdtype, "DecoderStatus"))
		{
			result_cmd_type = body_cmd_t::query_t::query_decorder_status;
		}

		return result_cmd_type;
	}


	inline  auto parser_cmdtype_in_config_body(const char* str_cmdtype)
	{
		auto result_cmd_type = body_cmd_t::config_t::default_none;
		//查询<query>
		if (Utils::findstr(str_cmdtype, "ModifyPassword"))
		{
			result_cmd_type = body_cmd_t::config_t::modify_password;
		}
		return result_cmd_type;
	}

	inline  auto parser_cmdtype_in_response_body(const char* str_cmdtype)
	{

		auto result_cmd_type = body_cmd_t::response_t::default_none;
		//查询<query>
		if (Utils::findstr(str_cmdtype, "Catalog"))
		{
			result_cmd_type = body_cmd_t::response_t::response_catalog;
		}
		else if (Utils::findstr(str_cmdtype, "AlarmInfo"))
		{
			result_cmd_type = body_cmd_t::response_t::response_alarm;
		}
		else if (Utils::findstr(str_cmdtype, "RecordInfo"))
		{
			result_cmd_type = body_cmd_t::response_t::response_record_info;
		}
		else if (Utils::findstr(str_cmdtype, "DecoderInfo"))
		{
			result_cmd_type = body_cmd_t::response_t::response_decorder_division;
		}
		else if (Utils::findstr(str_cmdtype, "DecoderStatus"))
		{
			result_cmd_type = body_cmd_t::response_t::response_decorder_status;
		}

		return result_cmd_type;

	}
	inline  auto parser_cmdtype_in_control_body(const char* str_cmdtype)
	{
		auto result_cmd_type = body_cmd_t::control_t::default_none;
		//控制<control>
		if (Utils::findstr(str_cmdtype, "DeviceControl"))
		{
			result_cmd_type = body_cmd_t::control_t::control_device_ptz;
		}
		else if (Utils::findstr(str_cmdtype, "DragZoom"))
		{
			result_cmd_type = body_cmd_t::control_t::control_dargzoom;
		}
		else if (Utils::findstr(str_cmdtype, "ModifyPassword"))
		{
			result_cmd_type = body_cmd_t::control_t::modify_password;
		}
		return result_cmd_type;
	}
	inline  auto parser_cmdtype_in_notify_body(const char* str_cmdtype)
	{
		auto result_cmd_type = body_cmd_t::notify_t::default_none;
		if (Utils::findstr(str_cmdtype, "Keepalive"))
		{
			result_cmd_type = body_cmd_t::notify_t::notify_keep_alive;
		}
		else if (Utils::findstr(str_cmdtype, "Mediastatus"))
		{
			result_cmd_type = body_cmd_t::notify_t::notify_media_status;
		}

		return result_cmd_type;
	}

public:
	//for message query/response/control/notify
	inline  auto parser_message_body(ipc_sip_block_t* p_sip_block)
	{
		body_cmd_t result_cmd_type;
		XMLDocument xmldoc;
		const char* str_cmdtype = nullptr;
		if (!check_body_cmd_type(xmldoc, &str_cmdtype)) return result_cmd_type;

		auto xml_root_node = xmldoc.RootElement();


		auto str_root_name = xml_root_node->Name();

		if (Utils::findstr(str_root_name, "query"))
		{
			result_cmd_type.content_type = body_content_type_t::BT_QUERY;
			result_cmd_type.u_cases.e_query = parser_cmdtype_in_query_body(str_cmdtype);
		}
		else if (Utils::findstr(str_root_name, "response"))
		{
			result_cmd_type.content_type = body_content_type_t::BT_RESPONSE;
			result_cmd_type.u_cases.e_response = parser_cmdtype_in_response_body(str_cmdtype);
		}
		else if (Utils::findstr(str_root_name, "control"))
		{
			result_cmd_type.content_type = body_content_type_t::BT_CONTROL;
			result_cmd_type.u_cases.e_control = parser_cmdtype_in_control_body(str_cmdtype);
		}
		else if (Utils::findstr(str_root_name, "notify"))
		{
			result_cmd_type.content_type = body_content_type_t::BT_NOTIFY;
			result_cmd_type.u_cases.e_notify = parser_cmdtype_in_notify_body(str_cmdtype);
		}
		else
		{
			//出错了，什么都没解析到
			return  result_cmd_type;
		}


		p_sip_block->cmd_body = result_cmd_type;

		auto psz_sn = xml_root_node->FirstChildElement("SN")->GetText();
		auto psz_deviceid = xml_root_node->FirstChildElement("DeviceID")->GetText();

		memcpy(p_sip_block->szToDeviceID, psz_deviceid, ID_LEN);
		p_sip_block->tHeader.nExtBodySize = p_body->length;
		memcpy(p_sip_block->szSN, psz_sn, CSEQ_LEN);

		return  result_cmd_type;
	}

	//当前会话的request message 是不是keep alive
	inline  bool is_keep_alive()
	{
		XMLDocument xmldoc;
		const char* str_cmdtype = nullptr;
		if (!check_body_cmd_type(xmldoc, &str_cmdtype)) return false;
		auto  e_cmd_type = parser_cmdtype_in_notify_body(str_cmdtype);
		return e_cmd_type == body_cmd_t::notify_t::notify_keep_alive;

	}

	// for subscription
	inline  auto parser_subscription_body(ipc_sip_block_t* p_sip_block)
	{
		auto result_cmd_type = body_cmd_t::subscribe_t::default_none;
		XMLDocument xmldoc;
		const char* str_cmdtype = nullptr;
		if (!check_body_cmd_type(xmldoc, &str_cmdtype)) return result_cmd_type;

		if (Utils::findstr(str_cmdtype, "Catalog"))
		{
			result_cmd_type = body_cmd_t::subscribe_t::subscribe_catalog;
		}
		else if (Utils::findstr(str_cmdtype, "Alarm"))
		{
			result_cmd_type = body_cmd_t::subscribe_t::subscribe_alarm;
		}
		p_sip_block->cmd_body.u_cases.e_subscribe = result_cmd_type;
		//解析消息体
		auto xml_root_node = xmldoc.RootElement();
		auto psz_sn = xml_root_node->FirstChildElement("SN")->GetText();
		auto psz_deviceid = xml_root_node->FirstChildElement("DeviceID")->GetText();
		memcpy(p_sip_block->szToDeviceID, psz_deviceid, ID_LEN);
		p_sip_block->tHeader.nExtBodySize = p_body->length;
		memcpy(p_sip_block->szSN, psz_sn, CSEQ_LEN);

		return  result_cmd_type;
	}

	// for  subscription_notify /
	inline  auto parser_ss_notify_body(ipc_sip_block_t* p_sip_block)
	{
		auto result_cmd_type = body_cmd_t::notify_t::default_none;
		XMLDocument xmldoc;
		const char* str_cmdtype = nullptr;
		if (!check_body_cmd_type(xmldoc, &str_cmdtype)) return result_cmd_type;

		if (Utils::findstr(str_cmdtype, "Catalog"))
		{
			result_cmd_type = body_cmd_t::notify_t::notify_catalog;
		}
		if (Utils::findstr(str_cmdtype, "Alarm"))
		{
			result_cmd_type = body_cmd_t::notify_t::notify_alarm;
		}

		p_sip_block->cmd_body.u_cases.e_notify = result_cmd_type;

		auto xml_root_node = xmldoc.RootElement();
		auto psz_sn = xml_root_node->FirstChildElement("SN")->GetText();
		auto psz_deviceid = xml_root_node->FirstChildElement("DeviceID")->GetText();

		memcpy(p_sip_block->szToDeviceID, psz_deviceid, ID_LEN);
		p_sip_block->tHeader.nExtBodySize = p_body->length;
		memcpy(p_sip_block->work_params.event_notify.szSN, psz_sn, CSEQ_LEN);

		return  result_cmd_type;
	}

	inline void fill_sip_block_base_info(ipc_sip_block_t& to_sip_block)
	{
		memcpy(to_sip_block.szToDeviceID, this->psz_req_uri_id, ID_BUF_LEN); // this is the Default Target DeviceID.
		//memcpy(to_sip_block.szCSeq, this->psz_cseq, CSEQ_LEN);	
		//to_sip_block.nSeq = std::stoi(this->psz_cseq);
		if (this->psz_expires!=NULL)
		{
			memcpy(to_sip_block.szExpiresReason, this->psz_expires, EXPIRES_LEN);
		}
		memcpy(to_sip_block.szFromDeviceID, this->psz_from_id, ID_BUF_LEN);
		memcpy(to_sip_block.work_params.video_call.szSubjectID, this->psz_to_id, ID_LEN);

		if (psz_subject)
		{
			memcpy(to_sip_block.work_params.video_call.szSubjectContent, this->psz_subject, SUBJECT_BUF_LEN);
		}

	}

	inline auto parser_sdp_body()
	{
		return SDPParser(psz_body).GetSDPStack();
	}

	inline std::string getid_from_subject()
	{
		if (!psz_subject) return nullptr;

		std::string str_subject(psz_subject);
		auto ncount = str_subject.find(':');
		return str_subject.substr(0, ncount);

	}


};


class SipMessageMaker
{
public:

	SipMessageMaker(eXosip_t * pContext, SipRegMgr *pRegMgr = nullptr) :
		m_pContext(pContext),
		m_pRegMgr(pRegMgr)
	{
		m_ptarget_domain = nullptr;
		m_pmessage = nullptr;
	}
	~SipMessageMaker() = default;


	inline SipMessageMaker& init_from_to(std::string p_to_deviceid, CSipDomain* targetDomain)
	{
		//44198700004000000000
		//如果ID是域ID则替换,如果ID是设备则先比较realm，再替换。

		if (p_to_deviceid.compare(m_pRegMgr->GetID()) == 0)
		{
			m_str_final_target_id = targetDomain->GetID();
		}
		else
		{
			//发给设备的消息
			m_str_final_target_id = p_to_deviceid;
			auto target_range = p_to_deviceid.substr(0, 8);
			auto target_domain_range = targetDomain->GetRealm();
			//判断目标设备是否在路由的目标域内
			if (target_range.compare(target_domain_range) != 0)
			{
				m_str_final_target_id = target_domain_range + p_to_deviceid.substr(8);
			}
		}
		m_str_sip_to = m_pRegMgr->GetSIPTo(m_str_final_target_id, targetDomain->GetIP(), targetDomain->GetPort());

		m_str_sip_from = m_pRegMgr->GetSIPFrom();

		m_str_reuest_target_id = p_to_deviceid;

		m_ptarget_domain = targetDomain;
		return(*this);
	}


	inline SipMessageMaker& set_sip_to(std::string sip_to_str)
	{
		this->m_str_sip_to = sip_to_str;
		m_str_sip_from = m_pRegMgr->GetSIPFrom();
		return(*this);
	}
	//将消息体内的id替换成目标域内的id.
	//id_to_replace :要替换掉的id.
	inline SipMessageMaker&  replace_id_inbody(const char* psz_body = nullptr)
	{
		//m_str_message_body = psz_body;
		//替换域id
		m_str_message_body = Utils::replace(psz_body, m_pRegMgr->GetID(), m_ptarget_domain->GetID());
		//替换设备id.
		m_str_message_body = Utils::replace(m_str_message_body, m_str_reuest_target_id, m_str_final_target_id);

		return(*this);
	}

	inline  SipMessageMaker& build_new_request_message(const char* psz_method = "MESSAGE")
	{
		// 生成SIP协议的message类型包
		eXosip_message_build_request(m_pContext,
			&m_pmessage,
			psz_method,
			m_str_sip_to.c_str(),
			m_str_sip_from.c_str(),
			nullptr);

		osip_message_set_contact(m_pmessage, m_str_sip_from.c_str());

		return(*this);

	}

	inline  SipMessageMaker& set_message_header(const char* psz_name = nullptr, const char* psz_value = nullptr)
	{
		if (psz_name && psz_value)
		{
			osip_message_set_header(m_pmessage, psz_name, psz_value);
		}
		return(*this);
	}


	inline void set_message_cseq(osip_message_t* p_osip_msg, const char* psz_cseq)
	{
		if (psz_cseq)
		{
			osip_free(p_osip_msg->cseq->number);
			osip_cseq_set_number(p_osip_msg->cseq, osip_strdup(psz_cseq));
		}
	}

	inline void set_message_body(osip_message_t* p_osip_msg, const char* psz_body = nullptr, size_t body_lenth = 0, const char* psz_bodycontent_type = "APPLICATION/MANSCDP+XML")
	{
		if (!m_str_message_body.empty())
		{
			osip_message_set_body(p_osip_msg, m_str_message_body.c_str(), m_str_message_body.length());
			osip_message_set_content_type(p_osip_msg, psz_bodycontent_type);

		}
		else if (psz_body) {
			body_lenth == 0 ? strlen(psz_body) : body_lenth;
			osip_message_set_body(p_osip_msg, psz_body, body_lenth);
			osip_message_set_content_type(p_osip_msg, psz_bodycontent_type);

		}

	}

	inline int build_call_invite_send(const char* pstrSubject, const char* strBody, size_t body_lenth = 0)
	{
		osip_message_t * pInvite = nullptr;
		eXosip_call_build_initial_invite(m_pContext,
			&pInvite,
			m_str_sip_to.c_str(),		//"sip:34020000001320000010@10.10.124.174:12330",	//to
			m_str_sip_from.c_str(),		//"sip:scgw@10.10.124.174:5060",				//from	
			nullptr,															//route
			pstrSubject			//"This is a call for a conversation"			//subject
		);
		osip_message_set_contact(m_pmessage, m_str_sip_from.c_str());
		set_message_body(pInvite, strBody, body_lenth);

		//发出去
		eXosip_lock(m_pContext);
		auto  nRet = eXosip_call_send_initial_invite(m_pContext, pInvite);
		eXosip_unlock(m_pContext);


		return nRet;
	}

	inline int build_call_ack_send(int ntid, int nstatus_code, const char* strBody, size_t body_lenth = 0)
	{
		osip_message_t *answer = nullptr;
		eXosip_call_build_answer(m_pContext, ntid, nstatus_code, &answer);
		set_message_body(answer, strBody, body_lenth);


		eXosip_lock(m_pContext);
		auto nRet = eXosip_call_send_answer(m_pContext, ntid, nstatus_code, answer);
		eXosip_unlock(m_pContext);

		return nRet;
	}

	inline  int build_new_request_message_send(const char* psz_Seq = nullptr, const char* psz_body = nullptr, size_t body_lenth = 0, const char* psz_method = "MESSAGE", const char* psz_body_type = "APPLICATION/MANSCDP+XML")
	{

		build_new_request_message(psz_method);
		set_message_cseq(m_pmessage, psz_Seq);
		set_message_body(m_pmessage, psz_body, body_lenth, psz_body_type);
		return send_request_message();

	}


	inline  int build_new_request_message_send(const int n_Seq = 0, const char* psz_body = nullptr, size_t body_lenth = 0, const char* psz_method = "MESSAGE", const char* psz_body_type = "APPLICATION/MANSCDP+XML")
	{
		auto pstr_cseq = std::to_string(n_Seq);
		return build_new_request_message_send(pstr_cseq.c_str(), psz_body, body_lenth, psz_method, psz_body_type);
	}

	inline  int send_request_message()
	{
		// 发送SIP协议包
		eXosip_lock(m_pContext);
		auto ret = eXosip_message_send_request(m_pContext, m_pmessage);
		eXosip_unlock(m_pContext);
		return ret;
	}




	//构建新的订阅请求
	inline  int build_new_subscription_query_send(const char* psz_cseq = nullptr, const char* psz_experies = nullptr, const char* psz_body = nullptr, size_t body_lenth = 0)
	{
		build_new_request_message("SUBSCRIBE");
		set_message_header("Expires", psz_experies);
		set_message_header("Event", "Catalog");
		set_message_cseq(m_pmessage, psz_cseq);
		set_message_body(m_pmessage, psz_body, body_lenth);
		return send_request_message();
	}

	//在上级订阅的会话内发起新的请求
	inline  int build_insubscription_request_send(int ndid, const char* psz_cseq = nullptr, const char* psz_experies = nullptr, const char* psz_body = nullptr, size_t body_lenth = 0)
	{
		osip_message_t *message = nullptr;

		auto nRet = eXosip_insubscription_build_request(m_pContext, ndid, "SUBSCRIBE", &message);

		set_message_body(message, psz_body, body_lenth, "APPLICATION/MANSCDP+XML");
		// 设置CSeq
		set_message_cseq(message, psz_cseq);

		osip_message_set_header(message, "Event", "Catalog");
		if (psz_experies)
		{
			osip_message_set_header(message, "Expires", psz_experies);
		}
		// 发送SIP协议包
		eXosip_lock(m_pContext);
		nRet = eXosip_insubscription_send_request(m_pContext, ndid, message);
		eXosip_unlock(m_pContext);

		return nRet;

	}

	inline  int build_insubscription_notify_send(int ntid, int ndid, const char* psz_Seq = nullptr, const char* psz_body = nullptr, size_t body_lenth = 0, int ss_status = EXOSIP_SUBCRSTATE_ACTIVE, int reason = TIMEOUT)
	{
		osip_message_t *message = nullptr;

		// 生成SIP协议的message类型包
		auto nRet = eXosip_insubscription_build_notify(m_pContext, ntid, ss_status, reason, &message);
		if (nRet != 0)return -1;

		set_message_body(message, psz_body, body_lenth, "APPLICATION/MANSCDP+XML");
		osip_message_set_header(message, "Event", "presence");
		// 设置CSeq
		set_message_cseq(message, psz_Seq);

		// 发送SIP协议包
		eXosip_lock(m_pContext);
		nRet = eXosip_insubscription_send_request(m_pContext, ndid, message);
		eXosip_unlock(m_pContext);

		return nRet;
	}


	inline  int build_insubscription_notify_send(int ntid, int ndid, const int n_Seq = 0, const char* psz_body = nullptr, size_t body_lenth = 0, int ss_status = EXOSIP_SUBCRSTATE_ACTIVE, int reason = TIMEOUT)
	{
		auto pstr_cseq = std::to_string(n_Seq);
		return	build_insubscription_notify_send(ntid, ndid, pstr_cseq.c_str(), psz_body, body_lenth, ss_status, reason);

	}

	inline  int build_request_message_info_send(int ndid, const char *psz_body, size_t body_lenth = 0)
	{

		osip_message_t *message = nullptr;

		// 生成SIP协议的INFO类型包
		auto nRet = eXosip_call_build_info(m_pContext, ndid, &message);

		if (nRet != 0) return BUILD_ANSWER_ERROR;

		set_message_body(message, psz_body, body_lenth, "Application/MANSRTSP");

		eXosip_lock(m_pContext);
		// 发送SIP协议包
		nRet = eXosip_call_send_request(m_pContext, ndid, message);

		eXosip_unlock(m_pContext);

		return nRet;
	}

	inline int build_message_notify_send(const int n_Seq = 0, const char* strBody = nullptr, size_t body_lenth = 0)
	{
		return build_new_request_message_send(n_Seq, strBody, body_lenth, "NOTIFY");
	}

	inline int build_call_closed_send(int ncid, int ndid)
	{
		if (nullptr == m_pContext)
			return CONTEXT_NOTINIT_ERROR;
		eXosip_lock(m_pContext);
		auto nRet = eXosip_call_terminate(m_pContext, ncid, ndid);
		eXosip_unlock(m_pContext);

		return nRet;
	}

	//maybe 200ok ,with or without body.
	inline int build_message_answer_send(int tid, int ncode, char * psz_content = nullptr, const char* psz_body = nullptr, size_t body_lenth = 0)
	{

		if (nullptr == m_pContext)
			return CONTEXT_NOTINIT_ERROR;

		eXosip_lock(m_pContext);
		osip_message_t *answer = nullptr;

		if (eXosip_message_build_answer(m_pContext, tid, ncode, &answer))
		{
			eXosip_unlock(m_pContext);
			return BUILD_ANSWER_ERROR;
		}
		set_message_body(answer, psz_body, body_lenth, psz_content);

		auto nRet = eXosip_message_send_answer(m_pContext, tid, ncode, answer);

		eXosip_unlock(m_pContext);

		return nRet;

	}

	inline  int build_call_request_message_send(int ndid, const char *psz_body = nullptr, size_t body_lenth = 0, const char* psz_body_type = "Application/MANSRTSP")
	{

		osip_message_t *p_osip_msg = nullptr;

		auto nRet = eXosip_call_build_request(m_pContext, ndid, "MESSAGE", &p_osip_msg);
		if (nRet != 0) return -1;
		set_message_body(p_osip_msg, psz_body, body_lenth, psz_body_type);
		// 发送SIP协议包
		eXosip_lock(m_pContext);
		nRet = eXosip_call_send_request(m_pContext, ndid, p_osip_msg);
		eXosip_unlock(m_pContext);

		return nRet;
	}

private:
	eXosip_t * m_pContext;
	SipRegMgr *m_pRegMgr;
	std::string m_str_sip_to;
	std::string m_str_sip_from;
	std::string m_str_reuest_target_id;
	std::string m_str_final_target_id;
	std::string m_str_message_body;
	CSipDomain* m_ptarget_domain;
	osip_message_t *m_pmessage;

	const char* content_type[APPLICATION_CONTENT_TYPE_COUNT] = {
	  "APPLICATION/SDP",
	  "APPLICATION/MANSCDP+XML",
	  "APPLICATION/MANSRTSP"
	};
};

