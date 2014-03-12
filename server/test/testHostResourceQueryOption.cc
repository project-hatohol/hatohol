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
#include <gcutter.h>
#include <StringUtils.h>
#include "HostResourceQueryOption.h"
#include "TestHostResourceQueryOption.h"
#include "DBClientHatohol.h"

using namespace std;
using namespace mlpl;

namespace testHostResourceQueryOption {

void test_constructorDataQueryContext(void)
{
	const UserIdType userId = USER_ID_SYSTEM;
	DataQueryContextPtr dqCtxPtr =
	  DataQueryContextPtr(new DataQueryContext(userId), false);
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
	{
		HostResourceQueryOption opt(dqCtxPtr);
		cppcut_assert_equal((DataQueryContext *)dqCtxPtr,
		                    &opt.getDataQueryContext());
		cppcut_assert_equal(2, dqCtxPtr->getUsedCount());
	}
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
}

void test_copyConstructor(void)
{
	HostResourceQueryOption opt0;
	cppcut_assert_equal(1, opt0.getDataQueryContext().getUsedCount());
	{
		HostResourceQueryOption opt1(opt0);
		cppcut_assert_equal(&opt0.getDataQueryContext(),
		                    &opt1.getDataQueryContext());
		cppcut_assert_equal(2,
			            opt0.getDataQueryContext().getUsedCount());
	}
	cppcut_assert_equal(1, opt0.getDataQueryContext().getUsedCount());
}

void test_makeConditionServer(void)
{
	const string serverIdColumnName = "cat";
	const ServerIdType serverIds[] = {5, 15, 105, 1080};
	const size_t numServerIds = sizeof(serverIds) / sizeof(ServerIdType);

	ServerIdSet svIdSet;
	for (size_t i = 0; i < numServerIds; i++)
		svIdSet.insert(serverIds[i]);

	string actual = TestHostResourceQueryOption::callMakeConditionServer(
	                  svIdSet, serverIdColumnName);

	// check
	string expectHead = serverIdColumnName;
	expectHead += " IN (";
	string actualHead(actual, 0, expectHead.size());
	cppcut_assert_equal(expectHead, actualHead);

	string expectTail = ")";
	string actualTail(actual, actual.size()-1, expectTail.size());
	cppcut_assert_equal(expectTail, actualTail);

	StringVector actualIds;
	size_t len = actual.size() -  expectHead.size() - 1;
	string actualBody(actual, expectHead.size(), len);
	StringUtils::split(actualIds, actualBody, ',');
	cppcut_assert_equal(numServerIds, actualIds.size());
	ServerIdSetIterator expectId = svIdSet.begin(); 
	for (int i = 0; expectId != svIdSet.end(); ++expectId, i++) {
		string expect =
		  StringUtils::sprintf("%"FMT_SERVER_ID, *expectId);
		cppcut_assert_equal(expect, actualIds[i]);
	}
}

void test_makeConditionServerWithEmptyIdSet(void)
{
	ServerIdSet svIdSet;
	string actual = TestHostResourceQueryOption::callMakeConditionServer(
	                  svIdSet, "meet");
	cppcut_assert_equal(DBClientHatohol::getAlwaysFalseCondition(), actual);
}

void test_defaultValueOfFilterForDataOfDefunctServers(void)
{
	HostResourceQueryOption opt;
	cppcut_assert_equal(true, opt.getFilterForDataOfDefunctServers());
}

void data_setGetOfFilterForDataOfDefunctServers(void)
{
	gcut_add_datum("Disable filtering",
	               "enable", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Eanble filtering",
	               "enable", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_setGetOfFilterForDataOfDefunctServers(gconstpointer data)
{
	HostResourceQueryOption opt;
	bool enable = gcut_data_get_boolean(data, "enable");
	opt.setFilterForDataOfDefunctServers(enable);
	cppcut_assert_equal(enable, opt.getFilterForDataOfDefunctServers());
}

} // namespace testHostResourceQueryOption
