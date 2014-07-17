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
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertStartObject(g_parser, "issueTrackers");
	for (size_t i = 0; i < NumTestIssueTrackerInfo; i++) {
		const IssueTrackerInfo &tracker = testIssueTrackerInfo[i];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "id", tracker.id);
		assertValueInParser(g_parser, "type", tracker.type);
		assertValueInParser(g_parser, "nickname", tracker.nickname);
		assertValueInParser(g_parser, "projectId", tracker.projectId);
		assertValueInParser(g_parser, "trackerId", tracker.trackerId);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertIssueTrackers(P,...) \
cut_trace(_assertIssueTrackers(P,##__VA_ARGS__))

void test_getIssueTracker()
{
	assertIssueTrackers();
}

} // namespace testFaceRestIssueTracker
