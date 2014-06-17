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
#include "HostInfoCache.h"
using namespace std;

namespace testHostInfoCache {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

void test_updateAndGetName(void)
{
	HostInfo hostInfo;
	hostInfo.serverId = 100;
	hostInfo.id = 2;
	hostInfo.hostName = "foo";

	HostInfoCache hiCache;
	hiCache.update(hostInfo);
	string name;
	cppcut_assert_equal(true, hiCache.getName(hostInfo.id, name));
	cppcut_assert_equal(hostInfo.hostName, name);
}

} // namespace testHostInfoCache
