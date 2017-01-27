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
#include "FaceRest.h"
#include "Helpers.h"
#include "JSONParser.h"
#include "DBTablesTest.h"
#include "MultiLangTest.h"
#include "ThreadLocalDBCache.h"
#include "FaceRestTestUtils.h"

using namespace std;
using namespace mlpl;

namespace testFaceRestSeverityRank {

static JSONParser *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBSeverityRankInfo();
	loadTestDBTablesUser();
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;
}

static void _assertSeverityRanks(
  const string &path = "/severity-rank", const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = 1;
	OperationPrivilege privilege(arg.userId);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertStartObject(g_parser, "SeverityRanks");
	for (size_t i = 0; i < NumTestSeverityRankInfoDef; i++) {
		const SeverityRankInfo &severityRank = testSeverityRankInfoDef[i];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "id", i + 1);
		assertValueInParser(g_parser, "status", severityRank.status);
		assertValueInParser(g_parser, "color", severityRank.color);
		assertValueInParser(g_parser, "label", severityRank.label);
		assertValueInParser(g_parser, "asImportant", severityRank.asImportant);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertSeverityRanks(P,...) \
cut_trace(_assertSeverityRanks(P,##__VA_ARGS__))

#define assertAddSeverityRank(P, ...) \
cut_trace(_assertAddRecord(P, "/severity-rank", ##__VA_ARGS__))

#define assertUpdateSeverityRank(P, ...) \
cut_trace(_assertUpdateRecord(P, "/severity-rank", ##__VA_ARGS__))

static void _assertSeverityRankInfoInDB(
  const SeverityRankInfo &expectedSeverityRankInfo,
  const SeverityRankIdType &targetId = ALL_SEVERITY_RANKS)
{
	string statement;

	if (targetId == ALL_SEVERITY_RANKS) {
		statement = "select * from severity_ranks "
			    "order by id desc limit 1";
	} else {
		statement = StringUtils::sprintf(
			      "select * from severity_ranks where id=%"
			      FMT_SEVERITY_RANK_ID,
			      targetId);
	}
	string expected = makeSeverityRankInfoOutput(
			    expectedSeverityRankInfo);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expected);
}
#define assertSeverityRankInDB(E, ...) \
cut_trace(_assertSeverityRankInfoInDB(E, ##__VA_ARGS__))


void _assertSeverityRanksInDB(const set<SeverityRankIdType> &excludeSeverityRankIdSet)
{
	string statement = "select * from severity_ranks ";
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestSeverityRankInfoDef; i++) {
		SeverityRankIdType severityRankId = i + 1;
		SeverityRankInfo severityRankInfo
		  = testSeverityRankInfoDef[i];
		severityRankInfo.id = severityRankId;
		set<SeverityRankIdType>::iterator it
		  = excludeSeverityRankIdSet.find(severityRankId);
		if (it != excludeSeverityRankIdSet.end())
			continue;
		expect += makeSeverityRankInfoOutput(severityRankInfo);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expect);
}
#define assertSeverityRanksInDB(E) cut_trace(_assertSeverityRanksInDB(E))

void test_getSeverityRankInfo()
{
	assertSeverityRanks();
}

static void createTestSeverityRank(SeverityRankInfo &severityRank)
{
	severityRank.id     = AUTO_INCREMENT_VALUE;
	// should be create new status value
	severityRank.status = static_cast<int>(TRIGGER_SEVERITY_EMERGENCY) + 1;
	severityRank.color  = "#00FF00";
	severityRank.label  = "UserDefined 1";
	severityRank.asImportant  = true;
}

static void createPostData(const SeverityRankInfo &severityRank,
			   StringMap &params)
{
	params["id"]     = to_string(severityRank.id);
	params["status"] = to_string(static_cast<int>(severityRank.status));
	params["color"]  = severityRank.color;
	params["label"]  = severityRank.label;
	if (severityRank.asImportant) {
		params["asImportant"] = "true";
	} else {
		params["asImportant"] = "false";
	}
}

void test_addSeverityRank(void)
{
	const SeverityRankIdType expectedId
	  = NumTestSeverityRankInfoDef + 1;
	SeverityRankInfo severityRank;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_SEVERITY_RANK);
	createTestSeverityRank(severityRank);
	createPostData(severityRank, params);

	assertAddSeverityRank(params, userId, HTERR_OK, expectedId);
	severityRank.id = expectedId;
	assertSeverityRankInDB(severityRank);
}

void test_updateSeverityRank(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_SEVERITY_RANK);
	const SeverityRankIdType targetId = 2;
	SeverityRankInfo severityRank
	  = testSeverityRankInfoDef[targetId - 1];
	severityRank.id    = targetId;
	severityRank.color = "#00FAFA";
	severityRank.label = "Information";
	severityRank.asImportant = false;
	StringMap params;
	createPostData(severityRank, params);

	assertUpdateSeverityRank(params, targetId, userId, HTERR_OK);
	assertSeverityRankInDB(severityRank, targetId);
}

void test_deleteSeverityRank(void)
{
	startFaceRest();

	const SeverityRankIdType severityRankId = 1;
	string url = StringUtils::sprintf(
	  "/severity-rank/%" FMT_SEVERITY_RANK_ID, severityRankId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_SEVERITY_RANK);
	g_parser = getResponseAsJSONParser(arg);

	assertErrorCode(g_parser);
	set<SeverityRankIdType> severityRankIdSet;
	severityRankIdSet.insert(severityRankId);
	assertSeverityRanksInDB(severityRankIdSet);
}

} // namespace testFaceRestSeverityRank
