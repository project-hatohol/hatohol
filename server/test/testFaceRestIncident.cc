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
}

static void incidentInfo2StringMap(
  const IncidentInfo &src, StringMap &dest)
{
	dest["status"] = src.status;
	dest["priority"] = src.priority;
	dest["assignee"] = src.assignee;
	dest["doneRatio"] = to_string(src.doneRatio);

	/* Hatohol doesn't allow changing following properties:
	dest["trackerId"] = to_string(src.trackerId);
	dest["serverId"] = to_string(serc.serverId);
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
		"1412957360\\|0\\|\\d+\\|\\d+\\|HIGH\\|30\\|123\\|0$";
	cut_assert_match(expected.c_str(), actual.c_str());

	// check history
	actual = execSQL(&dbMonitoring.getDBAgent(),
			 "select * from incident_histories");
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
			 "select * from incident_histories");
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

void test_putIncidentComment(void)
{
	loadTestDBIncidents();
	startFaceRest();

	size_t index = 2;
	IncidentInfo expectedIncident = testIncidentInfo[index];
	expectedIncident.status = "IN PROGRESS";
	expectedIncident.priority = "HIGH";
	expectedIncident.assignee = "@Mnagakawa";
	expectedIncident.doneRatio = 30;
	string path(
	  StringUtils::sprintf("/incident/%" FMT_UNIFIED_EVENT_ID,
			       expectedIncident.unifiedEventId));
	RequestArg arg(path);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "PUT";
	incidentInfo2StringMap(expectedIncident, arg.parameters);
	arg.parameters["comment"] = "Assigned to @Mnagakawa";

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
		"^5\\|2\\|2\\|3\\|123\\|\\|IN PROGRESS\\|@Mnagakawa\\|"
		"1412957360\\|0\\|\\d+\\|\\d+\\|HIGH\\|30\\|123\\|1$";
	cut_assert_match(expected.c_str(), actual.c_str());

	// check history
	actual = execSQL(&dbMonitoring.getDBAgent(),
			 "select * from incident_histories");
	expected =
		"^1\\|123\\|2\\|IN PROGRESS\\|Assigned to @Mnagakawa\\|"
		"\\d+\\|\\d+$";
	cut_assert_match(expected.c_str(), actual.c_str());
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
	  "\"optionMessages\":\"unifiedEventId: \""
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
	loadTestDBIncidentHistory();
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
	  "\"userName\":\"cheesecake\","
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
	loadTestDBIncidentHistory();
	startFaceRest();

	RequestArg arg("/incident/4/history");
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
	  "\"unifiedEventId\":4,"
	  "\"userId\":2,"
	  "\"userName\":\"pineapple\","
	  "\"status\":\"IN PROGRESS\","
	  "\"time\":1412957290,"
	  "\"comment\":\"This is a comment.\""
	  "}"
	  "]"
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncidentHistoryForNonExistentIncident(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	RequestArg arg("/incident/1711718/history");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(200, arg.httpStatusCode);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":10,"
	  "\"errorMessage\":\"Not found target record.\","
	  "\"optionMessages\":\"unifiedEventId: 1711718\""
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

void test_updateIncidentComment(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	const string comment = "Assign to @cosmo920";
	RequestArg arg("/incident-comment/2");
	arg.userId = testIncidentHistory[1].userId;
	arg.request = "PUT";
	arg.parameters["comment"] = comment;
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"id\":2"
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incident_histories"
				" where id=2");
	IncidentHistory history = testIncidentHistory[1];
	history.id = 2;
	history.comment = comment;
	string expected = makeIncidentHistoryOutput(history);
	cppcut_assert_equal(expected, actual);
}

void test_updateIncidentCommentByNonOwner(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	const string comment = "Assign to @cosmo920";
	RequestArg arg("/incident-comment/2");
	arg.userId = testIncidentHistory[1].userId + 1;
	arg.request = "PUT";
	arg.parameters["comment"] = comment;
	getServerResponse(arg);
	cppcut_assert_equal(403, arg.httpStatusCode);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incident_histories"
				" where id=2");
	IncidentHistory history = testIncidentHistory[1];
	history.id = 2;
	string expected = makeIncidentHistoryOutput(history);
	cppcut_assert_equal(expected, actual);
}

void test_updateIncidentCommentWithoutParameter(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	const string comment = "Assign to @cosmo920";
	RequestArg arg("/incident-comment/2");
	arg.userId = testIncidentHistory[1].userId;
	arg.request = "PUT";
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":41,"
	  "\"errorMessage\":\"Not found parameter.\","
	  "\"optionMessages\":\"comment\""
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incident_histories"
				" where id=2");
	IncidentHistory history = testIncidentHistory[1];
	history.id = 2;
	string expected = makeIncidentHistoryOutput(history);
	cppcut_assert_equal(expected, actual);
}

void test_deleteIncidentComment(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	RequestArg arg("/incident-comment/2");
	arg.userId = testIncidentHistory[1].userId;
	arg.request = "DELETE";
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"id\":2"
	  "}");
	assertEqualJSONString(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incident_histories"
				" where id=2");
	IncidentHistory history = testIncidentHistory[1];
	history.id = 2;
	history.comment = "";
	string expected = makeIncidentHistoryOutput(history);
	cppcut_assert_equal(expected, actual);
}

void test_deleteIncidentCommentByNonOwner(void)
{
	loadTestDBIncidents();
	loadTestDBIncidentHistory();
	startFaceRest();

	RequestArg arg("/incident-comment/2");
	arg.userId = testIncidentHistory[1].userId + 1;
	arg.request = "DELETE";
	getServerResponse(arg);
	cppcut_assert_equal(403, arg.httpStatusCode);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incident_histories"
				" where id=2");
	IncidentHistory history = testIncidentHistory[1];
	history.id = 2;
	string expected = makeIncidentHistoryOutput(history);
	cppcut_assert_equal(expected, actual);
}

} // namespace testFaceRestIncident
