#pragma once
#include "SQliteORM.h"
class SearchObject
{
public:
	typedef DataBaseMgr::TimeSpaceCondition TimeSpaceCondition;
	typedef DataBaseMgr::ReadDelegate ReadDelegate;
	SearchObject(SQLiteModule& p_sql, DeviceInfoList& results);
	~SearchObject(void);

	BOOL Search(SearchRequest& rqst);

private:
	BOOL Add(RECORD_PTR device_info_);
	bool SearchCallBack(Request& p_result);

	SQLiteModule&    sql_search_core_;
	ReadDelegate     on_get_result_;
	//interrupt parameters
	LONG            n_total_need_count_;

	//outputs
	DeviceInfoList&   v_results_;
	LONG              n_total_searched_count_;
};
