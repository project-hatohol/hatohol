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

#ifndef HatoholDBUtils_h
#define HatoholDBUtils_h

#include "DBTablesMonitoring.h"
#include "HostInfoCache.h"

class HatoholDBUtils {
public:
	static void transformTriggersToHatoholFormat(
	  TriggerInfoList &trigInfoList, const ItemTablePtr triggers,
	  const ServerIdType &serverId, const HostInfoCache &hostInfoCache);

	static void transformEventsToHatoholFormat(
	  EventInfoList &eventInfoList, const ItemTablePtr events,
	  const ServerIdType &serverId);

	static void transformGroupsToHatoholFormat(
	  HostgroupVect &hostgroups, const ItemTablePtr groups,
	  const ServerIdType &serverId);

	static void transformHostsGroupsToHatoholFormat(
	  HostgroupElementList &hostgroupElementList,
	  const ItemTablePtr mapHostHostgroups,
	  const ServerIdType &serverId);

	static void transformHostsToHatoholFormat(
	  HostInfoList &hostInfoList, const ItemTablePtr hosts,
	  const ServerIdType &serverId);

	static void transformItemsToHatoholFormat(
	  ItemInfoList &itemInfoList, MonitoringServerStatus &serverStatus,
	  const ItemTablePtr items, const ItemTablePtr applications);

	static void transformHistoryToHatoholFormat(
	  HistoryInfoVect &historyInfoVect, const ItemTablePtr items,
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
	  HostgroupElement &hostgroupElement,
	  const ItemGroup *groupHostsGroups);

	static void transformHostsItemGroupToHatoholFormat(
	  HostInfo &hostInfo, const ItemGroup *groupHosts);

	static bool transformItemItemGroupToItemInfo(
	  ItemInfo &itemInfo, const ItemGroup *item,
	  const ItemCategoryNameMap &itemCategoryNameMap);

	static void transformHistoryItemGroupToHistoryInfo(
	  HistoryInfo &historyInfo, const ItemGroup *item);
};

#endif // HatoholDBUtils_h
