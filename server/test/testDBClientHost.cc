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
#include "Hatohol.h"
#include "DBClientHost.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testDBClientHost {

void cut_setup(void)
{
	hatoholInit();
	const bool dbRecreate = true;
	setupTestDBHost(dbRecreate);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientHost dbHost;
	DBAgent *dbAgent = dbHost.getDBAgent();
	cppcut_assert_not_null(dbAgent);
	cppcut_assert_equal(DB_DOMAIN_ID_HOST, dbAgent->getDBDomainId());
}

void test_setDefaultDBParams(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_HOST);
	cppcut_assert_equal(string(TEST_DB_USER), connInfo.user);
	cppcut_assert_equal(string(TEST_DB_PASSWORD), connInfo.password);
}

} // namespace testDBClientHost
