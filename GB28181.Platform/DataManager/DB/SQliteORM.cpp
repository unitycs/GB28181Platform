#include "stdafx.h"
#include "SQliteORM.h"
#include "SuperFileProc.h"
#include "SearchObject.h"
#include "./../RecordAdaptor/GBDeviceRecord.h"

DataBaseMgr::DataBaseMgr(void) :
	b_sql_opened_(FALSE),
	s_db_root_(_T("")),
	s_db_name_(_T("")),
	s_action_cache_path_(_T(""))
{
	v_excep_reqs_.clear();
	on_write_operator = bind(&DataBaseMgr::RequestCallBack, this, std::placeholders::_1, std::placeholders::_2);
}

DataBaseMgr::~DataBaseMgr(void)
{
	Close();
}

BOOL DataBaseMgr::Open(const CString& s_database_root, const CString& s_database_name, const TableList& table_list)
{
	CString strRootPath(s_database_root);

	// Normalize path.
	CSuperFileProc pFileProc;
	strRootPath.TrimRight(_T(" "));
	strRootPath.TrimRight(_T("\\"));

	if (strRootPath.IsEmpty()) return FALSE;

	// Check path existence
	strRootPath += "\\";
	if (!PathIsDirectory(strRootPath)) {
		if (!pFileProc.CreateFolder(strRootPath)) {
			return FALSE;
		}
	}

	CString str_db_file = strRootPath + s_database_name + SQLITE_DB_EXT;

	//if opened a db already, check if the same one,
	//if the same, return TRUE,or return FALSE
	if (b_sql_opened_ == TRUE)
	{
		if (str_db_file.Compare(SQLITE_DB_FULL_PATH) == 0) {
			return TRUE;
		}
		return FALSE;
	}

	CString s_cache_log_root = strRootPath + s_database_name + _T("\\");
	if (!PathIsDirectory(s_cache_log_root)) {
		if (!pFileProc.CreateFolder(s_cache_log_root)) {
			return FALSE;
		}
	}
	SetFileAttributes(s_cache_log_root, FILE_ATTRIBUTE_HIDDEN);

	if (0 == sqlite_.SQLiteInitial(str_db_file, s_database_name, table_list, on_write_operator)) {
		b_sql_opened_ = TRUE;
		s_db_root_ = strRootPath;
		s_db_name_ = s_database_name;
		s_action_cache_path_ = s_cache_log_root;
	}

	return b_sql_opened_;
}

BOOL DataBaseMgr::Close()
{
	if (b_sql_opened_) {
		b_sql_opened_ = !(0 == sqlite_.SQLiteUninitial());
	}

	return (!b_sql_opened_);
}

BOOL DataBaseMgr::Insert(const GBDeviceInfo&  object_record, BOOL b_last_operate, OnOperationCmd p_call_back, FIELD_CONLIST_PTR p_con_lst)
{
	if (!b_sql_opened_
		&& object_record._gb_device_id_.IsEmpty()
		|| object_record._gb_device_id_.GetLength() != 20) {
		return FALSE;
	}
	RECORD_PTR table_record(new GBRecordAdaptor(object_record));
	return ApplyRequest(table_record, Request::kActionInsert, p_call_back, b_last_operate);
}

BOOL DataBaseMgr::Delete(const GBDeviceInfo&  object_record, OnOperationCmd p_call_back, FIELD_CONLIST_PTR p_con_lst)//ActionCallBack p_callback)
{
	if (!b_sql_opened_ || object_record._gb_device_id_.GetLength() != 20) {
		return FALSE;
	}
	RECORD_PTR table_record(new GBRecordAdaptor(object_record));
	return ApplyRequest(table_record, Request::kActionDelete, p_call_back);
}

BOOL DataBaseMgr::Update(const GBDeviceInfo&  object_record, OnOperationCmd p_call_back, FIELD_CONLIST_PTR p_con_lst, BOOL b_last_update)
{
	if (!b_sql_opened_ || (!object_record._gb_device_id_.IsEmpty() && object_record._gb_device_id_.GetLength() != 20)) {
		return FALSE;
	}
	//GBRecordAdaptor table_record(object_record);

	RECORD_PTR table_record(new GBRecordAdaptor(object_record));

	return ApplyRequest(table_record, Request::KActionUpdate, p_call_back, b_last_update, p_con_lst);
}

BOOL DataBaseMgr::ApplyRequest(RECORD_PTR record, Request::Action action, OnOperationCmd p_call_back, BOOL b_last_update, FIELD_CONLIST_PTR p_con_lst)
{
	if (!LogRequest(record, action)) {
		return FALSE;
	}

	// Need to refine here! #####
	Request req;
	req.action_ = action;
	req.b_last_update_ = b_last_update;
	record->toRequest(req);
	if (p_call_back) p_call_back(req);

	if (p_con_lst)
	{
		req.p_conList_ = p_con_lst;
	}

	if (0 != sqlite_.SQLiteWriteRequest(req)) {
		CString _log_msg_;
		_log_msg_.Format(_T("Apply: %d, for table: %s,faield: Lots requests in queue"), req.action_, req.table_name_);
		sqlite_.PrintfLog(_log_msg_);
	}
	return TRUE;
}

BOOL DataBaseMgr::Request2LogString(const CString& p_file_name, Request::Action action, CString& p_log_file)
{
	// Need to check the method    #####
	//filepath.action
	TCHAR pAction[8] = { 0 };
	TCHAR pFilePath[_MAX_PATH] = { 0 };
	TCHAR pFilePathOld[_MAX_PATH] = { 0 };

	TCHAR * Pt = pFilePath;
	TCHAR * ptOld = pFilePathOld;

	_stprintf_s(pAction, _T("[%d]"), action);
	//file path string
	_tcscpy_s(pFilePathOld, p_file_name);

	while (*ptOld != _T('\0'))
	{
		if (*ptOld == _T('\\')) {
			*Pt++ = _T(']');
			*Pt++ = _T('[');
			ptOld++;
		}
		else if (*ptOld == _T(':') && *(ptOld + 1) == _T('\\'))
		{
			*Pt++ = _T(']');
			*Pt++ = _T('[');
			ptOld += 2;
		}
		else {
			*Pt++ = *ptOld++;
		}
	}
	*Pt++ = _T('.');
	*Pt = _T('\0');

	p_log_file = pFilePath;
	p_log_file += pAction;
	return TRUE;
}

BOOL DataBaseMgr::LogRequest(RECORD_PTR /*record*/, Request::Action /*action*/)
{
	//CString logFile;

	/*if (!Request2LogString(record._gb_device_id_, action, logFile)) {
		return FALSE;
	}

	logFile = s_index_log_path_ + logFile;
	if (logFile.GetLength() > _MAX_PATH) {
		return FALSE;
	}

	HANDLE hFile = CreateFile(logFile, GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);*/
	return TRUE;
}

bool DataBaseMgr::RequestCallBack(Request& sql_req, bool result)
{
	GBRecordAdaptor record;

	if (result&&record.fromRequested(sql_req)) {
		//UnLogRequest(record, sql_req.action_);
	}
	else {
		//report alarm
	}

	//no matter what, we return true
	return TRUE;
}

BOOL DataBaseMgr::OpenAndRecoverExpReqs(const CString& s_database_root, const CString& s_database_name, const TableList& field_header_list)
{
	if (!Open(s_database_root, s_database_name, field_header_list)) {
		return FALSE;
	}

	return RecoverExpReqs();
}

BOOL DataBaseMgr::RecoverExpReqs()
{
	if (!b_sql_opened_) {
		return FALSE;
	}

	if (!PathIsDirectory(s_action_cache_path_))
	{
		return FALSE;
	}
	CSuperFileProc file_proc;

	//pCallBack.bind(this, &DataBaseMgr::OnSearchBeforeRecover);
	//pEvent += pCallBack;
	v_excep_reqs_.clear();

	std::map<CString, int> m_request;
	//combine the log strings
	for (auto it = v_excep_reqs_.begin(); it != v_excep_reqs_.end(); ++it)
	{
		if (it->GetLength() < 3) continue;

		CString file_name;
		int action;

		TCHAR gard1 = it->GetAt(it->GetLength() - 1);
		TCHAR gard2 = it->GetAt(it->GetLength() - 3);
		action = it->GetAt(it->GetLength() - 2) - '0';

		if (gard1 != _T(']') ||
			gard2 != _T('[') ||
			action > Request::kActionDelete ||
			action < Request::kActionInsert - 1) {
			//file_proc.Delete(nullptr, s_index_log_path_ + *it, nullptr);
			continue;
		}

		file_name = it->Left(it->GetLength() - 3);

		if (m_request.find(file_name) == m_request.end()) {
			m_request.emplace(file_name, action);
		}
		else {
			int old_action = m_request[file_name];
			int del_action = action;
			switch (action)
			{
			case Request::kActionInsert:
			{
				if (old_action != Request::kActionDelete) {
					del_action = old_action;
					m_request[file_name] = action;
				}
			}
			break;
			case Request::kActionDelete:
				del_action = old_action;
				m_request[file_name] = action;
				break;
			case Request::KActionUpdate:
				break;
			case Request::kActionInsert - 1:
				break;
			}

			file_name = s_action_cache_path_ + *it;

			if (del_action != action) {
				TCHAR del_act[1];
				_stprintf_s(del_act, _T("%d"), del_action);
				file_name.SetAt(file_name.GetLength() - 2, del_act[0]);
			}

			//file_proc.Delete(nullptr, file_name, FALSE);
		}
	}

	//handle all request
	/*for (auto it = m_request.begin(); it != m_request.end(); ++it)
	{
	}*/

	return TRUE;
}

CString DataBaseMgr::GetTrashFolder(CString file_name) {
	CString ret = _T("");

	if (file_name.IsEmpty()) {
		return ret;
	}

	TCHAR drive_str[_MAX_PATH + 1] = { 0 };
	_tcscpy_s(drive_str, file_name);
	if (!PathStripToRoot(drive_str)) {
		return ret;
	}

	if (!PathIsDirectory(drive_str)) {
		return ret;
	}

	ret = drive_str;
	ret.TrimRight(_T(" "));
	ret.TrimRight(_T("\\"));
	ret += _T("\\hus_trash_videos\\");
	// Check path existence
	if (!PathIsDirectory(ret)) {
		CSuperFileProc pFileProc;
		if (!pFileProc.CreateFolder(ret)) {
			return _T("");
		}

		SetFileAttributes(ret, FILE_ATTRIBUTE_HIDDEN);
	}
	return ret;
}

BOOL DataBaseMgr::ExcpRecoverDeleteCallBack(GBDeviceInfo & /*device_info_*/)
{
	return 0;
}

BOOL DataBaseMgr::ExcpRecoverUpdateCallBack(GBDeviceInfo & /*device_info_*/)
{
	return 0;
}

BOOL DataBaseMgr::Search(GBDeviceInfo& _tosearch_info_, DeviceInfoList& results, LONG need_count, const TCHAR* sort_col, FIELD_CONLIST_PTR p_con_lst)
{
	GBRecordAdaptor record_condition_(_tosearch_info_);
	SearchRequest search_condition_;
	search_condition_.need_count_ = need_count;
	search_condition_.sort_col_ = sort_col;
	search_condition_.p_conList_ = p_con_lst;
	record_condition_.toRequest(search_condition_);
	SearchObject search_obj(sqlite_, results);
	return search_obj.Search(search_condition_);
}

BOOL DataBaseMgr::IsFind(const CString & /*gb_device_id_*/)
{
	//DiskCondition d_c;
	//d_c.s_disks_.push_back(gb_device_id_);
	//d_c.b_exc_ = FALSE;
	DeviceInfoList result;

	//if (Search(NULL, NULL, &d_c, 1, 0, FALSE, result) && result.size()>0) {
	//	return TRUE;
	//}

	return FALSE;
}

void DataBaseMgr::StartDirtyWorkProc(LPVOID /*lParam*/)
{
	sqlite_._SQLTaskProc();
}

BOOL DataBaseMgr::OpenAndStartTransaction(const CString& s_database_root, const CString& s_database_name, const TableList& field_header_list)
{
	CString database_path(s_database_root);
	CString dbFile = database_path + "\\" + s_database_name + SQLITE_DB_EXT;
	if (PathFileExists(dbFile)) {
		if (!DeleteFile(dbFile))
		{
			return FALSE;
		}
	}

	CString database_journal_path(dbFile);
	database_journal_path.Replace(_T(".db"), _T(".db-journal"));
	if (PathFileExists(database_journal_path)) {
		if (!DeleteFile(database_journal_path))
		{
			return FALSE;
		}
	}

	int rc = this->Open(s_database_root, s_database_name, field_header_list);
	if (rc)
	{
		rc = sqlite_.SQLiteBeginTrasaction() == 0;
	}

	return rc;
}

BOOL DataBaseMgr::SubmitTransactionAndClose()
{
	int rc = sqlite_.SQLiteCommitTrasaction() == 0;
	if (rc)
	{
		rc = Close();
	}

	return rc;
}

BOOL DataBaseMgr::InsertInTransaction(CString _table_name, CString /*device_id_*/, long /*start_time*/, long /*end_time*/, int /*n_file_size*/, CString /*path*/) const
{
	FieldInfoList field_info_list;
	field_info_list.clear();

	//field_info_list.push_back(FieldInfoT(device_id_, FieldInfoT::kFieldNull));
	//field_info_list.push_back(FieldInfoT(start_time, FieldInfoT::kFieldNull));
	//field_info_list.push_back(FieldInfoT(end_time, FieldInfoT::kFieldNull));
	//field_info_list.push_back(FieldInfoT(n_file_size, FieldInfoT::kFieldNull));
	//field_info_list.push_back(FieldInfoT(path, FieldInfoT::kFieldNull));

	return sqlite_.Insert(_table_name, field_info_list) == 0;
}