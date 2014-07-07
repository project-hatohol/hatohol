/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include "RestResourceHost.h"
#include "UnifiedDataStore.h"
#include <string.h>

using namespace std;
using namespace mlpl;

const char *RestResourceHost::pathForOverview  = "/overview";
const char *RestResourceHost::pathForHost      = "/host";
const char *RestResourceHost::pathForTrigger   = "/trigger";
const char *RestResourceHost::pathForEvent     = "/event";
const char *RestResourceHost::pathForItem      = "/item";
const char *RestResourceHost::pathForHostgroup = "/hostgroup";

static HatoholError parseSortTypeFromQuery(
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

static HatoholError parseSortOrderFromQuery(
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

static HatoholError parseHostResourceQueryParameter(
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
	HostIdType targetHostgroupId = ALL_HOST_GROUPS;
	err = getParam<HostgroupIdType>(query, "hostgroupId",
					"%" FMT_HOST_GROUP_ID,
					targetHostgroupId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetHostgroupId(targetHostgroupId);

	// target host id
	HostIdType targetHostId = ALL_HOSTS;
	err = getParam<HostIdType>(query, "hostId",
				   "%" FMT_HOST_ID,
				   targetHostId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetHostId(targetHostId);

	// maximum number
	size_t maximumNumber = 0;
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

static HatoholError parseTriggerParameter(TriggersQueryOption &option,
					  GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = parseHostResourceQueryParameter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

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

	return HatoholError(HTERR_OK);
}

HatoholError RestResourceHost::parseEventParameter(EventsQueryOption &option,
						   GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = parseHostResourceQueryParameter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

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

	return HatoholError(HTERR_OK);
}

static HatoholError parseItemParameter(ItemsQueryOption &option,
				       GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = parseHostResourceQueryParameter(option, query);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// itemGroupName
	const gchar *value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "itemGroupName"));
	if (value && *value)
		option.setTargetItemGroupName(value);

	return HatoholError(HTERR_OK);
}

static HatoholError
addHostgroupsMap(FaceRest::ResourceHandler *job, JsonBuilderAgent &outputJson,
                 const MonitoringServerInfo &serverInfo,
                 HostgroupInfoList &hostgroupList)
{
	HostgroupsQueryOption option(job->dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->getHostgroupInfoList(hostgroupList,
	                                                   option);
	if (err != HTERR_OK) {
		MLPL_ERR("Error: %d, user ID: %" FMT_USER_ID ", "
		         "sv ID: %" FMT_SERVER_ID "\n",
		         err.getCode(), job->userId, serverInfo.id);
		return err;
	}

	HostgroupInfoListIterator it = hostgroupList.begin();
	for (; it != hostgroupList.end(); ++it) {
		const HostgroupInfo &hostgroupInfo = *it;
		outputJson.startObject(
		  StringUtils::toString(hostgroupInfo.groupId));
		outputJson.add("name", hostgroupInfo.groupName);
		outputJson.endObject();
	}
	return HTERR_OK;
}

static HatoholError
addHostgroupsMap(FaceRest::ResourceHandler *job, JsonBuilderAgent &outputJson,
                 const MonitoringServerInfo &serverInfo)
{
	HostgroupInfoList hostgroupList;
	return addHostgroupsMap(job, outputJson, serverInfo, hostgroupList);
}

static HatoholError addOverviewEachServer(FaceRest::ResourceHandler *job,
					  JsonBuilderAgent &agent,
					  MonitoringServerInfo &svInfo,
					  bool &serverIsGoodStatus)
{
	HatoholError err;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	agent.add("serverId", svInfo.id);
	agent.add("serverHostName", svInfo.hostName);
	agent.add("serverIpAddr", svInfo.ipAddress);
	agent.add("serverNickname", svInfo.nickname);

	// TODO: This implementeation is not effective.
	//       We should add a function only to get the number of list.
	HostInfoList hostInfoList;
	HostsQueryOption option(job->dataQueryContextPtr);
	option.setTargetServerId(svInfo.id);
	dataStore->getHostList(hostInfoList, option);
	agent.add("numberOfHosts", hostInfoList.size());

	ItemInfoList itemInfoList;
	ItemsQueryOption itemsQueryOption(job->dataQueryContextPtr);
	bool fetchItemsSynchronously = true;
	itemsQueryOption.setTargetServerId(svInfo.id);
	dataStore->getItemList(itemInfoList, itemsQueryOption,
			       fetchItemsSynchronously);
	agent.add("numberOfItems", itemInfoList.size());

	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->dataQueryContextPtr);
	triggersQueryOption.setTargetServerId(svInfo.id);
	agent.add("numberOfTriggers",
		  dataStore->getNumberOfTriggers(triggersQueryOption));
	agent.add("numberOfBadHosts",
		  dataStore->getNumberOfBadHosts(triggersQueryOption));
	size_t numBadTriggers = dataStore->getNumberOfBadTriggers(
				  triggersQueryOption, TRIGGER_SEVERITY_ALL);
	agent.add("numberOfBadTriggers", numBadTriggers);

	// TODO: These elements should be fixed
	// after the funtion concerned is added
	agent.add("numberOfUsers", 0);
	agent.add("numberOfOnlineUsers", 0);
	DataQueryOption dataQueryOption(job->userId);
	MonitoringServerStatus serverStatus;
	serverStatus.serverId = svInfo.id;
	err = dataStore->getNumberOfMonitoredItemsPerSecond(
	  dataQueryOption, serverStatus);
	if (err != HTERR_OK)
		return err;
	string nvps = StringUtils::sprintf("%.2f", serverStatus.nvps);
	agent.add("numberOfMonitoredItemsPerSecond", nvps);

	// Hostgroups
	// TODO: We temtatively returns 'No group'. We should fix it
	//       after host group is supported in Hatohol server.
	agent.startObject("hostgroups");
	HostgroupInfoList hostgroupInfoList;
	err = addHostgroupsMap(job, agent, svInfo, hostgroupInfoList);
	if (err != HTERR_OK) {
		HostgroupInfo hgrpInfo;
		hgrpInfo.id = 0;
		hgrpInfo.serverId = svInfo.id;
		hgrpInfo.groupId = ALL_HOST_GROUPS;
		hgrpInfo.groupName = "All host groups";
		hostgroupInfoList.push_back(hgrpInfo);
	}
	agent.endObject();

	// SystemStatus
	agent.startArray("systemStatus");
	HostgroupInfoListConstIterator hostgrpItr = hostgroupInfoList.begin();
	for (; hostgrpItr != hostgroupInfoList.end(); ++ hostgrpItr) {
		const HostgroupIdType hostgroupId = hostgrpItr->groupId;
		for (int severity = 0;
		     severity < NUM_TRIGGER_SEVERITY; severity++) {
			TriggersQueryOption option(job->dataQueryContextPtr);
			option.setTargetServerId(svInfo.id);
			option.setTargetHostgroupId(hostgroupId);
			agent.startObject();
			agent.add("hostgroupId", hostgroupId);
			agent.add("severity", severity);
			agent.add(
			  "numberOfTriggers",
			  dataStore->getNumberOfBadTriggers
			    (option, (TriggerSeverityType)severity));
			agent.endObject();
		}
	}
	agent.endArray();

	// HostStatus
	serverIsGoodStatus = true;
	agent.startArray("hostStatus");
	hostgrpItr = hostgroupInfoList.begin();
	for (; hostgrpItr != hostgroupInfoList.end(); ++ hostgrpItr) {
		const HostgroupIdType hostgroupId = hostgrpItr->groupId;
		TriggersQueryOption option(job->dataQueryContextPtr);
		option.setTargetServerId(svInfo.id);
		option.setTargetHostgroupId(hostgroupId);
		size_t numBadHosts = dataStore->getNumberOfBadHosts(option);
		agent.startObject();
		agent.add("hostgroupId", hostgroupId);
		agent.add("numberOfGoodHosts",
		          dataStore->getNumberOfGoodHosts(option));
		agent.add("numberOfBadHosts", numBadHosts);
		agent.endObject();
		if (numBadHosts > 0)
			serverIsGoodStatus =false;
	}
	agent.endArray();

	return HTERR_OK;
}

static HatoholError addOverview(FaceRest::ResourceHandler *job, JsonBuilderAgent &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(job->dataQueryContextPtr);
	dataStore->getTargetServers(monitoringServers, option);
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("serverStatus");
	size_t numGoodServers = 0, numBadServers = 0;
	for (; it != monitoringServers.end(); ++it) {
		bool serverIsGoodStatus = false;
		agent.startObject();
		HatoholError err = addOverviewEachServer(job, agent, *it, serverIsGoodStatus);
		if (err != HTERR_OK)
			return err;
		agent.endObject();
		if (serverIsGoodStatus)
			numGoodServers++;
		else
			numBadServers++;
	}
	agent.endArray();

	agent.add("numberOfGoodServers", numGoodServers);
	agent.add("numberOfBadServers", numBadServers);

	return HTERR_OK;
}

static void addHosts(FaceRest::ResourceHandler *job, JsonBuilderAgent &agent,
                     const ServerIdType &targetServerId,
                     const HostgroupIdType &targetHostgroupId,
                     const HostIdType &targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostInfoList hostInfoList;
	HostsQueryOption option(job->dataQueryContextPtr);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(targetHostgroupId);
	option.setTargetHostId(targetHostId);
	dataStore->getHostList(hostInfoList, option);

	agent.add("numberOfHosts", hostInfoList.size());
	agent.startArray("hosts");
	HostInfoListIterator it = hostInfoList.begin();
	for (; it != hostInfoList.end(); ++it) {
		HostInfo &hostInfo = *it;
		agent.startObject();
		agent.add("id", hostInfo.id);
		agent.add("serverId", hostInfo.serverId);
		agent.add("hostName", hostInfo.hostName);
		agent.endObject();
	}
	agent.endArray();
}

static void addHostsMap(
  FaceRest::ResourceHandler *job, JsonBuilderAgent &agent,
  const MonitoringServerInfo &serverInfo)
{
	HostgroupIdType targetHostgroupId = ALL_HOST_GROUPS;
	char *value = (char *)g_hash_table_lookup(job->query, "hostgroupId");
	if (value)
		sscanf(value, "%" FMT_HOST_GROUP_ID, &targetHostgroupId);

	HostInfoList hostList;
	HostsQueryOption option(job->dataQueryContextPtr);
	option.setTargetServerId(serverInfo.id);
	option.setTargetHostgroupId(targetHostgroupId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getHostList(hostList, option);
	HostInfoListIterator it = hostList.begin();
	agent.startObject("hosts");
	for (; it != hostList.end(); ++it) {
		HostInfo &host = *it;
		agent.startObject(StringUtils::toString(host.id));
		agent.add("name", host.hostName);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  FaceRest::ResourceHandler *job, const ServerIdType serverId, const TriggerIdType triggerId)
{
	string triggerBrief;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->dataQueryContextPtr);
	triggersQueryOption.setTargetServerId(serverId);
	triggersQueryOption.setTargetId(triggerId);
	dataStore->getTriggerList(triggerInfoList, triggersQueryOption);

	TriggerInfoListIterator it = triggerInfoList.begin();
	TriggerInfo &firstTriggerInfo = *it;
	TriggerIdType firstId = firstTriggerInfo.id;
	for (; it != triggerInfoList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		if (firstId == triggerInfo.id) {
			triggerBrief = triggerInfo.brief;
		} else {
			MLPL_WARN("Failed to getTriggerInfo: "
			          "%" FMT_SERVER_ID ", %" FMT_TRIGGER_ID "\n",
			          serverId, triggerId);
		}
	}

	return triggerBrief;
}

static void addTriggersIdBriefHash(
  FaceRest::ResourceHandler *job,
  JsonBuilderAgent &agent, const MonitoringServerInfo &serverInfo,
  TriggerBriefMaps &triggerMaps, bool lookupTriggerBrief = false)
{
	TriggerBriefMaps::iterator server_it = triggerMaps.find(serverInfo.id);
	agent.startObject("triggers");
	ServerIdType serverId = server_it->first;
	TriggerBriefMap &triggers = server_it->second;
	TriggerBriefMap::iterator it = triggers.begin();
	for (; server_it != triggerMaps.end() && it != triggers.end(); ++it) {
		TriggerIdType triggerId = it->first;
		string &triggerBrief = it->second;
		if (lookupTriggerBrief)
			triggerBrief = getTriggerBrief(job,
						       serverId,
						       triggerId);
		agent.startObject(StringUtils::toString(triggerId));
		agent.add("brief", triggerBrief);
		agent.endObject();
	}
	agent.endObject();
}

void FaceRest::ResourceHandler::addServersMap(
  JsonBuilderAgent &agent,
  TriggerBriefMaps *triggerMaps, bool lookupTriggerBrief)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(dataQueryContextPtr);
	dataStore->getTargetServers(monitoringServers, option);

	HatoholError err;
	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		const MonitoringServerInfo &serverInfo = *it;
		agent.startObject(StringUtils::toString(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		agent.add("type", serverInfo.type);
		agent.add("ipAddress", serverInfo.ipAddress);
		addHostsMap(this, agent, serverInfo);
		if (triggerMaps) {
			addTriggersIdBriefHash(this, agent, serverInfo,
					       *triggerMaps,
			                       lookupTriggerBrief);
		}
		agent.startObject("groups");
		// Even if the following function retrun an error,
		// We cannot do anything. The receiver (client) should handle
		// the returned empty or unperfect group information.
		addHostgroupsMap(this, agent, serverInfo);
		agent.endObject(); // "gropus"
		agent.endObject(); // toString(serverInfo.id)
	}
	agent.endObject();
}

void RestResourceHost::handlerGetOverview(ResourceHandler *job)
{
	JsonBuilderAgent agent;
	HatoholError err;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	err = addOverview(job, agent);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}
	agent.endObject();

	job->replyJsonData(agent);
}

void RestResourceHost::handlerGetHost(ResourceHandler *job)
{
	HostsQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseHostResourceQueryParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addHosts(job, agent,
		 option.getTargetServerId(),
		 option.getTargetHostgroupId(),
		 option.getTargetHostId());
	agent.endObject();

	job->replyJsonData(agent);
}

void RestResourceHost::handlerGetTrigger(ResourceHandler *job)
{
	TriggersQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseTriggerParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	TriggerInfoList triggerList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getTriggerList(triggerList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		agent.startObject();
		agent.add("id",       triggerInfo.id);
		agent.add("status",   triggerInfo.status);
		agent.add("severity", triggerInfo.severity);
		agent.add("lastChangeTime",
		          triggerInfo.lastChangeTime.tv_sec);
		agent.add("serverId", triggerInfo.serverId);
		agent.add("hostId",   triggerInfo.hostId);
		agent.add("brief",    triggerInfo.brief);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfTriggers", triggerList.size());
	job->addServersMap(agent, NULL, false);
	agent.endObject();

	job->replyJsonData(agent);
}

static uint64_t getLastUnifiedEventId(FaceRest::ResourceHandler *job)
{
	EventsQueryOption option(job->dataQueryContextPtr);
	option.setMaximumNumber(1);
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
			   DataQueryOption::SORT_DESCENDING);

	EventInfoList eventList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getEventList(eventList, option);

	uint64_t lastUnifiedId = 0;
	if (!eventList.empty()) {
		lastUnifiedId = eventList.begin()->unifiedId;
		eventList.clear();
	}
	return lastUnifiedId;
}

static void addIssue(FaceRest::ResourceHandler *job, JsonBuilderAgent &agent,
		     const IssueInfo &issue)
{
	agent.startObject("issue");
	agent.add("location", issue.location);
	// TODO: remaining properties will be added later
	agent.endObject();
}

void RestResourceHost::handlerGetEvent(ResourceHandler *job)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventsQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseEventParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	bool addIssues = dataStore->isIssueSenderActionEnabled();
	IssueInfoVect issueVect;
	if (addIssues) {
		err = dataStore->getEventList(eventList, option, &issueVect);
		HATOHOL_ASSERT(eventList.size() == issueVect.size(),
			       "eventList: %zd, issueVect: %zd\n",
			       eventList.size(), issueVect.size());
	} else {
		err = dataStore->getEventList(eventList, option);
	}
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	// TODO: should use transaction to avoid conflicting with event list
	agent.add("lastUnifiedEventId", getLastUnifiedEventId(job));
	if (addIssues)
		agent.addTrue("haveIssue");
	else
		agent.addFalse("haveIssue");
	agent.startArray("events");
	EventInfoListIterator it = eventList.begin();
	for (size_t i = 0; it != eventList.end(); ++i, ++it) {
		EventInfo &eventInfo = *it;
		agent.startObject();
		agent.add("unifiedId", eventInfo.unifiedId);
		agent.add("serverId",  eventInfo.serverId);
		agent.add("time",      eventInfo.time.tv_sec);
		agent.add("type",      eventInfo.type);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("status",    eventInfo.status);
		agent.add("severity",  eventInfo.severity);
		agent.add("hostId",    eventInfo.hostId);
		agent.add("brief",     eventInfo.brief);
		if (addIssues)
			addIssue(job, agent, issueVect[i]);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfEvents", eventList.size());
	job->addServersMap(agent, NULL, false);
	agent.endObject();

	job->replyJsonData(agent);
}

struct GetItemClosure : Closure<FaceRest::ResourceHandler>
{
	GetItemClosure(FaceRest::ResourceHandler *receiver,
		       callback func)
	: Closure<FaceRest::ResourceHandler>::Closure(receiver, func)
	{
		m_receiver->ref();
	}

	virtual ~GetItemClosure()
	{
		m_receiver->unref();
	}
};

void RestResourceHost::replyGetItem(ResourceHandler *job)
{
	ItemsQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseItemParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	ItemInfoList itemList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getItemList(itemList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("items");
	ItemInfoListIterator it = itemList.begin();
	for (; it != itemList.end(); ++it) {
		ItemInfo &itemInfo = *it;
		agent.startObject();
		agent.add("id",        itemInfo.id);
		agent.add("serverId",  itemInfo.serverId);
		agent.add("hostId",    itemInfo.hostId);
		agent.add("brief",     itemInfo.brief.c_str());
		agent.add("lastValueTime",
		          itemInfo.lastValueTime.tv_sec);
		agent.add("lastValue", itemInfo.lastValue);
		agent.add("prevValue", itemInfo.prevValue);
		agent.add("itemGroupName", itemInfo.itemGroupName);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfItems", itemList.size());
	job->addServersMap(agent, NULL, false);
	agent.endObject();

	job->replyJsonData(agent);
}

// TODO: Move to RestResourceHost
void FaceRest::ResourceHandler::itemFetchedCallback(ClosureBase *closure)
{
	RestResourceHost::replyGetItem(this);
	unpauseResponse();
}

void RestResourceHost::handlerGetItem(ResourceHandler *job)
{
	ItemsQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseItemParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerIdType serverId = option.getTargetServerId();
	GetItemClosure *closure =
	  new GetItemClosure(
	    job, &FaceRest::ResourceHandler::itemFetchedCallback);

	bool handled = dataStore->fetchItemsAsync(closure, serverId);
	if (!handled) {
		replyGetItem(job);
		delete closure;
	}
}

static void addHostsIsMemberOfGroup(
  FaceRest::ResourceHandler *job, JsonBuilderAgent &agent,
  uint64_t targetServerId, uint64_t targetGroupId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	HostgroupElementList hostgroupElementList;
	HostgroupElementQueryOption option(job->dataQueryContextPtr);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(targetGroupId);
	dataStore->getHostgroupElementList(hostgroupElementList, option);

	agent.startArray("hosts");
	HostgroupElementListIterator it = hostgroupElementList.begin();
	for (; it != hostgroupElementList.end(); ++it) {
		HostgroupElement hostgroupElement = *it;
		agent.add(hostgroupElement.hostId);
	}
	agent.endArray();
}

void RestResourceHost::handlerGetHostgroup(ResourceHandler *job)
{
	HostgroupsQueryOption option(job->dataQueryContextPtr);
	HatoholError err = parseHostResourceQueryParameter(option, job->query);
	if (err != HTERR_OK) {
		job->replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupInfoList hostgroupInfoList;
	err = dataStore->getHostgroupInfoList(hostgroupInfoList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("numberOfHostgroups", hostgroupInfoList.size());
	agent.startArray("hostgroups");
	HostgroupInfoListIterator it = hostgroupInfoList.begin();
	for (; it != hostgroupInfoList.end(); ++it) {
		HostgroupInfo hostgroupInfo = *it;
		agent.startObject();
		agent.add("id", hostgroupInfo.id);
		agent.add("serverId", hostgroupInfo.serverId);
		agent.add("groupId", hostgroupInfo.groupId);
		agent.add("groupName", hostgroupInfo.groupName.c_str());
		addHostsIsMemberOfGroup(job, agent,
		                        hostgroupInfo.serverId,
		                        hostgroupInfo.groupId);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	job->replyJsonData(agent);
}

RestResourceHostFactory::RestResourceHostFactory(
  FaceRest *faceRest, RestHandler handler)
: FaceRest::ResourceHandlerFactory(faceRest, handler)
{
}

FaceRest::ResourceHandler *RestResourceHostFactory::createHandler()
{
	return new RestResourceHost();
}
