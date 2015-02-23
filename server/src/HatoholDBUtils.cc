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

#include "HatoholDBUtils.h"
#include "DBTablesMonitoring.h"
#include <ZabbixAPI.h>

using namespace std;
using namespace mlpl;

enum {
	EVENT_OBJECT_TRIGGER = 0,
	EVENT_OBJECT_DHOST,
	EVENT_OBJECT_DSERVICE,
	EVENT_OBJECT_ZABBIX_ACTIVE
};

struct BriefElem {
	string word;
	int    variableIndex;
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
void HatoholDBUtils::transformTriggersToHatoholFormat(
  TriggerInfoList &trigInfoList, const ItemTablePtr triggers,
  const ServerIdType &serverId, const HostInfoCache &hostInfoCache)
{
	const ItemGroupList &trigGrpList = triggers->getItemGroupList();
	ItemGroupListConstIterator trigGrpItr = trigGrpList.begin();
	for (; trigGrpItr != trigGrpList.end(); ++trigGrpItr) {
		ItemGroupStream trigGroupStream(*trigGrpItr);
		TriggerInfo trigInfo;
		std::string expandedDescription;

		trigInfo.serverId = serverId;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
		trigGroupStream >> trigInfo.id;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_VALUE);
		trigGroupStream >> trigInfo.status;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_PRIORITY);
		trigGroupStream >> trigInfo.severity;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_LASTCHANGE);
		trigGroupStream >> trigInfo.lastChangeTime.tv_sec;
		trigInfo.lastChangeTime.tv_nsec = 0;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_DESCRIPTION);
		trigGroupStream >> trigInfo.brief;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_HOSTID);
		trigGroupStream >> trigInfo.hostId;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_EXPANDED_DESCRIPTION);
		trigGroupStream >> expandedDescription;

		if (!expandedDescription.empty()) {
			JSONBuilder agent;
			agent.startObject();
			agent.add("expandedDescription", expandedDescription);
			agent.endObject();
			trigInfo.extendedInfo = agent.generate();
		}

		if (trigInfo.hostId != INAPPLICABLE_HOST_ID &&
		    !hostInfoCache.getName(trigInfo.hostId,
		                           trigInfo.hostName)) {
			MLPL_WARN(
			  "Ignored a trigger whose host name was not found: "
			  "server: %" FMT_SERVER_ID ", host: %" FMT_HOST_ID
			  "\n",
			  serverId, trigInfo.hostId);
			continue;
		}

		trigInfoList.push_back(trigInfo);
	}
}

void HatoholDBUtils::transformEventsToHatoholFormat(
  EventInfoList &eventInfoList, const ItemTablePtr events,
  const ServerIdType &serverId)
{
	const ItemGroupList &itemGroupList = events->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		EventInfo eventInfo;
		initEventInfo(eventInfo);
		eventInfo.serverId = serverId;
		if (!transformEventItemGroupToEventInfo(eventInfo, *it))
			continue;
		eventInfoList.push_back(eventInfo);
	}
}

void HatoholDBUtils::transformGroupsToHatoholFormat(
  HostgroupVect &hostgroups, const ItemTablePtr groups,
  const ServerIdType &serverId)
{
	const ItemGroupList &itemGroupList = groups->getItemGroupList();
	hostgroups.reserve(itemGroupList.size());
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		Hostgroup hostgrp;
		hostgrp.serverId = serverId;
		transformGroupItemGroupToHostgroupInfo(hostgrp, *it);
		hostgroups.push_back(hostgrp);
	}
}

void HatoholDBUtils::transformHostsGroupsToHatoholFormat(
  HostgroupMemberVect &hostgroupMembers,
  const ItemTablePtr mapHostHostgroups, const ServerIdType &serverId)
{
	const ItemGroupList &itemGroupList = mapHostHostgroups->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		HostgroupMember hostgrpMember;
		hostgrpMember.serverId = serverId;
		transformHostsGroupsItemGroupToHatoholFormat(
		  hostgrpMember, *it);
		hostgroupMembers.push_back(hostgrpMember);
	}
}

void HatoholDBUtils::transformHostsToHatoholFormat(
  ServerHostDefVect &svHostDefs, const ItemTablePtr hosts,
  const ServerIdType &serverId)
{
	const ItemGroupList &itemGroupList = hosts->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		ServerHostDef svHostDef;
		ItemGroupStream itemGroupStream(*it);

		svHostDef.id = AUTO_INCREMENT_VALUE;
		svHostDef.hostId = AUTO_ASSIGNED_ID;
		svHostDef.serverId = serverId;

		itemGroupStream.seek(ITEM_ID_ZBX_HOSTS_HOSTID);
		svHostDef.hostIdInServer =
		  itemGroupStream.read<uint64_t, string>();

		itemGroupStream.seek(ITEM_ID_ZBX_HOSTS_NAME);
		itemGroupStream >> svHostDef.name;

		svHostDef.status = HOST_STAT_NORMAL;

		svHostDefs.push_back(svHostDef);
	}
}

void HatoholDBUtils::transformItemsToHatoholFormat(
  ItemInfoList &itemInfoList, MonitoringServerStatus &serverStatus,
  const ItemTablePtr items, const ItemTablePtr applications)
{
	// Make application map
	ItemCategoryNameMap itemCategoryNameMap;
	const ItemGroupList &appGroupList = applications->getItemGroupList();
	ItemGroupListConstIterator appGrpItr = appGroupList.begin();
	for (; appGrpItr != appGroupList.end(); ++appGrpItr) {
		uint64_t appId;
		string   appName;
		ItemGroupStream itemGroupStream(*appGrpItr);

		itemGroupStream.seek(ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID);
		itemGroupStream >> appId;

		itemGroupStream.seek(ITEM_ID_ZBX_APPLICATIONS_NAME);
		itemGroupStream >> appName;

		itemCategoryNameMap[appId] = appName;
	}

	// Make ItemInfoList
	const ItemGroupList &itemGroupList = items->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		ItemInfo itemInfo;
		itemInfo.serverId = serverStatus.serverId;
		if (!transformItemItemGroupToItemInfo(itemInfo, *it,
		                                      itemCategoryNameMap))
			continue;
		itemInfoList.push_back(itemInfo);
	}

	ItemInfoListConstIterator item_it = itemInfoList.begin();
	serverStatus.nvps = 0.0;
	for (; item_it != itemInfoList.end(); ++item_it) {
		if ((*item_it).delay != 0)
			serverStatus.nvps += 1.0/(*item_it).delay;
	}
}

void HatoholDBUtils::transformHistoryToHatoholFormat(
  HistoryInfoVect &historyInfoVect,  const ItemTablePtr items,
  const ServerIdType &serverId)
{
	// Make HistoryInfoVect
	const ItemGroupList &itemGroupList = items->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		HistoryInfo historyInfo;
		historyInfo.serverId = serverId;
		transformHistoryItemGroupToHistoryInfo(historyInfo, *it);
		historyInfoVect.push_back(historyInfo);
	}
}

// ----------------------------------------------------------------------------
// Protected methods
// ----------------------------------------------------------------------------
int HatoholDBUtils::getItemVariable(const string &word)
{
	if (word.empty())
		return -1;

	// check the first '$'
	const char *wordPtr = word.c_str();
	if (wordPtr[0] != '$')
		return -1;

	// check if the remaining characters are all numbers.
	int val = 0;
	for (size_t idx = 1; wordPtr[idx]; idx++) {
		val *= 10;
		if (wordPtr[idx] < '0')
			return -1;
		if (wordPtr[idx] > '9')
			return -1;
		val += wordPtr[idx] - '0';
	}
	return val;
}

void HatoholDBUtils::extractItemKeys(StringVector &params, const string &key)
{
	// find '['
	size_t pos = key.find_first_of('[');
	if (pos == string::npos)
		return;

	// we assume the last character is ']'
	if (key[key.size()-1] != ']') {
		MLPL_WARN("The last charater is not ']': %s", key.c_str());
		return;
	}

	// split parameters
	string paramString(key, pos+1, key.size()-pos-2);
	if (paramString.empty()) {
		params.push_back("");
		return;
	}
	const bool doMerge = false;
	StringUtils::split(params, paramString, ',', doMerge);
}

string HatoholDBUtils::makeItemBrief(const ItemGroup *itemItemGroup)
{
	ItemGroupStream itemGroupStream(itemItemGroup);

	// get items.name
	string name;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_NAME);
	itemGroupStream >> name;
	StringVector vect;
	StringUtils::split(vect, name, ' ');

	// summarize word and variables ($n : n = 1,2,3,..)
	vector<BriefElem> briefElemVect;
	for (size_t i = 0; i < vect.size(); i++) {
		briefElemVect.push_back(BriefElem());
		BriefElem &briefElem = briefElemVect.back();
		briefElem.variableIndex = getItemVariable(vect[i]);
		if (briefElem.variableIndex <= 0)
			briefElem.word = vect[i];
	}

	// extract words to be replace
	string itemKey;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_KEY_);
	itemGroupStream >> itemKey;

	StringVector params;
	extractItemKeys(params, itemKey);

	// make a brief
	string brief;
	for (size_t i = 0; i < briefElemVect.size(); i++) {
		BriefElem &briefElem = briefElemVect[i];
		if (briefElem.variableIndex <= 0) {
			brief += briefElem.word;
		} else if (params.size() >= (size_t)briefElem.variableIndex) {
			// normal case
			brief += params[briefElem.variableIndex-1];
		} else {
			// error case
			MLPL_WARN("Not found index: %d, %s, %s\n",
			          briefElem.variableIndex,
			          name.c_str(), itemKey.c_str());
			brief += "<INTERNAL ERROR>";
		}
		if (i < briefElemVect.size()-1)
			brief += " ";
	}

	return brief;
}

bool HatoholDBUtils::transformEventItemGroupToEventInfo(
  EventInfo &eventInfo, const ItemGroup *eventItemGroup)
{
	ItemGroupStream itemGroupStream(eventItemGroup);

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_EVENTID);
	itemGroupStream >> eventInfo.id;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_OBJECT);
	if (itemGroupStream.read<int>() != EVENT_OBJECT_TRIGGER)
		return false;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_OBJECTID);
	itemGroupStream >> eventInfo.triggerId;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_CLOCK);
	itemGroupStream >> eventInfo.time.tv_sec;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_VALUE);
	itemGroupStream >> eventInfo.type;

	itemGroupStream.seek(ITEM_ID_ZBX_EVENTS_NS);
	itemGroupStream >> eventInfo.time.tv_nsec;

	// Trigger's value. This can be transformed from Event's value
	// This value is refered in ActionManager. So we set here.
	switch (eventInfo.type) {
	case EVENT_TYPE_GOOD:
		eventInfo.status = TRIGGER_STATUS_OK;
		break;
	case EVENT_TYPE_BAD:
		eventInfo.status = TRIGGER_STATUS_PROBLEM;
		break;
	case EVENT_TYPE_UNKNOWN:
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
		break;
	default:
		MLPL_ERR("Unknown type: %d\n", eventInfo.type);
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
	}

	return true;
}

void HatoholDBUtils::transformGroupItemGroupToHostgroupInfo(
  Hostgroup &hostgrp, const ItemGroup *groupItemGroup)
{
	hostgrp.id = AUTO_INCREMENT_VALUE;

	ItemGroupStream itemGroupStream(groupItemGroup);

	itemGroupStream.seek(ITEM_ID_ZBX_GROUPS_GROUPID);
	hostgrp.idInServer = itemGroupStream.read<uint64_t, string>();

	itemGroupStream.seek(ITEM_ID_ZBX_GROUPS_NAME);
	itemGroupStream >> hostgrp.name;
}

void HatoholDBUtils::transformHostsGroupsItemGroupToHatoholFormat(
  HostgroupMember &hostgrpMember, const ItemGroup *groupHostsGroups)
{
	hostgrpMember.id = AUTO_INCREMENT_VALUE;

	ItemGroupStream itemGroupStream(groupHostsGroups);

	itemGroupStream.seek(ITEM_ID_ZBX_HOSTS_GROUPS_HOSTID);
	hostgrpMember.hostIdInServer =
	  itemGroupStream.read<HostIdType, string>();

	itemGroupStream.seek(ITEM_ID_ZBX_HOSTS_GROUPS_GROUPID);
	hostgrpMember.hostgroupIdInServer =
	  itemGroupStream.read<HostgroupIdType, string>();
}

bool HatoholDBUtils::transformItemItemGroupToItemInfo(
  ItemInfo &itemInfo, const ItemGroup *itemItemGroup,
  const ItemCategoryNameMap &itemCategoryNameMap)
{
	itemInfo.lastValueTime.tv_nsec = 0;
	itemInfo.brief = makeItemBrief(itemItemGroup);

	ItemGroupStream itemGroupStream(itemItemGroup);

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_ITEMID);
	itemGroupStream >> itemInfo.id;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_HOSTID);
	itemGroupStream >> itemInfo.hostId;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_LASTCLOCK),
	itemGroupStream >> itemInfo.lastValueTime.tv_sec;
	if (itemInfo.lastValueTime.tv_sec == 0) {
		// We assume that the item in this case is a kind of
		// template such as 'Incoming network traffic on {#IFNAME}'.
		return false;
	}

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_LASTVALUE);
	itemGroupStream >> itemInfo.lastValue;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_PREVVALUE);
	itemGroupStream >> itemInfo.prevValue;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_DELAY);
	itemGroupStream >> itemInfo.delay;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_UNITS);
	itemGroupStream >> itemInfo.unit;

	int valueType;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_VALUE_TYPE);
	itemGroupStream >> valueType;
	itemInfo.valueType
	  = ZabbixAPI::toItemValueType(
	      static_cast<ZabbixAPI::ValueType>(valueType));

	ItemCategoryIdType itemCategoryId;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_APPLICATIONID);
	itemGroupStream >> itemCategoryId;

	if (itemCategoryId == NO_ITEM_CATEGORY_ID) {
		itemInfo.itemGroupName = "N/A";
	} else {
		ItemCategoryNameMapConstIterator it =
		  itemCategoryNameMap.find(itemCategoryId);
		if (it == itemCategoryNameMap.end()) {
			MLPL_ERR("Failed to get item category name: %"
			         FMT_ITEM_CATEGORY_ID "\n", itemCategoryId);
			return false;
		}
		itemInfo.itemGroupName = it->second;
	}

	return true;
}

void HatoholDBUtils::transformHistoryItemGroupToHistoryInfo(
  HistoryInfo &historyInfo, const ItemGroup *item)
{
	ItemGroupStream itemGroupStream(item);

	itemGroupStream.seek(ITEM_ID_ZBX_HISTORY_ITEMID);
	itemGroupStream >> historyInfo.itemId;

	uint64_t clock, ns;
	itemGroupStream.seek(ITEM_ID_ZBX_HISTORY_CLOCK);
	itemGroupStream >> clock;
	historyInfo.clock.tv_sec = static_cast<time_t>(clock);

	itemGroupStream.seek(ITEM_ID_ZBX_HISTORY_NS);
	itemGroupStream >> ns;
	historyInfo.clock.tv_nsec = static_cast<long>(ns);

	itemGroupStream.seek(ITEM_ID_ZBX_HISTORY_VALUE);
	itemGroupStream >> historyInfo.value;
}
