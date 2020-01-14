#pragma once
#include <vector>
#include "stdafx.h"
#include <map>
#include <memory>
#include <list>

//Fied info for a colum of a table
struct  FieldInfo {
	FieldInfo() = default;
	enum DataType {
		kNull = 0, //
		kInteger,	//	for long int
		kReal,
		kText, //for guid string
		kBoolean,
		kBlob
		/*
		#define SQLITE_INTEGER  1
		#define SQLITE_FLOAT    2
		#define SQLITE_BLOB     4
		#define SQLITE_NULL     5
		#ifdef SQLITE_TEXT
		# undef SQLITE_TEXT
		#else
		# define SQLITE_TEXT     3
		#endif
		#define SQLITE3_TEXT     3
		*/
	};
	enum Constraint {     //Col_Constraint
		c_NoConstraint = 0, // common filed;
		c_PrimaryKey, // PramiryKey;
		c_IndexKey // indexd Colum;
	};
	DataType			GetType() const { return data_type_; }
	const BOOL&         IsValid() const { return b_valid_; }
	CString				GetValue() const { return field_value_; }
	Constraint			GetConstraint() const { return constraint_; }

protected:
	BOOL          b_valid_;
	//date type
	DataType      data_type_;
	//real field value
	CString       field_value_;
	Constraint    constraint_;
};

class FieldInfoT :public FieldInfo {
public:
	FieldInfoT() = default;
	~FieldInfoT() = default;

	explicit FieldInfoT(DataType data_type)
	{
		b_valid_ = FALSE;
		data_type_ = data_type;
		constraint_ = c_NoConstraint;
	}

	FieldInfoT(const long& long_value, Constraint constraint = c_NoConstraint)
	{
		data_type_ = kInteger;
		CString long_value_field;
		long_value_field.Format(_T("%d"), long_value);
		field_value_ = std::move(long_value_field);
		constraint_ = constraint;
		b_valid_ = TRUE;
	}
	FieldInfoT(const int& int_value, Constraint constraint = c_NoConstraint)
	{
		data_type_ = kInteger;
		CString int_value_field;
		int_value_field.Format(_T("%d"), int_value);
		field_value_ = std::move(int_value_field);
		constraint_ = constraint;
		b_valid_ = TRUE;
	}
	FieldInfoT(const bool& bool_value, Constraint constraint = c_NoConstraint)
	{
		data_type_ = kBoolean;
		if (bool_value)
			field_value_ = _T("1");
		field_value_ = _T("0");
		constraint_ = constraint;
		b_valid_ = TRUE;
	}
	FieldInfoT(const CString& cstring_value, Constraint constraint = c_NoConstraint)
	{
		data_type_ = kText;
		field_value_ = cstring_value;
		constraint_ = constraint;
		b_valid_ = TRUE;
	}
};

typedef  std::map<CString, FieldInfoT> FieldInfoList;

struct FieldCond
{
	CString   _columName;		// DeviceID| ptzType
	CString   _conditon;		// Equal   | Larger | between and
	CString	  _fieldValue;		// "340..1"| 0
	CString  _lowerBound;		//					10
	CString  _upperBound;		//					100
	bool	 _b_Str = false;   /// mark for parental ptztype and so on....
	bool	_b_and_next = false;
};

struct  FieldCondT :FieldCond
{
	FieldCondT() = default;
	~FieldCondT() = default;

	FieldCondT(const TCHAR*  columName, CString conditon, TCHAR*  fieldValue, bool	and_next = true)
	{
		_columName = columName;
		_conditon = conditon;
		_fieldValue = fieldValue;
		_b_Str = true;
		_b_and_next = and_next;
	}
	FieldCondT(const TCHAR*  columName, CString conditon, const INT fieldValue, bool and_next = true)
	{
		_columName = columName;
		_conditon = conditon;
		_fieldValue.Format(_T("%d"), fieldValue);
		_b_and_next = and_next;
	}
	FieldCondT(const TCHAR*  columName, CString conditon, const INT lowerBound, const INT upperBound, bool	and_next = true)
	{
		_columName = columName;
		_conditon = conditon;
		_lowerBound.Format(_T("%d"), lowerBound);
		_upperBound.Format(_T("%d"), upperBound);
		_b_and_next = and_next;
	}
};

using ConditionList = std::list<FieldCondT>;

using FIELD_CONLIST_PTR = std::shared_ptr<ConditionList>;
//ColumName
class FieldHeader :public FieldInfo {
public:
	FieldHeader() = default;
	~FieldHeader() = default;

	FieldHeader(CString s_field_header_name, CString s_data_type, CString b_field_primary_key, CString b_field_index)
	{
		field_value_ = s_field_header_name;
		if (s_data_type.CompareNoCase(_T("ktext")) == NULL)
		{
			data_type_ = FieldInfo::kText;
		}
		else if (s_data_type.CompareNoCase(_T("kInteger")) == NULL)
		{
			data_type_ = FieldInfo::kInteger;
		}
		else if (s_data_type.CompareNoCase(_T("kNull")) == NULL)
		{
			data_type_ = FieldInfo::kNull;
		}
		else if (s_data_type.CompareNoCase(_T("KBoolean")) == NULL)
		{
			data_type_ = FieldInfo::kBoolean;
		}

		if (b_field_primary_key.CompareNoCase(_T("T")) == NULL)
		{
			constraint_ = c_PrimaryKey;
		}
		else if (b_field_index.CompareNoCase(_T("T")) == NULL)
		{
			constraint_ = c_IndexKey;
		}
		else
		{
			constraint_ = c_NoConstraint;
		}
	}

	FieldHeader(FieldInfo::DataType data_type, CString s_field_header_name, BOOL b_field_primary_key, BOOL b_field_index)
	{
		field_value_ = s_field_header_name;
		constraint_ = b_field_primary_key ? c_PrimaryKey : (b_field_index ? c_IndexKey : c_NoConstraint);
		data_type_ = data_type;
	}

	CString GetFieldName() const { return field_value_; }
	FieldInfo::DataType GetFieldType() const { return data_type_; }
	BOOL isPrimaryKey() const { return constraint_ == c_PrimaryKey; }
	BOOL isIndex() const { return constraint_ != c_NoConstraint; }
};

typedef  std::vector<FieldHeader> FieldHeaderList;

class TableStructInfo
{
public:
	TableStructInfo() = default;
	~TableStructInfo() = default;

	TableStructInfo(CString  _new_table_name, FieldHeaderList && _header_list_)
	{
		s_table_name_ = _new_table_name;
		field_header_list_ = _header_list_;
	};

	TableStructInfo(CString  _new_table_name, std::initializer_list<const TCHAR*> initList)
	{
		s_table_name_ = _new_table_name;
		for (auto it = initList.begin(); it != initList.end();)
		{
			auto colName = *it++;
			auto dataType = *it++;
			auto isPriKey = *it++;
			auto isNeedIndex = *it++;
			field_header_list_.push_back(FieldHeader(colName, dataType, isPriKey, isNeedIndex));
		}
	}

	CString GetName() const { return s_table_name_; }
	FieldHeaderList GetFieldHeaderList() const { return field_header_list_; }
private:
	CString s_table_name_;
	FieldHeaderList field_header_list_;
};

typedef  std::vector<TableStructInfo>  TableList;

//table write request
typedef struct _Request {
	//database write operations
	enum Action {
		kActionSelect = 0,
		kActionInsert,
		KActionUpdate,
		kActionDelete,
	};
	CString			 table_name_;
	Action		     action_;
	FieldInfoList    fields_;
	BOOL			 b_last_update_;
	FIELD_CONLIST_PTR p_conList_;
} Request, ActionInfo;

typedef struct _SearchRequest : _Request
{
	LONG need_count_; //需要select出的数目 <=0为表示选出全部。
	CString sort_col_; //排序列
}SearchRequest, SearchActionInfo;
