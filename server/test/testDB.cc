/*
 * Copyright (C) 2014 Project Hatohol
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
#include <StringUtils.h>
#include "DB.h"
#include "DBTest.h"
using namespace std;
using namespace mlpl;

namespace testDB {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	TestDB::setup();
	TestDB db;
	cppcut_assert_equal(
	  true, db.getDBAgent().isTableExisting("_tables_version"));
}

void test_getAlwaysFalseConditionIsNotEmpty()
{
	cppcut_assert_equal(false, DB::getAlwaysFalseCondition().empty());
}

void test_isAlwaysFalseCondition(void)
{
	bool actual = DB::isAlwaysFalseCondition(DB::getAlwaysFalseCondition());
	cppcut_assert_equal(true, actual);
}

void test_isAlwaysFalseConditionReturnFalse(void)
{
	bool actual = DB::isAlwaysFalseCondition("1");
	cppcut_assert_equal(false, actual);
}

} // namespace testDB
