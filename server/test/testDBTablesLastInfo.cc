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

static void _assertLastInfo(const LastInfoDef &expect, const LastInfoDef &actual)
{
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.dataType, actual.dataType);
	cppcut_assert_equal(expect.value, actual.value);
	cppcut_assert_equal(expect.serverId, actual.serverId);
}
#define assertLastInfo(E,A) cut_trace(_assertLastInfo(E,A))

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

void test_upsertLastInfo(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);
	LastInfoDef lastInfo;
	lastInfo.id = AUTO_INCREMENT_VALUE;
	lastInfo.dataType = LAST_INFO_HOST;
	lastInfo.value = "201505151340.0000000000";
	lastInfo.serverId = 10001;
	OperationPrivilege privilege(USER_ID_SYSTEM);
	LastInfoIdType lastInfoId = dbLastInfo.upsertLastInfo(lastInfo, privilege);
	const string statement = "SELECT * FROM last_info WHERE last_info_id = "+
		to_string(static_cast<int>(NumTestLastInfoDef + 1));
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%d",
			       lastInfoId, lastInfo.dataType,
			       lastInfo.value.c_str(), lastInfo.serverId);
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((LastInfoIdType)AUTO_INCREMENT_VALUE, lastInfoId);
}

void test_upsertLastInfoUpdate(void)
{
	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);
	LastInfoDef lastInfo;
	lastInfo.id = AUTO_INCREMENT_VALUE;
	lastInfo.dataType = LAST_INFO_HOST;
	lastInfo.value = "1432103640";
	lastInfo.serverId = 10001;

	OperationPrivilege privilege(USER_ID_SYSTEM);
	LastInfoIdType id0 = dbLastInfo.upsertLastInfo(lastInfo, privilege);
	lastInfo.id = id0;
	LastInfoIdType id1 = dbLastInfo.upsertLastInfo(lastInfo, privilege);
	const string statement = "SELECT * FROM last_info";
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%d",
			       id1, lastInfo.dataType,
			       lastInfo.value.c_str(), lastInfo.serverId);
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id1);
}

void test_updateLastInfo(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);

	LastInfoDef lastInfo;
	lastInfo.id = 3;
	lastInfo.dataType = LAST_INFO_TRIGGER;
	lastInfo.value = "201505201540.0000000000";
	lastInfo.serverId = 12001;
	OperationPrivilege privilege(USER_ID_SYSTEM);
	dbLastInfo.updateLastInfo(lastInfo, privilege);
	const string statement = "SELECT * FROM last_info WHERE last_info_id = 3";
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%d",
			       lastInfo.id, lastInfo.dataType,
			       lastInfo.value.c_str(), lastInfo.serverId);
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);
}

void test_getLastInfoListWithoutOption(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);

	LastInfoDefList lastInfoList;
	LastInfoQueryOption option(USER_ID_SYSTEM);
	dbLastInfo.getLastInfoList(lastInfoList, option);
	{
		size_t i = 0;
		for (auto lastInfo : lastInfoList) {
			// ignore id assertion. Because id is auto increment.
			lastInfo.id = 0;
			assertLastInfo(testLastInfoDef[i], lastInfo);
			i++;
		}
	}
}

void test_getLastInfoListWithOption(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);

	LastInfoDefList lastInfoList;
	LastInfoQueryOption option(USER_ID_SYSTEM);
	option.setLastInfoType(LAST_INFO_TRIGGER);
	dbLastInfo.getLastInfoList(lastInfoList, option);
	LastInfoDefListIterator it = lastInfoList.begin();
	cppcut_assert_equal((size_t)1, lastInfoList.size());
	LastInfoDef &lastInfoFirst = *it;
	lastInfoFirst.id = 0; // ignore id assertion. Because id is auto increment.
	int targetId = 4;
	assertLastInfo(testLastInfoDef[targetId], lastInfoFirst);
}

void test_deleteLastInfoList(void)
{
	loadTestDBLastInfo();

	DECLARE_DBTABLES_LAST_INFO(dbLastInfo);

	LastInfoIdType targetId = 5;
	LastInfoIdType actualId = targetId + 1;
	// check existence
	const string statement =
	  "SELECT * FROM last_info WHERE last_info_id = " +
	  to_string(actualId);
	const string expect =
	  StringUtils::sprintf("%" FMT_LAST_INFO_ID "|%d|%s|%d",
	                       actualId,
	                       testLastInfoDef[targetId].dataType,
	                       testLastInfoDef[targetId].value.c_str(),
	                       testLastInfoDef[targetId].serverId);
	assertDBContent(&dbLastInfo.getDBAgent(), statement, expect);

	LastInfoIdList idList = {actualId};
	LastInfoQueryOption option(USER_ID_SYSTEM);
	dbLastInfo.deleteLastInfoList(idList, option);

	const string afterDeleteStatement =
	  "SELECT * FROM last_info WHERE last_info_id = " +
	  to_string(actualId);
	const string afterDeleteExpect = "";
	assertDBContent(&dbLastInfo.getDBAgent(),
	                afterDeleteStatement, afterDeleteExpect);
}
} // namespace testDBTablesLastInfo
