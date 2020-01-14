#ifndef __SQLiteModule__H___62C3AE79_6077_4026_A4A1_AB90A60160DB
#define _SQLiteModule__H___62C3AE79_6077_4026_A4A1_AB90A60160DB

#include <fstream>
#include "db_common.h"
#include "utility.h"
#include <mutex>
#include <atomic>
#include <thread>
struct sqlite3;
class SQLiteModule
{
public:
	typedef CSynQueueT<Request> ListSQLiteRequest;
	typedef std::vector<CString> VectSQLiteFieldName;
	typedef std::vector<FieldInfo::DataType> VectSQLiteDataType;
	typedef std::vector<int> VectSQLitePrimaryKeyPos;
	typedef std::vector<CString> VectConnectedDatabase;
	typedef std::vector<CString> VectIndexFieldName;

	typedef  std::function<bool(Request&, bool)> WriteDelegate;
	typedef std::function<bool(Request&)>  ReadDelegate;
	typedef struct _TableObject
	{
		CString m_table_name_;
		FieldHeaderList m_field_header_list_;
		VectSQLiteFieldName m_field_names_;
		VectSQLiteDataType m_field_types_;
		VectSQLiteDataType m_data_types_;
		VectSQLitePrimaryKeyPos m_primary_key_pos_;
		VectIndexFieldName m_index_field_name_;
	}TableObject;

	SQLiteModule();
	~SQLiteModule();

	int Insert(CString _table_name, const FieldInfoList &) const;

	int SQLiteInitial(const CString &database_filename, const CString &database_name, const TableList& tables_list, WriteDelegate write_delegate_);
	int SQLiteUninitial();
	int SQLiteBeginTrasaction();
	int SQLiteCommitTrasaction();
	int SQLiteWriteRequest(const Request &rst);

	int SQLiteSelect(CString _table_name, const FieldInfoList& field_info_list, const CString limit_row_count, const CString& sort_field_name, ReadDelegate read_delegate_);
	void _SQLTaskProc();
protected:
	std::vector<std::thread> threadsWorkers;		// 存储所有工作线程的对象指针
	int SQLiteCreateTable(const TableList & tables_list);

private:
	int SQLiteSelectCoreExcute(CString _table_name, const char *sql_string, ReadDelegate read_delegate_);
	int SQLiteInsert(CString _table_name, FieldInfoList &rst);
	int SQLiteUpdate(CString _table_name, FieldInfoList &rst, FIELD_CONLIST_PTR p_con_lst = nullptr);
	int SQLiteDelete(CString _table_name, FieldInfoList &rst);

	CString GetFieldNames(CString _table_name, int vector_field_names_offset)
	{
		auto _table_obj = db_tables[_table_name];
		if (&_table_obj != nullptr)
			return _table_obj.m_field_names_[vector_field_names_offset];
		return _T("");
	}
	FieldInfo::DataType GetFieldTypes(CString _table_name, int vector_field_types_offset)
	{
		auto _table_obj = db_tables[_table_name];
		if (&_table_obj != nullptr)
			return _table_obj.m_field_types_[vector_field_types_offset];
		return FieldInfo::kNull;
	}
	FieldInfo::DataType GetDataTypes(CString _table_name, int vector_data_types_offset)
	{
		auto _table_obj = db_tables[_table_name];
		if (&_table_obj != nullptr)
			return _table_obj.m_data_types_[vector_data_types_offset];
		return FieldInfo::kNull;
	}
	int GetPrimaryKeyPos(CString _table_name, int vector_primary_key_offset)
	{
		auto _table_obj = db_tables[_table_name];
		if (&_table_obj != nullptr)
			return _table_obj.m_primary_key_pos_[vector_primary_key_offset];
		return -1;
	}
	void PrintfLog(const char * err);
	WriteDelegate m_write_delegate_;

	sqlite3 *m_database_connect_;
	std::ofstream m_sqlite_log;
	ListSQLiteRequest m_queue_request_;

	std::mutex m_pMutex;
	CString m_database_full_path;
	CString m_database_name;
	CString m_log_path;

	friend class DataBaseMgr;

	std::map<CString, TableObject> db_tables;
	std::atomic<bool> m_bExiting;
	static VectConnectedDatabase m_connected_database_;

};

#endif // !__SQLiteModule__H___