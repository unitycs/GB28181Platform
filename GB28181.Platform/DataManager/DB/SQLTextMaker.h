#ifndef __SQLTextMaker__H___62C3AE79_6077_4026_A4A1_AB90A60160DA
#define __SQLTextMaker__H___62C3AE79_6077_4026_A4A1_AB90A60160DA
#pragma once

#pragma region SQLite3 const string

const CString cstring_Boolean = "BOOLEAN ";
const CString cstring_integer = "INTEGER ";
const CString cstring_real = "REAL ";
const CString cstring_null = "NULL ";
const CString cstring_text = "TEXT ";
const CString cstring_blob = "BLOB ";
const CString cstring_create = "create ";
const CString cstring_select = "select ";
const CString cstring_insert = "insert ";
const CString cstring_update = "update ";
const CString cstring_delete = "delete ";
const CString cstring_from = "from ";
const CString cstring_where = "where ";
const CString cstring_group_by = "group by ";
const CString cstring_having = "having ";
const CString cstring_order_by = "order by ";
const CString cstring_sort_by = "sort by ";
const CString cstring_indexed_by = "indexed by ";
const CString cstring_limit = "limit ";
const CString cstring_asc = "ASC ";
const CString cstring_desc = "DESC ";
const CString cstring_into = "into ";
const CString cstring_values = "values ";
const CString cstring_set = "set ";
const CString cstring_table = "table ";
const CString cstring_index = "index ";
const CString cstring_if = "if ";
const CString cstring_exists = "exists ";
const CString cstring_left_bracket = "( ";
const CString cstring_right_bracket = ") ";
const CString cstring_on = "on ";
const CString cstring_star = "* ";
const CString cstring_single_quote = "\'";
const CString cstring_quote = "\'";     //old  "\"";
const CString cstring_comma = ", ";
const CString cstring_semicolon = ";";
const CString cstring_primary_key = "PRIMARY KEY ";
const CString cstring_equal = "=";
const CString cstring_not_equal = "<>";
const CString cstring_larger = ">";
const CString cstring_larger_or_equal = ">=";
const CString cstring_smaller = "<";
const CString cstring_smaller_or_equal = "<=";
const CString cstring_between = "between ";
const CString cstring_not_between = "not between ";
const CString cstring_not = "not ";
const CString cstring_and = "and ";
const CString cstring_or = "or ";
const CString cstring_not_like = "not like ";
const CString cstring_like = "like ";
const CString cstring_percent = "%";
const CString cstring_empty = "";
const CString cstring_space = " ";
const CString cstring_not_null = "NOT NULL";

#define SQLITE_EXCUTE_SUCCESS 0
#define SQLITE_EXCUTE_FAILD -1
#define SQLITE_QUEUE_SIZE 10240
#define SQLITE_BUSY_TIMEOUT 1000

#pragma endregion

class SQLiteTextMaker
{
public:
	SQLiteTextMaker() = default;
	~SQLiteTextMaker() = default;
	using SQL_LIST = std::vector<CString>;

private:
	enum _SQL_TABLE_CLASS_OP_TYPE :UINT {
		_SELECT = 0,
		_INSERT,
		_UPDATE,
		_DELETE,
	};
	enum _SQL_DB_CLASS_OP_TYPE :UINT
	{
		_CREATE_TABLE = 0,
		_CREATE_INDEX_TABLE
	};

public:

	bool MakeInserSql(CString _table_name, const FieldInfoList &field_info_list, CString& out_sq_string, CString& out_msg) const
	{
		CString cstring_sql = cstring_insert + cstring_into + _table_name + cstring_space;//insert into table_name

		CString cstring_result = FormatSQL(field_info_list, _INSERT); //(ID,NAME,AGE,ADDRESS,SALARY) VALUES(6, 'Kim', 22, 'South-Hall', 45000.00);

		if (cstring_result.IsEmpty())
		{
			CString logmsg(_T("insert abort , for no field info to insert into: "));
			logmsg += _table_name;
			out_msg = std::move(logmsg);
			return false;
		}

		out_sq_string = std::move(cstring_sql + cstring_result);

		return true;
	}

	bool MakeSelectSql(CString _table_name, const FieldInfoList &field_info_list, const CString limit_row_count, const CString sort_field_name, CString& out_sq_string, CString& out_msg) const
	{
		if (_table_name.Trim().IsEmpty())
		{
			CString logmsg(_T("Select abort , table name is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}

		CString cstring_sql = cstring_select + cstring_star + cstring_from + _table_name + cstring_space;//select * from table_name;

		CString cstring_result = FormatSQL(field_info_list, _SELECT);

		if (!cstring_result.IsEmpty())
		{
			cstring_sql += std::move(cstring_result);
		}
		if (!sort_field_name.IsEmpty())
		{
			cstring_sql += cstring_order_by + sort_field_name + cstring_space;
		}

		if (!limit_row_count.IsEmpty())
		{
			cstring_sql += cstring_limit + limit_row_count;
		}

		out_sq_string = std::move(cstring_sql + cstring_semicolon);
		return true;
	}

	bool MakeUpdateSql(CString _table_name, const FieldInfoList &field_info_list, CString& out_sq_string, CString& out_msg, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		if (_table_name.Trim().IsEmpty())
		{
			CString logmsg(_T("Update abort , table name is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}
		CString cstring_sql = cstring_update + _table_name + cstring_space;//update table_name;

		CString cstring_result = FormatSQL(field_info_list, _UPDATE, p_Con_Lst);
		cstring_sql += cstring_result;
		out_sq_string = std::move(cstring_sql);
		return true;
	}
	bool MakeDeleteSql(CString _table_name, const FieldInfoList &field_info_list, CString& out_sq_string, CString& out_msg, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		if (_table_name.Trim().IsEmpty())
		{
			CString logmsg(_T("Delete abort , table name is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}
		CString cstring_sql = cstring_delete + cstring_from + _table_name + cstring_space;//delete table_name;

		CString cstring_result = FormatSQL(field_info_list, _DELETE, p_Con_Lst);

		cstring_sql += cstring_result;
		out_sq_string = std::move(cstring_sql);
		return true;
	}

	bool MakeCreateTableSql(const TableList& tables_list, SQL_LIST& table_Create_sql_list, CString& out_msg) const
	{
		if (tables_list.empty())
		{
			CString logmsg(_T("Create Tables abort , Table List to crerate is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}
		//	CString cstring_sql = cstring_create + cstring_table + cstring_if + cstring_not + cstring_exists;//Create table if not  exist ;

		for (auto& item : tables_list)
		{
			CString cstring_result = FormatSQL(item, _CREATE_TABLE);
			if (!cstring_result.Trim().IsEmpty())
				table_Create_sql_list.push_back(cstring_result);
		}
		if (table_Create_sql_list.empty())
		{
			CString logmsg(_T("Create Tables failed : make table-create-sql-sentence failed£¡"));
			out_msg = std::move(logmsg);
			return false;
		}

		return true;
	}

	bool MakeCreateTableSql(const TableStructInfo& _table_structure, CString& out_sql_string, CString& out_msg) const
	{
		if (_table_structure.GetName().IsEmpty())
		{
			CString logmsg(_T("Create Tables abort , Table List to crerate is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}
		auto cstring_result = FormatSQL(_table_structure, _CREATE_TABLE);

		if (cstring_result.IsEmpty())
		{
			CString logmsg(_T("Create Tables failed : make table-create-sql-sentence failed£¡"));
			out_msg = std::move(logmsg);
			return false;
		}
		out_sql_string = std::move(cstring_result);

		return true;


	}

	bool MakeCreateIndexTableSql(const TableStructInfo& _table_structure, CString& out_sql_string, CString& out_msg) const
	{
		auto _table_name = _table_structure.GetName();
		if (_table_name.IsEmpty())
		{
			CString logmsg(_T("Create Index_Tables abort , Table List to crerate is empty!"));
			out_msg = std::move(logmsg);
			return false;
		}
		auto cstring_result = FormatSQL(_table_structure, _CREATE_INDEX_TABLE);

		if (cstring_result.IsEmpty())
		{
			CString logmsg;
			logmsg.FormatMessage(_T("Create Index Tables failed :no index table to create for table: %s."), _table_name);
			out_msg = std::move(logmsg);
			return false;
		}
		out_sql_string = std::move(cstring_result);

		return true;
	}



private:

	CString FormatSQL(const FieldInfoList& field_info_list, _SQL_TABLE_CLASS_OP_TYPE operation_type, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		FieldInfoList valid_field_info_list;
		for (auto& item : field_info_list)
		{
			if (item.second.IsValid())
			{
				auto keyname = item.first;
				auto fieldinfo = item.second;
				valid_field_info_list.emplace(keyname, fieldinfo);
			}
		}
		CString cstring_sql;
		auto field_info_list_count = valid_field_info_list.size();
		if (field_info_list_count == 0) return cstring_sql;

		switch (operation_type)
		{
		case	_SELECT:
			cstring_sql = MakeSelectTextBody(valid_field_info_list, field_info_list_count);
			break;
		case	_INSERT:
			cstring_sql = MakeInsertTextBody(valid_field_info_list, field_info_list_count);
			break;
		case	_UPDATE:
			cstring_sql = MakeUpdateTextBody(valid_field_info_list, field_info_list_count, p_Con_Lst);
			break;
		case	_DELETE:
			cstring_sql = MakeDeleteTextBody(valid_field_info_list, field_info_list_count);
			break;
		default:
			break;
		}

		return cstring_sql;
	}

	CString FormatSQL(const TableStructInfo& _table_structure, _SQL_DB_CLASS_OP_TYPE operation_type) const
	{
		CString cstring_sql;
		switch (operation_type)
		{
		case _CREATE_TABLE:
			cstring_sql = MakeCreateTableText(_table_structure);
			break;
		case _CREATE_INDEX_TABLE:
			cstring_sql = MakeCreateIndexTableText(_table_structure);
			break;
		default: break;
		}

		return cstring_sql;
	}

	CString MakeInsertTextBody(const FieldInfoList& valid_field_info_list, size_t valid_field_list_cont, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		CString cstring_sql;
		CString left_part_sql = cstring_left_bracket; //(
		CString right_part_sql = cstring_values + cstring_left_bracket;
		//(ID,NAME,AGE,ADDRESS,SALARY) VALUES(6, 'Kim', 22, 'South-Hall', 45000.00);
		size_t i = 0;
		for (auto& item : valid_field_info_list)
		{
			auto filed_name = item.first;
			auto field_data = item.second.GetValue();
			left_part_sql += filed_name;
			switch (item.second.GetType())
			{
			case FieldInfo::kText:
			{
				right_part_sql += cstring_quote + field_data + cstring_quote;
			}
			break;
			case FieldInfo::kBoolean:
			case FieldInfo::kInteger:
			{
				right_part_sql += field_data;
			}
			break;

			default:
				break;
			}
			if (i++ != valid_field_list_cont - 1)
			{
				left_part_sql += cstring_comma;
				right_part_sql += cstring_comma;
			}
			else
			{
				left_part_sql += cstring_right_bracket;
				right_part_sql += cstring_right_bracket + cstring_semicolon;
			}
		}
		cstring_sql += std::move(left_part_sql) + std::move(right_part_sql);

		if (!p_Con_Lst)
			return cstring_sql;

		auto  con_part_sql = GetConditonSql(*p_Con_Lst);
		cstring_sql = std::move(cstring_sql) + std::move(con_part_sql);
		return cstring_sql;
	}

	CString MakeSelectTextBody(const FieldInfoList& valid_field_info_list, size_t valid_field_list_cont, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		CString cstring_sql;
		CString part_sql = cstring_where; //(
		// where name like "%name%" and type="type";
		size_t i = 0;
		for (auto& item : valid_field_info_list)
		{
			auto filed_name = item.first;
			auto filed_data = item.second.GetValue();
			part_sql += filed_name;
			switch (item.second.GetType())
			{
			case FieldInfo::kText:
			{
				if (filed_name.MakeLower().Find(_T("name")) >= 0)
				{
					part_sql += cstring_like;
					part_sql += cstring_quote + cstring_percent;
					part_sql += filed_data;
					part_sql += cstring_percent + cstring_quote;
				}
				else
				{
					part_sql += cstring_equal;
					part_sql += cstring_quote + filed_data + cstring_quote;
				}
				part_sql += cstring_space;
			}
			break;
			case FieldInfo::kInteger:
			case FieldInfo::kBoolean:
			{
				part_sql += cstring_equal;
				part_sql += filed_data;
				part_sql += cstring_space;
			}
			break;

			default:
				break;
			}

			if (i++ != valid_field_list_cont - 1)
			{
				part_sql += cstring_and;
			}
		}
		cstring_sql += std::move(part_sql);
		if (!p_Con_Lst)
			return cstring_sql;

		auto  con_part_sql = GetConditonSql(*p_Con_Lst);
		cstring_sql = std::move(cstring_sql) + std::move(con_part_sql);
		return cstring_sql;
	}

	CString MakeUpdateTextBody(const FieldInfoList& valid_field_info_list, size_t valid_field_list_cont, FIELD_CONLIST_PTR p_Con_Lst = nullptr) const
	{
		CString cstring_sql;
		CString left_part_sql = cstring_set; //( set DeviceID = "34010000001310000008" ,DeviceType = "IPC"
		CString right_part_sql = cstring_where; //( Where DeviceID ="34010000001310000001"
		size_t i = 0;
		FieldInfoT primirykeyInfo;
		CString primirykey;
		for (auto& item : valid_field_info_list)
		{
			if (item.second.GetConstraint() == FieldInfo::c_PrimaryKey)
			{
				primirykey = item.first;
				primirykeyInfo = item.second;
				i++;
				continue;
			}

			auto filed_name = item.first;
			auto field_data = item.second.GetValue();
			left_part_sql += filed_name + cstring_equal;
			switch (item.second.GetType())
			{
			case FieldInfo::kText:
			{
				left_part_sql += cstring_quote;
				left_part_sql += field_data;
				left_part_sql += cstring_quote;
			}
			break;
			case FieldInfo::kInteger:
			case FieldInfo::kBoolean:
			{
				left_part_sql += field_data;
			}
			break;

			default:
				break;
			}

			if (i++ != valid_field_list_cont - 1)
			{
				left_part_sql += cstring_comma;
			}
			else
			{
				left_part_sql += cstring_space;
			}
		}

		if (!p_Con_Lst)
		{
			right_part_sql += primirykey + cstring_equal;
			right_part_sql += cstring_quote + primirykeyInfo.GetValue() + cstring_quote;
			right_part_sql += cstring_semicolon;
		}
		else
		{
			right_part_sql += GetConditonSql(*p_Con_Lst);
		}

		cstring_sql = std::move(left_part_sql) + std::move(right_part_sql);
		return cstring_sql;
	}

	static CString MakeDeleteTextBody(const FieldInfoList& valid_field_info_list, size_t valid_field_list_cont, FIELD_CONLIST_PTR p_Con_Lst = nullptr)
	{
		CString cstring_sql;
		CString part_sql = cstring_where; //( Where DeviceID ="34010000001310000001"

		size_t i = 0;
		for (auto& item : valid_field_info_list)
		{
			auto filed_name = item.first;
			auto field_data = item.second.GetValue();
			part_sql += filed_name + cstring_equal;
			switch (item.second.GetType())
			{
			case FieldInfo::kText:
			{
				part_sql += cstring_quote;
				part_sql += field_data;
				part_sql += cstring_quote;
				part_sql += cstring_space;
			}
			break;
			case FieldInfo::kInteger:
			case FieldInfo::kBoolean:
			default:
			{
				part_sql += field_data;
				part_sql += cstring_space;
			}
			break;
			}

			if (i++ != valid_field_list_cont - 1)
			{
				part_sql += cstring_and;
			}
			else
			{
				part_sql += cstring_semicolon;
			}
		}

		if (p_Con_Lst)
			part_sql += GetConditonSql(*p_Con_Lst);

		cstring_sql += part_sql;
		return cstring_sql;
	}

	static CString GetConditonSql(const ConditionList& conldtion_list)
	{
		CString part_sql;
		auto valid_list_cont = conldtion_list.size();
		size_t i = 0;
		for (auto& item : conldtion_list)
		{
			auto  filed_name = item._columName;
			auto field_data = item._fieldValue;
			auto condition = item._conditon;
			part_sql += filed_name + cstring_space + condition;

			if (!item._b_Str)
			{
				if (condition.MakeLower().Find(_T("between")) >= 0)
				{
					part_sql += item._lowerBound + cstring_space;
					part_sql += cstring_and;
					part_sql += item._upperBound + cstring_space;
				}
				else
					part_sql += cstring_space + field_data + cstring_space;
			}
			else
			{
				part_sql += cstring_space + cstring_quote;
				part_sql += field_data;
				part_sql += cstring_quote + cstring_space;
			}
			if (i++ != valid_list_cont - 1)
			{
				if (item._b_and_next)
					part_sql += cstring_and;
				else
					part_sql += cstring_or;
			}
			else
			{
				part_sql += cstring_semicolon;
			}
		}
		return part_sql;
	}

	CString MakeCreateTableText(const TableStructInfo& _table_structure) const
	{
		auto field_header_list = _table_structure.GetFieldHeaderList();
		auto _table_name = _table_structure.GetName();
		CString cstring_sql;
		CString left_part_sql = cstring_create + cstring_table + cstring_if + cstring_not + cstring_exists +
			_table_name + cstring_space + cstring_left_bracket + cstring_space;//create table if not exists table_name(

		CString right_part_sql = cstring_primary_key + cstring_left_bracket; //PRIMARY KEY(
		//auto m_field_header_count_ = field_header_list.size();

		for (auto& item : field_header_list)
		{
			left_part_sql += item.GetFieldName();//create table if not exists table_name(field_name...
			left_part_sql += cstring_space;

			switch (item.GetType())
			{
			case FieldInfo::kNull:
				left_part_sql += cstring_null;
				break;
			case FieldInfo::kInteger:
				left_part_sql += cstring_integer;
				break;
			case FieldInfo::kBoolean:
				left_part_sql += cstring_Boolean;
				break;
			case FieldInfo::kReal:
				left_part_sql += cstring_real;
				break;
			case FieldInfo::kText:
				left_part_sql += cstring_text;
				break;
			case FieldInfo::kBlob:
				left_part_sql += cstring_blob;
				break;
			}
			left_part_sql += cstring_comma;

			if (item.isPrimaryKey())
			{
				right_part_sql += item.GetFieldName();
				right_part_sql += cstring_comma;
			}
		}

		right_part_sql.TrimRight();//remove space 
		right_part_sql.TrimRight(cstring_comma); //remove ,
		right_part_sql += cstring_right_bracket + cstring_right_bracket;
		right_part_sql += cstring_semicolon;//create table if not exists table_name(field_name1 field_type1, field_name2 field_type2,...,PRIMARY KEY(field_name,...,);
		cstring_sql = std::move(left_part_sql) + std::move(right_part_sql);

		return cstring_sql;
	}

	CString MakeCreateIndexTableText(const TableStructInfo& _table_structure) const
	{
		CString cstring_sql;
		auto field_header_list = _table_structure.GetFieldHeaderList();
		auto field_header_count = field_header_list.size();
		if (field_header_count == 0) return cstring_sql;

		auto _table_name = _table_structure.GetName();
		CString index_table_name = _table_name + cstring_index;
		CString left_part = cstring_create + cstring_index + cstring_if + cstring_not + cstring_exists +
			cstring_space + index_table_name + cstring_on + _table_name + cstring_left_bracket; //create index if not exists dabase_nameIndex on table_name(;
		CString right_part;
		for (auto& item : field_header_list)
		{
			if (item.GetConstraint() != FieldInfo::c_IndexKey)
				continue;
			right_part += item.GetFieldName();
			right_part += cstring_comma;
		}
		if (right_part.IsEmpty()) return right_part;
		right_part.TrimRight();
		right_part.TrimRight(cstring_comma);
		right_part += cstring_right_bracket;
		right_part += cstring_semicolon;
		cstring_sql = std::move(left_part) + std::move(right_part);
		return cstring_sql;
	}
};
#endif
