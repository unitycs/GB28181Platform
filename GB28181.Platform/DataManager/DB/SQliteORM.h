#pragma once

#include "SqliteModule.h"
#include "db_tabledef.h"
#include <functional>
#include <vector>

//DataBaseMgr: class to manage the table with the GBDeviceInfo format record
class DataBaseMgr
{
public:

	typedef SQLiteModule::ReadDelegate OnOperationCmd;
	typedef SQLiteModule::WriteDelegate WriteDelegate; //wirte callback
	typedef SQLiteModule::ReadDelegate ReadDelegate;

	struct DeviceCondition {
		std::vector<CString> _device_id_list_;
		BOOL  b_exc_;

		DeviceCondition() {
			_device_id_list_.clear();
			b_exc_ = FALSE;
		}
	};

	//set time
	struct TimeSpaceCondition {
		long n_start_time_;
		long n_end_time_;
		long n_duration_;

		TimeSpaceCondition() {
			n_start_time_ = -1;
			n_end_time_ = -1;
			n_duration_ = -1;
		}
		BOOL IsStartTimeValid() const { return n_start_time_ > 0; }
		BOOL IsEndTimeValid() const { return n_end_time_ > 0; }
		BOOL IsDurationValid() const { return n_duration_ > 0; }
	};

	DataBaseMgr(void);
	virtual ~DataBaseMgr(void);

	//for rebuild database
	BOOL OpenAndStartTransaction(const CString& s_database_root, const CString& s_database_name, const TableList& field_header_list);
	BOOL SubmitTransactionAndClose();
	BOOL InsertInTransaction(CString _table_name, CString device_id_, long start_time, long end_time, int n_file_size, CString path) const;

	//database operators
	BOOL Open(const CString& s_database_root, const CString& s_database_name, const TableList& table_list);
	BOOL Close();
	BOOL OpenAndRecoverExpReqs(const CString& s_database_root, const CString& s_database_name, const TableList& field_header_list);
	BOOL RecoverExpReqs();

	BOOL Insert(const GBDeviceInfo&  object_record, BOOL b_last_operate, OnOperationCmd p_call_back = nullptr, FIELD_CONLIST_PTR p_con_lst = nullptr);

	BOOL Delete(const GBDeviceInfo&  object_record, OnOperationCmd p_call_back = nullptr, FIELD_CONLIST_PTR p_con_lst = nullptr);
	BOOL Update(const GBDeviceInfo&  object_record, OnOperationCmd p_call_back = nullptr, FIELD_CONLIST_PTR p_con_lst = nullptr, BOOL b_last_update = FALSE);

	BOOL Search(GBDeviceInfo& _tosearch_info_, DeviceInfoList& results, LONG need_count = 0, const TCHAR* sort_col = nullptr, FIELD_CONLIST_PTR p_con_lst = nullptr);
	BOOL IsFind(const CString& gb_device_id_);

	void StartDirtyWorkProc(LPVOID lParam);


private:
	// log and apply all the write requests
	BOOL ApplyRequest(RECORD_PTR  record, Request::Action action, OnOperationCmd p_call_back, BOOL b_last_update = 0, FIELD_CONLIST_PTR p_con_lst = nullptr);
	BOOL Request2LogString(const CString& p_file_name, Request::Action action, CString& p_log_file);
	BOOL LogRequest(RECORD_PTR record, Request::Action action);
	//BOOL UnLogRequest(const GBRecordAdaptor& record, Request::Action action) const;

	//For Recovery
	//BOOL OnSearchBeforeRecover(CSuperFileProc::_FindFile& p_find_obj, int n_find_nums);

	BOOL ExcpRecoverDeleteCallBack(GBDeviceInfo& device_info_);
	BOOL ExcpRecoverUpdateCallBack(GBDeviceInfo& device_info_);

	//BOOL GetFileInfo(const CString& gb_device_id_, GBDeviceInfo& device_info_);
	//call back of write operations registered to the sqlite module
	bool RequestCallBack(Request& sql_req, bool result);

	static CString GetTrashFolder(CString file_name);

#define SQLITE_DB_EXT   _T(".db")
#define SQLITE_DB_PATH  (s_db_root_)
#define SQLITE_DB_FULL_PATH  (s_db_root_+ s_db_name_ + SQLITE_DB_EXT)
	//#define INDEX_LOG_PATH (s_db_root_+ s_db_name_ + _T("\\"))

	//db object
	SQLiteModule     sqlite_;
	BOOL             b_sql_opened_;
	CString          s_db_root_;
	CString          s_db_name_;
	CString          s_action_cache_path_;

	WriteDelegate    on_write_operator;
	//ReadDelegate     on_read_operator;
	std::vector<CString>   v_excep_reqs_;

	//DISALLOW_COPY_AND_ASSIGN
	DataBaseMgr(const DataBaseMgr&);
	//void operator=(const DataBaseMgr&);
};