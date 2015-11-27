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
#include "RestResourceHostUtils.h"
#include "UnifiedDataStore.h"
#include "IncidentSenderHatohol.h"

using namespace std;

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceSummary>
  RestResourceSummaryFactory;

const char *RestResourceSummary::pathForSummary = "/summary";

void RestResourceSummary::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForSummary,
	  new RestResourceSummaryFactory(
	        faceRest, &RestResourceSummary::handlerSummary));
}

RestResourceSummary::RestResourceSummary(
  FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

static void setAssignedIncidentStatusCondition(
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
	customIncidentStatusMap.erase(IncidentSenderHatohol::STATUS_HOLD);

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
}

void RestResourceSummary::handlerSummary(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	EventsQueryOption option(m_dataQueryContextPtr);
	SeverityRankQueryOption severityRankOption(m_dataQueryContextPtr);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	SeverityRankInfoVect importantSeverityRanks;
	severityRankOption.setTargetAsImportant(static_cast<int>(true));
	dataStore->getSeverityRanks(importantSeverityRanks, severityRankOption);
	bool isCountOnly = false;
	HatoholError err =
	  RestResourceHostUtils::parseEventParameter(option, m_query, isCountOnly);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	EventsQueryOption importantEventOption(option);
	std::set<TriggerSeverityType> importantStatusSet;
	for (auto &severityRank : importantSeverityRanks) {
		importantStatusSet.insert(static_cast<TriggerSeverityType>(severityRank.status));
	}
	importantEventOption.setTriggerSeverities(importantStatusSet);
	int64_t numOfImportantEvents =
	  dataStore->getNumberOfEvents(importantEventOption);
	int64_t numOfImportantEventOccurredHosts =
	  dataStore->getNumberOfHostsWithSpecifiedEvents(importantEventOption);
	std::vector<DBTablesMonitoring::EventSeverityStatistics> severityStatisticsVect;
	err =
	  dataStore->getEventSeverityStatistics(severityStatisticsVect,
	                                        importantEventOption);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	HostsQueryOption hostsOption(m_dataQueryContextPtr);
	int64_t numOfAllHosts =
	  dataStore->getNumberOfHosts(hostsOption);

	EventsQueryOption assignedEventOption(option);
	assignedEventOption.setTriggerSeverities(importantStatusSet);
	setAssignedIncidentStatusCondition(assignedEventOption, option);
	int64_t numOfAssignedEvents =
	  dataStore->getNumberOfEvents(assignedEventOption);

	JSONBuilder reply;
	reply.startObject();
	reply.add("numOfImportantEvents", numOfImportantEvents);
	reply.add("numOfImportantEventOccurredHosts",
		  numOfImportantEventOccurredHosts);
	reply.add("numOfAllHosts",
		  numOfAllHosts);
	reply.add("numOfAssignedImportantEvents", numOfAssignedEvents);
	reply.startArray("statistics");
	for (auto &statistics : severityStatisticsVect) {
		reply.startObject();
		reply.add("severity", statistics.severity);
		reply.add("times", statistics.num);
		reply.endObject();
	}
	reply.endArray(); // statistics

	addHatoholError(reply, HatoholError(HTERR_OK));
	reply.endObject();
	replyJSONData(reply);
}
