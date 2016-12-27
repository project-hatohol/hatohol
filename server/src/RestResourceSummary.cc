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

#include "RestResourceSummary.h"
#include "RestResourceUtils.h"
#include "UnifiedDataStore.h"
#include "IncidentSenderHatohol.h"

using namespace std;

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceSummary>
  RestResourceSummaryFactory;

const char *RestResourceSummary::pathForImportantEventSummary
  = "/summary/important-event";

struct ImportantEventStatistics
{
	int64_t numOfImportantEvents;
	int64_t numOfImportantEventOccurredHosts;
	int64_t numOfAssignedEvents;
	vector<DBTablesMonitoring::EventSeverityStatistics> severityStatisticsVect;

	ImportantEventStatistics()
	: numOfImportantEvents(0),
	  numOfImportantEventOccurredHosts(0),
	  numOfAssignedEvents(0)
	{
	}
};

void RestResourceSummary::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForImportantEventSummary,
	  new RestResourceSummaryFactory(
	        faceRest, &RestResourceSummary::handlerImportantEventSummary));
}

RestResourceSummary::RestResourceSummary(
  FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

static void getImportantSeveritySet(
  const DataQueryContextPtr &dataQueryContextPtr,
  set<TriggerSeverityType> &severitySet)
{
	SeverityRankQueryOption severityRankOption(dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	SeverityRankInfoVect importantSeverityRanks;
	severityRankOption.setTargetAsImportant(static_cast<int>(true));
	dataStore->getSeverityRanks(importantSeverityRanks, severityRankOption);

	for (auto &severityRank : importantSeverityRanks) {
		TriggerSeverityType severity =
		  static_cast<TriggerSeverityType>(severityRank.status);
		severitySet.insert(severity);
	}
}

static bool setSeverityCondition(
  EventsQueryOption &option,
  const set<TriggerSeverityType> &importantSeveritySet,
  const EventsQueryOption &userFilter)
{
	const set<TriggerSeverityType> &baseSeverities =
	  userFilter.getTriggerSeverities();

	if (baseSeverities.empty()) {
		option.setTriggerSeverities(importantSeveritySet);
		return !importantSeveritySet.empty();
	}

	set<TriggerSeverityType> severitySet;
	for (auto &severity: importantSeveritySet) {
		if (baseSeverities.find(severity) == baseSeverities.end()) {
			// The user doesn't want to include it.
			continue;
		}
		severitySet.insert(severity);
	}
	option.setTriggerSeverities(severitySet);

	return !severitySet.empty();
}

static bool setAssignedIncidentStatusCondition(
  EventsQueryOption &option, const EventsQueryOption &userFilter)
{
	const set<string> &baseStatuses = userFilter.getIncidentStatuses();
	set<string> assignedStatusSet;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	map<string, CustomIncidentStatus> customIncidentStatusMap;
	dataStore->getCustomIncidentStatusesCache(customIncidentStatusMap);

	// Make sure to exist system-defined statuses
	CustomIncidentStatus dummy;
	customIncidentStatusMap[IncidentSenderHatohol::STATUS_IN_PROGRESS] = dummy;
	customIncidentStatusMap[IncidentSenderHatohol::STATUS_DONE] = dummy;

	// Remove system-defined statues that shouldn't be treated as "Assigned".
	customIncidentStatusMap.erase(IncidentSenderHatohol::STATUS_NONE);

	for (auto &pair: customIncidentStatusMap) {
		const string &status = pair.first;
		if (!baseStatuses.empty() &&
		    baseStatuses.find(status) == baseStatuses.end())
		{
			// The user doesn't want to include it.
			continue;
		}
		assignedStatusSet.insert(status.c_str());
	}

	option.setIncidentStatuses(assignedStatusSet);

	return !assignedStatusSet.empty();
}

HatoholError getImportantEventStatistics(ImportantEventStatistics &statistics,
					 const EventsQueryOption &userFilter)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventsQueryOption importantEventOption(userFilter);
	statistics.numOfImportantEvents =
	  dataStore->getNumberOfEvents(importantEventOption);
	statistics.numOfImportantEventOccurredHosts =
	  dataStore->getNumberOfHostsWithSpecifiedEvents(importantEventOption);
	HatoholError err =
	  dataStore->getEventSeverityStatistics(
	    statistics.severityStatisticsVect,
	    importantEventOption);
	if (err != HTERR_OK)
		return err;

	EventsQueryOption assignedEventOption(userFilter);
	bool hasIncidentStatusCondition =
	  setAssignedIncidentStatusCondition(assignedEventOption, userFilter);
	if (hasIncidentStatusCondition) {
		statistics.numOfAssignedEvents =
		  dataStore->getNumberOfEvents(assignedEventOption);
	}

	return err;
}

void RestResourceSummary::handlerImportantEventSummary(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	EventsQueryOption userFilter(m_dataQueryContextPtr);
	bool isCountOnly = false;
	HatoholError err =
	  RestResourceUtils::parseEventParameter(userFilter, m_query, isCountOnly);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	set<TriggerSeverityType> importantSeveritySet;
	getImportantSeveritySet(m_dataQueryContextPtr, importantSeveritySet);
	bool hasSeverityCondition =
	  setSeverityCondition(userFilter, importantSeveritySet, userFilter);

	ImportantEventStatistics statistics;
	if (hasSeverityCondition) {
		err = getImportantEventStatistics(statistics, userFilter);
		if (err != HTERR_OK) {
			replyError(err);
			return;
		}
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostsQueryOption hostsOption(m_dataQueryContextPtr);
	int64_t numOfAllHosts =
	  dataStore->getNumberOfHosts(hostsOption);

	JSONBuilder reply;
	reply.startObject();
	reply.add("numOfImportantEvents", statistics.numOfImportantEvents);
	reply.add("numOfImportantEventOccurredHosts",
		  statistics.numOfImportantEventOccurredHosts);
	reply.add("numOfAllHosts",
		  numOfAllHosts);
	reply.add("numOfAssignedImportantEvents", statistics.numOfAssignedEvents);
	reply.startArray("statistics");
	for (auto &element : statistics.severityStatisticsVect) {
		reply.startObject();
		reply.add("severity", element.severity);
		reply.add("times", element.num);
		reply.endObject();
	}
	reply.endArray(); // statistics

	addHatoholError(reply, HatoholError(HTERR_OK));
	reply.endObject();
	replyJSONData(reply);
}
