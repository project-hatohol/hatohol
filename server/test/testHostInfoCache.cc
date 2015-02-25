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
#include "HostInfoCache.h"
using namespace std;
using namespace mlpl;

namespace testHostInfoCache {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

void test_updateAndGetName(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = INVALID_HOST_ID;
	const HostIdType hostIdInServer = 2;
	svHostDef.hostIdInServer = StringUtils::sprintf("%" FMT_HOST_ID,
	                                                hostIdInServer);
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	string name;
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, name));
	cppcut_assert_equal(svHostDef.name, name);
}

void test_getNameExpectFalse(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = INVALID_HOST_ID;
	svHostDef.serverId = 100;
	svHostDef.hostIdInServer = "2";
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	string name;
	cppcut_assert_equal(false, hiCache.getName(500, name));
}

void test_updateTwice(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = INVALID_HOST_ID;
	svHostDef.serverId = 100;
	const HostIdType hostIdInServer = 2;
	svHostDef.hostIdInServer = StringUtils::sprintf("%" FMT_HOST_ID,
	                                                hostIdInServer);
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	string name;
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, name));
	cppcut_assert_equal(svHostDef.name, name);

	// update again
	svHostDef.name = "Dog Dog Dog Cat";
	hiCache.update(svHostDef);
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, name));
	cppcut_assert_equal(svHostDef.name, name);
}

void test_getNameFromMany(void)
{
	struct DataArray {
		HostIdType id;
		const char *name;
	} dataArray [] = {
		{105,   "You"},
		{211,   "Hydrogen"},
		{5,     "foo"},
		{10555, "3K background radition is not 4K display"},
		{4,     "I like strawberry."},
	};
	const size_t numData = ARRAY_SIZE(dataArray);

	HostInfoCache hiCache;
	for (size_t i = 0; i < numData; i++) {
		ServerHostDef svHostDef;
		svHostDef.id = AUTO_INCREMENT_VALUE;
		svHostDef.hostId = INVALID_HOST_ID;
		svHostDef.serverId = 100;
		svHostDef.hostIdInServer =
		  StringUtils::sprintf("%" FMT_HOST_ID,  dataArray[i].id);
		svHostDef.name = dataArray[i].name;
		hiCache.update(svHostDef);
	}

	// check
	for (size_t i = 0; i < numData; i++) {
		string name;
		const HostIdType id = dataArray[i].id;
		cppcut_assert_equal(true, hiCache.getName(id, name));
		cppcut_assert_equal(string(dataArray[i].name), name);
	}
}

} // namespace testHostInfoCache
