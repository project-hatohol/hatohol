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

#pragma once
#include "DBTablesMonitoring.h"
#include "HostInfoCache.h"

class HatoholDBUtils {
public:
	static void transformTriggersToHatoholFormat(
	  TriggerInfoList &trigInfoList, const std::shared_ptr<const ItemTable> triggers,
	  const ServerIdType &serverId, const HostInfoCache &hostInfoCache);

	static void transformEventsToHatoholFormat(
	  EventInfoList &eventInfoList, const std::shared_ptr<const ItemTable> events,
	  const ServerIdType &serverId);

	static void transformGroupsToHatoholFormat(
	  HostgroupVect &hostgroups, const std::shared_ptr<const ItemTable> groups,
	  const ServerIdType &serverId);

	static void transformHostsGroupsToHatoholFormat(
	  HostgroupMemberVect &hostgroupMembers,
	  const std::shared_ptr<const ItemTable> mapHostHostgroups,
	  const ServerIdType &serverId, const HostInfoCache &hostInfoCache);

	static void transformHostsToHatoholFormat(
	  ServerHostDefVect &svHostDefs, const std::shared_ptr<const ItemTable> hosts,
	  const ServerIdType &serverId);

	static void transformHistoryToHatoholFormat(
	  HistoryInfoVect &historyInfoVect, const std::shared_ptr<const ItemTable> items,
	  const ServerIdType &serverId);

protected:
	static int getItemVariable(const std::string &word);

	/**
	 * check if the given word is a variable (e.g. $1, $2, ...).
	 * @params word.
	 * A word to be checked.
	 * @return
	 * A variable number if the given word is a variable. Otherwise
	 * -1 is returned.
	 */
	static void extractItemKeys(mlpl::StringVector &params,
	                            const std::string &key);

	static std::string makeItemBrief(const ItemGroup *itemItemGroup);

	static bool transformEventItemGroupToEventInfo(
	  EventInfo &eventInfo, const ItemGroup *event);

	static void transformGroupItemGroupToHostgroupInfo(
	  Hostgroup &hostgroup, const ItemGroup *groupItemGroup);

	static void transformHostsGroupsItemGroupToHatoholFormat(
	  HostgroupMember &hostgrpMember, const ItemGroup *groupHostsGroups,
	  const ServerIdType &serverId, const HostInfoCache &hostInfoCache);

	static void transformHistoryItemGroupToHistoryInfo(
	  HistoryInfo &historyInfo, const ItemGroup *item);
};

