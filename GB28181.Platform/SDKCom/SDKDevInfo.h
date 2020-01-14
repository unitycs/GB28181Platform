#pragma once
#include "Common/TimeLimitMap.h"
#include "Main/UnifiedMessage.h"

class DevicesInfoMgr :private SiteJobWoker
{
public:
	DevicesInfoMgr(void);
	void SetContext(CSDKCom* m_pSDKCom, VmsSiteProxy * pVmsSiteProxy = nullptr, CAllocator<CModMessage> * pMemAllocator = nullptr) override;
	~DevicesInfoMgr(void) = default;

protected:

	// 解码通道信息
	CMapWithLock<CString, LPCSTR, HUSDeviceConnect_T*, HUSDeviceConnect_T*&> m_oDecoderMap;

	// Decoder IP地址到ID的映射
	CMapWithLock<CString, LPCSTR, CString, CString&> m_oDecoderIPtoIDMap;

	// Decoder播放的配对信息
	CTimeLimitMap<CString, LPCSTR, DecoderPairInfo_t, DecoderPairInfo_t&> m_oDecoderPairMap;


	// 正在上线或下线的设备
	CMapWithLock<CString, LPCSTR, INT, INT&> m_oStatusChangeMap;

	GUID m_guidGateway;
	//ECID与DeviceID的map关系.
	CMapWithLock<CString, LPCSTR, GUID, GUID&> m_oECToDevMap;

	static DevicesInfoMgr * m_pCSDKDevInfo;
public:

	void Cleanup();

	// 通道对象信息查询
	static BOOL ChannelLookup(const char *pszKey, DeviceObject &husdeviceInfo);

	// 通道对象信息完成
	static void ChannelLookupEnd();

	// 解码器通道对象信息查询
	BOOL DecoderLookup(const char *pszKey, HUSDeviceConnect_T *pInfo);

	// 解码器通道对象信息查询完成
	void DecoderLookupEnd();

	// 删除解码器和前端设备的匹配对
	BOOL DecoderPairRemove(const char *pszKey, DecoderPairInfo_t &pInfo);

	// 停止设备报警上报
	void RemoveAlarming(const char *pszKey = nullptr);

	// 设置设备在线状态
	void SetOnlineStatusByGUID(const char *pszGUID, int isOffline);

	// 删除设备对象
	void DeleteDeviceObject(const char *pszGUID);

	// 更新设备对应的GBID
	void UpdateGBDeviceIDByGUID(const char *pszGUID);

	// 解码器-前端设备的匹配对过期操作
	void DecoderPairTimeout();

	DeviceChangedMap& GetStatusChangedMap();

	// 报警对象信息查询
	BOOL AlarmLookup(const char *pszKey, DeviceAlarmInfo &pInfo);

	// 报警对象信息查询完成
	void AlarmLookupEnd();

	// 添加报警对象信息
	void AlarmSetAt(const char *pszKey, DeviceAlarmInfo &pInfo, BOOL bIsLock = TRUE);

	// 解码器IP对应的设备GUID查询
	BOOL DecoderIPLookup(const char *pszKey, CString &pszID);

	// 解码器IP对应的设备GUID查询完成
	void DecoderIPLookupEnd();

	// 添加解码器-前端设备匹配对
	void DecoderPairPush(const char *pszKey, DecoderPairInfo_t &info);

	// 设备信息查询
	static BOOL DeviceLookup(const char *pszKey, DeviceObject &pInfo);

	// 设备信息查询完成
	static void DeviceLookupEnd();

	// 添加正在报警的设备信息
	int HandleAlarmInfoFromDevice(const char *pszGUID, const char *pszDescribe, const char *pszLevel /*same with：strAlarmSeverity*/, const char *pszTime, int nAlarmType, int nAlarmMethord, CString& strAlarmStatus);

	// 取得设备参数
	BOOL GetSettingsParam(GUID guidDevice, const WCHAR *pwszParamName, CString &strParmaValue, _ECElementPtr  p_EleSetting = nullptr);

	// 添加设备对象
	BOOL AddDeviceObject(const GUID &guidDevcie);

	// 初始化设备属性
	void IniDeviveProperty(_ECElementPtr ptrECElement);

	// 取得设备对应的EC列表
	ECInfoMap &GetECToDevMap();

	// 解码器视频流绑定
	void DecoderBind(const GUID &guidDecoder, const GUID &guidChannel);

	// 解除解码器视频流绑定
	void DecoderUnbind(const GUID &guidDecoder);

	// GUID转换成GBID
	static void GUIDTranslatedIntoGBID(const GUID &guidDevice, CString &strGBID);

	const CString GetSiteIP();
	const CString GetGatewyGUID(BOOL bIsWithBraces = FALSE);

private:
	std::unordered_map<std::string, std::string> m_DeviceTypeDef;
	//添加到数据存储
	void AddToDataStore(DeviceObject & objec2add, DeviceObject* parentObject = nullptr);
	void FillSubDevInfoByparent(DeviceObject & sub_device, const DeviceObject & parent_device);
	//分层写入HUS站点上设备的基本信息
	void WriteDeviceInfoConfig(const char *pszConfigPath, _bstr_t &bstrType, _bstr_t &bstrName,
		const GUID &guidDevice, const GUID &guidEC,
		const char *pszDevIP, const char *pszDevPort);

	// 写入DVR层的设备对象的基本信息
	void WriteDvrDeviceConfig(const CString &strConfigName, DeviceObject &newDVRDeviceObject);

	// 写入Channel层报警通道的基本信息
	void WriteAlarmChannelConfig(int indexOrder,
		const CString &strConfigName,
		DeviceObject &subAlarmObject);

	// 写入Stremaer层视频通道的基本信息
	HRESULT WriteVideoChannelConfig(int indexOrder,
		const CString &strConfigName,
		DeviceObject &subVideoObject);

	HRESULT WriteNvrVideoChannelConfig(int indexOrder,
		const CString &strConfigName,
		DeviceObject &subVideoObject,
		DeviceObject &Devicetype);

	// 写入解码器通道的基本信息
	void WriteDecorderChannelConfig(int indexOrder,
		const CString &strConfigName,
		DeviceObject &subDisplayObject);
	bool IsMainStreamer(GUID guidDevice);

	std::tuple<GBDevice_T, HUSDevice_T> ParserDevTypeByStrMark(CString & strType, BOOL isDvrLayer = FALSE);

	static void AddHUSDeviceLinkInfo(const TCHAR *pszGUID, HUSDeviceConnect_T& deviceLinkinfo);
	GBID_Creator m_GBID_Creater;

};
