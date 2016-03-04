/*
 * Copyright (C) 2013-2016 Project Hatohol
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
#include "FaceRest.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "UnifiedDataStore.h"
#include "testDBTablesMonitoring.h"
#include "FaceRestTestUtils.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestIncident {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBUser();
}

void cut_teardown(void)
{
	stopFaceRest();

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->setCopyOnDemandEnabled(false);
}

static void incidentInfo2StringMap(
  const IncidentInfo &src, StringMap &dest)
{
	dest["status"] = src.status;
	dest["priority"] = src.priority;
	dest["assignee"] = src.assignee;
	dest["doneRatio"] = StringUtils::toString(src.doneRatio);

	/* Hatohol doesn't allow changing following properties:
	dest["trackerId"] = StringUtils::toString(src.trackerId);
	dest["serverId"] = StringUtils::toString(serc.serverId);
	dest["eventId"] = src.eventId;
	dest["triggerId"] = src.triggerId;
	dest["identifier"] = src.identifier;
	dest["location"] = src.location;
	dest["createdAtSec"] = src.createdAt.tv_sec;
	dest["createdAtNSec"] = src.createdAt.tv_nsec;
	dest["updatedAtSec"] = src.updatedAt.tv_sec;
	dest["updatedAtNSec"] = src.updatedAt.tv_nsec;
	dest["unifiedEventId"] = src.unifiedEventId;
	*/
}

void test_putIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	size_t index = 2;
	IncidentInfo expectedIncident = testIncidentInfo[index];
	expectedIncident.status = "IN PROGRESS";
	expectedIncident.priority = "HIGH";
	expectedIncident.assignee = "taro";
	expectedIncident.doneRatio = 30;
	string path(
	  StringUtils::sprintf("/incident/%" FMT_UNIFIED_EVENT_ID,
			       expectedIncident.unifiedEventId));
	RequestArg arg(path);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "PUT";
	incidentInfo2StringMap(expectedIncident, arg.parameters);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"unifiedEventId\":123"
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=123");
	string expected =
		"^5\\|2\\|2\\|3\\|123\\|\\|IN PROGRESS\\|taro\\|"
		"1412957360\\|0\\|\\d+\\|\\d+\\|HIGH\\|30\\|123$";
	cut_assert_match(expected.c_str(), actual.c_str());

	// check history
	actual = execSQL(&dbMonitoring.getDBAgent(),
			 "select * from incident_status_histories");
	expected = "^1\\|123\\|2\\|IN PROGRESS\\|\\|\\d+\\|\\d+$";
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_putInvalidIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	size_t index = 2;
	IncidentInfo expectedIncident = testIncidentInfo[index];
	expectedIncident.status = "INVALID STATUS";
	string path(
	  StringUtils::sprintf("/incident/%" FMT_UNIFIED_EVENT_ID,
			       expectedIncident.unifiedEventId));
	RequestArg arg(path);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "PUT";
	incidentInfo2StringMap(expectedIncident, arg.parameters);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":42,"
	  "\"errorMessage\":\"Invalid parameter.\","
	  "\"optionMessages\":\"Unknown status: INVALID STATUS\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=123");
	string expected = makeIncidentOutput(testIncidentInfo[index]);
	cppcut_assert_equal(expected, actual);
	assertEqualJSONString(expectedResponse, arg.response);

	// check history
	actual = execSQL(&dbMonitoring.getDBAgent(),
			 "select * from incident_status_histories");
	expected = "";
	cppcut_assert_equal(expected.c_str(), actual.c_str());
}


void test_postDuplicateIncident(void)
{
	loadTestDBEvents();
	loadTestDBIncidents();
	startFaceRest();

	IncidentTrackerIdType trackerId
	  = findIncidentTrackerIdByType(INCIDENT_TRACKER_HATOHOL);
	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "POST";
	arg.parameters["unifiedEventId"] = "3";
	arg.parameters["incidentTrackerId"]
	  = StringUtils::sprintf("%" FMT_INCIDENT_TRACKER_ID, trackerId);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":65,"
	  "\"errorMessage\":\"The record already exists.\","
	  "\"optionMessages\":\"id: 3\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	assertEqualJSONString(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=3");
	string expected = makeIncidentOutput(testIncidentInfo[0]);
	cppcut_assert_equal(expected, actual);
}

void test_postIncidentForUnknownEvent(void)
{
	startFaceRest();

	IncidentTrackerIdType trackerId
	  = findIncidentTrackerIdByType(INCIDENT_TRACKER_HATOHOL);
	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "POST";
	arg.parameters["unifiedEventId"] = "3";
	arg.parameters["incidentTrackerId"]
	  = StringUtils::sprintf("%" FMT_INCIDENT_TRACKER_ID, trackerId);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":10,"
	  "\"errorMessage\":\"Not found target record.\","
	  "\"optionMessages\":\"id: 3\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncidentWithoutId(void)
{
	loadTestDBIncidents();
	startFaceRest();

	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(200, arg.httpStatusCode);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":40,"
	  "\"errorMessage\":\"Not found ID in the URL.\","
	  "\"optionMessages\":\"id: \""
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	RequestArg arg("/incident/3");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(200, arg.httpStatusCode);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":3,"
	  "\"errorMessage\":\"Not implemented.\","
	  "\"optionMessages\":\"Getting an incident isn't implemented yet!\""
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncidentHistory(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentStatusHistory();
	startFaceRest();

	RequestArg arg("/incident/3/history");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(200, arg.httpStatusCode);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"incidentHistory\":["
	  "{"
	  "\"id\":1,"
	  "\"unifiedEventId\":3,"
	  "\"userId\":1,"
	  "\"status\":\"NONE\","
	  "\"time\":1412957260"
	  "}"
	  "]"
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncidentHistoryWithComment(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentStatusHistory();
	startFaceRest();

	RequestArg arg("/incident/10/history");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(200, arg.httpStatusCode);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"incidentHistory\":["
	  "{"
	  "\"id\":2,"
	  "\"unifiedEventId\":10,"
	  "\"userId\":2,"
	  "\"status\":\"IN PROGRESS\","
	  "\"time\":1412957290,"
	  "\"comment\":\"This is a comment.\""
	  "}"
	  "]"
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getUnknownSubResource(void)
{
	loadTestDBIncidents();
	startFaceRest();

	RequestArg arg("/incident/3/comment");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(404, arg.httpStatusCode);
}

} // namespace testFaceRestIncident
