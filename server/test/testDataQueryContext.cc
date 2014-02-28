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
#include "OperationPrivilege.h"
#include "DataQueryContext.h"
#include "Hatohol.h"
#include "Helpers.h"

namespace testDataQueryContext {

void cut_setup(void)
{
	hatoholInit();
}

static DataQueryContextPtr setupAndCreateDataQueryContext(void)
{
	setupTestDBUser(true, true);
	const UserIdType userId = 1;
	DataQueryContextPtr dqctx(new DataQueryContext(userId), false);
	return dqctx;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getServerHostGrpSetMap(void)
{
	setupTestDBUser();
	const UserIdType userId = 1;
	DataQueryContextPtr dqctx(new DataQueryContext(userId), false);
	const ServerHostGrpSetMap &setMap = dqctx->getServerHostGrpSetMap();

	// We only confirm the returned value has a valid address.
	// The sanity of the content shall be checked in testDBClientUser.
	cppcut_assert_not_null(&setMap);
}

void test_getValidServerIdSet(void)
{
	DataQueryContextPtr dqctx = setupAndCreateDataQueryContext();
	const ServerIdSet &svIdSet = dqctx->getValidServerIdSet();

	// We only confirm the returned value has a valid address.
	// The sanity of the content shall be checked in testDBClientConfig.
	cppcut_assert_not_null(&svIdSet);
}

} // namespace testDataQueryContext
