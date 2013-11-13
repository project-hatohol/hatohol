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

#include "Hatohol.h"
#include "DataQueryOption.h"
#include "DBClientUser.h"
#include "DBClientTest.h"
#include "Helpers.h"

namespace testDataQueryOption {

class TestQueryOption : public DataQueryOption {
public:
	const string getServerIdColumnName(void)
	{
		return "";
	}

	const string getHostGroupIdColumnName(void)
	{
		return "";
	}
};

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_initialValue(void)
{
	TestQueryOption opt;
	cppcut_assert_equal(INVALID_USER_ID, opt.getUserId());
}

void test_operatorEqToMySelf(void)
{
	DataQueryOption lhs;
	cppcut_assert_equal(true, lhs == lhs);
}

void test_operatorEqInitailObject(void)
{
	DataQueryOption lhs;
	DataQueryOption rhs;
	cppcut_assert_equal(true, lhs == rhs);
}

void test_operatorEq(void)
{
	DataQueryOption lhs;
	DataQueryOption rhs;
	lhs.setMaximumNumber(2);
	rhs.setMaximumNumber(2);
	cppcut_assert_equal(true, lhs == rhs);
}

void test_operatorEqFail(void)
{
	DataQueryOption lhs;
	DataQueryOption rhs;
	rhs.setMaximumNumber(2);
	cppcut_assert_equal(false, lhs == rhs);
}

void test_copyConstructor(void)
{
	DataQueryOption opt;
	opt.setSortOrder(DataQueryOption::SORT_ASCENDING);
	opt.setMaximumNumber(1234);
	DataQueryOption copied(opt);
	cppcut_assert_equal(true, opt == copied);
}

void test_setGetUserId(void)
{
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);
	const UserIdType userId = testUserInfo[2].id;
	TestQueryOption opt;
	opt.setUserId(userId);
	cppcut_assert_equal(userId, opt.getUserId());
}

void test_setGetMaximumNumber(void)
{
	size_t maxNum = 12345;
	DataQueryOption option;
	option.setMaximumNumber(maxNum);
	cppcut_assert_equal(maxNum, option.getMaximumNumber());
}

void test_getDefaultMaximumNumber(void)
{
	DataQueryOption option;
	cppcut_assert_equal(DataQueryOption::NO_LIMIT,
	                    option.getMaximumNumber());
}

void test_setGetSortOrder(void)
{
	DataQueryOption option;
	DataQueryOption::SortOrder order = DataQueryOption::SORT_ASCENDING;
	option.setSortOrder(order);
	cppcut_assert_equal(order, option.getSortOrder());
}

void test_getDefaultSortOrder(void)
{
	DataQueryOption option;
	cppcut_assert_equal(DataQueryOption::SORT_DONT_CARE,
	                    option.getSortOrder());
}

void test_setGetStartId(void)
{
	uint64_t startId = 8;
	DataQueryOption option;
	option.setStartId(startId);
	cppcut_assert_equal(startId, option.getStartId());
}

void test_getDefaultStartId(void)
{
	DataQueryOption option;
	cppcut_assert_equal((uint64_t)0, option.getStartId());
}

} // namespace testDataQueryOption
