#pragma once
#include <memory>
#include "Memory/MemPoolUnit.h"
#include "DevInfo/ChannelInfo.h"
#include "SDKCom/HUSSDKUnified.h"
#include "ModOperation.h"
class CModMessage :public CMemPoolUnit, public sip_packet_t
{
public:
	CModMessage(void);
	~CModMessage(void) = default;
	CModMessage(const CModMessage& msgSrc);
	void operator=(const CModMessage& msgSrc);

	//转换进程间通信的SIP消息数据块
	void FromSipDataBlock(ipc_sip_block_t * p_sip_block);

	// 设置操作类型
	void SetModAction(mod_op_t::u_op_type_t eOperateType);

	// 设置查询类型
	void SetSearchType(const char *pszSearchType);

	// 设置控制指令
	void SetCtrlCmd(const char *pszCtrlCmd);

	void SetSubjectID(const char* pszSubjectID);
	// 设置设备ID
	void SetDeviceID(const char *pszDeviceID);

	// 设置tid
	void SetTID(int n_TID);

	// 设置call dialog ID
	void SetCallDialogID(INT64 n_CallDialogID);

	// 设置新数据
	void SetSearchData(void *pUpdateData);

	// 设置播放数据
	void SetPlayData(void *pPlayData);

	// 设置通知内容
	void SetNotifyData(void *pNotifyData, const char *pszSN = nullptr);

	// 设置SN号
	void SetQuerySN(const char *pszSN);

	// 设置设备类型
	void SetDeviceType(HUSDevice_T eDeviceType);

	// 设置媒体接收端地址
	void SetRecvAddress(const char *pszIP, u_short  uPort);

	// 设置文件查询的开始时间
	void SetSearchStartTime(const char *pszStartTime);

	// 设置文件查询的结束时间
	void SetSearchEndTime(const char *pszEndTime);

	// 设置录像文件查询的录像类型
	void SetRecordType(record_type_t eRecordType);

	// 设置录像查询的地址
	void SetRecordAddr(const char *pszRecordAddr);

	// 设置录像查询的录像位置
	void SetRecLocation(const char *pszRecLocation);

	// 设置播放设备的GUID
	void SetPlayGUID(const char *pszPlayDeviceGUID);

	// 设置更新数据设备的GUID,比如是一個streamerid.
	void SetUpdataGUID(const char *pszDeviceGUID);

	// 设置更新类型
	void SetNotifyUpdataType(event_notify_t::e_update_t eUpdateType);

	// 设置NVR IP
	void SetNVRIP(const char *pszNVRIP);

	//设置subject
	void SetSubject(const char *pszSubject);

	//设置pInviteSender
	void SetCallSender(void *pInviteSender);

	void SetRecordID(const char *recordID);

	const char * GetRecordID() const;

	//取broadcastSrcId信息
	void SetBroadcastSrcID(const char *broadcastSrcId);

	//取broadcastTargetId信息
	void SetBroadcastTargetID(const char *broadcastTargetId);

	// 命令参数
	void SetCmdParam(void *pParam);

	// 设置订阅命令
	void SetSubType(const char *pszCmdType);

	// 设置订阅刷新周期
	void SetSubExpires(const char *pszExpires);

	// 设置订阅结果
	void SetSubResult(int nResult);

	// 取得操作类型
	mod_op_t::u_op_type_t GetModAction() const;

	// 取得控制指令
	char *GetCtrlCmd();

	// 取得查询类型
	char *GetQueryType();

	// 取得文件查询的开始时间
	char *GetSearchStartTime();

	// 取得文件查询的结束时间
	char *GetSearchEndTime();

	char *GetRemoteID();
	// 取得设备信息
	char *GetDeviceID();
	// 取得目标设备ID
	char *GetSubjectID();

	// 取得更新数据
	void *GetSearchData() const;

	// 取得DID
	int GetDID() const;

	// 取得call dialog ID
	INT64 GetCallDialogID() const;

	// 取得播放GUID
	char *GetPlayGUID();

	// 取得更新数据设备的GUID
	char *GetUpdataGUID();

	// 取得更新类型
	event_notify_t::e_update_t GetUpdataType() const;

	// 取得播放数据
	void *GetPlayData() const;

	// 取得通知内容
	void *GetNotifyData(CString *pstrSNBuf = nullptr);

	// 取得SN号
	char *GetQuerySN();

	// 取得设备类型
	HUSDevice_T GetDeviceType() const;

	// 取得媒体接收端IP
	char *GetRecvIP();

	// 取得媒体接收端Port
	int  GetRecvPort();

	// 取得TID
	int GetTID() const;

	// 取得录像类型
	const record_type_t GetRecordType() const;

	// 取得录像地址
	const char *GetRecordAddr() const;

	//取subject信息
	const char *GetSubject() const;

	//取broadcastSrcId信息
	const char *GetBroadcastSrcID() const;

	//取broadcastTargetId信息
	const char *GetBroadcastTargetID() const;

	//获取pInviteSender
	void *GetCallSender() const;

	// 取得录像位置
	const char *GetRecLocation() const;

	// 取得NVR IP
	char *GetNVRIP();

	// 取得命令参数
	void *GetCmdParam() const;

	// 取得订阅命令
	const char *GetSubType() const;

	// 取得订阅周期
	const char *GetSubExpires() const;

	void Free();

	//
	int GetSubResult() const;
private:
	void Clone(const CModMessage* srcUnifiedMessage);
	mod_op_t::u_op_type_t  m_eModAction;					// 操作类型
	void            *m_tParam;
	bool            b_freeParam;
	char		    szGUID[GUID_BUF_LEN];				     // 查询到的设备对应的GUID

public:
	char            m_szNvrIP[IP_BUF_LEN];
	HUSDevice_T	    eHusDevice;						         //VMS设备类型
};
#define  SAFE_FREE_MOD_MSG(pUnifiedMsg);  {\
if(pUnifiedMsg!=nullptr){\
auto pSDPFile = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData()); \
if (pSDPFile){\
pSDPFile->Free();}\
pUnifiedMsg->Free();} }


// 设备对象的状态情况(离在线/报警)
class StateCollections : public DeviceBasicObject
{
public:
	StateCollections()
		: m_eDevType(HUSDevice_T::ENCODER_DVR) {}
	~StateCollections() {}

protected:
	CString m_strOnline;
	CString m_strStatus;
	CString m_strEncode;
	CString m_strRecord;
	char m_szCurDevTime[TIME_BUF_LEN];
	CMapWithLock<CString, LPCSTR, DeviceAlarmInfo, DeviceAlarmInfo&> m_oAlarmStatusMap;
	HUSDevice_T m_eDevType;

public:

	// 设置在线状态
	void SetOnlineStatus(const char * pszOnline)
	{
		m_strOnline = pszOnline;
	}

	// 设置设备状态
	void SetStatus(const char *pszStatus)
	{
		m_strStatus = pszStatus;
	}

	// 设置当前时间
	void SetCurDevTime(const char *pszCurDevTime)
	{
		Utils::StringCpy_s(m_szCurDevTime, TIME_BUF_LEN, pszCurDevTime);
	}

	// 添加报警通道及状态
	void AddAlarm(const char *pszAlarmID, DeviceAlarmInfo oAlarmStatus)
	{
		m_oAlarmStatusMap.SetAt(pszAlarmID, oAlarmStatus);
	}

	// 取得报警通道状态
	DeviceAlarmInfo::DutyStatus GetAlarmStatus(char *pszAlarmID)
	{
		DeviceAlarmInfo oAlarmStatus;
		if (m_oAlarmStatusMap.Lookup(pszAlarmID, oAlarmStatus))
		{
			return oAlarmStatus.eStatus;
		}

		return DeviceAlarmInfo::DutyStatus::OFFDUTY;
	}

	// 设置报警通道状态
	void SetAlarmStatus(const char *pszAlarmID, DeviceAlarmInfo::DutyStatus eDutyStatus)
	{
		DeviceAlarmInfo oAlarmStatus;
		if (m_oAlarmStatusMap.Lookup(pszAlarmID, oAlarmStatus))
		{
			oAlarmStatus.eStatus = eDutyStatus;
			m_oAlarmStatusMap.RemoveKey(pszAlarmID);
			m_oAlarmStatusMap.SetAt(pszAlarmID, oAlarmStatus);
		}
	}

	// 取得下一个报警通道
	void GetNextAlarm(POSITION &pos, CString &strID, DeviceAlarmInfo &oAlamDevInfo)
	{
		m_oAlarmStatusMap.GetNext(pos, strID, oAlamDevInfo);
	}

	// 获取设备状态
	CString GetStatus()
	{
		return m_strStatus;
	}
};

// GB设备对象
struct GBDevInfoDescribe_T
{
	void SetDeviceID(const char *pszDeviceID)
	{
		m_strDeviceID = pszDeviceID;
	}
	const CString & GetDeviceID()
	{
		return m_strDeviceID;
	}
	void SetParentID(const char *pszDeviceID)
	{
		m_strParentID = pszDeviceID;
	}
	const CString & GetParentID()
	{
		return m_strParentID;
	}
	////GUID Handling ////////
	void SetGUID(const char *pszGUID)
	{
		m_strGUID = pszGUID;
	}
	const CString & GetGUID()
	{
		return m_strGUID;
	}

	CDevProperty *GetProperty() { return &m_devInfo; }
	CatalogItem *GetCatalog() { return &m_devCatalog; }
	CatalogCollections *GetSubCatalogs() { return &m_CatalogCollections; }
	DeviceAlarmInfo    *GetAlarmInfo() { return  &m_AlarmInfo; }

protected:

	CDevProperty	m_devInfo;
	CatalogItem		m_devCatalog;
	CatalogCollections	m_CatalogCollections;
	DeviceAlarmInfo       m_AlarmInfo;
	CString			m_strDeviceID;
	CString			m_strParentID;
	// Mork Added.
	CString			m_strGUID;
};

typedef struct _HUSDeviceDescribe_T
{
	// 设备的关联信息，比如EC/NVR的ID。
	HUSDeviceConnect_T linkedInfo;
	// 子设备GUID
	std::unordered_map <std::string, GUID>	 subDevStrGUIDMap;

}HUSDeviceDescribe_T;

// HUS的设备对象，需要注意的是，一个Streamer 一个alarmchannel或PTZChannel都会被抽象成Device.
class  DeviceObject :public DeviceBasicObject
{

	std::shared_ptr<GBDevInfoDescribe_T>   p_GBDevDesInfo;

	std::shared_ptr<HUSDeviceDescribe_T>   p_HUSDevDesInfo;

	//指向包含所有设备状态
	std::shared_ptr<StateCollections>	   p_DevStateInfo;

	typedef struct _DeviceConfig_T_
	{
		//for deviceconfig 主要是用来区分作为GB设备时候是camera还是virAlarm
		CString		strModel;

		//for deviceconfig  [CHANNEL_CATALOG0]
		CString     strSection;

		// 子设备GUID对应的[CHANNEL_CATALOG0]索引
		std::unordered_map<std::string, std::string>	 oSubStrGUIDSecNameMap;
	}DeviceConfig_T;

public:
	// 报警ID
	// 普通channel同时具有报警功能
	CString		   strAlarmID;
	//父设备的的ID
	CString		   strParentID;

	// 父设备的的GUID
	CString		   strParentGUID;

	CString		   strDeviceTyeMark;

	DeviceConfig_T tDevinfoInConf;

	HUSDevice_T eHUSDevType;

	GUID guidDevice;

	//HUS站点上的设备对象GUID字符串
	CString strDeviceGUID;

	//HUS站点上的设备对象的名字
	CString strName;

	//HUS站点上的设备对象的IP
	CString strIP;

	//HUS站点上的设备对象的PORT
	CString strPort;

	// 设备状态信息，离在线状态，报警编解码器信息
	StateCollections	 tDevStateInfoAll;

	// 设备的关联信息，比如EC/NVR的ID。
	HUSDeviceConnect_T linked;


	std::shared_ptr<GBDevInfoDescribe_T> GetGBDescription()
	{

		if (!p_GBDevDesInfo)
		{
			p_GBDevDesInfo = std::make_shared<GBDevInfoDescribe_T>();
		}
		return p_GBDevDesInfo;
	}

	std::shared_ptr<HUSDeviceDescribe_T> GetVMSDescription()
	{
		if (!p_HUSDevDesInfo)
		{
			p_HUSDevDesInfo = std::make_shared<HUSDeviceDescribe_T>();
		}
		return p_HUSDevDesInfo;
	}
	CDevProperty * GetProperty()
	{
		GetGBDescription();
		return  p_GBDevDesInfo->GetProperty();
	}

	DeviceAlarmInfo    *GetAlarmInfo()
	{
		GetGBDescription();
		return  p_GBDevDesInfo->GetAlarmInfo();

	}
	const CString & GetStrGUIDParent()
	{
		return strParentGUID;
	}
};

// 信息改变的设备列表
typedef struct ChangInfo {
	CatalogItem oBefore;
	CatalogItem oAfter;
	int		 nChangeType; //1：GBID变更 2：其他信息变更 3：GBID和其他信息均有变更
}ChangInfo_t;
typedef CList<ChangInfo_t, ChangInfo_t&> InfoChangedList;

class CSDKCom;
class ECClient;
class VmsSiteProxy;

class SiteJobWoker
{
public:
	SiteJobWoker() = default;
	virtual	void SetContext(CSDKCom* p_SDKCom, VmsSiteProxy * pVmsSiteProxy = nullptr, CAllocator<CModMessage> * pMemAllocator = nullptr)
	{
		this->m_pSDKCom = p_SDKCom;
		m_pVmsSiteProxy = pVmsSiteProxy;
		this->m_pMemAllocator = pMemAllocator;

	}


protected:
	CSDKCom                  * m_pSDKCom;
	VmsSiteProxy             * m_pVmsSiteProxy;
	CAllocator<CModMessage>  * m_pMemAllocator;

};
