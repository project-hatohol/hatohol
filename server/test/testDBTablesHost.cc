/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include "DBHatohol.h"
#include "DBTablesHost.h"
#include "Hatohol.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testDBTablesHost {

// This is temporary, I will make the setup function with the same name in
// DBClientTest. Then this method should be removed.
static void setupTestDBHatohol(void)
{
	setupTestDBConfig();
	static const char *TEST_DB_NAME = "test_db_hatohol";
	DBHatohol::setDefaultDBParams(TEST_DB_NAME,
	                              TEST_DB_USER, TEST_DB_PASSWORD);
	const bool dbRecreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, dbRecreate);
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDBHatohol();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	cppcut_assert_not_null(&dbHost);
	assertDBTablesVersion(dbHatohol.getDBAgent(),
	                      DB_TABLES_ID_HOST, DBTablesHost::TABLES_VERSION);
}

} // namespace testDBTablesHost
