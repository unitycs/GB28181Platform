#pragma once
#include "db_common.h"

//table virtual base class
class Record {
public:
	Record() = default;
	virtual ~Record()
	{
	}
	//CString _table_name_;
	virtual BOOL toRequest(Request& rqst_info) = 0;
	virtual BOOL fromRequested(Request& rqsted_rslt) = 0;
};

using RECORD_PTR = std::shared_ptr<Record>;
//table informations for one item GBDevice info you can define new class for new table

struct GBDeviceInfo {
	GBDeviceInfo() : s_device_type_(_T("Encoder")), _gb_is_Parental_(false) {
		n_ptzType_ = -1;
		//memset(&_gb_device_id_, 0, sizeof(GUID));
	}

	GBDeviceInfo(GBDeviceInfo&& device_info_) {
		_gb_device_id_ = device_info_._gb_device_id_;
		_gb_device_name_ = device_info_._gb_device_name_;
		n_ptzType_ = device_info_.n_ptzType_;
		s_device_type_ = device_info_.s_device_type_;
		_gb_is_Parental_ = device_info_._gb_is_Parental_;
	}
	GBDeviceInfo(const GBDeviceInfo& device_info_) {
		_gb_device_id_ = device_info_._gb_device_id_;
		n_ptzType_ = device_info_.n_ptzType_;
		s_device_type_ = device_info_.s_device_type_;
		_gb_device_name_ = device_info_._gb_device_name_;
		_gb_is_Parental_ = device_info_._gb_is_Parental_;
	}

	GBDeviceInfo(const CString device_id, const CString gb_device_name_, const CString& device_type, const bool is_Parental = false) {
		_gb_device_id_ = device_id;
		_gb_device_name_ = gb_device_name_;
		n_ptzType_ = -1;
		s_device_type_ = device_type;
		_gb_is_Parental_ = is_Parental;
	}

	GBDeviceInfo& operator=(const GBDeviceInfo& rhs)
	{
		n_ptzType_ = rhs.n_ptzType_;
		_gb_device_id_ = rhs._gb_device_id_;
		s_device_type_ = rhs.s_device_type_;
		return *this;
	}

	friend bool operator==(const GBDeviceInfo& lhs, const GBDeviceInfo& rhs)
	{
		return 	lhs._gb_device_id_.Compare(rhs._gb_device_id_) == 0;
	}

	friend bool operator!=(const GBDeviceInfo& lhs, const GBDeviceInfo& rhs)
	{
		return !(lhs == rhs);
	}
	//record format

	CString   _gb_device_id_;
	CString   _gb_parent_device_id_;
	CString   s_device_type_;
	CString   _gb_device_name_;
	bool	  _gb_is_Parental_;
	LONG       n_ptzType_;
	CString   _husDevice_guid_;
};

//record list of deviceInfo table
typedef std::list<RECORD_PTR> DeviceInfoList;

