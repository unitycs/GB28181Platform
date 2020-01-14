// SqliteModule.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <thread>
#include "./sqlite/sqlite3.h"
#include "SqliteModule.h"
#include "SQLTextMaker.h"

SQLiteModule::VectConnectedDatabase SQLiteModule::m_connected_database_;

SQLiteModule::SQLiteModule()
	: m_write_delegate_(nullptr),
	m_database_connect_(nullptr), m_bExiting(false)
{
}

SQLiteModule::~SQLiteModule()
{
}

int SQLiteModule::SQLiteInitial(const CString &database_filename, const CString &database_name, const TableList& tables_list, WriteDelegate write_delegate_)
{
	//can't connect a database which is connected by another SQLiteModule object
	for (size_t i = 0; i < m_connected_database_.size(); i++)
	{
		if (database_filename == m_connected_database_[i])
		{
			return SQLITE_EXCUTE_FAILD;
		}
	}
	m_connected_database_.push_back(database_filename);

	//create database log
	m_log_path = database_filename;
	m_log_path = m_log_path.Left(m_log_path.ReverseFind(_T('.'))) + _T(".log");

	m_sqlite_log.open(m_log_path, std::ios::app);

	m_database_full_path = database_filename;
	m_database_name = database_name;
	m_write_delegate_ = write_delegate_;

	int result_init = sqlite3_initialize();
	if (SQLITE_OK != result_init)
	{
		PrintfLog("sqlite init error");
		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("sqlite init success");

#ifdef UNICODE
	_bstr_t db_name(database_filename);
	int result_open_database = sqlite3_open_v2(static_cast<const char*>(db_name), &m_database_connect_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
#else
	int result_open_database = sqlite3_open_v2(database_filename, &m_database_connect_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
#endif
	if (SQLITE_OK != result_open_database)
	{
		PrintfLog("sqlite3_open_v2 error is :");
		PrintfLog(sqlite3_errmsg(m_database_connect_));
		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("sqlite3_open_v2 success ");

	int result_wal_mode = sqlite3_exec(m_database_connect_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
	if (SQLITE_OK != result_wal_mode)
	{
		PrintfLog("sqlite3 database mode error is :");
		PrintfLog(sqlite3_errmsg(m_database_connect_));
		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("sqlite3  open database mode is WAL");

	if (tables_list.size() <= 0)
	{
		PrintfLog("open sqlite3 database success ,no Table to Create");
		return 0;
	}

	if (SQLITE_EXCUTE_SUCCESS != SQLiteCreateTable(tables_list))
	{
		PrintfLog("sqlite3 Create Table Failed!");
		return SQLITE_EXCUTE_FAILD;
	}

	//this->Start(); //开始任务线程
	//_beginthread(_TaskProc,nullptr,NULL);
	//auto fthreadFunc = std::bind(&SQLiteModule::_SQLTaskProc, this);
	//threadsWorkers.push_back(std::thread(fthreadFunc));
	//threadsWorkers[0].detach();
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteUninitial()
{
	//m_pTaskTimer.Set();

	//waiting the queue to be empty

	int result_close = sqlite3_close_v2(m_database_connect_);
	if (SQLITE_OK != result_close)
	{
		PrintfLog("sqlite close error :");
		PrintfLog(sqlite3_errmsg(m_database_connect_));
	}
	else
	{
		PrintfLog("sqlite success ");
	}

	for (auto iter = m_connected_database_.begin(); iter != m_connected_database_.end(); ++iter)
	{
		if (*iter == m_database_name)
		{
			m_connected_database_.erase(iter);
			break;
		}
	}

	m_sqlite_log.close();
	m_queue_request_.clear();
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteBeginTrasaction()
{
	char *sql_begin_transaction = "begin";
	int result_begin_transaction = sqlite3_exec(m_database_connect_, sql_begin_transaction, nullptr, nullptr, nullptr);
	if (SQLITE_OK != result_begin_transaction)
	{
		PrintfLog("SQLiteBegin error : ");
		PrintfLog(sqlite3_errmsg(m_database_connect_));
		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("SQLiteBegin success ");
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteCommitTrasaction()
{
	char *sql_commit_transaction = "commit";
	int result_commit_transaction = sqlite3_exec(m_database_connect_, sql_commit_transaction, nullptr, nullptr, nullptr);
	if (SQLITE_OK != result_commit_transaction)
	{
		PrintfLog("SQLiteCommit error : ");
		PrintfLog(sqlite3_errmsg(m_database_connect_));
		return SQLITE_EXCUTE_FAILD;
	}
	else
	{
		PrintfLog("sqliteCommit success");
	}
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteWriteRequest(const Request &rst)
{
	PrintfLog("SQLiteModule::SQLiteWriteRequest Enter");

	CString _table_name_ = rst.table_name_;
	int return_value;
	if (!db_tables.count(_table_name_))
	{
		return_value = SQLITE_EXCUTE_FAILD;
		PrintfLog("table is not exist");
	}

	if (m_queue_request_.Count() < SQLITE_QUEUE_SIZE)
	{
		m_queue_request_.push(rst);
		//m_pTaskTimer.do_unlock();
		return_value = SQLITE_EXCUTE_SUCCESS;
		PrintfLog("SQLITE queue is not full");
	}
	else
	{
		return_value = SQLITE_EXCUTE_FAILD;
		PrintfLog("SQLITE queue is full");
	}
	return return_value;
}

void  SQLiteModule::_SQLTaskProc()
{
	while (!m_bExiting)
	{
		Request rqst;
		auto ret = m_queue_request_.Pop(rqst);
		if (!ret) continue;
		auto action = rqst.action_;
		switch (action)
		{
		case Request::kActionInsert:
			SQLiteInsert(rqst.table_name_, rqst.fields_);
			break;
		case Request::KActionUpdate:
			SQLiteUpdate(rqst.table_name_, rqst.fields_, rqst.p_conList_);
			break;
		case Request::kActionDelete:
			SQLiteDelete(rqst.table_name_, rqst.fields_);
			break;
		default:
			break;
		}
	}
}

int SQLiteModule::SQLiteCreateTable(const TableList& tables_list)
{
	if (tables_list.size() <= 0) return 0;
	SQLiteTextMaker sqlMaker;
	CString sql_maker_msg;

	for (auto& item : tables_list)
	{
		TableObject table_object;
		auto field_header_list = item.GetFieldHeaderList();
		auto m_field_header_count_ = field_header_list.size();

		for (size_t i = 0; i < m_field_header_count_; i++)
		{
			//record the information of field_header_list
			table_object.m_field_names_.push_back(field_header_list[i].GetFieldName());
			table_object.m_field_types_.push_back(field_header_list[i].GetFieldType());

			if (field_header_list[i].isIndex())
			{
				table_object.m_index_field_name_.push_back(field_header_list[i].GetFieldName());
			}

			if (field_header_list[i].isPrimaryKey())
			{
				table_object.m_primary_key_pos_.push_back(i);
			}

		}
		auto _table_name = item.GetName();
		table_object.m_field_header_list_ = std::move(field_header_list);
		table_object.m_table_name_ = _table_name;

		CString cstring_sql;
		auto b_ret = sqlMaker.MakeCreateTableSql(item, cstring_sql, sql_maker_msg);
		if (!b_ret)
		{
			PrintfLog(sql_maker_msg);
			return SQLITE_EXCUTE_FAILD;
		}

		int result_create_table;
#ifdef UNICODE
		_bstr_t str_sql_table(cstring_sql);
		result_create_table = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql_table), nullptr, nullptr, nullptr);
#else
		result_create_table = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif
		CString result_msg;
		if (SQLITE_OK != result_create_table)
		{
			result_msg.FormatMessage("create table  { %s } error :", _table_name);
			PrintfLog(result_msg);
			PrintfLog(sqlite3_errmsg(m_database_connect_));
			return SQLITE_EXCUTE_FAILD;
		}
		result_msg.FormatMessage("create tables { %s } success: ", _table_name);
		PrintfLog("create tables success ");

		db_tables.emplace(_table_name, table_object); //add table object info.

		b_ret = sqlMaker.MakeCreateIndexTableSql(item, cstring_sql, sql_maker_msg);
		if (!b_ret)
		{
			PrintfLog(sql_maker_msg);
			return SQLITE_EXCUTE_FAILD;
		}

		int result_create_index;
#ifdef UNICODE
		_bstr_t str_sql_index(cstring_sql);
		result_create_index = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql_index), nullptr, nullptr, nullptr);
#else
		result_create_index = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif

		if (SQLITE_OK != result_create_index)
		{
			result_msg.FormatMessage("create index_table for table  { %s } error :", _table_name);
			PrintfLog(result_msg);
			PrintfLog(sqlite3_errmsg(m_database_connect_));
			return SQLITE_EXCUTE_FAILD;
		}
		result_msg.FormatMessage("create index_table for  table { %s } success: ", _table_name);
		PrintfLog("create Index_table success ");

	}

	return 0;
}

int SQLiteModule::SQLiteInsert(CString _table_name, FieldInfoList &field_info_list)
{
	PrintfLog("SQLiteModule::SQLiteInsert Enter");

	CString cstring_sql, sql_maker_msg;

	SQLiteTextMaker sql_maker;

	auto b_ret = sql_maker.MakeInserSql(_table_name, field_info_list, cstring_sql, sql_maker_msg);
	if (!b_ret)
	{
		PrintfLog(sql_maker_msg);
		return SQLITE_EXCUTE_FAILD;
	}

	int result_insert;

#ifdef UNICODE
	_bstr_t str_sql(cstring_sql);
	result_insert = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql), nullptr, nullptr, nullptr);
#else
	result_insert = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif
	if (SQLITE_OK != result_insert)
	{
		Request request_insert;
		request_insert.action_ = Request::kActionInsert;
		request_insert.fields_ = field_info_list;

		if (SQLITE_CONSTRAINT == result_insert)
		{
			if (m_write_delegate_)
				m_write_delegate_(request_insert, true);
			PrintfLog("insert abort : ");
			PrintfLog(sqlite3_errmsg(m_database_connect_));
		}
		else
		{
			if (m_write_delegate_)
				m_write_delegate_(request_insert, true);
			PrintfLog("insert error : ");
			PrintfLog(sqlite3_errmsg(m_database_connect_));
		}

		return SQLITE_EXCUTE_FAILD;
	}
	CString logmsg = _T("sqlite insert success with table name : ");
	logmsg += _table_name;
	PrintfLog(logmsg);

	Request request_insert;
	request_insert.action_ = Request::kActionInsert;
	request_insert.fields_ = field_info_list;
	//auto b_true = std::is_bind_expression<decltype(m_write_delegate_)>::value;
	if (m_write_delegate_)
		m_write_delegate_(request_insert, true);  //sucess then call back.

	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteUpdate(CString _table_name, FieldInfoList &field_info_list, FIELD_CONLIST_PTR p_con_lst)
{
	PrintfLog("SQLiteModule::SQLiteUpdate Enter");

	if (db_tables.count(_table_name) == 0)
	{
		CString error_msg(_T("SQLiteModule::SQLiteUpdate error, table not exist"));
		PrintfLog(error_msg);
		return SQLITE_EXCUTE_FAILD;
	}

	CString cstring_sql, sql_maker_msg;

	SQLiteTextMaker sql_maker;

	auto b_ret = sql_maker.MakeUpdateSql(_table_name, field_info_list, cstring_sql, sql_maker_msg, p_con_lst);
	if (!b_ret)
	{
		PrintfLog(sql_maker_msg);
		return SQLITE_EXCUTE_FAILD;
	}

	int result_update;
#ifdef UNICODE
	_bstr_t str_sql(cstring_sql);
	result_update = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql), nullptr, nullptr, nullptr);
#else
	result_update = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif
	if (SQLITE_OK != result_update)
	{
		PrintfLog("update error");
		PrintfLog(sqlite3_errmsg(m_database_connect_));

		Request request_update;
		if (m_write_delegate_)
		{
			request_update.action_ = Request::KActionUpdate;
			request_update.fields_ = field_info_list;
			this->m_write_delegate_(request_update, false);
		}

		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("update success ");

	Request request_update;
	request_update.action_ = Request::KActionUpdate;
	request_update.fields_ = field_info_list;
	this->m_write_delegate_(request_update, true);
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteDelete(CString _table_name, FieldInfoList &field_info_list)
{
	PrintfLog("SQLiteModule::SQLiteDelete Enter");
	CString cstring_sql, sql_maker_msg;

	SQLiteTextMaker sql_maker;

	auto b_ret = sql_maker.MakeDeleteSql(_table_name, field_info_list, cstring_sql, sql_maker_msg);
	if (!b_ret)
	{
		PrintfLog(sql_maker_msg);
		return SQLITE_EXCUTE_FAILD;
	}

	int result_delete;
#ifdef UNICODE
	_bstr_t str_sql(cstring_sql);
	result_delete = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql), nullptr, nullptr, nullptr);
#else
	result_delete = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif

	if (SQLITE_OK != result_delete)
	{
		PrintfLog("delete error : ");
		PrintfLog(sqlite3_errmsg(m_database_connect_));

		Request request_delete;
		if (m_write_delegate_)
		{
			request_delete.action_ = Request::kActionDelete;
			request_delete.fields_ = field_info_list;
			this->m_write_delegate_(request_delete, false);
		}

		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("delete success ");
	Request request_delete;
	if (m_write_delegate_)
	{
		request_delete.action_ = Request::kActionDelete;
		request_delete.fields_ = field_info_list;
		this->m_write_delegate_(request_delete, true);
	}
	return SQLITE_EXCUTE_SUCCESS;
}

int SQLiteModule::SQLiteSelect(CString _table_name, const FieldInfoList& field_info_list, const CString limit_row_count, const CString& sort_field_name, ReadDelegate read_delegate_)
{
	PrintfLog("SQLiteModule::SQLiteSelect Enter");
	CString cstring_sql, sql_maker_msg;
	SQLiteTextMaker sql_maker;

	auto b_ret = sql_maker.MakeSelectSql(_table_name, field_info_list, limit_row_count, sort_field_name, cstring_sql, sql_maker_msg);

	if (!b_ret)
	{
		PrintfLog(sql_maker_msg);
		return SQLITE_EXCUTE_FAILD;
	}

	int result_select;
#ifdef UNICODE
	_bstr_t str_sql(cstring_sql);
	result_select = SQLiteSelectCoreExcute(_table_name, static_cast<const char*>(str_sql), read_delegate_);
#else
	result_select = SQLiteSelectCoreExcute(_table_name, cstring_sql, read_delegate_);
#endif
	return result_select;
}

int SQLiteModule::SQLiteSelectCoreExcute(CString _table_name, const char* sql_string, ReadDelegate read_delegate_)
{
	PrintfLog("SQLiteModule::SQLiteSelectCoreExcute Enter");
	sqlite3 *db_select_connetion_;
	int sqlite_result_open;
#ifdef UNICODE
	_bstr_t db_name(m_database_full_path);
	sqlite_result_open = sqlite3_open_v2(static_cast<const char*>(db_name), &db_select_connetion_, SQLITE_OPEN_READWRITE, nullptr);
#else
	sqlite_result_open = sqlite3_open_v2(m_database_full_path, &db_select_connetion_, SQLITE_OPEN_READWRITE, nullptr);
#endif

	if (SQLITE_OK != sqlite_result_open)
	{
		PrintfLog("sqlite3_select_open_v2 error is :");
		PrintfLog(sqlite3_errmsg(db_select_connetion_));
		return SQLITE_EXCUTE_FAILD;
	}
	PrintfLog("sqlite3_select_open_v2 success ");

	const char* tail;
	sqlite3_stmt *stmt;

	sqlite3_busy_timeout(db_select_connetion_, SQLITE_BUSY_TIMEOUT);
	int sqlite_result = sqlite3_prepare(db_select_connetion_, sql_string, static_cast<int>(strlen(sql_string)), &stmt, &tail);
	if (SQLITE_OK != sqlite_result)
	{
		PrintfLog("SQLiteSelectCore sqlite3_prepare error: ");
		PrintfLog(sqlite3_errmsg(db_select_connetion_));
		return SQLITE_EXCUTE_FAILD;
	}

	auto  sq_dataSet_cursor = sqlite3_step(stmt);
	int column_count = sqlite3_column_count(stmt);
	while (sq_dataSet_cursor == SQLITE_ROW)
	{
		CString field_name;
		FieldInfoList field_info_list;

		for (auto i = 0; i < column_count; i++)
		{
			field_name = sqlite3_column_name(stmt, i);
			auto _col_field_type = sqlite3_column_type(stmt, i);
			switch (_col_field_type)
			{
			case SQLITE3_TEXT:
			{
				CString field_data(sqlite3_column_text(stmt, i));
				FieldInfoT fieldinfo(field_data);
				field_info_list.emplace(field_name, fieldinfo);
			}
			break;

			case  SQLITE_INTEGER:  //FieldInfo::kInteger,FieldInfo::kBollean
			{
				auto int_result = sqlite3_column_int(stmt, i);
				CString field_data;
				field_data.Format(_T("%ld"), int_result);
				FieldInfoT fieldinfo(field_data);
				field_info_list.emplace(field_name, fieldinfo);
			}
			break;

			case  SQLITE_NULL:  //FieldInfo::kInteger,FieldInfo::kBollean
			{
				CString field_data(_T(""));
				FieldInfoT fieldinfo(field_data);
				field_info_list.emplace(field_name, fieldinfo);
			}
			break;
			case FieldInfo::kReal:
				break;
			default:
				break;
			}
		}
		if (read_delegate_)
		{
			Request search_rqst_result;
			search_rqst_result.table_name_ = _table_name;
			search_rqst_result.action_ = Request::kActionSelect;
			search_rqst_result.fields_ = std::move(field_info_list);
			read_delegate_(search_rqst_result);
		}

		sq_dataSet_cursor = sqlite3_step(stmt);
	}

	int sqlite_finalize_result = sqlite3_finalize(stmt);
	if (SQLITE_OK != sqlite_finalize_result)
	{
		PrintfLog("sqlite3_finalize error ");
		PrintfLog(sqlite3_errmsg(db_select_connetion_));
	}

	int select_func_return;
	if (SQLITE_DONE == sq_dataSet_cursor)
	{
		select_func_return = SQLITE_EXCUTE_SUCCESS;
		PrintfLog("SQLiteSelectCore excute success ");
	}
	else if (SQLITE_ROW == sq_dataSet_cursor)
	{
		select_func_return = SQLITE_EXCUTE_SUCCESS;
		PrintfLog("SQLiteSelectCore excute stop ");
	}
	else
	{
		select_func_return = SQLITE_EXCUTE_FAILD;
		PrintfLog("SQLiteSelectCore excute error ");
	}
	sqlite3_close(db_select_connetion_);
	return select_func_return;
}

int SQLiteModule::Insert(CString _table_name, const FieldInfoList &field_info_list) const
{
	CString cstring_sql;
	cstring_sql = cstring_insert + cstring_into + _table_name + cstring_space + cstring_values + cstring_left_bracket;//insert into table_name values(;

	int field_info_list_count = field_info_list.size();

	auto i = 0;
	for (auto iter = field_info_list.begin(); iter != field_info_list.end(); ++iter)
	{
		if (iter->second.IsValid())
		{
			CString type_guid;;
			CString type_cstring;
			CString long_value;
			CString data_type_cstring;

			switch (iter->second.GetType())
			{
			case FieldInfo::kText:
				data_type_cstring = iter->second.GetValue();
				{
					CString cstring_value;
					cstring_value = data_type_cstring;
					cstring_sql += cstring_quote;
					cstring_sql += cstring_value;
					cstring_sql += cstring_quote;
				}

				break;
			case FieldInfo::kInteger:
				long_value = iter->second.GetValue();
				{
					cstring_sql += long_value;
				}

				break;

			default:
				break;
			}
		}
		if (i != field_info_list_count - 1)
		{
			cstring_sql += cstring_comma;
		}
		else
		{
			cstring_sql += cstring_right_bracket;
			cstring_sql += cstring_semicolon;
		}
		i++;
	}

	int result_insert;

#ifdef UNICODE
	_bstr_t str_sql(cstring_sql);
	result_insert = sqlite3_exec(m_database_connect_, static_cast<const char*>(str_sql), nullptr, nullptr, nullptr);
#else
	result_insert = sqlite3_exec(m_database_connect_, cstring_sql, nullptr, nullptr, nullptr);
#endif
	if (SQLITE_OK != result_insert)
	{
	}
	return SQLITE_EXCUTE_SUCCESS;
}

void SQLiteModule::PrintfLog(const char *err)
{
	std::unique_lock<std::mutex> lock(m_pMutex);

	m_sqlite_log.seekp(0, std::ios::end);
	if (m_sqlite_log.tellp() >= 64 * 1024 * 1024)
	{
		m_sqlite_log.close();
		m_sqlite_log.open(m_log_path, std::ios::ate);
	}

	SYSTEMTIME st;
	GetLocalTime(&st);

	char logBuffer[1024] = { 0 };
	int length = sprintf_s(logBuffer, "%02d:%02d:%02d:%03d-->", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	logBuffer[length] = '\0';

	m_sqlite_log << logBuffer << err << std::endl;
}