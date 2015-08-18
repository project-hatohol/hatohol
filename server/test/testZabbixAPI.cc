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

#include <string>
#include <gcutter.h>
#include "Helpers.h"
#include "ZabbixAPITestUtils.h"

using namespace std;
using namespace mlpl;

namespace testZabbixAPI {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getAPIVersion_2_2_0(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getAPIVersion(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION);
}

void data_checkAPIVersion(void)
{
	gcut_add_datum("Equal",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower micro version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 3,
		       NULL);
	gcut_add_datum("Higher micro version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 5,
		       NULL);
	gcut_add_datum("Higher minor version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 1,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower major version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 1,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Higher major version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 3,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
}

void test_checkAPIVersion(gconstpointer data)
{
	assertCheckAPIVersion(gcut_data_get_boolean(data, "expected"),
			      gcut_data_get_int(data, "major"),
			      gcut_data_get_int(data, "minor"),
			      gcut_data_get_int(data, "micro"));
}

void test_openSession(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
}

void test_getAuthToken(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);

	string firstToken, secondToken;
	firstToken = zbxApiTestee.callAuthToken();
	secondToken = zbxApiTestee.callAuthToken();
	cppcut_assert_equal(firstToken, secondToken);
}

void test_verifyTriggers(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();

	ItemTablePtr actualTriggersTablePtr;
	ItemTablePtr expectTriggersTablePtr;

	zbxApiTestee.makeTriggersItemTable(expectTriggersTablePtr);
	actualTriggersTablePtr = zbxApiTestee.callGetTrigger();

	assertItemTable(expectTriggersTablePtr, actualTriggersTablePtr);
}

void test_verifyTriggerExpandedDescriptions(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();

	ItemTablePtr actualExpandedDescTablePtr;
	ItemTablePtr expectedExpandedDescTablePtr;

	zbxApiTestee.makeTriggerExpandedDescriptionItemTable(expectedExpandedDescTablePtr);
	actualExpandedDescTablePtr = zbxApiTestee.callGetTriggerExpandedDescription();

	assertItemTable(expectedExpandedDescTablePtr, actualExpandedDescTablePtr);
}

void test_verifyMergeTriggerExpandedDescriptions(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();

	ItemTablePtr plainTriggersTablePtr;
	ItemTablePtr expandedDescriptionTablePtr;
	ItemTablePtr mergedTablePtr;

	plainTriggersTablePtr = zbxApiTestee.callGetTrigger();
	expandedDescriptionTablePtr = zbxApiTestee.callGetTriggerExpandedDescription();
	mergedTablePtr = zbxApiTestee.callMergePlainTriggersAndExpandedDescriptions(
	  plainTriggersTablePtr, expandedDescriptionTablePtr);

	// TODO: check the ItemTablePtr content
}

void test_verifyGroupsAndHostsGroups(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();

	ItemTablePtr expectGroupsTablePtr;
	ItemTablePtr expectHostsGroupsTablePtr;
	ItemTablePtr actualGroupsTablePtr;
	ItemTablePtr actualHostsGroupsTablePtr;
	ItemTablePtr dummyHostsTablePtr;

	zbxApiTestee.makeGroupsItemTable(expectGroupsTablePtr);
	zbxApiTestee.makeMapHostsHostgroupsItemTable(expectHostsGroupsTablePtr);
	zbxApiTestee.callGetGroups(actualGroupsTablePtr);
	zbxApiTestee.callGetHosts(dummyHostsTablePtr,
	                          actualHostsGroupsTablePtr);

	assertItemTable(expectGroupsTablePtr, actualGroupsTablePtr);
	assertItemTable(expectHostsGroupsTablePtr, actualHostsGroupsTablePtr);
}

void test_getLastEventId(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
	cppcut_assert_equal((uint64_t)8697, zbxApiTestee.callGetLastEventId());
}

void test_getHistory(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
	ZabbixAPI::ValueType valueTypeFloat = ZabbixAPI::VALUE_TYPE_FLOAT;
	ItemTablePtr history =
	  zbxApiTestee.callGetHistory("25490", valueTypeFloat,
				      1413265550, 1413268970);
	const ItemGroupList &list = history->getItemGroupList();
	ItemGroupListConstIterator it = list.begin();
	string json;
	for (; it != list.end(); ++it) {
		ItemIdType itemid;
		uint64_t clock, ns;
		string value;
		ItemGroupStream historyGroupStream(*it);
		historyGroupStream.seek(ITEM_ID_ZBX_HISTORY_ITEMID);
		historyGroupStream >> itemid;
		historyGroupStream.seek(ITEM_ID_ZBX_HISTORY_CLOCK);
		historyGroupStream >> clock;
		historyGroupStream.seek(ITEM_ID_ZBX_HISTORY_NS);
		historyGroupStream >> ns;
		historyGroupStream.seek(ITEM_ID_ZBX_HISTORY_VALUE);
		historyGroupStream >> value;
		if (!json.empty())
			json += ",";
		json += mlpl::StringUtils::sprintf(
			"{"
			"\"itemid\":\"%" FMT_ITEM_ID "\","
			"\"clock\":\"%" PRIu64 "\","
			"\"value\":\"%s\","
			"\"ns\":\"%" PRIu64 "\""
			"}",
			itemid.c_str(), clock, value.c_str(), ns);
	}
	string path = getFixturesDir() + "zabbix-api-res-history.json";
	gchar *contents = NULL;
	g_file_get_contents(path.c_str(), &contents, NULL, NULL);
	string expected = contents ? contents : "";
	json = string("{\"jsonrpc\":\"2.0\",\"result\":[")
		+ json + string("],\"id\":1}");
	cppcut_assert_equal(expected, json);
}

void data_pushStringWithPadding(void)
{
	gcut_add_datum("Fill",
	               "digitNum", G_TYPE_INT,    8,
	               "body",     G_TYPE_STRING, "ABC",
	               "padChar",  G_TYPE_CHAR,   '#',
	               "expect",   G_TYPE_STRING, "#####ABC", NULL);
	gcut_add_datum("Same size",
	               "digitNum", G_TYPE_INT,    3,
	               "body",     G_TYPE_STRING, "ABC",
	               "padChar",  G_TYPE_CHAR,   '#',
	               "expect",   G_TYPE_STRING, "ABC", NULL);
	gcut_add_datum("Long string",
	               "digitNum", G_TYPE_INT,    3,
	               "body",     G_TYPE_STRING, "ABCDEF",
	               "padChar",  G_TYPE_CHAR,   '#',
	               "expect",   G_TYPE_STRING, "ABCDEF", NULL);
}

void test_pushStringWithPadding(gconstpointer data)
{
	using StringUtils::sprintf;
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);

	const ItemId  itemId   = 100;
	const char   *name     = "foo";
	const size_t  digitNum = gcut_data_get_int(data, "digitNum");
	const char   *body     = gcut_data_get_string(data, "body");
	const char    padChar  = gcut_data_get_char(data, "padChar");
	const string  expected = gcut_data_get_string(data, "expect");
	JSONParser parser(sprintf("{\"%s\":\"%s\"}", name, body));
	VariableItemGroupPtr itemGrpPtr;
	
	const string createdString =
	  zbxApiTestee.callPushString(parser, itemGrpPtr, name, itemId,
	                              digitNum, padChar);
	cppcut_assert_equal(expected, createdString);
	cppcut_assert_equal((size_t)1, itemGrpPtr->getNumberOfItems());
	const ItemData *itemData = itemGrpPtr->getItem(itemId);
	cppcut_assert_not_null(itemData);
	cppcut_assert_equal(expected, itemData->getString());
}

} // namespace testZabbixAPI
