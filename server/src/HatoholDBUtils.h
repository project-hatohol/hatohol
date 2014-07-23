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

class HatoholDBUtils {
public:
	static void transformEventsToHatoholFormat(
	  EventInfoList &eventInfoList, const ItemTablePtr events,
	  const ServerIdType &serverId);
	static bool transformEventItemGroupToEventInfo(
	  EventInfo &eventInfo, const ItemGroup *event);
	static void transformGroupItemGroupToHostgroupInfo(
	  HostgroupInfo &groupInfo, const ItemGroup *groupItemGroup);
	static void transformGroupsToHatoholFormat(
	  HostgroupInfoList &groupInfoList, const ItemTablePtr groups,
	  const ServerIdType &serverId);
	static void transformHostsGroupsItemGroupToHatoholFormat(
	  HostgroupElement &hostgroupElement,
	  const ItemGroup *groupHostsGroups);
	static void transformHostsGroupsToHatoholFormat(
	  HostgroupElementList &hostgroupElementList,
	  const ItemTablePtr mapHostHostgroups,
	  const ServerIdType &serverId);
};

#endif // HatoholDBUtils_h
