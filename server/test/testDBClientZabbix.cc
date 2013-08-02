/*
 * Copyright (C) 2013 Project Hatohol
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
#include <cutter.h>
#include <unistd.h>

#include "Hatohol.h"
#include "DBClientZabbix.h"
#include "ConfigManager.h"
#include "Helpers.h"

#include "StringUtils.h"
using namespace mlpl;

namespace testDBClientZabbix {

static const int TEST_ZABBIX_SERVER_ID = 3;

#define DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX(SVID, OBJNAME, DBPATH) \
string DBPATH = deleteDBClientZabbixDB(SVID); \
DBClientZabbix OBJNAME(SVID);

#define DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX_STDID(OBJNAME) \
DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX(TEST_ZABBIX_SERVER_ID, OBJNAME, _dbPath)

class DBClientZabbixTester : public DBClientZabbix {
public:
	static void extractItemKeys(StringVector &params, const string &key)
	{
		DBClientZabbix::extractItemKeys(params, key);
	}

	static int getItemVariable(const string &word) 
	{
		return DBClientZabbix::getItemVariable(word);
	}

	static void testMakeItemBrief(const string &name, const string &key,
	                              const string &expected)
	{
		VariableItemGroupPtr itemGroup;
		itemGroup->ADD_NEW_ITEM(String,
		                        ITEM_ID_ZBX_ITEMS_NAME, name);
		itemGroup->ADD_NEW_ITEM(String,
		                        ITEM_ID_ZBX_ITEMS_KEY_, key);
		cppcut_assert_equal(
		  expected, DBClientZabbixTester::makeItemBrief(itemGroup));
	}
};

static void _assertCreateTableZBX(int svId, const string &tableName)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX(svId, dbClientZabbix, dbPath)
	assertCreateTable(dbClientZabbix.getDBAgent(), tableName);
}
#define assertCreateTableZBX(I,T) cut_trace(_assertCreateTableZBX(I,T))

static ItemTablePtr makeTestTriggerData(void)
{
	VariableItemTablePtr triggers;
	VariableItemGroupPtr grp;
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_TRIGGERS_TRIGGERID, 1);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_TRIGGERS_EXPRESSION, "");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_TRIGGERS_DESCRIPTION,"");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_TRIGGERS_URL,        "");
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_STATUS,     0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_VALUE,      0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_PRIORITY,   0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_LASTCHANGE, 0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_TRIGGERS_COMMENTS,   "");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_TRIGGERS_ERROR,      "");
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_TRIGGERS_TEMPLATEID, 0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_TYPE,       0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS,0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_TRIGGERS_FLAGS,      0);
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_TRIGGERS_HOSTID,     1);
	triggers->add(grp);

	return (ItemTablePtr)triggers;
}

static ItemTablePtr makeTestHostData(bool returnEmptyTable = false)
{
	VariableItemTablePtr hosts;
	if (returnEmptyTable)
		return (ItemTablePtr)hosts;
	VariableItemGroupPtr grp;
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_HOSTS_HOSTID,            1);
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_HOSTS_PROXY_HOSTID,      0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_HOST,              "host");
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_STATUS,            0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_DISABLE_UNTIL,     0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_ERROR,             "");
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_AVAILABLE,         0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_ERRORS_FROM,       0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_LASTACCESS,        0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_IPMI_AUTHTYPE,     0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_IPMI_PRIVILEGE,    0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_IPMI_USERNAME,     "");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_IPMI_PASSWORD,     "");
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_IPMI_DISABLE_UNTIL,0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_IPMI_AVAILABLE,    0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_SNMP_DISABLE_UNTIL,0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_SNMP_AVAILABLE,    0);
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_HOSTS_MAINTENANCEID,     0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_MAINTENANCE_STATUS,0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_MAINTENANCE_TYPE,  0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_MAINTENANCE_FROM,  0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_IPMI_ERRORS_FROM,  0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_SNMP_ERRORS_FROM,  0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_IPMI_ERROR,        "");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_SNMP_ERROR,        "");
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_JMX_DISABLE_UNTIL, 0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_JMX_AVAILABLE,     0);
	grp->ADD_NEW_ITEM(Int,    ITEM_ID_ZBX_HOSTS_JMX_ERRORS_FROM,   0);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_JMX_ERROR,         "");
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_HOSTS_NAME,              "name" );
	hosts->add(grp);
	return (ItemTablePtr)hosts;
}

static ItemTablePtr makeTestApplicationData(void)
{
	VariableItemTablePtr app;
	VariableItemGroupPtr grp;
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID, 1);
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_APPLICATIONS_HOSTID,        3);
	grp->ADD_NEW_ITEM(String, ITEM_ID_ZBX_APPLICATIONS_NAME,        "App");
	grp->ADD_NEW_ITEM(Uint64, ITEM_ID_ZBX_APPLICATIONS_TEMPLATEID,    0);
	app->add(grp);
	return (ItemTablePtr)app;
}

static void _assertGetEventsAsHatoholFormat(bool noHostData = false)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX_STDID(dbZabbix);
	
	// write test dat to DB
	dbZabbix.addTriggersRaw2_0(makeTestTriggerData());
	dbZabbix.addHostsRaw2_0(makeTestHostData(noHostData));

	// get hatohol format data and check
	TriggerInfoList triggerInfoList;
	dbZabbix.getTriggersAsHatoholFormat(triggerInfoList);

	TriggerInfoListIterator it = triggerInfoList.begin();
	for (; it != triggerInfoList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		cppcut_assert_equal((uint64_t)1, triggerInfo.id);

		uint64_t expectedHostId = noHostData ? 0 : 1;
		cppcut_assert_equal(expectedHostId, triggerInfo.hostId);

		string expectedHostName = noHostData ? "" : "name";
		cppcut_assert_equal(expectedHostName, triggerInfo.hostName);
	}
}
#define assertGetEventsAsHatoholFormat(...) \
cut_trace(_assertGetEventsAsHatoholFormat(__VA_ARGS__))

void _assertAddHostsRaw2_0(bool writeTwice = false)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX_STDID(dbZabbix);

	// The first write is insertion, the second is the update.
	dbZabbix.addHostsRaw2_0(makeTestHostData());
	if (writeTwice)
		dbZabbix.addHostsRaw2_0(makeTestHostData());
}

#define assertAddHostsRaw2_0(...) \
cut_trace(_assertAddHostsRaw2_0(__VA_ARGS__))

void _assertAddApplicationsRaw2_0(bool writeTwice = false)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX_STDID(dbZabbix);

	// The first write is insertion, the second is the update.
	dbZabbix.addApplicationsRaw2_0(makeTestApplicationData());
	if (writeTwice)
		dbZabbix.addApplicationsRaw2_0(makeTestApplicationData());
}

#define assertAddApplicationsRaw2_0(...) \
cut_trace(_assertAddApplicationsRaw2_0(__VA_ARGS__))

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createDB(void)
{
	// remove the DB that already exists
	string dbPath = deleteDBClientZabbixDB(TEST_ZABBIX_SERVER_ID);

	// create an instance (the database will be automatically created)
	DBClientZabbix dbCliZBX(TEST_ZABBIX_SERVER_ID);
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient";
	string output = execSqlite3ForDBClientZabbix(TEST_ZABBIX_SERVER_ID,
	                                             statement);
	string expectedOut = StringUtils::sprintf
	                       ("%d|%d\n", DBClient::DBCLIENT_DB_VERSION,
	                                   DBClientZabbix::ZABBIX_DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableSystem(void)
{
	int svId = TEST_ZABBIX_SERVER_ID + 1;
	assertCreateTableZBX(svId, "system");

	// check content
	string statement = "select * from system";
	string output = execSqlite3ForDBClientZabbix(svId, statement);
	int dummyValue = 0;
	string expectedOut =
	   StringUtils::sprintf("%d\n", dummyValue);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableTriggersRaw2_0(void)
{
	assertCreateTableZBX(TEST_ZABBIX_SERVER_ID + 3, "triggers_raw_2_0");
}

void test_createTableFunctionsRaw2_0(void)
{
	assertCreateTableZBX(TEST_ZABBIX_SERVER_ID + 3, "functions_raw_2_0");
}

void test_createTableItemsRaw2_0(void)
{
	assertCreateTableZBX(TEST_ZABBIX_SERVER_ID + 3, "items_raw_2_0");
}

void test_createTableHostsRaw2_0(void)
{
	assertCreateTableZBX(TEST_ZABBIX_SERVER_ID + 3, "hosts_raw_2_0");
}

void test_createTableEventsRaw2_0(void)
{
	assertCreateTableZBX(TEST_ZABBIX_SERVER_ID + 3, "events_raw_2_0");
}

void test_getEventsAsHatoholFormat(void)
{
	assertGetEventsAsHatoholFormat();
}

void test_getEventsAsHatoholFormatWithMissingData(void)
{
	assertGetEventsAsHatoholFormat(true);
}

void test_addHostsRaw2_0Insert(void)
{
	assertAddHostsRaw2_0();
}

void test_addHostsRaw2_0Update(void)
{
	assertAddHostsRaw2_0(true);
}

void test_addApplicationsRaw2_0Insert(void)
{
	assertAddApplicationsRaw2_0();
}

void test_addApplicationsRaw2_0Update(void)
{
	assertAddApplicationsRaw2_0(true);
}

void test_extractItemKeys(void)
{
	StringVector vect;
	DBClientZabbixTester::extractItemKeys
	  (vect, "vm.memory.size[available]");
	assertStringVectorVA(vect, "available", NULL);
}

void test_extractItemKeysNoBracket(void)
{
	StringVector vect;
	DBClientZabbixTester::extractItemKeys(vect, "system.uname");
	assertStringVectorVA(vect, NULL);
}

void test_extractItemKeysNullParams(void)
{
	StringVector vect;
	DBClientZabbixTester::extractItemKeys(vect, "proc.num[]");
	assertStringVectorVA(vect, "", NULL);
}

void test_extractItemKeysTwo(void)
{
	StringVector vect;
	DBClientZabbixTester::extractItemKeys(vect, "vfs.fs.size[/boot,free]");
	assertStringVectorVA(vect, "/boot", "free", NULL);
}

void test_extractItemKeysWithEmptyParams(void)
{
	StringVector vect;
	DBClientZabbixTester::extractItemKeys(vect, "proc.num[,,run]");
	assertStringVectorVA(vect, "", "", "run", NULL);
}

void test_getItemVariable(void)
{
	cppcut_assert_equal(1, DBClientZabbixTester::getItemVariable("$1"));
}

void test_getItemVariableMultipleDigits(void)
{
	cppcut_assert_equal(123, DBClientZabbixTester::getItemVariable("$123"));
}

void test_getItemVariableWord(void)
{
	cppcut_assert_equal(-1, DBClientZabbixTester::getItemVariable("abc"));
}

void test_getItemVariableDoubleDollar(void)
{
	cppcut_assert_equal(-1, DBClientZabbixTester::getItemVariable("$$"));
}

void test_makeItemBrief(void)
{
	DBClientZabbixTester::testMakeItemBrief(
	  "CPU $2 time", "system.cpu.util[,idle]",
	  "CPU idle time");
}

void test_makeItemBriefOverNumVariable10(void)
{
	DBClientZabbixTester::testMakeItemBrief(
	  "ABC $12", "foo[P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13]",
	  "ABC P12");
}

void test_makeItemBriefTwoParam(void)
{
	DBClientZabbixTester::testMakeItemBrief(
	  "Free disk space on $1", "vfs.fs.size[/,free]",
	  "Free disk space on /");
}

void test_makeItemBriefTwoKeys(void)
{
	DBClientZabbixTester::testMakeItemBrief(
	  "Zabbix $4 $2 processes, in %",
	  "zabbix[process,unreachable poller,avg,busy]",
	  "Zabbix busy unreachable poller processes, in %");
}

void test_makeItemBriefNoVariables(void)
{
	DBClientZabbixTester::testMakeItemBrief(
	  "Host name", "system.hostname", "Host name");
}

} // testDBClientZabbix

