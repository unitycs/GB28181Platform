#pragma once
#include "./../DB/db_tabledef.h"
// table record for GBDevice information
struct GBRecordAdaptor : GBDeviceInfo, Record {
	static const		TCHAR * _device_id;
	static const	    TCHAR *	_device_name;
	static const	  	TCHAR *	_device_type;
	static const	  	TCHAR *	_parent_id;
	static const	  	TCHAR *	_model;
	static const	  	TCHAR *	_Parental;
	static const	  	TCHAR *	_ptzType;
	static const	  	TCHAR *	_device_guid;

	CString _table_name_ = _T("Tbl_GBDevice");
	GBRecordAdaptor()
	{
		n_ptzType_ = -1;
		s_device_type_ = _T("Encoder");
		_gb_is_Parental_ = false;
	}

	GBRecordAdaptor(const GBDeviceInfo&& device_info_)
	{
		_gb_device_id_ = device_info_._gb_device_id_;
		n_ptzType_ = device_info_.n_ptzType_;
		s_device_type_ = device_info_.s_device_type_;
		_gb_device_name_ = device_info_._gb_device_name_;
		_gb_is_Parental_ = device_info_._gb_is_Parental_;
	}

	GBRecordAdaptor(const GBDeviceInfo& device_info_)
	{
		_gb_device_id_ = device_info_._gb_device_id_;
		n_ptzType_ = device_info_.n_ptzType_;
		s_device_type_ = device_info_.s_device_type_;
		_gb_device_name_ = device_info_._gb_device_name_;
		_gb_is_Parental_ = device_info_._gb_is_Parental_;
	}

	GBRecordAdaptor(const GBRecordAdaptor& record_info_)
	{
		_gb_device_id_ = record_info_._gb_device_id_;
		n_ptzType_ = record_info_.n_ptzType_;
		s_device_type_ = record_info_.s_device_type_;
		_gb_device_name_ = record_info_._gb_device_name_;
		_gb_is_Parental_ = record_info_._gb_is_Parental_;
	}
	GBRecordAdaptor(const GBRecordAdaptor&& record_info_)
	{
		_gb_device_id_ = record_info_._gb_device_id_;
		n_ptzType_ = record_info_.n_ptzType_;
		s_device_type_ = record_info_.s_device_type_;
		_gb_device_name_ = record_info_._gb_device_name_;
		_gb_is_Parental_ = record_info_._gb_is_Parental_;
		_table_name_ = record_info_._table_name_;
	}

	GBRecordAdaptor& operator=(const GBRecordAdaptor& record_info_)
	{
		_gb_device_id_ = record_info_._gb_device_id_;
		n_ptzType_ = record_info_.n_ptzType_;
		s_device_type_ = record_info_.s_device_type_;
		_gb_device_name_ = record_info_._gb_device_name_;
		_gb_is_Parental_ = record_info_._gb_is_Parental_;
		_table_name_ = record_info_._table_name_;
		return *this;
	}

	BOOL toRequest(Request& rqst_info) override
	{
		rqst_info.fields_.clear();
		rqst_info.table_name_ = _table_name_;

		if (!_gb_device_id_.IsEmpty() && _gb_device_id_.GetLength() != 20) return  FALSE;

		BOOL b_ptztype_invalid = (n_ptzType_ == -1);
		rqst_info.fields_.emplace(_device_id, FieldInfoT(_gb_device_id_, FieldInfo::c_PrimaryKey));
		if (!_gb_device_name_.IsEmpty())
			rqst_info.fields_.emplace(_device_name, FieldInfoT(_gb_device_name_));
		if (!s_device_type_.IsEmpty())
			rqst_info.fields_.emplace(_device_type, FieldInfoT(s_device_type_));
		rqst_info.fields_.emplace(_model, FieldInfoT(FieldInfo::kText));
		if (_gb_is_Parental_)
			rqst_info.fields_.emplace(_Parental, FieldInfoT(_gb_is_Parental_));
		rqst_info.fields_.emplace(_ptzType, b_ptztype_invalid ? FieldInfoT(FieldInfo::kInteger) : FieldInfoT(n_ptzType_));

		return TRUE;
	}

	BOOL fromRequested(Request& rqsted_rslt) override
	{
		if (rqsted_rslt.table_name_.IsEmpty()) return FALSE;
		try
		{
			_table_name_ = rqsted_rslt.table_name_;
			_gb_device_id_ = rqsted_rslt.fields_[_device_id].GetValue();
			_gb_parent_device_id_ = rqsted_rslt.fields_[_parent_id].GetValue();

			s_device_type_ = rqsted_rslt.fields_[_device_type].GetValue();

			auto parental_str = rqsted_rslt.fields_[_Parental].GetValue();

			if (parental_str.CompareNoCase(_T("0")))
				_gb_is_Parental_ = false;
			_gb_is_Parental_ = true;

			auto  _ptzTypeStr = rqsted_rslt.fields_[_ptzType].GetValue();
			if (!_ptzTypeStr.Trim().IsEmpty())
				n_ptzType_ = _ttoi(_ptzTypeStr);
		}
		catch (std::exception ex)
		{
			throw ex.what();
		}

		return TRUE;
	}

};

