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
	svHostDef.hostId = 0x1234;
	const LocalHostIdType hostIdInServer = "2";
	svHostDef.hostIdInServer = hostIdInServer;
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	HostInfoCache::Element cacheElem;
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, cacheElem));
	cppcut_assert_equal(svHostDef.name, cacheElem.name);
	cppcut_assert_equal(svHostDef.hostId, cacheElem.hostId);
}

void test_getNameExpectFalse(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 0;
	svHostDef.serverId = 100;
	svHostDef.hostIdInServer = "2";
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	HostInfoCache::Element cacheElem;
	cppcut_assert_equal(false, hiCache.getName("500", cacheElem));
}

void test_updateTwice(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 0x7890;
	svHostDef.serverId = 100;
	const LocalHostIdType hostIdInServer = "2";
	svHostDef.hostIdInServer = hostIdInServer;
	svHostDef.name = "foo";

	HostInfoCache hiCache;
	hiCache.update(svHostDef);
	HostInfoCache::Element cacheElem;
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, cacheElem));
	cppcut_assert_equal(svHostDef.name, cacheElem.name);
	cppcut_assert_equal(svHostDef.hostId, cacheElem.hostId);

	// update again
	svHostDef.name = "Dog Dog Dog Cat";
	hiCache.update(svHostDef);
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, cacheElem));
	cppcut_assert_equal(svHostDef.name, cacheElem.name);
	cppcut_assert_equal(svHostDef.hostId, cacheElem.hostId);
}

void test_getNameFromMany(void)
{
	struct DataArray {
		LocalHostIdType id;
		const char *name;
	} dataArray [] = {
		{"105",   "You"},
		{"211",   "Hydrogen"},
		{"5",     "foo"},
		{"10555", "3K background radition is not 4K display"},
		{"4",     "I like strawberry."},
	};
	const size_t numData = ARRAY_SIZE(dataArray);

	struct HostIdGen {
		HostIdType operator ()(size_t i)
		{
			return (i + 1) * 2;
		}
	} hostIdGen;

	HostInfoCache hiCache;
	for (size_t i = 0; i < numData; i++) {
		ServerHostDef svHostDef;
		svHostDef.id = AUTO_INCREMENT_VALUE;
		svHostDef.hostId = hostIdGen(i);
		svHostDef.serverId = 100;
		svHostDef.hostIdInServer = dataArray[i].id;
		svHostDef.name = dataArray[i].name;
		hiCache.update(svHostDef);
	}

	// check
	for (size_t i = 0; i < numData; i++) {
		HostInfoCache::Element cacheElem;
		const LocalHostIdType &id = dataArray[i].id;
		cppcut_assert_equal(true, hiCache.getName(id, cacheElem));
		cppcut_assert_equal(string(dataArray[i].name), cacheElem.name);
		cppcut_assert_equal(hostIdGen(i), cacheElem.hostId);
	}
}

void test_registerAdHoc(void)
{
	HostInfoCache hiCache;
	HostInfoCache::Element elem;
	const LocalHostIdType hostIdInServer = "host ideee";
	const string name = "onamae";
	hiCache.registerAdHoc(hostIdInServer, name, elem);
	cppcut_assert_equal(INVALID_HOST_ID, elem.hostId);
	cppcut_assert_equal(name, elem.name);

	HostInfoCache::Element elem2;
	cppcut_assert_equal(true, hiCache.getName(hostIdInServer, elem2));
	cppcut_assert_equal(INVALID_HOST_ID, elem2.hostId);
	cppcut_assert_equal(name, elem2.name);
}

} // namespace testHostInfoCache
