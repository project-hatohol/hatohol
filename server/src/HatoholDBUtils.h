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

#ifndef HatoholDBUtils_h
#define HatoholDBUtils_h

#include "DBClientHatohol.h"
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
	  HostgroupInfoList &groupInfoList, const ItemTablePtr groups,
	  const ServerIdType &serverId);

	static void transformHostsGroupsToHatoholFormat(
	  HostgroupElementList &hostgroupElementList,
	  const ItemTablePtr mapHostHostgroups,
	  const ServerIdType &serverId);

	static void transformHostsToHatoholFormat(
	  HostInfoList &hostInfoList, const ItemTablePtr hosts,
	  const ServerIdType &serverId);
	static void transformItemsToHatoholFormat(
	  ItemInfoList &eventInfoList, MonitoringServerStatus &serverStatus,
	  const ItemTablePtr items, const ItemTablePtr applications);

protected:
	typedef std::map<uint64_t, std::string> AppliationIdNameMap;
	typedef AppliationIdNameMap::iterator   ApplicationIdNameMapIterator;
	typedef AppliationIdNameMap::const_iterator
	   ApplicationIdNameMapConstIterator;

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
	  HostgroupInfo &groupInfo, const ItemGroup *groupItemGroup);

	static void transformHostsGroupsItemGroupToHatoholFormat(
	  HostgroupElement &hostgroupElement,
	  const ItemGroup *groupHostsGroups);

	static void transformHostsItemGroupToHatoholFormat(
	  HostInfo &hostInfo, const ItemGroup *groupHosts);

	static bool transformItemItemGroupToItemInfo(
	  ItemInfo &itemInfo, const ItemGroup *item,
	  const AppliationIdNameMap &appIdNameMap);
};

#endif // HatoholDBUtils_h
