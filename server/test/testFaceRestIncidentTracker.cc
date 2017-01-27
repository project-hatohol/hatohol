/*
 * Copyright (C) 2014 Project Hatohol
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
#include "JSONParser.h"
#include "DBTablesTest.h"
#include "MultiLangTest.h"
#include "ThreadLocalDBCache.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestIncidentTracker {

static JSONParser *g_parser = NULL;

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

	delete g_parser;
	g_parser = NULL;
}

static void _assertIncidentTrackers(
  const string &path = "/incident-tracker", const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_INCIDENT_SETTINGS);
	OperationPrivilege privilege(arg.userId);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertStartObject(g_parser, "incidentTrackers");
	for (size_t i = 0; i < NumTestIncidentTrackerInfo; i++) {
		const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[i];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "id", tracker.id);
		assertValueInParser(g_parser, "type", tracker.type);
		assertValueInParser(g_parser, "nickname", tracker.nickname);
		assertValueInParser(g_parser, "baseURL", tracker.baseURL);
		assertValueInParser(g_parser, "projectId", tracker.projectId);
		assertValueInParser(g_parser, "trackerId", tracker.trackerId);
		if (privilege.has(OPPRVLG_UPDATE_INCIDENT_SETTING)) {
			assertValueInParser(g_parser, "userName",
					    tracker.userName);
		} else {
			assertNoValueInParser(g_parser, "userName");
		}
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertIncidentTrackers(P,...) \
cut_trace(_assertIncidentTrackers(P,##__VA_ARGS__))

#define assertAddIncidentTracker(P, ...) \
cut_trace(_assertAddRecord(P, "/incident-tracker", ##__VA_ARGS__))

#define assertUpdateIncidentTracker(P, ...) \
cut_trace(_assertUpdateRecord(P, "/incident-tracker", ##__VA_ARGS__))

static void _assertIncidentTrackerInDB(
  const IncidentTrackerInfo &expectedIncidentTracker,
  const IncidentTrackerIdType &targetId = ALL_INCIDENT_TRACKERS)
{
	string statement;

	if (targetId == ALL_INCIDENT_TRACKERS) {
		statement = "select * from incident_trackers "
			    "order by id desc limit 1";
	} else {
		statement = StringUtils::sprintf(
			      "select * from incident_trackers where id=%"
			      FMT_INCIDENT_TRACKER_ID,
			      targetId);
	}
	string expected = makeIncidentTrackerInfoOutput(
			    expectedIncidentTracker);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expected);
}
#define assertIncidentTrackerInDB(E, ...) \
cut_trace(_assertIncidentTrackerInDB(E, ##__VA_ARGS__))

void test_getIncidentTracker()
{
	assertIncidentTrackers();
}

static void createTestIncidentTracker(IncidentTrackerInfo &incidentTracker)
{
	incidentTracker.id        = 0;
	incidentTracker.type      = INCIDENT_TRACKER_REDMINE;
	incidentTracker.nickname  = "Redmine";
	incidentTracker.baseURL   = "http://example.com/redmine/";
	incidentTracker.projectId = "incidents";
	incidentTracker.trackerId = "";
	incidentTracker.userName  = "y@ru0";
	incidentTracker.password  = "w(^_^)d";

}

static void createPostData(const IncidentTrackerInfo &incidentTracker,
			   StringMap &params)
{
	params["id"]        = to_string(incidentTracker.id);
	params["type"]      = to_string((int)incidentTracker.type);
	params["nickname"]  = incidentTracker.nickname;
	params["baseURL"]   = incidentTracker.baseURL;
	params["projectId"] = incidentTracker.projectId;
	params["trackerId"] = incidentTracker.trackerId;
	params["userName"]  = incidentTracker.userName;
	params["password"]  = incidentTracker.password;
}

void test_addIncidentTrackerWithoutTrackerId(void)
{
	const IncidentTrackerIdType expectedId
	  = NumTestIncidentTrackerInfo + 1;
	IncidentTrackerInfo incidentTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_INCIDENT_SETTING);
	createTestIncidentTracker(incidentTracker);
	createPostData(incidentTracker, params);

	assertAddIncidentTracker(params, userId, HTERR_OK, expectedId);
	incidentTracker.id = expectedId;
	assertIncidentTrackerInDB(incidentTracker);
}

void test_addIncidentTrackerWithInvalidType(void)
{
	IncidentTrackerInfo incidentTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_INCIDENT_SETTING);
	createTestIncidentTracker(incidentTracker);
	createPostData(incidentTracker, params);
	params["type"] = "9999";

	assertAddIncidentTracker(params, userId,
			      HTERR_INVALID_INCIDENT_TRACKER_TYPE);
}

void test_addIncidentTrackerWithoutNickname(void)
{
	IncidentTrackerInfo incidentTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_INCIDENT_SETTING);
	createTestIncidentTracker(incidentTracker);
	createPostData(incidentTracker, params);
	params.erase("nickname");

	assertAddIncidentTracker(params, userId, HTERR_NOT_FOUND_PARAMETER);
}

void test_addIncidentTrackerWithoutPrivilege(void)
{
	IncidentTrackerInfo incidentTracker;
	StringMap params;
	UserIdType userId = findUserWithout(OPPRVLG_CREATE_INCIDENT_SETTING);
	createTestIncidentTracker(incidentTracker);
	createPostData(incidentTracker, params);

	assertAddIncidentTracker(params, userId, HTERR_NO_PRIVILEGE);
}

void test_updateIncidentTracker(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_INCIDENT_SETTING);
	const IncidentTrackerIdType targetId = 2;
	IncidentTrackerInfo incidentTracker
	  = testIncidentTrackerInfo[targetId - 1];
	incidentTracker.id = targetId;
	incidentTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(incidentTracker, params);

	assertUpdateIncidentTracker(params, targetId, userId, HTERR_OK);
	assertIncidentTrackerInDB(incidentTracker, targetId);
}

void test_updateIncidentTrackerWithoutAccountParams(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_INCIDENT_SETTING);
	const IncidentTrackerIdType targetId = 2;
	IncidentTrackerInfo incidentTracker
	  = testIncidentTrackerInfo[targetId - 1];
	incidentTracker.id = targetId;
	incidentTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(incidentTracker, params);
	params.erase("userName");
	params.erase("password");

	assertUpdateIncidentTracker(params, targetId, userId, HTERR_OK);
	assertIncidentTrackerInDB(incidentTracker, targetId);
}

void test_updateIncidentTrackerWithoutPrivilege(void)
{
	UserIdType userId = findUserWithout(OPPRVLG_UPDATE_INCIDENT_SETTING);
	const IncidentTrackerIdType targetId = 2;
	IncidentTrackerInfo incidentTracker
	  = testIncidentTrackerInfo[targetId - 1];
	incidentTracker.id = targetId;
	incidentTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(incidentTracker, params);

	HatoholErrorCode expectedCode = HTERR_NO_PRIVILEGE;
	if (!(testUserInfo[userId - 1].flags & OPPRVLG_GET_ALL_INCIDENT_SETTINGS))
		expectedCode = HTERR_NOT_FOUND_TARGET_RECORD;
	assertUpdateIncidentTracker(params, targetId, userId, expectedCode);

	IncidentTrackerInfo expectedIncidentTracker
	  = testIncidentTrackerInfo[targetId - 1];
	expectedIncidentTracker.id = targetId;
	assertIncidentTrackerInDB(expectedIncidentTracker, targetId);
}

void test_deleteIncidentTracker(void)
{
	startFaceRest();

	const IncidentTrackerIdType incidentTrackerId = 1;
	string url = StringUtils::sprintf(
	  "/incident-tracker/%" FMT_INCIDENT_TRACKER_ID, incidentTrackerId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_INCIDENT_SETTING);
	g_parser = getResponseAsJSONParser(arg);

	assertErrorCode(g_parser);
	IncidentTrackerIdSet incidentTrackerIdSet;
	incidentTrackerIdSet.insert(incidentTrackerId);
	assertIncidentTrackersInDB(incidentTrackerIdSet);
}

void test_deleteIncidentTrackerWithoutId(void)
{
	startFaceRest();

	string url = StringUtils::sprintf("/incident-tracker");
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_INCIDENT_SETTING);
	g_parser = getResponseAsJSONParser(arg);

	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
	assertIncidentTrackersInDB(EMPTY_INCIDENT_TRACKER_ID_SET);
}

void test_deleteIncidentTrackerWithoutPrivelge(void)
{
	startFaceRest();

	const IncidentTrackerIdType incidentTrackerId = 1;
	string url = StringUtils::sprintf(
	  "/incident-tracker/%" FMT_INCIDENT_TRACKER_ID, incidentTrackerId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWithout(OPPRVLG_DELETE_INCIDENT_SETTING);
	g_parser = getResponseAsJSONParser(arg);

	assertErrorCode(g_parser, HTERR_NO_PRIVILEGE);
	assertIncidentTrackersInDB(EMPTY_INCIDENT_TRACKER_ID_SET);
}

} // namespace testFaceRestIncidentTracker
