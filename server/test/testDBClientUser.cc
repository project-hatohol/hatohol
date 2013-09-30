/*
 * Copyright (C) 2013 Project Hatohol
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
#include <cutter.h>

#include "DBClientUser.h"
#include "Helpers.h"
#include "Hatohol.h"

namespace testDBClientUser {

static const char *TEST_DB_NAME = "test_db_user";
static const char *TEST_DB_USER = "hatohol_test_user";
static const char *TEST_DB_PASSWORD = ""; // empty: No password is used

void cut_setup(void)
{
	hatoholInit();
	DBClient::setDefaultDBParams(
	  DB_DOMAIN_ID_USERS, TEST_DB_NAME, TEST_DB_USER, TEST_DB_PASSWORD);

	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
}

void cut_teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientUser dbUser;
	cppcut_assert_equal(DB_DOMAIN_ID_USERS,
	                    dbUser.getDBAgent()->getDBDomainId());
}

} // namespace testDBClientUser
