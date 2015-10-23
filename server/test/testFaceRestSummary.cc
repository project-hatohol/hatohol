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

using namespace std;
using namespace mlpl;

namespace testFaceRestSummary {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

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

	DBTablesMonitoring::EventSeverityStatistics
	expectedSeverityStatistics[] = {
		{TRIGGER_SEVERITY_CRITICAL, 1},
	};

	startFaceRest();
	RequestArg arg("/summary");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SEVERITY_RANK);
	unique_ptr<JSONParser> parserPtr(getResponseAsJSONParser(arg));
	JSONParser *parser = parserPtr.get();
	assertErrorCode(parser);
	assertValueInParser(parser, "numOfImportantEvents", 1);
	assertValueInParser(parser, "numOfImportantEventOccurredHosts", 1);
	assertValueInParser(parser, "numOfNotImportantEvents", 6);
	assertValueInParser(parser, "numOfNotImportantEventOccurredHosts", 4);
	assertValueInParser(parser, "numOfAssignedEvents", 0);
	assertValueInParser(parser, "numOfUnAssignedEvents", 0);
	assertStartObject(parser, "statistics");
	parser->endObject();
}

} // namespace testFaceRestSummary
