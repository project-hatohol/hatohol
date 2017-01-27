/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include "RestResourceUtils.h"
#include "UnifiedDataStore.h"
#include <memory>
#include <string.h>

using namespace std;
using namespace mlpl;

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceMonitoring>
  RestResourceMonitoringFactory;

const char *RestResourceMonitoring::pathForOverview  = "/overview";
const char *RestResourceMonitoring::pathForHost      = "/host";
const char *RestResourceMonitoring::pathForTrigger   = "/trigger";
const char *RestResourceMonitoring::pathForEvent     = "/event";
const char *RestResourceMonitoring::pathForItem      = "/item";
const char *RestResourceMonitoring::pathForHistory   = "/history";
const char *RestResourceMonitoring::pathForHostgroup = "/hostgroup";
const char *RestResourceMonitoring::pathForTriggerBriefs = "/trigger/briefs";

void RestResourceMonitoring::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForOverview,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetOverview));
	faceRest->addResourceHandlerFactory(
	  pathForHost,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetHost));
	faceRest->addResourceHandlerFactory(
	  pathForHostgroup,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetHostgroup));
	faceRest->addResourceHandlerFactory(
	  pathForTrigger,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetTrigger));
	faceRest->addResourceHandlerFactory(
	  pathForEvent,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetEvent));
	faceRest->addResourceHandlerFactory(
	  pathForItem,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetItem));
	faceRest->addResourceHandlerFactory(
	  pathForHistory,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetHistory));
	faceRest->addResourceHandlerFactory(
	  pathForTriggerBriefs,
	  new RestResourceMonitoringFactory(
	    faceRest, &RestResourceMonitoring::handlerGetTriggerBriefs));
}

RestResourceMonitoring::RestResourceMonitoring(FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

RestResourceMonitoring::~RestResourceMonitoring()
{
}

static HatoholError parseItemParameter(ItemsQueryOption &option,
				       GHashTable *query)
{
	if (!query)
		return HatoholError(HTERR_OK);

	HatoholError err;

	// query parameters for HostResourceQueryOption
	err = RestResourceUtils::parseHostResourceQueryParameter(option, query);
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
		option.setTargetItemCategoryName(value);

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
	option.setStatusSet({HOST_STAT_NORMAL});
	err = dataStore->getServerHostDefs(svHostDefs, option);
	if (err != HTERR_OK)
		return err;
	agent.add("numberOfHosts", svHostDefs.size());

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

static bool checkEmptyFlag(GHashTable *query)
{
	const gchar *value = static_cast<const gchar*>(
	                       g_hash_table_lookup(query, "empty"));
	if (value && *value) {
		if (strcmp(value, "true") == 0) {
			return true ;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void RestResourceMonitoring::handlerGetOverview(void)
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

void RestResourceMonitoring::handlerGetHost(void)
{
	HostsQueryOption option(m_dataQueryContextPtr);
	HatoholError err
	  = RestResourceUtils::parseHostResourceQueryParameter(option, m_query);
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

void RestResourceMonitoring::handlerGetTrigger(void)
{
	if (checkEmptyFlag(m_query)) {
		replyOnlyServers();
		return;
	}

	TriggersQueryOption option(m_dataQueryContextPtr);
	HatoholError err = RestResourceUtils::parseTriggerParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	TriggerInfoList triggerList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	RestResourceUtils::parseHostgroupNameParameter(option, m_query,
						       m_dataQueryContextPtr);
	dataStore->getTriggerList(triggerList, option);

	JSONBuilder agent;
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
		agent.add("hostId",   triggerInfo.hostIdInServer);
		agent.add("brief",    triggerInfo.brief);
		agent.add("extendedInfo", triggerInfo.extendedInfo);
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
	agent.add("trackerId", incident.trackerId);
	agent.add("identifier", incident.identifier);
	agent.add("location", incident.location);
	agent.add("status", incident.status);
	agent.add("priority", incident.priority);
	agent.add("assignee", incident.assignee);
	agent.add("doneRatio", incident.doneRatio);
	agent.add("commentCount", incident.commentCount);
	agent.add("createdAt", incident.createdAt.tv_sec);
	agent.add("updatedAt", incident.updatedAt.tv_sec);
	agent.endObject();
}

void RestResourceMonitoring::handlerGetEvent(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventsQueryOption option(m_dataQueryContextPtr);
	bool isCountOnly = false;
	HatoholError err =
	  RestResourceUtils::parseEventParameter(option, m_query, isCountOnly);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	RestResourceUtils::parseHostgroupNameParameter(option, m_query,
						       m_dataQueryContextPtr);
	if (isCountOnly) {
		JSONBuilder agent;
		agent.startObject();
		addHatoholError(agent, HatoholError(HTERR_OK));
		agent.add("numberOfEvents", dataStore->getNumberOfEvents(option));
		agent.endObject();
		replyJSONData(agent);
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
		agent.add("eventId",   eventInfo.id);
		agent.add("status",    eventInfo.status);
		agent.add("severity",  eventInfo.severity);
		agent.add("hostId",    eventInfo.hostIdInServer);
		agent.add("brief",     eventInfo.brief);
		agent.add("extendedInfo", eventInfo.extendedInfo);
		if (addIncidents)
			addIncident(this, agent, incidentVect[i]);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfEvents", eventList.size());
	addServersMap(agent, NULL, false);
	addIncidentTrackersMap(agent);
	agent.endObject();

	replyJSONData(agent);
}

// TODO: Add a macro or template to simplify the definition
struct GetItemClosure : ClosureTemplate0<RestResourceMonitoring>
{
	GetItemClosure(RestResourceMonitoring *receiver,
		       callback func)
	: ClosureTemplate0<RestResourceMonitoring>(receiver, func)
	{
		m_receiver->ref();
	}

	virtual ~GetItemClosure()
	{
		m_receiver->unref();
	}
};

void RestResourceMonitoring::replyGetItem(void)
{
	ItemsQueryOption option(m_dataQueryContextPtr);
	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	ItemsQueryOption itemCategoryOption(m_dataQueryContextPtr);
	itemCategoryOption.setExcludeFlags(EXCLUDE_INVALID_HOST);
	HatoholError err = parseItemParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	ItemInfoList itemList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getItemList(itemList, option);
	vector<string> itemCategoryNames;
	dataStore->getItemCategoryNames(itemCategoryNames, itemCategoryOption);

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

		agent.startArray("itemGroupNames");
		for (const auto &categoryName: itemInfo.categoryNames)
			agent.add(categoryName);
		agent.endArray();

		agent.add("unit", itemInfo.unit);
		agent.add("valueType", static_cast<int>(itemInfo.valueType));
		agent.endObject();
	}
	agent.endArray();
	agent.startArray("itemGroups");
	for (const auto &name: itemCategoryNames) {
		agent.startObject();
		agent.add("name", name);
		agent.endObject();
	}
	agent.endArray();
	agent.add("numberOfItems", itemList.size());
	agent.add("totalNumberOfItems", dataStore->getNumberOfItems(option));
	addServersMap(agent, NULL, false);
	agent.endObject();

	replyJSONData(agent);
}

void RestResourceMonitoring::replyOnlyServers(void)
{
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addServersMap(agent, NULL, false);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceMonitoring::itemFetchedCallback(Closure0 *closure)
{
	replyGetItem();
	unpauseResponse();
}

void RestResourceMonitoring::handlerGetItem(void)
{
	if (checkEmptyFlag(m_query)) {
		replyOnlyServers();
		return;
	}

	ItemsQueryOption option(m_dataQueryContextPtr);
	HatoholError err = parseItemParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	GetItemClosure *closure =
	  new GetItemClosure(
	    this, &RestResourceMonitoring::itemFetchedCallback);

	bool handled = dataStore->fetchItemsAsync(closure, option);
	if (!handled) {
		replyGetItem();
		delete closure;
	}
}

struct GetHistoryClosure : ClosureTemplate1<RestResourceMonitoring, HistoryInfoVect>
{
	shared_ptr<DataStore> m_dataStore;

	GetHistoryClosure(RestResourceMonitoring *receiver,
			  callback func, shared_ptr<DataStore> dataStore)
	: ClosureTemplate1<RestResourceMonitoring, HistoryInfoVect>(receiver, func),
	  m_dataStore(dataStore)
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

void RestResourceMonitoring::handlerGetHistory(void)
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
	    this, &RestResourceMonitoring::historyFetchedCallback,
	    unifiedDataStore->getDataStore(serverId));
	if (closure->m_dataStore) {
		closure->m_dataStore->startOnDemandFetchHistory(
		  itemInfo, beginTime, endTime, closure);
	} else {
		HistoryInfoVect historyInfoVect;
		(*closure)(historyInfoVect);
		delete closure;
	}
}

void RestResourceMonitoring::historyFetchedCallback(
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
		string itemId = to_string(historyInfo.itemId);
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

void RestResourceMonitoring::handlerGetHostgroup(void)
{
	HostgroupsQueryOption option(m_dataQueryContextPtr);
	HatoholError err
	  = RestResourceUtils::parseHostResourceQueryParameter(option, m_query);
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

void RestResourceMonitoring::handlerGetTriggerBriefs(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Method %s is not allowed.\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	TriggersQueryOption option(m_dataQueryContextPtr);
	option.setExcludeFlags(EXCLUDE_INVALID_HOST);
	HatoholError err
	  = RestResourceUtils::parseHostResourceQueryParameter(option, m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	list<string> triggerBriefList;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getTriggerBriefList(triggerBriefList, option);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("briefs");
	for (auto brief : triggerBriefList) {
		agent.startObject();
		agent.add("brief", brief);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJSONData(agent);
}
