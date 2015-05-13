/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "DBTablesLastInfo.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBHatohol.h"
#include "DBTablesTest.h"

using namespace std;
using namespace mlpl;

namespace testDBTablesLastInfo {

#define DECLARE_DBTABLES_LAST_INFO(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesLastInfo &VAR_NAME = _dbHatohol.getDBTablesLastInfo();

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);
	assertDBTablesVersion(
	  dbLastInfo.getDBAgent(),
	  DB_TABLES_ID_LAST_INFO, DBTablesLastInfo::LAST_INFO_DB_VERSION);
}

void test_addLastInfo(void)
{
	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);
	LastInfoDef lastInfo;
	lastInfo.dataType = LAST_INFO_HOST;
	lastInfo.value = "20150515";
	lastInfo.serverId = "10001";
	OperationPrivilege privilege(USER_ID_SYSTEM);
	LastInfoIdType lastInfoId = dbLastInfo.addLastInfo(lastInfo, privilege);
	const string statement = "SELECT * FROM last_info";
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%s",
			       lastInfoId, lastInfo.dataType,
			       lastInfo.value.c_str(), lastInfo.serverId.c_str());
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);
}

void test_updateLastInfo(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);

	LastInfoDef lastInfo;
	lastInfo.id = 3;
	lastInfo.dataType = LAST_INFO_TRIGGER;
	lastInfo.value = "20150531";
	lastInfo.serverId = "12001";
	OperationPrivilege privilege(USER_ID_SYSTEM);
	dbLastInfo.updateLastInfo(lastInfo, privilege);
	const string statement = "SELECT * FROM last_info WHERE last_info_id = 3";
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%s",
			       lastInfo.id, lastInfo.dataType,
			       lastInfo.value.c_str(), lastInfo.serverId.c_str());
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);
}
} // namespace testDBTablesLastInfo
