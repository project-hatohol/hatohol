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

void RestResourceSummary::handlerSummary(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	EventsQueryOption option(m_dataQueryContextPtr);
	SeverityRankQueryOption severityRankOption(m_dataQueryContextPtr);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	SeverityRankInfoVect importantSeverityRanks, notImportantSeverityRanks;
	severityRankOption.setTargetAsImportant(static_cast<int>(true));
	dataStore->getSeverityRanks(importantSeverityRanks, severityRankOption);
	severityRankOption.setTargetAsImportant(static_cast<int>(false));
	dataStore->getSeverityRanks(notImportantSeverityRanks, severityRankOption);
	bool isCountOnly = false;
	HatoholError err =
	  RestResourceHostUtils::parseEventParameter(option, m_query, isCountOnly);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	EventsQueryOption importantEventOption(option);
	std::set<TriggerSeverityType> importantStatusSet, notImportantStatusSet;
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

	EventsQueryOption notImportantEventOption(option);
	for (auto &severityRank : notImportantSeverityRanks) {
		notImportantStatusSet.insert(static_cast<TriggerSeverityType>(severityRank.status));
	}
	notImportantEventOption.setTriggerSeverities(notImportantStatusSet);
	int64_t numOfNotImportantEvents =
	  dataStore->getNumberOfEvents(notImportantEventOption);
	HostsQueryOption hostsOption(m_dataQueryContextPtr);
	int64_t numOfAllHosts =
	  dataStore->getNumberOfHosts(hostsOption);

	EventsQueryOption assignedEventOption(option);
	std::set<std::string> assignedStatusSet, unAssignedStatusSet;
	assignedStatusSet.insert(IncidentSenderHatohol::STATUS_IN_PROGRESS.c_str());
	assignedStatusSet.insert(IncidentSenderHatohol::STATUS_DONE.c_str());
	assignedEventOption.setIncidentStatuses(assignedStatusSet);
	int64_t numOfAssignedEvents =
	  dataStore->getNumberOfEvents(assignedEventOption);

	EventsQueryOption unAssignedEventOption(option);
	unAssignedStatusSet.insert(IncidentSenderHatohol::STATUS_NONE.c_str());
	unAssignedStatusSet.insert(IncidentSenderHatohol::STATUS_HOLD.c_str());
	unAssignedEventOption.setIncidentStatuses(unAssignedStatusSet);
	int64_t numOfUnAssignedEvents =
	  dataStore->getNumberOfEvents(unAssignedEventOption);

	JSONBuilder reply;
	reply.startObject();
	reply.add("numOfImportantEvents", numOfImportantEvents);
	reply.add("numOfImportantEventOccurredHosts",
		  numOfImportantEventOccurredHosts);
	reply.add("numOfNotImportantEvents", numOfNotImportantEvents);
	reply.add("numOfAllHosts",
		  numOfAllHosts);
	reply.add("numOfAssignedEvents", numOfAssignedEvents);
	reply.add("numOfUnAssignedEvents", numOfUnAssignedEvents);
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
