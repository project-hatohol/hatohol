#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "Asura.h"
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

static void _assertCreateTableZBX(int svId, const string &tableName)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX(svId, dbClientZabbix, dbPath)
	assertCreateTable(DBClientZabbix::getDBDomainId(svId),tableName);
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

static void _assertGetEventsAsAsuraFormat(bool noHostData = false)
{
	DELETE_DB_AND_DEFINE_DBCLIENT_ZABBIX_STDID(dbZabbix);
	
	// write test dat to DB
	dbZabbix.addTriggersRaw2_0(makeTestTriggerData());
	dbZabbix.addHostsRaw2_0(makeTestHostData(noHostData));

	// get asura format data and check
	TriggerInfoList triggerInfoList;
	dbZabbix.getTriggersAsAsuraFormat(triggerInfoList);

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
#define assertGetEventsAsAsuraFormat(...) \
cut_trace(_assertGetEventsAsAsuraFormat(__VA_ARGS__))

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

void setup(void)
{
	asuraInit();
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

void test_getEventsAsAsuraFormat(void)
{
	assertGetEventsAsAsuraFormat();
}

void test_getEventsAsAsuraFormatWithMissingData(void)
{
	assertGetEventsAsAsuraFormat(true);
}

void test_addApplicationsRaw2_0Insert(void)
{
	assertAddApplicationsRaw2_0();
}

void test_addApplicationsRaw2_0Update(void)
{
	assertAddApplicationsRaw2_0(true);
}

} // testDBClientZabbix

