/*
 * Copyright (C) 2015 Project Hatohol
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

#include "TestHostResourceQueryOption.h"
#include "DBAgentTest.h"

using namespace std;

// This is just to pass the build and won't work.
static HostResourceQueryOption::Synapse synapse(
  tableProfileTest, 0, 0,
  tableProfileTest, 0, false,
  tableProfileTest, 0, 0, 0);

TestHostResourceQueryOption::TestHostResourceQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapse, userId)
{
}

string TestHostResourceQueryOption::callGetServerIdColumnName(void) const
{
	return getServerIdColumnName();
}

string TestHostResourceQueryOption::callMakeConditionServer(
  const ServerIdSet &serverIdSet, const string &serverIdColumnName) const
{
	return makeConditionServer(serverIdSet, serverIdColumnName);
}

string TestHostResourceQueryOption::callMakeCondition(
  const ServerHostGrpSetMap &srvHostGrpSetMap,
  const string &serverIdColumnName,
  const string &hostgroupIdColumnName,
  const string &hostIdColumnName,
  const ServerIdType &targetServerId,
  const HostgroupIdType &targetHostgroupId,
  const LocalHostIdType &targetHostId) const
{
	return makeCondition(srvHostGrpSetMap,
	                     serverIdColumnName,
	                     hostgroupIdColumnName,
	                     hostIdColumnName,
	                     targetServerId,
	                     targetHostgroupId,
	                     targetHostId);
}
