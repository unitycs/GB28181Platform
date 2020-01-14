#pragma once
#include "RegMgrCom/SipDomain.h"
#include "eXosip2/eXosip.h"
#include "EventTimer.hpp"
class CSIPService;
class SipRegMgr
{
public:
	SipRegMgr(CSIPService *pSipServiceWorker = nullptr);
	~SipRegMgr() = default;

	CSIPService * GetSipServiceWorker();

	void InitSipContext(eXosip_t * pSipContext);

	void SetDevInfoConfigPath(std::string path);

	void AddRegisterObjectInfo(const char * str_DeviceID, const char * str_IP, const unsigned n_Port, unsigned int ntype = SIP_OBJECT_DEVICE, const char * str_Password = "12345678", const char * str_username = nullptr);

	void AddRegisterObjectInfo(SIP_PLT_INFO_T domain_info);

	void InitAllRemoteDomain();

	//添加经过本域的转发路由
	void AddRelayRouting(CSipDomain * domianNode1 = nullptr, CSipDomain * domianNode2 = nullptr);

	CSipDomain&  GetDomain(const char * str_DeviceID);

	const char*  GetID();
	const char*  GetIP();

	int OnRegister(void* xeosipEvent);

	int GetRegisteringStatus(SIP_REGISTER_OBJECT_T &registeringObject);

	void ConnectToRemoteDomains();

	int RegisterTo(CSipDomain & pDomain);

	int OnRegisterToSucess(eXosip_event_t * m_pEvent);

	int UnRegisterFrom(CSipDomain & pDomain);

	int KeepAliveTo(CSipDomain & pDomain);

	int OnKeepAlive(eXosip_event_t * m_pEvent);

	int OnModifyPassword(void* xeosipEvent, std::string xml);

	const std::string & GetSIPFrom();

	//生成串的ID将会指向目标设备
	const std::string  GetSIPTo(std::string target_id, std::string str_ip, int n_port);

	DOMAINS_ROUTING_T * GetRelayRoutings();

	int m_nUseAuth = 1;

	//当前域
	SIP_PLT_INFO_T m_CurPlatform;


	//上级域
	CSipDomain & SipRegMgr::UpperDomain()
	{
		return m_oUpperPLTForm;
	}
	//当前域
	CSipDomain & SipRegMgr::CurrentDomain()
	{
		return m_oCurrentPLTForm;
	}

	//下级域
	CSipDomain & SipRegMgr::LowerDomain()
	{
		return m_oLowerPLTForm;
	}

private:

	void CirclingCheckingTask();

	bool GetRegisterObjectInfo(std::string& id, SIP_REGISTER_OBJECT_T** ppRegisterObjInfo);

	int  DealStatus_Send401(void* p_eXoSipEvent, SIP_REGISTER_OBJECT_T* userInfo);

	void DealStatus_Sent401(void* p_eXoSipEvent, SIP_REGISTER_OBJECT_T* userInfo);

	void DealStatus_Send403(void * p_eXoSipEvent, SIP_REGISTER_OBJECT_T * registeringObject);

	void DealStatus_DefaultRegister(void* p_eXoSipEvent, SIP_REGISTER_OBJECT_T* userInfo);

	bool CheckRegMsgHeader(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T * registeringObject);

	bool CheckMsgHeader_FromTo(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T * registeringObject);

	bool CheckMsgHeader_Contact(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T * registeringObject);

	bool CheckMsgHeader(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T *registeringObject, bool with_contact = true);

	bool CheckRegAuthentication(eXosip_event_t * m_pEvent, SIP_REGISTER_OBJECT_T * registeringObject);

	int  SendRegAnsweredByCode(int tid, int nCode, int nExpires) const;

	int  SendAnsweredByCode(int tid, int nCode, char * strContentType = nullptr, const char* strbody = nullptr, size_t lenth = 0) const;

	std::string GetAuthorizationMessage_qop(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationResponse(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationUsername(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationRealm(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationCnonce(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationUri(osip_authorization_t* pAuth) const;

	std::string GetAuthorizationNonce(osip_authorization_t* pAuth) const;


	bool   ModifyDomainPasswdById(std::string& id, std::string& newPassWord);

	int    ModifyPassword(std::string password);

	bool   ParseModifyuserPassword(std::string & xml, SIP_OBJECT_PASSWD_T & passWordInfo);

	std::string PackModifyPasswordRet(SIP_OBJECT_PASSWD_T & passWordInfo);

	void   GetMd5String(std::string & strMD5, const char * input, int inputlen);

 	SIP_REGISTER_OBJECT_MAP_T m_RegisterObjects; //所有注册对象的描述信息，比如,SIP域,IPC,流媒体,报警设备

	SIP_DOMAIN_MAP_T m_RemoteDomains; //与域描述信息对应的域对象，比如,上级域，下级域，本域

	DOMAINS_ROUTING_T m_RelayRoutingTable; //用于上下级域的转发路由表 


	std::string m_strConfigpath;

	CSipDomain m_oCurrentPLTForm;

	CSipDomain m_oUpperPLTForm;
	CSipDomain m_oLowerPLTForm;

	CEventTimer<std::chrono::seconds> m_CircleTaskWorker;

	eXosip_t*       m_pContext_eXosip;

	CSIPService*    m_pSipWorker;

};


