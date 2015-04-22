/*
 * Copyright (C) 2013-2014 Project Hatohol
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
const char *RestResourceHost::pathForHistory   = "/history";
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
	faceRest->addResourceHandlerFactory(
	  pathForHistory,
	  new RestResourceHostFactory(
	    faceRest, &RestResourceHost::handlerGetHistory));
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

	// trigger ID
	TriggerIdType triggerId = ALL_TRIGGERS;
	err = getParam<TriggerIdType>(query, "triggerId",
				      "%" FMT_TRIGGER_ID, triggerId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTriggerId(triggerId);

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

	// item ID
	ItemIdType itemId = ALL_ITEMS;
	err = getParam<ItemIdType>(query, "itemId",
				   "%" FMT_ITEM_ID, itemId);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;
	option.setTargetId(itemId);

	// itemGroupName
	const gchar *value = static_cast<const gchar*>(
	  g_hash_table_lookup(query, "itemGroupName"));
	if (value && *value)
		option.setTargetItemGroupName(value);

	// appName
	const char *key = "appName";
	char *application = (char *)g_hash_table_lookup(query, key);
	if (application)
		option.setAppName(application);

	return HatoholError(HTERR_OK);
}

static HatoholError addOverviewEachServer(FaceRest::ResourceHandler *job,
					  JSONBuilder &agent,
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
	ServerHostDefVect svHostDefs;
	HostsQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(svInfo.id);
	option.setStatus(HOST_STAT_NORMAL);
	err = dataStore->getServerHostDefs(svHostDefs, option);
	if (err != HTERR_OK)
		return err;
	agent.add("numberOfHosts", svHostDefs.size());

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
	triggersQueryOption.setExcludeFlags(EXCLUDE_INVALID_HOST|EXCLUDE_SELF_MONITORING);
	agent.add("numberOfTriggers",
		  dataStore->getNumberOfTriggers(triggersQueryOption));
	const size_t numBadHosts =
	  dataStore->getNumberOfBadHosts(triggersQueryOption);
	agent.add("numberOfBadHosts", numBadHosts);
	serverIsGoodStatus = (numBadHosts == 0);
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
	HostgroupVect hostgroups;
	err = job->addHostgroupsMap(agent, svInfo, hostgroups);
	if (err != HTERR_OK) {
		Hostgroup hostgrp;
		hostgrp.id         = 0;
		hostgrp.serverId   = svInfo.id;
		hostgrp.idInServer = ALL_HOST_GROUPS;
		hostgrp.name       = "All host groups";
		hostgroups.push_back(hostgrp);
	}
	agent.endObject();

	// SystemStatus
	agent.startArray("systemStatus");
	HostgroupVectConstIterator hostgrpItr = hostgroups.begin();
	for (; hostgrpItr != hostgroups.end(); ++ hostgrpItr) {
		const HostgroupIdType &hostgroupId = hostgrpItr->idInServer;
		for (int severity = 0;
		     severity < NUM_TRIGGER_SEVERITY; severity++) {
			TriggersQueryOption option(job->m_dataQueryContextPtr);
			option.setExcludeFlags(EXCLUDE_INVALID_HOST|EXCLUDE_SELF_MONITORING);
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
	agent.startArray("hostStatus");
	hostgrpItr = hostgroups.begin();
	for (; hostgrpItr != hostgroups.end(); ++ hostgrpItr) {
		const HostgroupIdType &hostgroupId = hostgrpItr->idInServer;
		TriggersQueryOption option(job->m_dataQueryContextPtr);
		option.setExcludeFlags(EXCLUDE_INVALID_HOST|EXCLUDE_SELF_MONITORING);
		option.setTargetServerId(svInfo.id);
		option.setTargetHostgroupId(hostgroupId);
		size_t numBadHosts = dataStore->getNumberOfBadHosts(option);
		agent.startObject();
		agent.add("hostgroupId", hostgroupId);
		agent.add("numberOfGoodHosts",
		          dataStore->getNumberOfGoodHosts(option));
		agent.add("numberOfBadHosts", numBadHosts);
		agent.endObject();
	}
	agent.endArray();

	return HTERR_OK;
}

static HatoholError addOverview(FaceRest::ResourceHandler *job, JSONBuilder &agent)
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
	JSONBuilder agent;
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

bool RestResourceHost::parseExtendedInfo(const std::string &extendedInfo,
                                         std::string &extendedInfoValue)
{
	if (extendedInfo.empty())
		return false;

	JSONParser parser(extendedInfo);
	if (parser.hasError())
		return false;

	return parser.read("expandedDescription", extendedInfoValue);
}

static void addHosts(FaceRest::ResourceHandler *job, JSONBuilder &agent,
                     const ServerIdType &targetServerId,
                     const HostgroupIdType &targetHostgroupId,
                     const LocalHostIdType &targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect svHostDefs;
	HostsQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(targetHostgroupId);
	option.setTargetHostId(targetHostId);
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  dataStore->getServerHostDefs(svHostDefs, option));

	agent.add("numberOfHosts", svHostDefs.size());
	agent.startArray("hosts");
	ServerHostDefVectConstIterator svHostIt = svHostDefs.begin();
	for (; svHostIt != svHostDefs.end(); ++svHostIt) {
		const ServerHostDef &svHostDef = *svHostIt;
		agent.startObject();
		agent.add("id", svHostDef.hostIdInServer);
		agent.add("serverId", svHostDef.serverId);
		agent.add("hostName", svHostDef.name);
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

	JSONBuilder agent;
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

	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	TriggerInfoList triggerList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getTriggerList(triggerList, option);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		string extendedInfoValue = "";
		bool foundExtendedInfo = false;
		foundExtendedInfo = parseExtendedInfo(triggerInfo.extendedInfo,
		                                      extendedInfoValue);

		agent.startObject();
		agent.add("id",       triggerInfo.id);
		agent.add("status",   triggerInfo.status);
		agent.add("severity", triggerInfo.severity);
		agent.add("lastChangeTime",
		          triggerInfo.lastChangeTime.tv_sec);
		agent.add("serverId", triggerInfo.serverId);
		agent.add("hostId",   triggerInfo.hostIdInServer);
		agent.add("brief",    triggerInfo.brief);
		if (foundExtendedInfo)
			agent.add("expandedDescription", extendedInfoValue);
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

static void addIncident(FaceRest::ResourceHandler *job, JSONBuilder &agent,
			const IncidentInfo &incident)
{
	agent.startObject("incident");
	agent.add("identifier", incident.identifier);
	agent.add("location", incident.location);
	agent.add("status", incident.status);
	agent.add("priority", incident.priority);
	agent.add("assignee", incident.assignee);
	agent.add("doneRatio", incident.doneRatio);
	agent.add("createdAt", incident.createdAt.tv_sec);
	agent.add("updatedAt", incident.updatedAt.tv_sec);
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

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	// TODO: should use transaction to avoid conflicting with event list
	agent.add("lastUnifiedEventId", getLastUnifiedEventId(this));
	if (addIncidents)
		agent.add("haveIncident", JSONTrue);
	else
		agent.add("haveIncident", JSONFalse);
	agent.startArray("events");
	EventInfoListIterator it = eventList.begin();
	for (size_t i = 0; it != eventList.end(); ++i, ++it) {
		EventInfo &eventInfo = *it;
		string extendedInfoValue = "";
		bool foundExtendedInfo = false;
		foundExtendedInfo = parseExtendedInfo(eventInfo.extendedInfo,
		                                      extendedInfoValue);

		agent.startObject();
		agent.add("unifiedId", eventInfo.unifiedId);
		agent.add("serverId",  eventInfo.serverId);
		agent.add("time",      eventInfo.time.tv_sec);
		agent.add("type",      eventInfo.type);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("eventId",   eventInfo.id);
		agent.add("status",    eventInfo.status);
		agent.add("severity",  eventInfo.severity);
		agent.add("hostId",    eventInfo.hostIdInServer);
		agent.add("brief",     eventInfo.brief);
		if (foundExtendedInfo)
			agent.add("expandedDescription", extendedInfoValue);
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

// TODO: Add a macro or template to simplify the definition
struct GetItemClosure : ClosureTemplate0<RestResourceHost>
{
	GetItemClosure(RestResourceHost *receiver,
		       callback func)
	: ClosureTemplate0<RestResourceHost>(receiver, func)
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
	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	ItemsQueryOption applicationOption(m_dataQueryContextPtr);
	applicationOption.setExcludeFlags(EXCLUDE_INVALID_HOST);
	HatoholError err = parseItemParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	ItemInfoList itemList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getItemList(itemList, option);
	ApplicationInfoVect applicationInfoVect;
	dataStore->getApplicationVect(applicationInfoVect, applicationOption);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("items");
	ItemInfoListIterator it = itemList.begin();
	for (; it != itemList.end(); ++it) {
		ItemInfo &itemInfo = *it;
		agent.startObject();
		agent.add("id",        itemInfo.id);
		agent.add("serverId",  itemInfo.serverId);
		agent.add("hostId",    itemInfo.hostIdInServer);
		agent.add("brief",     itemInfo.brief.c_str());
		agent.add("lastValueTime",
		          itemInfo.lastValueTime.tv_sec);
		agent.add("lastValue", itemInfo.lastValue);
		agent.add("prevValue", itemInfo.prevValue);
		agent.add("itemGroupName", itemInfo.itemGroupName);
		agent.add("unit", itemInfo.unit);
		agent.add("valueType", static_cast<int>(itemInfo.valueType));
		agent.endObject();
	}
	agent.endArray();
	agent.startArray("applications");
	ApplicationInfoVectIterator itApp = applicationInfoVect.begin();
	for (; itApp != applicationInfoVect.end(); ++itApp) {
		ApplicationInfo &applicationInfo = *itApp;
		agent.startObject();
		agent.add("name", applicationInfo.applicationName.c_str());
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfItems", itemList.size());
	agent.add("totalNumberOfItems", dataStore->getNumberOfItems(option));
	addServersMap(agent, NULL, false);
	agent.endObject();

	replyJSONData(agent);
}

void RestResourceHost::itemFetchedCallback(Closure0 *closure)
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

struct GetHistoryClosure : ClosureTemplate1<RestResourceHost, HistoryInfoVect>
{
	DataStorePtr m_dataStorePtr;

	GetHistoryClosure(RestResourceHost *receiver,
			  callback func, DataStorePtr dataStorePtr)
	: ClosureTemplate1<RestResourceHost, HistoryInfoVect>(receiver, func),
	  m_dataStorePtr(dataStorePtr)
	{
		m_receiver->ref();
	}

	virtual ~GetHistoryClosure()
	{
		m_receiver->unref();
	}
};

static HatoholError parseHistoryParameter(
  GHashTable *query, ServerIdType &serverId, ItemIdType &itemId,
  time_t &beginTime, time_t &endTime)
{
	if (!query)
		return HatoholError(HTERR_INVALID_PARAMETER);

	HatoholError err;

	// serverId
	err = getParam<ServerIdType>(query, "serverId",
				     "%" FMT_SERVER_ID,
				     serverId);
	if (err != HTERR_OK)
		return err;
	if (serverId == ALL_SERVERS) {
		return HatoholError(HTERR_INVALID_PARAMETER,
				    "serverId: ALL_SERVERS");
	}

	// itemId
	err = getParam<ItemIdType>(query, "itemId",
				   "%" FMT_ITEM_ID,
				   itemId);
	if (err != HTERR_OK)
		return err;
	if (itemId == ALL_ITEMS) {
		return HatoholError(HTERR_INVALID_PARAMETER,
				    "itemId: ALL_ITEMS");
	}

	// beginTime
	err = getParam<time_t>(query, "beginTime",
			       "%ld", beginTime);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	// endTime
	err = getParam<time_t>(query, "endTime",
			       "%ld", endTime);
	if (err != HTERR_OK && err != HTERR_NOT_FOUND_PARAMETER)
		return err;

	return HatoholError(HTERR_OK);
}

void RestResourceHost::handlerGetHistory(void)
{
	ServerIdType serverId = ALL_SERVERS;
	ItemIdType itemId = ALL_ITEMS;
	const time_t SECONDS_IN_A_DAY = 60 * 60 * 24;
	time_t endTime = time(NULL);
	time_t beginTime = endTime - SECONDS_IN_A_DAY;

	HatoholError err = parseHistoryParameter(m_query, serverId, itemId,
						 beginTime, endTime);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *unifiedDataStore = UnifiedDataStore::getInstance();

	// Check the specified item
	ItemsQueryOption option(m_dataQueryContextPtr);
	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	option.setTargetId(itemId);
	ItemInfoList itemList;
	unifiedDataStore->getItemList(itemList, option);
	if (itemList.empty()) {
		// We assume that items are alreay fetched.
		// Because clients can't know the itemId wihout them.
		string message = StringUtils::sprintf("itemId: %" FMT_ITEM_ID,
						      itemId.c_str());
		HatoholError err(HTERR_NOT_FOUND_TARGET_RECORD, message);
		replyError(err);
		return;
	}

	// Queue fetching history
	ItemInfo &itemInfo = *itemList.begin();
	GetHistoryClosure *closure =
	  new GetHistoryClosure(
	    this, &RestResourceHost::historyFetchedCallback,
	    unifiedDataStore->getDataStore(serverId));
	if (closure->m_dataStorePtr.hasData()) {
		closure->m_dataStorePtr->startOnDemandFetchHistory(
		  itemInfo, beginTime, endTime, closure);
	} else {
		HistoryInfoVect historyInfoVect;
		(*closure)(historyInfoVect);
		delete closure;
	}
}

void RestResourceHost::historyFetchedCallback(
  Closure1<HistoryInfoVect> *closure, const HistoryInfoVect &historyInfoVect)
{
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("history");
	HistoryInfoVectConstIterator it = historyInfoVect.begin();
	for (; it != historyInfoVect.end(); ++it) {
		const HistoryInfo &historyInfo = *it;
		agent.startObject();
		// Omit serverId & itemId to reduce data.
		// They are obvious because they are provided by the client.
		/*
		// use string to treat 64bit value properly on certain browsers
		string itemId = StringUtils::toString(historyInfo.itemId);
		agent.add("serverId",  serverId);
		agent.add("itemId",    itemId);
		*/
		agent.add("value",     historyInfo.value);
		agent.add("clock",     historyInfo.clock.tv_sec);
		agent.add("ns",        historyInfo.clock.tv_nsec);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJSONData(agent);
	unpauseResponse();
}

static void addHostsIsMemberOfGroup(
  FaceRest::ResourceHandler *job, JSONBuilder &agent,
  uint64_t targetServerId, const string &targetGroupId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	HostgroupMemberVect hostgrpMembers;
	HostgroupMembersQueryOption option(job->m_dataQueryContextPtr);
	option.setTargetServerId(targetServerId);

	option.setTargetHostgroupId(targetGroupId);
	dataStore->getHostgroupMembers(hostgrpMembers, option);

	agent.startArray("hosts");
	HostgroupMemberVectConstIterator hostgrpMemberIt =
	  hostgrpMembers.begin();
	for (; hostgrpMemberIt != hostgrpMembers.end(); ++hostgrpMemberIt)
		agent.add(hostgrpMemberIt->hostIdInServer);
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
	HostgroupVect hostgroups;
	err = dataStore->getHostgroups(hostgroups, option);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("numberOfHostgroups", hostgroups.size());
	agent.startArray("hostgroups");
	HostgroupVectConstIterator it = hostgroups.begin();
	for (; it != hostgroups.end(); ++it) {
		const Hostgroup &hostgrp = *it;
		agent.startObject();
		agent.add("id",        hostgrp.id);
		agent.add("serverId",  hostgrp.serverId);
		agent.add("groupId",   hostgrp.idInServer);
		agent.add("groupName", hostgrp.name);
		addHostsIsMemberOfGroup(this, agent,
		                        hostgrp.serverId, hostgrp.idInServer);
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
