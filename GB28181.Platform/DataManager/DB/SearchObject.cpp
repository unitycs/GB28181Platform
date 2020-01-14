#include "stdafx.h"
#include "SearchObject.h"
#include "./../RecordAdaptor/GBDeviceRecord.h"

SearchObject::SearchObject(SQLiteModule& p_sql, DeviceInfoList& results) :
	sql_search_core_(p_sql),
	on_get_result_(nullptr),
	n_total_need_count_(0),
	v_results_(results),
	n_total_searched_count_(0)
{
	//on_get_result_.bind(this, &SearchObject::SearchCallBack);
	on_get_result_ = bind(&SearchObject::SearchCallBack, this, std::placeholders::_1);
}

SearchObject::~SearchObject(void)
{
}

BOOL SearchObject::Search(SearchRequest& search_conditon)
{
	n_total_need_count_ = max(0, search_conditon.need_count_);

	LONG need_row_ = n_total_need_count_;
	CString row_cnt_for_limit;
	if (need_row_ > 0)
	{
		row_cnt_for_limit.Format(_T("%ld"), need_row_);
	}

	//sqlite> SELECT ID, NAME, SALARY FROM TABLE WHER TYPE ="IPC" AND NAME Like "%SSS%" LIMIT 6;;
	auto ret = sql_search_core_.SQLiteSelect(search_conditon.table_name_, search_conditon.fields_, row_cnt_for_limit, search_conditon.sort_col_, on_get_result_);
	return ret == 0;
}

BOOL SearchObject::Add(RECORD_PTR device_info_)
{
	for (auto& item : v_results_)
		if (item == device_info_)
			return FALSE;

	v_results_.push_back(device_info_);
	n_total_searched_count_++;
	return TRUE;
}

bool SearchObject::SearchCallBack(Request& p_result)
{
	RECORD_PTR record(new GBRecordAdaptor());

	if (!record->fromRequested(p_result)) {
		return false;
	}

	if (!Add(record)) {
		return true;
	}

	if (n_total_need_count_ > 0 && n_total_need_count_ == n_total_searched_count_) {
		return false;
	}

	return true;
}