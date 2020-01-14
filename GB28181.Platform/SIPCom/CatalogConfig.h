#pragma once
#include "Common/common.h"
// 静态的设备目录
// 从配置文件中读取
class CatalogItem
{

public:
	enum Status {
		CS_NOCHANGE = -1,
		CS_ADD,
		CS_UPDATE,
		CS_DEL,
		CS_OFF,
		CS_ON,
		CS_VLOST,
		CS_DEFECT
	};
	void SetDeviceID(const CString &strDeviceID) { m_strDeviceID = strDeviceID; }
	void SetName(const CString &strName) { m_strName = strName; }
	void SetManufacturer(const CString &strManufacturer) { m_strManufacturer = strManufacturer; }
	void SetModel(const CString &strModel) { m_strModel = strModel; }
	void SetOwner(const CString &strOwner) { m_strOwner = strOwner; }
	void SetCivilCode(const CString &strCivilCode) { m_strCivilCode = strCivilCode; }
	void SetBlock(const CString &strBlock) { m_strBlock = strBlock; }
	void SetAddress(const CString &strAddress) { m_strAddress = strAddress; }
	void SetParental(const CString &strParental) { m_strParental = strParental; }
	void SetParentID(const CString &strParentID) { m_strParentID = strParentID; }
	void SetGUID(const CString &strGUID) { m_strGUID = strGUID; }
	void SetSafetyWay(const CString &strSafetyWay) { m_strSafetyWay = strSafetyWay; }
	void SetRegisterWay(const CString &strRegisterWay) { m_strRegisterWay = strRegisterWay; }
	// certificate
	void SetCertNum(const CString &strCertNum) { m_strCertNum = strCertNum; }
	void SetCertifiable(const CString &strCertifiable) { m_strCertifiable = strCertifiable; }
	void SetErrcode(const CString &strErrCode) { m_strErrCode = strErrCode; }
	void SetEndTime(const CString &strEndTime) { m_strEndTime = strEndTime; }
	//
	void SetSecrecy(const CString &strSecrecy) { m_strSecrecy = strSecrecy; }
	void SetIPAddress(const CString &strIPAddress) { m_strIPAddress = strIPAddress; }
	void SetPort(const CString &strPort) { m_strPort = strPort; }
	void SetPassword(const CString &strPassword) { m_strPassword = strPassword; }
	void SetStatus(const CString &strStatus) { m_strStatus = strStatus; }
	void SetLongitude(const CString &strLongitude) { m_strLongitude = strLongitude; }
	void SetLatitude(const CString &strLatitude) { m_strLatitude = strLatitude; }
	// info add
	void SetPTZType(const CString &strPTZType) { m_strInfoPTZType = strPTZType; }
	void SetEvent(const CString &strEvent) { m_strEvent = strEvent; }
	void SetPositionType(const CString &strPositionType) { m_strInfoPositionType = strPositionType; }
	void SetRoomType(const CString &strRoomType) { m_strInfoRoomType = strRoomType; }
	void SetUseType(const CString &strUseType) { m_strInfoUseType = strUseType; }
	void SetSupplyLightType(const CString &strSupplyLightType) { m_strInfoSupplyLightType = strSupplyLightType; }
	void SetDirectionType(const CString &strDirectionType) { m_strInfoDirectionType = strDirectionType; }
	void SetResolution(const CString &strResolution) { m_strInfoResolution = strResolution; }
	void SetBusinessGroupID(const CString &strBusinessGroupID) { m_strInfoBusinessGroupID = strBusinessGroupID; }
	// STB add
	void SetResType(const CString &strResType) { m_strResType = strResType; }
	void SetRecLocation(const CString &strRecLocation) { m_strRecLocation = strRecLocation; }
	void SetOperateType(const CString &strOperateType) { m_strOperateType = strOperateType; }
	void SetPlayUrl(const CString &strPlayUrl) { m_strPlayUrl = strPlayUrl; }
	void SetVirtualDirID(const CString &strVirtualDirID) { m_VirtualDirID = strVirtualDirID; }

	void SetType(const GBDevice_T e_DeviceType) { eGBDeviceType = e_DeviceType; }

	CString &GetDeviceID() { return m_strDeviceID; }
	CString &GetName() { return m_strName; }
	CString &GetManufacturer() { return m_strManufacturer; }
	CString &GetModel() { return m_strModel; }
	CString &GetOwner() { return m_strOwner; }
	CString &GetCivilCode() { return m_strCivilCode; }
	CString &GetBlock() { return m_strBlock; }
	CString &GetAddress() { return m_strAddress; }
	CString &GetParental() { return m_strParental; }
	CString &GetParentID() { return m_strParentID; }
	CString &GetGUID() { return m_strGUID; }
	CString &GetSafetyWay() { return m_strSafetyWay; }
	CString &GetRegisterWay() { return m_strRegisterWay; }
	// certificate
	CString &GetCertNum() { return m_strCertNum; }
	CString &GetCertifiable() { return m_strCertifiable; }
	CString &GetErrcode() { return m_strErrCode; }
	CString &GetEndTime() { return m_strEndTime; }
	//    
	CString &GetSecrecy() { return m_strSecrecy; }
	CString &GetIPAddress() { return m_strIPAddress; }
	CString &GetPort() { return m_strPort; }
	CString &GetPassword() { return m_strPassword; }
	CString &GetStatus() { return m_strStatus; }
	CString &GetLongitude() { return m_strLongitude; }
	CString &GetLatitude() { return m_strLatitude; }
	// info add
	CString &GetPTZType() { return m_strInfoPTZType; }
	CString &GetPositionType() { return m_strInfoPositionType; }
	CString &GetRoomType() { return m_strInfoRoomType; }
	CString &GetUseType() { return m_strInfoUseType; }
	CString &GetSupplyLightType() { return m_strInfoSupplyLightType; }
	CString &GetDirectionType() { return m_strInfoDirectionType; }
	CString &GetResolution() { return m_strInfoResolution; }
	CString &GetBusinessGroupID() { return m_strInfoBusinessGroupID; }
	// STB add
	CString &GetResType() { return m_strResType; }
	CString &GetRecLocation() { return m_strRecLocation; }
	CString &GetOperateType() { return m_strOperateType; }
	CString &GetPlayUrl() { return m_strPlayUrl; }
	CString &GetVirtualDirID() { return m_VirtualDirID; }
	CString &GetEvent() { return m_strEvent; }

	GBDevice_T &GetType() { return eGBDeviceType; }

private:
	CString m_strDeviceID;
	CString m_strName;
	CString m_strManufacturer;
	CString m_strModel;
	CString m_strOwner;
	CString m_strCivilCode;
	CString m_strBlock;
	CString m_strAddress;
	CString m_strParental;
	CString m_strParentID;
	CString m_strGUID;
	CString m_strSafetyWay;
	CString m_strRegisterWay;

	// certificate {
	CString m_strCertNum;
	CString m_strCertifiable;
	CString m_strErrCode;
	CString m_strEndTime;
	// }

	CString m_strSecrecy;
	CString m_strIPAddress;
	CString m_strPort;
	CString m_strPassword;
	CString m_strStatus;
	CString m_strLongitude;
	CString m_strLatitude;

	// info {
	CString m_strInfoPTZType;
	CString m_strInfoPositionType;
	CString m_strInfoRoomType;
	CString m_strInfoUseType;
	CString m_strInfoSupplyLightType;
	CString m_strInfoDirectionType;
	CString m_strInfoResolution;
	CString m_strInfoBusinessGroupID;
	// }

	// STB add
	CString m_strResType;
	CString m_strRecLocation;
	CString m_strOperateType;
	CString m_strPlayUrl;

	CString m_VirtualDirID;
	CString m_strEvent;

	GBDevice_T eGBDeviceType;

public:
	CString  m_strChnNum;
};
