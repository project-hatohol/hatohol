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

TestHostResourceQueryOption::TestHostResourceQueryOption(
  const Synapse &synapse, const UserIdType &userId)
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

string TestHostResourceQueryOption::callMakeConditionAllowedHosts(
  const ServerHostGrpSetMap &srvHostGrpSetMap) const
{
	return makeConditionAllowedHosts(srvHostGrpSetMap);
}
