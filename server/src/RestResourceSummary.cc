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

	std::set<TriggerSeverityType> importantStatusSet, notImportantStatusSet;
	for (auto &severityRank : importantSeverityRanks) {
		importantStatusSet.insert(static_cast<TriggerSeverityType>(severityRank.status));
	}
	option.setTriggerSeverities(importantStatusSet);
	int64_t numOfImportantEvents = dataStore->getNumberOfEvents(option);
	int64_t numOfImportantEventOccurredHosts =
		dataStore->getNumberOfHostsWithSpecifiedEvents(option);
	std::vector<DBTablesMonitoring::EventSeverityStatistics> severityStatisticsVect;
	err =
	  dataStore->getEventSeverityStatistics(severityStatisticsVect, option);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	for (auto &severityRank : notImportantSeverityRanks) {
		notImportantStatusSet.insert(static_cast<TriggerSeverityType>(severityRank.status));
	}
	option.setTriggerSeverities(notImportantStatusSet);
	int64_t numOfNotImportantEvents = dataStore->getNumberOfEvents(option);
	int64_t numOfNotImportantEventOccurredHosts =
		dataStore->getNumberOfHostsWithSpecifiedEvents(option);

	std::set<std::string> assignedStatusSet, unAssignedStatusSet;
	assignedStatusSet.insert(definedStatuses[static_cast<int>(Status::IN_PROGRESS)].label.c_str());
	option.setIncidentStatuses(assignedStatusSet);
	int64_t numOfAssignedEvents = dataStore->getNumberOfEvents(option);
	unAssignedStatusSet.insert(definedStatuses[static_cast<int>(Status::NONE)].label.c_str());
	unAssignedStatusSet.insert(definedStatuses[static_cast<int>(Status::HOLD)].label.c_str());
	option.setIncidentStatuses(unAssignedStatusSet);
	int64_t numOfUnAssignedEvents = dataStore->getNumberOfEvents(option);

	JSONBuilder reply;
	reply.startObject();
	reply.startArray("summary");
	reply.add("numOfImportantEvents", numOfImportantEvents);
	reply.add("numOfImportantEventOccurredHosts",
		  numOfImportantEventOccurredHosts);
	reply.add("numOfNotImportantEvents", numOfNotImportantEvents);
	reply.add("numOfNotImportantEventOccurredHosts",
		  numOfNotImportantEventOccurredHosts);
	reply.add("numOfAssignedEvents", numOfAssignedEvents);
	reply.add("numOfUnAssignedEvents", numOfUnAssignedEvents);
	reply.startArray("statistics");
	for (auto &statistics : severityStatisticsVect) {
		reply.add("severity", statistics.severity);
		reply.add("times", statistics.num);
	}
	reply.endArray(); // statistics
	reply.endArray(); // summary

	addHatoholError(reply, HatoholError(HTERR_OK));
	reply.endObject();
	replyJSONData(reply);
}
