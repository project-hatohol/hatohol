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

#include <cppcutter.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "RestResourceSummary.h"
#include "FaceRestTestUtils.h"
#include <ThreadLocalDBCache.h>

using namespace std;
using namespace mlpl;

namespace testFaceRestSummary {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
	loadTestDBServerHostDef();
}

const IncidentInfo unAssignedIncidentInfo[] = {
{
	5,                        // trackerId
	2,                        // serverId
	"2",                      // eventId
	"3",                      // triggerId
	"7",                      // identifier
	"",                       // location
	"NONE",                   // status
	"",                       // priority
	"",                       // assignee
	0,                        // doneRatio
	{1412957360, 0},          // createdAt
	{1412957360, 0},          // updatedAt
	IncidentInfo::STATUS_UNKNOWN, // statusCode
	7,                        // unifiedId
},
};
const size_t NumUnAssginedTestIncidentInfo = ARRAY_SIZE(unAssignedIncidentInfo);

void loadUnAssignedIncidentInfo(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	for (auto incidentInfo : unAssignedIncidentInfo) {
		dbMonitoring.addIncidentInfo(&incidentInfo);
	}
};

void cut_teardown(void)
{
	stopFaceRest();
}

void test_summary(void)
{
	loadTestDBSeverityRankInfo();
	loadTestDBEvents();
	loadTestDBIncidents();
	loadTestDBIncidentTracker();
	loadUnAssignedIncidentInfo();

	DBTablesMonitoring::EventSeverityStatistics
	expectedSeverityStatistics[] = {
		{TRIGGER_SEVERITY_CRITICAL, 1},
	};

	startFaceRest();
	RequestArg arg("/summary");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	unique_ptr<JSONParser> parserPtr(getResponseAsJSONParser(arg));
	JSONParser *parser = parserPtr.get();
	assertErrorCode(parser);
	assertValueInParser(parser, "numOfImportantEvents", 1);
	assertValueInParser(parser, "numOfImportantEventOccurredHosts", 1);
	assertValueInParser(parser, "numOfNotImportantEvents", 6);
	assertValueInParser(parser, "numOfAllHosts", 16);
	assertValueInParser(parser, "numOfAssignedEvents", 0);
	assertValueInParser(parser, "numOfUnAssignedEvents", 1);
	assertStartObject(parser, "statistics");
	{
		size_t i = 0;
		for (auto &statistic : expectedSeverityStatistics) {
			parser->startElement(i);
			assertValueInParser(parser, "severity", statistic.severity);
			uint64_t num = statistic.num;
			assertValueInParser(parser, "times", num);
			parser->endElement();
			++i;
		}
	}
	parser->endObject();
}

} // namespace testFaceRestSummary
