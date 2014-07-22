/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBClientTest.h"
#include "MultiLangTest.h"
#include "CacheServiceDBClient.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestIssueTracker {

static JsonParserAgent *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();

	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
}

void cut_teardown(void)
{
	stopFaceRest();

	if (g_parser) {
		delete g_parser;
		g_parser = NULL;
	}
}

static void _assertIssueTrackers(
  const string &path = "/issue-tracker", const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_ISSUE_SETTINGS);
	OperationPrivilege privilege(arg.userId);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertStartObject(g_parser, "issueTrackers");
	for (size_t i = 0; i < NumTestIssueTrackerInfo; i++) {
		const IssueTrackerInfo &tracker = testIssueTrackerInfo[i];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "id", tracker.id);
		assertValueInParser(g_parser, "type", tracker.type);
		assertValueInParser(g_parser, "nickname", tracker.nickname);
		assertValueInParser(g_parser, "baseURL", tracker.baseURL);
		assertValueInParser(g_parser, "projectId", tracker.projectId);
		assertValueInParser(g_parser, "trackerId", tracker.trackerId);
		if (privilege.has(OPPRVLG_UPDATE_ISSUE_SETTING))
			assertValueInParser(g_parser, "userName", tracker.userName);
		else
			assertNoValueInParser(g_parser, "userName");
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertIssueTrackers(P,...) \
cut_trace(_assertIssueTrackers(P,##__VA_ARGS__))

#define assertAddIssueTracker(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/issue-tracker", ##__VA_ARGS__))

#define assertUpdateIssueTracker(P, ...) \
cut_trace(_assertUpdateRecord(g_parser, P, "/issue-tracker", ##__VA_ARGS__))

void test_getIssueTracker()
{
	assertIssueTrackers();
}

static void createTestIssueTracker(IssueTrackerInfo &issueTracker)
{
	issueTracker.id        = 0;
	issueTracker.type      = ISSUE_TRACKER_REDMINE;
	issueTracker.nickname  = "Redmine";
	issueTracker.baseURL   = "http://example.com/redmine/";
	issueTracker.projectId = "incidents";
	issueTracker.trackerId = "";
	issueTracker.userName  = "y@ru0";
	issueTracker.password  = "w(^_^)d";

}

static void createPostData(const IssueTrackerInfo &issueTracker,
			   StringMap &params)
{
	params["id"]        = StringUtils::toString(issueTracker.id);
	params["type"]      = StringUtils::toString((int)issueTracker.type);
	params["nickname"]  = issueTracker.nickname;
	params["baseURL"]   = issueTracker.baseURL;
	params["projectId"] = issueTracker.projectId;
	params["trackerId"] = issueTracker.trackerId;
	params["userName"]  = issueTracker.userName;
	params["password"]  = issueTracker.password;
}

void test_addIssueTrackerWithoutTrackerId(void)
{
	const IssueTrackerIdType expectedId = NumTestIssueTrackerInfo + 1;
	IssueTrackerInfo issueTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_ISSUE_SETTING);
	createTestIssueTracker(issueTracker);
	createPostData(issueTracker, params);

	assertAddIssueTracker(params, userId, HTERR_OK, expectedId);

	// check the content in the DB
	string statement = "select * from ";
	statement += "issue_trackers";
	statement += " order by id desc limit 1";
	issueTracker.id = expectedId;
	string expect = makeIssueTrackerInfoOutput(issueTracker);
	DBClientConfig dbConfig;
	assertDBContent(dbConfig.getDBAgent(), statement, expect);
}

void test_addIssueTrackerWithInvalidType(void)
{
	IssueTrackerInfo issueTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_ISSUE_SETTING);
	createTestIssueTracker(issueTracker);
	createPostData(issueTracker, params);
	params["type"] = "9999";

	assertAddIssueTracker(params, userId,
			      HTERR_INVALID_ISSUE_TRACKER_TYPE);
}

void test_addIssueTrackerWithoutNickname(void)
{
	IssueTrackerInfo issueTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_ISSUE_SETTING);
	createTestIssueTracker(issueTracker);
	createPostData(issueTracker, params);
	params.erase("nickname");

	assertAddIssueTracker(params, userId, HTERR_NOT_FOUND_PARAMETER);
}

void test_addIssueTrackerWithoutPrivilege(void)
{
	IssueTrackerInfo issueTracker;
	StringMap params;
	UserIdType userId = findUserWithout(OPPRVLG_CREATE_ISSUE_SETTING);
	createTestIssueTracker(issueTracker);
	createPostData(issueTracker, params);

	assertAddIssueTracker(params, userId, HTERR_NO_PRIVILEGE);
}

void test_updateIssueTracker(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_ISSUE_SETTING);
	const IssueTrackerIdType targetId = 2;
	IssueTrackerInfo issueTracker = testIssueTrackerInfo[targetId - 1];
	issueTracker.id = targetId;
	issueTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(issueTracker, params);

	assertUpdateIssueTracker(params, targetId, userId, HTERR_OK);

	// check the content in the DB
	string statement = StringUtils::sprintf(
	  "select * from issue_trackers where id=%d", targetId);
	string expect = makeIssueTrackerInfoOutput(issueTracker);
	DBClientConfig dbConfig;
	assertDBContent(dbConfig.getDBAgent(), statement, expect);
}

void test_updateIssueTrackerWithoutAccountParams(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_ISSUE_SETTING);
	const IssueTrackerIdType targetId = 2;
	IssueTrackerInfo issueTracker = testIssueTrackerInfo[targetId - 1];
	issueTracker.id = targetId;
	issueTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(issueTracker, params);
	params.erase("userName");
	params.erase("password");

	assertUpdateIssueTracker(params, targetId, userId, HTERR_OK);

	// check the content in the DB
	string statement = StringUtils::sprintf(
	  "select * from issue_trackers where id=%d", targetId);
	string expect = makeIssueTrackerInfoOutput(issueTracker);
	DBClientConfig dbConfig;
	assertDBContent(dbConfig.getDBAgent(), statement, expect);
}

void test_updateIssueTrackerWithoutPrivilege(void)
{
	UserIdType userId = findUserWithout(OPPRVLG_UPDATE_ISSUE_SETTING);
	const IssueTrackerIdType targetId = 2;
	IssueTrackerInfo issueTracker = testIssueTrackerInfo[targetId - 1];
	issueTracker.id = targetId;
	issueTracker.baseURL = "https://example.com/redmine/";
	StringMap params;
	createPostData(issueTracker, params);

	HatoholErrorCode expectedCode = HTERR_NO_PRIVILEGE;
	if (!(testUserInfo[userId - 1].flags & OPPRVLG_GET_ALL_ISSUE_SETTINGS))
		expectedCode = HTERR_NOT_FOUND_TARGET_RECORD;
	assertUpdateIssueTracker(
	  params, targetId, userId, HTERR_NOT_FOUND_TARGET_RECORD);

	// check the content in the DB
	string statement = StringUtils::sprintf(
	  "select * from issue_trackers where id=%d", targetId);
	IssueTrackerInfo expectedIssueTracker = testIssueTrackerInfo[targetId - 1];
	expectedIssueTracker.id = targetId;
	string expect = makeIssueTrackerInfoOutput(expectedIssueTracker);
	DBClientConfig dbConfig;
	assertDBContent(dbConfig.getDBAgent(), statement, expect);
}

void test_deleteIssueTracker(void)
{
	startFaceRest();

	const IssueTrackerIdType issueTrackerId = 1;
	string url = StringUtils::sprintf(
	  "/issue-tracker/%" FMT_ISSUE_TRACKER_ID, issueTrackerId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_ISSUE_SETTING);
	g_parser = getResponseAsJsonParser(arg);

	assertErrorCode(g_parser);
	IssueTrackerIdSet issueTrackerIdSet;
	issueTrackerIdSet.insert(issueTrackerId);
	assertIssueTrackersInDB(issueTrackerIdSet);
}

void test_deleteIssueTrackerWithoutId(void)
{
	startFaceRest();

	string url = StringUtils::sprintf("/issue-tracker");
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_ISSUE_SETTING);
	g_parser = getResponseAsJsonParser(arg);

	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
	assertIssueTrackersInDB(EMPTY_ISSUE_TRACKER_ID_SET);
}

void test_deleteIssueTrackerWithoutPrivelge(void)
{
	startFaceRest();

	const IssueTrackerIdType issueTrackerId = 1;
	string url = StringUtils::sprintf(
	  "/issue-tracker/%" FMT_ISSUE_TRACKER_ID, issueTrackerId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWithout(OPPRVLG_DELETE_ISSUE_SETTING);
	g_parser = getResponseAsJsonParser(arg);

	assertErrorCode(g_parser, HTERR_NO_PRIVILEGE);
	assertIssueTrackersInDB(EMPTY_ISSUE_TRACKER_ID_SET);
}

} // namespace testFaceRestIssueTracker
