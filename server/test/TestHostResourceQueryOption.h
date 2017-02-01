/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#pragma once
#include <string>
#include <HostResourceQueryOption.h>

class TestHostResourceQueryOption : public HostResourceQueryOption {
public:
	TestHostResourceQueryOption(const Synapse &synapse,
				    const UserIdType &userId = INVALID_USER_ID);

	std::string callGetServerIdColumnName(void) const;

	std::string callMakeConditionServer(
	  const ServerIdSet &serverIdSet,
	  const std::string &serverIdColumnName) const;

	std::string callMakeConditionAllowedHosts(void) const;
	void callSetAllowedServersAndHostgroups(
	  const ServerHostGrpSetMap *map);
	void callSetValidServerIdSet(const ServerIdSet *set);
};

