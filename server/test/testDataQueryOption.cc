/*
 * Copyright (C) 2013-2014 Project Hatohol
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
using namespace std;;

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

static void getTestSortOrderList(DataQueryOption::SortOrderList &sortOrderList)
{
	sortOrderList.push_back(DataQueryOption::SortOrder(
	  "column1", DataQueryOption::SORT_DESCENDING));
	sortOrderList.push_back(DataQueryOption::SortOrder(
	  "column3", DataQueryOption::SORT_ASCENDING));
	sortOrderList.push_back(DataQueryOption::SortOrder(
	  "column2", DataQueryOption::SORT_DESCENDING));
}

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
	DataQueryOption::SortOrderList sortOrderList;
	getTestSortOrderList(sortOrderList);
	DataQueryOption lhs;
	DataQueryOption rhs;
	lhs.setMaximumNumber(2);
	rhs.setMaximumNumber(2);
	lhs.setSortOrderList(sortOrderList);
	rhs.setSortOrderList(sortOrderList);
	cppcut_assert_equal(true, lhs == rhs);
}

void test_operatorEqFail(void)
{
	DataQueryOption lhs;
	DataQueryOption rhs;
	rhs.setMaximumNumber(2);
	cppcut_assert_equal(false, lhs == rhs);
}

void test_operatorEqWithDifferentSortOrder(void)
{
	DataQueryOption::SortOrderList sortOrderList;
	getTestSortOrderList(sortOrderList);
	DataQueryOption lhs;
	DataQueryOption rhs;
	lhs.setSortOrderList(sortOrderList);
	cppcut_assert_equal(false, lhs == rhs);
}

void test_copyConstructor(void)
{
	DataQueryOption opt;
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

void test_getOrderByWithColumn(void)
{
	DataQueryOption option;
	DataQueryOption::SortOrder order1(
	  "column1", DataQueryOption::SORT_ASCENDING);
	option.setSortOrder(order1);
	cppcut_assert_equal(string("column1 ASC"), option.getOrderBy());
}

void test_getOrderByWithColumnDesc(void)
{
	DataQueryOption option;
	DataQueryOption::SortOrder order1(
	  "column1", DataQueryOption::SORT_DESCENDING);
	option.setSortOrder(order1);
	cppcut_assert_equal("column1 DESC", option.getOrderBy().c_str());
}

void test_getOrderByWithMultipleColumns(void)
{
	DataQueryOption option;
	DataQueryOption::SortOrderList sortOrderList;
	getTestSortOrderList(sortOrderList);
	option.setSortOrderList(sortOrderList);
	cppcut_assert_equal("column1 DESC, column3 ASC, column2 DESC",
			    option.getOrderBy().c_str());
}

void test_getDataQueryContextOfDefaultConstructor(void)
{
	TestQueryOption opt;
	DataQueryContext &dataQueryCtx = opt.getDataQueryContext();
	cppcut_assert_equal(1, dataQueryCtx.getUsedCount());
}

void test_getDataQueryContextOfCopyConstructor(void)
{
	TestQueryOption opt0;
	DataQueryContext &dataQueryCtx0 = opt0.getDataQueryContext();
	{
		TestQueryOption opt1(opt0);
		DataQueryContext &dataQueryCtx1 = opt1.getDataQueryContext();
		cppcut_assert_equal(&dataQueryCtx0, &dataQueryCtx1);
		cppcut_assert_equal(opt0.getUserId(), opt1.getUserId());
		cppcut_assert_equal(2, dataQueryCtx0.getUsedCount());
	}
	cppcut_assert_equal(1, dataQueryCtx0.getUsedCount());
}

} // namespace testDataQueryOption
