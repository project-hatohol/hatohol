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

} // namespace testDBTablesLastInfo
