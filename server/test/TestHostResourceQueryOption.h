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

#ifndef TestHostResourceQueryOption_h
#define TestHostResourceQueryOption_h

#include <string>

class TestHostResourceQueryOption : public HostResourceQueryOption {
public:
	std::string callGetServerIdColumnName(void) const
	{
		return getServerIdColumnName();
	}

	static std::string callMakeConditionServer(
	  const ServerIdSet &serverIdSet, const std::string &serverIdColumnName)
	{
		return makeConditionServer(serverIdSet, serverIdColumnName);
	}

	static std::string callMakeCondition(
	  const ServerHostGrpSetMap &srvHostGrpSetMap,
	  const std::string &serverIdColumnName,
	  const std::string &hostgroupIdColumnName,
	  const std::string &hostIdColumnName,
	  uint32_t targetServerId = ALL_SERVERS,
	  const HostgroupIdType &targetHostgroupId = ALL_HOST_GROUPS,
	  uint64_t targetHostId = ALL_HOSTS)
	{
		return makeCondition(srvHostGrpSetMap,
		                     serverIdColumnName,
		                     hostgroupIdColumnName,
		                     hostIdColumnName,
		                     targetServerId,
		                     targetHostgroupId,
		                     targetHostId);
	}
};

#endif // TestHostResourceQueryOption_h
