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
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertIssueTrackers(P,...) \
cut_trace(_assertIssueTrackers(P,##__VA_ARGS__))

#define assertAddIssueTracker(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/issue-tracker", ##__VA_ARGS__))

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
	IssueTrackerInfo issueTracker;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_ISSUE_SETTING);
	createTestIssueTracker(issueTracker);
	createPostData(issueTracker, params);

	assertAddIssueTracker(params, userId, HTERR_OK,
			      NumTestIssueTrackerInfo + 1);

	// check the content in the DB
	string statement = "select * from ";
	statement += "issue_trackers";
	statement += " order by id desc limit 1";
	issueTracker.id = NumTestIssueTrackerInfo + 1;
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

} // namespace testFaceRestIssueTracker
