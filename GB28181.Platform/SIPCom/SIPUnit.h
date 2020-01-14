#pragma once
#include "BodyBuilder.h"

typedef std::unordered_map<INT64, time_t> MExpiry_t;

enum PlatformStatus
{
	PF_STATUS_UNREGISTERED = 0,
	PF_STATUS_REGISTERING,
	PF_STATUS_REGISTERED,
};

class CSIPUnit
{
public:
	CSIPUnit(void);
	virtual ~CSIPUnit(void);

	int Init(const char *pszPlatformID, int nKeepaliveInterval, int nExpiryInterval);

	// 注册到平台
	int GetRegisterInfo(ipc_sip_block_t * pRegisterData);

	// 发送保活信息
	int GetKeepaliveInfo(ipc_sip_block_t * pRegisterData, char *pszData, int nBufSize, const char *pszGWID);

	// 心跳保活间隔检查
	// 判断是否应该发送保活消息
	// 返回值：0 为到期；1 已到期
	int CheckKeepalive(void);

	// 过期检测，
	// 达到过期时间重新注册
	// 使用和第一次注册连续的Seq
	// 使用和第一次注册相同的Call-ID
	int CheckExpiry(void);

	// 取得远程平台ID
	CString &GetPlatformID();

	// 设置和平台之间的状态
	void  SetPlatformStatus(PlatformStatus ePlatformStatus);

	// 取得和平台之间的状态
	PlatformStatus GetPlatformStatus();

	// 开始应答超时的计时
	void SetResponseTime();

	// 停止应答超时的计时
	void ResetResponseTime();

	// 应答超时检测
	int CheckResponseTime();

	// 判断是否是保活包的应答消息
	BOOL IsLastKeepaliveRes(const int nSeq);

	// 保留最后一次保活包的Seq
	void SetLastKeepaliveCSeq(const int nSeq);

	// 取得过期间隔
	int GetExpiry();

	// 生成CSeq号
	int GenerateCSeq();

	// 
	int AddSubscribeExpires(INT64 nCallID, int nSubExpiryInterval);

	// 检查订阅
	int CheckSubExpiry(MExpiry_t& szTimeOut);

public:
	// 应答超时时间
	const int RESPONSE_TIME;

	// 注册超时时间
	const int REGISTER_TIME;
protected:
	// 符合GB28181规范的平台ID
	CString m_strPFID;

	// 向平台发送带有xml格式的消息时消息的SN号
	USHORT m_unSN;

	// 平台过期的时间点
	time_t m_tmExpiry;

	// 平台保活的时间点
	time_t m_tmKeepalive;

	// 订阅保活时间点
	MExpiry_t m_tmSubExpiry;

	// 平台保活时长，单位秒
	int m_nKeepaliveInterval;

	// 平台过期时长，单位秒
	int m_nExpiryInterval;

	// 是否开始保活
	PlatformStatus m_ePlatformStatus;

	// 消息应答时间点
	time_t m_tmResponse;

	// 线程锁
	CCriticalSection m_oLock;

	// 最后一个保活包的CSeq
	int m_nLastKeepaliveSeq;

	// CSeq号码
	UINT m_unSeq;
protected:

	// 生成新的SN号
	int GenerateSN(CString &strSN);
};

