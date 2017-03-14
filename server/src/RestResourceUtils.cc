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

#include "RestResourceMonitoring.h"
#include "UnifiedDataStore.h"
#include "IncidentSenderManager.h"
#include "RestResourceUtils.h"
#include <string.h>

using namespace std;
using namespace mlpl;

struct SelectedHosts {
	struct FilterSet {
		map<string, map<string, string> > stringMap;
		ServerIdSet servers;
		ServerHostGrpSetMap hostgroups;
		ServerHostSetMap hosts;
	};

	FilterSet selected;
	FilterSet excluded;

	vector<HatoholError> errors;

	SelectedHosts(GHashTable *query)
	{
		parse(query);
	}

	void parse(GHashTable *query)
	{
		g_hash_table_foreach(query, gHashTableForeachFunc, this);
		convert();
	}

	static void gHashTableForeachFunc(gpointer key, gpointer value,
					  gpointer userData)
	{
		if (!key || !value)
			return;

		SelectedHosts &data =
		  *static_cast<SelectedHosts *>(userData);
		const string keyStr(static_cast<const char *>(key));
		const string valueStr(static_cast<const char *>(value));
		const string selectHosts("s");
		const string excludeHosts("e");

		if (StringUtils::hasPrefix(keyStr, selectHosts)) {
			data.parseHostQuery(data.selected.stringMap,
					    keyStr, valueStr, selectHosts);
		} else if (StringUtils::hasPrefix(keyStr, excludeHosts)) {
			data.parseHostQuery(data.excluded.stringMap,
					    keyStr, valueStr, excludeHosts);
		}
	}

	void parseHostQuery(map<string, map<string, string> > &hostsMap,
			    const string &key, const string &value,
			    const string &selectType)
	{
		parseId(hostsMap, key, value,
			selectType, string("serverId"));
		parseId(hostsMap, key, value,
			selectType, string("hostgroupId"));
		parseId(hostsMap, key, value,
			selectType, string("hostId"));
	}

	void parseId(map<string, map<string, string> > &hostsMap,
		     const string &key, const string &value,
		     const string &selectType, const string &idType)
	{
		string prefix(selectType + string("["));
		string suffix(StringUtils::sprintf("][%s]", idType.c_str()));
		if (!StringUtils::hasPrefix(key, prefix))
			return;
		if (!StringUtils::hasSuffix(key, suffix))
			return;
		size_t idLength = key.size() - prefix.size() - suffix.size();
		string index = key.substr(prefix.size(), idLength);
		if (index.empty()) {
			string message = StringUtils::sprintf(
			  "An index for a host filter is missing!: %s=%s",
			  key.c_str(), value.c_str());
			HatoholError err(HTERR_INVALID_PARAMETER, message);
			errors.push_back(err);
			return;
		}
		if (!StringUtils::isNumber(index)) {
			string message(
			  "An index for a host filter isn't numeric!: ");
			message += index;
			HatoholError err(HTERR_INVALID_PARAMETER, message);
			errors.push_back(err);
			return;
		}
		if (idType == "serverId" && !StringUtils::isNumber(value)) {
			string message(
			  "A serverId for a host filter isn't numeric!: ");
			message += value;
			HatoholError err(HTERR_INVALID_PARAMETER, message);
			errors.push_back(err);
			return;
		}
		hostsMap[index][idType] = value;
	}

	void convert(FilterSet &filterSet)
	{
		for (auto &pair: filterSet.stringMap) {
			map<string, string> &idMap = pair.second;
			if (idMap.find("serverId") == idMap.end()) {
				HatoholError err(
				  HTERR_INVALID_PARAMETER,
				  "serverId is missing for a host filter!");
				errors.push_back(err);
				continue;
			}
			if (idMap.find("hostgroupId") != idMap.end() &&
			    idMap.find("hostId") != idMap.end()) {
				HatoholError err(
				  HTERR_INVALID_PARAMETER,
				  "Both hostgroupId and hostId can't be "
				  "specified for a host filter!");
				errors.push_back(err);
				continue;
			}

			ServerIdType serverId =
			  StringUtils::toUint64(idMap["serverId"]);

			if (idMap.find("hostgroupId") != idMap.end()) {
				filterSet.hostgroups[serverId].insert(
				  idMap["hostgroupId"]);
			} else if (idMap.find("hostId") != idMap.end()) {
				filterSet.hosts[serverId].insert(
				  idMap["hostId"]);
			} else {
				filterSet.servers.insert(serverId);
			}
		}
	}

	void convert(void)
	{
		convert(selected);
		convert(excluded);
	}
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
HatoholError RestResourceUtils::parseHostsFilter(
  HostResourceQueryOption &option, GHashTable *query)
{
	SelectedHosts data(query);

	if (!data.errors.empty()) {
		// TODO: We should report all errors.
		return data.errors[0];
	}

	if (!data.selected.servers.empty())
		option.setSelectedServerIds(data.selected.servers);
	if (!data.excluded.servers.empty())
		option.setExcludedServerIds(data.excluded.servers);

	if (!data.selected.hostgroups.empty())
		option.setSelectedHostgroupIds(data.selected.hostgroups);
	if (!data.excluded.hostgroups.empty())
		option.setExcludedHostgroupIds(data.excluded.hostgroups);

	if (!data.selected.hosts.empty())
		option.setSelectedHostIds(data.selected.hosts);
	if (!data.excluded.hosts.empty())
		option.setExcludedHostIds(data.excluded.hosts);

	return HTERR_OK;
}

HatoholError RestResourceUtils::parseHostResourceQueryParameter(
  HostResourceQueryOption &option, GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// target server id
	ServerIdType targetServerId = ALL_SERVERS;
	err = getParam<ServerIdType>(query, "serverId",
				     "%" FMT_SERVER_ID,
				     targetServerId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetServerId(targetServerId);

	// target host group id
	HostgroupIdType targetHostgroupId = ALL_HOST_GROUPS;
	err = getParam<HostgroupIdType>(query, "hostgroupId",
					"%" FMT_HOST_GROUP_ID,
					targetHostgroupId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetHostgroupId(targetHostgroupId);

	// target host id
	LocalHostIdType targetHostId = ALL_LOCAL_HOSTS;
	err = getParam<LocalHostIdType>(query, "hostId", "%" FMT_LOCAL_HOST_ID,
	                                targetHostId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetHostId(targetHostId);

	// Plural hosts filter
	err = parseHostsFilter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// maximum number
	size_t maximumNumber = 0;
	err = getParam<size_t>(query, "limit", "%zd", maximumNumber);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	err = getParam<size_t>(query, "maximumNumber", "%zd", maximumNumber);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setMaximumNumber(maximumNumber);

	// offset
	uint64_t offset = 0;
	err = getParam<uint64_t>(query, "offset", "%" PRIu64, offset);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setOffset(offset);

	return HatoholError(HTERR_OK);
}

HatoholError RestResourceUtils::parseSortTypeFromQuery(
  EventsQueryOption::SortType &sortType, GHashTable *query)
{
	const char *key = "sortType";
	char *value = (char *)g_hash_table_lookup(query, key);
	if (!value)
		return HTERR_NOT_FOUND_PARAMETER;
	if (!strcasecmp(value, "time")) {
		sortType = EventsQueryOption::SORT_TIME;
	} else if (!strcasecmp(value, "unifiedId")) {
		sortType = EventsQueryOption::SORT_UNIFIED_ID;
	} else {
		string optionMessage
		  = StringUtils::sprintf("%s: %s", key, value);
		return HatoholError(HTERR_INVALID_PARAMETER, optionMessage);
	}
	return HatoholError(HTERR_OK);
}

HatoholError RestResourceUtils::parseSortOrderFromQuery(
  DataQueryOption::SortDirection &sortDirection, GHashTable *query)
{
	HatoholError err =
	   getParam<DataQueryOption::SortDirection>(query, "sortOrder", "%d",
						    sortDirection);
	if (err != HTERR_OK)
		return err;
	if (sortDirection != DataQueryOption::SORT_DONT_CARE &&
	    sortDirection != DataQueryOption::SORT_ASCENDING &&
	    sortDirection != DataQueryOption::SORT_DESCENDING) {
		return HatoholError(HTERR_INVALID_PARAMETER,
		                    StringUtils::sprintf("%d", sortDirection));
	}
	return HatoholError(HTERR_OK);
}

HatoholError RestResourceUtils::parseEventParameter(
  EventsQueryOption &option, GHashTable *query, bool &isCountOnly)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = parseHostResourceQueryParameter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// event type
	EventType type = EVENT_TYPE_ALL;
	err = getParam<EventType>(query, "type",
				  "%d", type);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setType(type);

	// minimum severity
	TriggerSeverityType severity = TRIGGER_SEVERITY_UNKNOWN;
	err = getParam<TriggerSeverityType>(query, "minimumSeverity",
					    "%d", severity);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setMinimumSeverity(severity);

	// trigger status
	TriggerStatusType status = TRIGGER_STATUS_ALL;
	err = getParam<TriggerStatusType>(query, "status",
					  "%d", status);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTriggerStatus(status);

	// trigger ID
	TriggerIdType triggerId = ALL_TRIGGERS;
	err = getParam<TriggerIdType>(query, "triggerId",
				      "%" FMT_TRIGGER_ID, triggerId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTriggerId(triggerId);

	// target hostname
	const char *targetHostname =
		static_cast<const char*>(g_hash_table_lookup(query, "hostname"));
	if (targetHostname && *targetHostname) {
		option.setHostnameList({targetHostname});
	}

	// sort type
	EventsQueryOption::SortType sortType = EventsQueryOption::SORT_TIME;
	err = parseSortTypeFromQuery(sortType, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// sort order
	DataQueryOption::SortDirection sortDirection
	  = DataQueryOption::SORT_DESCENDING;
	err = parseSortOrderFromQuery(sortDirection, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	option.setSortType(sortType, sortDirection);

	// limit of unifiedId
	uint64_t limitOfUnifiedId = 0;
	err = getParam<uint64_t>(query, "limitOfUnifiedId", "%" PRIu64,
				 limitOfUnifiedId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setLimitOfUnifiedId(limitOfUnifiedId);

	// begin time
	timespec beginTime = { 0, 0 };
	err = getParam<time_t>(query, "beginTime", "%ld", beginTime.tv_sec);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setBeginTime(beginTime);

	// end time
	timespec endTime = { 0, 0 };
	err = getParam<time_t>(query, "endTime", "%ld", endTime.tv_sec);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setEndTime(endTime);

	// types
	const gchar *value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "types"));
	if (value && *value) {
		StringVector values;
		StringUtils::split(values, value, ',');
		std::set<EventType> types;
		for (auto &type: values) {
			uint64_t v = StringUtils::toUint64(type);
			types.insert(static_cast<EventType>(v));
		}
		option.setEventTypes(types);
	}

	// severities
	value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "severities"));
	if (value && *value) {
		StringVector values;
		StringUtils::split(values, value, ',');
		std::set<TriggerSeverityType> severities;
		for (auto &severity: values) {
			uint64_t v = StringUtils::toUint64(severity);
			severities.insert(static_cast<TriggerSeverityType>(v));
		}
		option.setTriggerSeverities(severities);
	}

	// statuses
	value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "statuses"));
	if (value && *value) {
		StringVector values;
		StringUtils::split(values, value, ',');
		std::set<TriggerStatusType> statuses;
		for (auto &status: values) {
			uint64_t v = StringUtils::toUint64(status);
			statuses.insert(static_cast<TriggerStatusType>(v));
		}
		option.setTriggerStatuses(statuses);
	}

	// incident statuses
	value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "incidentStatuses"));
	if (value && *value) {
		StringVector values;
		StringUtils::split(values, value, ',', false);
		std::set<string> statuses;
		for (auto &status: values)
			statuses.insert(status);
		option.setIncidentStatuses(statuses);
	}

	// countOnly
	value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "countOnly"));
	if (value) {
		string val(value);
		isCountOnly = (val == "true");
		if (val != "true" && val != "false") {
			string message(
			  StringUtils::sprintf(
			    "Invalid value for countOnly: %s", value));
			return HatoholError(HTERR_INVALID_PARAMETER, message);
		}
	}

	return HatoholError(HTERR_OK);
}

HatoholError RestResourceUtils::parseHostgroupNameParameter(
  HostResourceQueryOption &option, GHashTable *query, DataQueryContextPtr &dataQueryContextPtr)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	// Add constraints with hostgroup name
	const char *targetHostgroupName =
	  static_cast<const char*>(g_hash_table_lookup(query, "hostgroupName"));
	if (targetHostgroupName && *targetHostgroupName) {
		string hostgroupName = targetHostgroupName;
		ServerHostGrpSetMap serverHostGrpSetMap;
		HostgroupsQueryOption hostgroupOption(dataQueryContextPtr);
		hostgroupOption.setTargetHostgroupName(hostgroupName);
		dataStore->getServerHostGrpSetMap(serverHostGrpSetMap,
						  hostgroupOption);
		option.setSelectedHostgroupIds(serverHostGrpSetMap);
	}

	return HatoholError(HTERR_OK);
}

HatoholError RestResourceUtils::parseTriggerParameter(
  TriggersQueryOption &option, GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = RestResourceUtils::parseHostResourceQueryParameter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// target hostname
	const char *targetHostname =
		static_cast<const char*>(g_hash_table_lookup(query, "hostname"));
	if (targetHostname && *targetHostname) {
		option.setHostnameList({targetHostname});
	}

	// target trigger brief
	const char *targetTriggerBrief =
		static_cast<const char*>(g_hash_table_lookup(query, "triggerBrief"));
	if (targetTriggerBrief && *targetTriggerBrief) {
		string triggerBrief = targetTriggerBrief;
		option.setTriggerBrief(triggerBrief);
	}

	// minimum severity
	TriggerSeverityType severity = TRIGGER_SEVERITY_UNKNOWN;
	err = getParam<TriggerSeverityType>(query, "minimumSeverity",
					    "%d", severity);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setMinimumSeverity(severity);

	// trigger status
	TriggerStatusType status = TRIGGER_STATUS_ALL;
	err = getParam<TriggerStatusType>(query, "status",
					  "%d", status);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTriggerStatus(status);

	// begin time
	timespec beginTime = { 0, 0 };
	err = getParam<time_t>(query, "beginTime", "%ld", beginTime.tv_sec);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setBeginTime(beginTime);

	// end time
	timespec endTime = { 0, 0 };
	err = getParam<time_t>(query, "endTime", "%ld", endTime.tv_sec);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setEndTime(endTime);

	return HatoholError(HTERR_OK);
}
