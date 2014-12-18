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
#include "OperationPrivilege.h"
#include "DataQueryContext.h"
#include "Hatohol.h"
#include "Helpers.h"
#include "DBTablesTest.h"

namespace testDataQueryContext {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

static DataQueryContextPtr setupAndCreateDataQueryContext(void)
{
	const UserIdType userId = 1;
	DataQueryContextPtr dqctx(new DataQueryContext(userId), false);
	return dqctx;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getServerHostGrpSetMap(void)
{
	DataQueryContextPtr dqctx = setupAndCreateDataQueryContext();
	const ServerHostGrpSetMap &setMap = dqctx->getServerHostGrpSetMap();

	// We only confirm the returned value has a valid address.
	// The sanity of the content shall be checked in testDBTablesUser.
	cppcut_assert_not_null(&setMap);
}

void test_getValidServerIdSet(void)
{
	DataQueryContextPtr dqctx = setupAndCreateDataQueryContext();
	const ServerIdSet &svIdSet = dqctx->getValidServerIdSet();

	// We only confirm the returned value has a valid address.
	// The sanity of the content shall be checked in testDBTablesConfig.
	cppcut_assert_not_null(&svIdSet);
}

} // namespace testDataQueryContext
