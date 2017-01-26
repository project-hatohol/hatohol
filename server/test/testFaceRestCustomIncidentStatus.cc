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

namespace testFaceRestCustomIncidentStatus {

static JSONParser *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBCustomIncidentStatusInfo();
	loadTestDBTablesUser();
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;
}

static void _assertCustomIncidentStatuses(
  const string &path = "/custom-incident-status", const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	// FIXME: We don't define GET_ALL privileges for customIncidentStatus.
	arg.userId = 2;
	OperationPrivilege privilege(arg.userId);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertStartObject(g_parser, "CustomIncidentStatuses");
	for (size_t i = 0; i < NumTestCustomIncidentStatus; i++) {
		const CustomIncidentStatus &customIncidentStatus =
			testCustomIncidentStatus[i];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "id", i + 1);
		assertValueInParser(g_parser, "code", customIncidentStatus.code);
		assertValueInParser(g_parser, "label", customIncidentStatus.label);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertCustomIncidentStatuses(P,...) \
cut_trace(_assertCustomIncidentStatuses(P,##__VA_ARGS__))

#define assertAddCustomIncidentStatus(P, ...) \
cut_trace(_assertAddRecord(P, "/custom-incident-status", ##__VA_ARGS__))

#define assertUpdateCustomIncidentStatus(P, ...) \
cut_trace(_assertUpdateRecord(P, "/custom-incident-status", ##__VA_ARGS__))

static void _assertCustomIncidentStatusInDB(
  const CustomIncidentStatus &expectedCustomIncidentStatus,
  const CustomIncidentStatusIdType &targetId = ALL_CUSTOM_INCIDENT_STATUSES)
{
	string statement;

	if (targetId == ALL_CUSTOM_INCIDENT_STATUSES) {
		statement = "select * from custom_incident_statuses "
			    "order by id desc limit 1";
	} else {
		statement = StringUtils::sprintf(
			      "select * from custom_incident_statuses where id=%"
			      FMT_CUSTOM_INCIDENT_STATUS_ID,
			      targetId);
	}
	string expected = makeCustomIncidentStatusOutput(
			    expectedCustomIncidentStatus);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expected);
}
#define assertCustomIncidentStatusInDB(E, ...) \
cut_trace(_assertCustomIncidentStatusInDB(E, ##__VA_ARGS__))

void _assertCustomIncidentStatusesInDB(
  const set<CustomIncidentStatusIdType> &excludeCustomIncidentStatusIdSet)
{
	string statement = "select * from custom_incident_statuses ";
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestCustomIncidentStatus; i++) {
		CustomIncidentStatusIdType customIncidentStatusId = i + 1;
		CustomIncidentStatus customIncidentStatus
		  = testCustomIncidentStatus[i];
		customIncidentStatus.id = customIncidentStatusId;
		set<CustomIncidentStatusIdType>::iterator it
		  = excludeCustomIncidentStatusIdSet.find(customIncidentStatusId);
		if (it != excludeCustomIncidentStatusIdSet.end())
			continue;
		expect += makeCustomIncidentStatusOutput(customIncidentStatus);
	}
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getConfig().getDBAgent(), statement, expect);
}
#define assertCustomIncidentStatusesInDB(E) \
cut_trace(_assertCustomIncidentStatusesInDB(E))

void test_getCustomIncidentStatuses()
{
	assertCustomIncidentStatuses();
}

static void createTestCustomIncidentStatus(
  CustomIncidentStatus &customIncidentStatus)
{
	customIncidentStatus.id     = AUTO_INCREMENT_VALUE;
	customIncidentStatus.code  = "__DEFAULT_CODE";
	customIncidentStatus.label  = "UserDefined 1";
}

static void createPostData(const CustomIncidentStatus &customIncidentStatus,
			   StringMap &params)
{
	params["id"]     = to_string(customIncidentStatus.id);
	params["code"]  = customIncidentStatus.code;
	params["label"]  = customIncidentStatus.label;
}

void test_addCustomIncidentStatus(void)
{
	const CustomIncidentStatusIdType expectedId
	  = NumTestCustomIncidentStatus + 1;
	CustomIncidentStatus customIncidentStatus;
	StringMap params;
	UserIdType userId = findUserWith(OPPRVLG_CREATE_INCIDENT_SETTING);
	createTestCustomIncidentStatus(customIncidentStatus);
	createPostData(customIncidentStatus, params);

	assertAddCustomIncidentStatus(params, userId, HTERR_OK, expectedId);
	customIncidentStatus.id = expectedId;
	assertCustomIncidentStatusInDB(customIncidentStatus);
}

void test_updateCustomIncidentStatus(void)
{
	UserIdType userId = findUserWith(OPPRVLG_UPDATE_INCIDENT_SETTING);
	const CustomIncidentStatusIdType targetId = 2;
	CustomIncidentStatus customIncidentStatus
	  = testCustomIncidentStatus[targetId - 1];
	customIncidentStatus.id    = targetId;
	customIncidentStatus.code = "IN PROGRESS";
	customIncidentStatus.label = "In progress (edited)";
	StringMap params;
	createPostData(customIncidentStatus, params);

	assertUpdateCustomIncidentStatus(params, targetId, userId, HTERR_OK);
	assertCustomIncidentStatusInDB(customIncidentStatus, targetId);
}
} // namespace testFaceRestCustomIncidentStatus
