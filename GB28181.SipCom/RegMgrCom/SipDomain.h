#pragma once
#include "Common/common.h"

class CSipDomain :protected SIP_REGISTER_OBJECT_T
{
public:
	CSipDomain() : SIP_REGISTER_OBJECT_T(),
		m_nRegisterID(0),
		m_nCSeq(0),
		m_bNeedConneted(false)
	{}
	virtual ~CSipDomain() = default;

	CSipDomain(const SIP_REGISTER_OBJECT_T& register_obejct);

	void Initialize(const char * str_DeviceID, const char * str_IP, const unsigned n_Port, const char * str_Password = "12345678", const char * str_username = nullptr);

	// 设置注册密码
	void SetNeedConected(bool need_connected)
	{
		m_bNeedConneted = need_connected;
	}

	// 设置注册密码
	void SetPassword(const char *pszPassword)
	{
		strPassWord = pszPassword;
	}

	// 设置过期时间间隔
	void SetExpiry(int nExpiry)
	{
		nExpires = nExpiry;
	}

	//设置注册会话ID
	void SetRegisgerID(int nRegisterID)
	{
		m_nRegisterID = nRegisterID;
	}

	// 设置注册的CallID
	void SetCallID(const char *pszCallID);

	// 生成序列号
	const std::string GetStrCSeq();

	const int GetnSeq();

	// 序列号回零
	void ResetCSeq()
	{
		m_nCSeq = 0;
	}

	// 取得注册密码
	const std::string & GetPassword()
	{
		return strPassWord;
	}

	const std::string &  GetID()
	{
		return strDeviceID;
	}

	const std::string &  GetUsername()
	{
		return userName;
	}

	// 取得过期时间间隔
	int GetExpiry() const
	{
		return nExpires;
	}

	// 取得注册会话ID
	int GetRegisgerID() const
	{
		return m_nRegisterID;
	}

	void AddRouteTo(CSipDomain* domainIterm);

	void RemoveRouteTo(CSipDomain* domainIterm);

	void RemoveRouteTo(std::string domainID);

	// 取得注册的CallID
	const std::string & GetCallID() const
	{
		return m_strCallID;
	}

	// 检测地址合法性
	bool SafeCheck(const char *pszIP)  const
	{
		return	strIP.compare(pszIP) == 0 ? true : false;
	}

	const std::string &  GetIP()  const
	{
		return strIP;
	}

	const unsigned GetPort() const
	{
		return nPort;
	}
	const std::string GetRealm() const
	{
		return strDeviceID.substr(0, 8);
	}

	const std::string GetAuthHeader()  const;

	const std::string & GetSIPFrom();

	const std::string  CSipDomain::GetSIPTo(std::string strDeviceID);


	int GetObjType()
	{
		return nType;
	}

	int NeedSendkeepAlive();
	int NeedSendRegister();

	int NeedSendSubscribe();

	void UpdateStatus(int NewStatus);

	void Fill_SipPacket(sip_packet_t & SipPacket);

protected:
	std::string 	m_strCallID;
	int		        m_nRegisterID;
	int		        m_nCSeq;
	std::string     m_strSipFrom;
	bool            m_bNeedConneted;


private:
	//表明当前域可以去哪里
	DOMAIN_ROUTE_TABLE_T m_RoutingTable;

};

typedef std::unordered_map<std::string, CSipDomain> SIP_DOMAIN_MAP_T;