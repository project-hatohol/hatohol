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

void RestResourceHost::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForOverview,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetOverview));
	faceRest->addResourceHandlerFactory(
	  pathForHost,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetHost));
	faceRest->addResourceHandlerFactory(
	  pathForHostgroup,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetHostgroup));
	faceRest->addResourceHandlerFactory(
	  pathForTrigger,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetTrigger));
	faceRest->addResourceHandlerFactory(
	  pathForEvent,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetEvent));
	faceRest->addResourceHandlerFactory(
	  pathForItem,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetItem));
}

RestResourceHost::RestResourceHost(FaceRest *faceRest, HandlerFunc handler)
: FaceRest::ResourceHandler(faceRest, NULL), m_handlerFunc(handler)
{
}

RestResourceHost::~RestResourceHost()
{
}

void RestResourceHost::handle(void)
{
	HATOHOL_ASSERT(m_handlerFunc, "No handler function!");
	(this->*m_handlerFunc)();
}

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

static HatoholError addOverviewEachServer(FaceRest::ResourceHandler *job,
					  JSONBuilderAgent &agent,
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
	HostsQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(svInfo.id);
	dataStore->getHostList(hostInfoList, option);
	agent.add("numberOfHosts", hostInfoList.size());

	ItemsQueryOption itemsQueryOption(job->m_dataQueryContextPtr);
	itemsQueryOption.setTargetServerId(svInfo.id);
	bool fetchItemsSynchronously = true;
	size_t numberOfItems =
	  dataStore->getNumberOfItems(itemsQueryOption,
				      fetchItemsSynchronously);
	agent.add("numberOfItems", numberOfItems);

	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(job->m_dataQueryContextPtr);
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

	DataQueryOption dataQueryOption(job->m_dataQueryContextPtr);
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
	err = job->addHostgroupsMap(agent, svInfo, hostgroupInfoList);
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
			TriggersQueryOption option(job->m_dataQueryContextPtr);
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
		TriggersQueryOption option(job->m_dataQueryContextPtr);
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

static HatoholError addOverview(FaceRest::ResourceHandler *job, JSONBuilderAgent &agent)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(job->m_dataQueryContextPtr);
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

void RestResourceHost::handlerGetOverview(void)
{
	JSONBuilderAgent agent;
	HatoholError err;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	err = addOverview(this, agent);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}
	agent.endObject();

	replyJSONData(agent);
}

static void addHosts(FaceRest::ResourceHandler *job, JSONBuilderAgent &agent,
                     const ServerIdType &targetServerId,
                     const HostgroupIdType &targetHostgroupId,
                     const HostIdType &targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostInfoList hostInfoList;
	HostsQueryOption option(job->m_dataQueryContextPtr);
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

void RestResourceHost::handlerGetHost(void)
{
	HostsQueryOption option(m_dataQueryContextPtr);
	HatoholError err
	  = parseHostResourceQueryParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addHosts(this, agent,
		 option.getTargetServerId(),
		 option.getTargetHostgroupId(),
		 option.getTargetHostId());
	agent.endObject();

	replyJSONData(agent);
}

void RestResourceHost::handlerGetTrigger(void)
{
	TriggersQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseTriggerParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	TriggerInfoList triggerList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getTriggerList(triggerList, option);

	JSONBuilderAgent agent;
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
	agent.add("totalNumberOfTriggers",
		  dataStore->getNumberOfTriggers(option));
	addServersMap(agent, NULL, false);
	agent.endObject();

	replyJSONData(agent);
}

static uint64_t getLastUnifiedEventId(FaceRest::ResourceHandler *job)
{
	EventsQueryOption option(job->m_dataQueryContextPtr);
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

static void addIncident(FaceRest::ResourceHandler *job, JSONBuilderAgent &agent,
			const IncidentInfo &incident)
{
	agent.startObject("incident");
	agent.add("location", incident.location);
	// TODO: remaining properties will be added later
	agent.endObject();
}

void RestResourceHost::handlerGetEvent(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventsQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseEventParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	bool addIncidents = dataStore->isIncidentSenderActionEnabled();
	IncidentInfoVect incidentVect;
	if (addIncidents) {
		err = dataStore->getEventList(eventList, option, &incidentVect);
		HATOHOL_ASSERT(eventList.size() == incidentVect.size(),
			       "eventList: %zd, incidentVect: %zd\n",
			       eventList.size(), incidentVect.size());
	} else {
		err = dataStore->getEventList(eventList, option);
	}
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	// TODO: should use transaction to avoid conflicting with event list
	agent.add("lastUnifiedEventId", getLastUnifiedEventId(this));
	if (addIncidents)
		agent.addTrue("haveIncident");
	else
		agent.addFalse("haveIncident");
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
		if (addIncidents)
			addIncident(this, agent, incidentVect[i]);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfEvents", eventList.size());
	addServersMap(agent, NULL, false);
	agent.endObject();

	replyJSONData(agent);
}

struct GetItemClosure : Closure<RestResourceHost>
{
	GetItemClosure(RestResourceHost *receiver,
		       callback func)
	: Closure<RestResourceHost>::Closure(receiver, func)
	{
		m_receiver->ref();
	}

	virtual ~GetItemClosure()
	{
		m_receiver->unref();
	}
};

void RestResourceHost::replyGetItem(void)
{
	ItemsQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseItemParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	ItemInfoList itemList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getItemList(itemList, option);

	JSONBuilderAgent agent;
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
	agent.add("totalNumberOfItems", dataStore->getNumberOfItems(option));
	addServersMap(agent, NULL, false);
	agent.endObject();

	replyJSONData(agent);
}

void RestResourceHost::itemFetchedCallback(ClosureBase *closure)
{
	replyGetItem();
	unpauseResponse();
}

void RestResourceHost::handlerGetItem(void)
{
	ItemsQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseItemParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerIdType serverId = option.getTargetServerId();
	GetItemClosure *closure =
	  new GetItemClosure(
	    this, &RestResourceHost::itemFetchedCallback);

	bool handled = dataStore->fetchItemsAsync(closure, serverId);
	if (!handled) {
		replyGetItem();
		delete closure;
	}
}

static void addHostsIsMemberOfGroup(
  FaceRest::ResourceHandler *job, JSONBuilderAgent &agent,
  uint64_t targetServerId, uint64_t targetGroupId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	HostgroupElementList hostgroupElementList;
	HostgroupElementQueryOption option(job->m_dataQueryContextPtr);
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

void RestResourceHost::handlerGetHostgroup(void)
{
	HostgroupsQueryOption option(m_dataQueryContextPtr);
	HatoholError err
	  = parseHostResourceQueryParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupInfoList hostgroupInfoList;
	err = dataStore->getHostgroupInfoList(hostgroupInfoList, option);

	JSONBuilderAgent agent;
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
		addHostsIsMemberOfGroup(this, agent,
		                        hostgroupInfo.serverId,
		                        hostgroupInfo.groupId);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJSONData(agent);
}

RestResourceHostFactory::RestResourceHostFactory(
  FaceRest *faceRest, RestResourceHost::HandlerFunc handler)
: FaceRest::ResourceHandlerFactory(faceRest, NULL), m_handlerFunc(handler)
{
}

FaceRest::ResourceHandler *RestResourceHostFactory::createHandler()
{
	return new RestResourceHost(m_faceRest, m_handlerFunc);
}
