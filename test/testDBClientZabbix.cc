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

static void _assertCreateTableZBX(int svId, const string &tableName)
{
	string dbPath = deleteDBClientZabbixDB(svId);
	DBClientZabbix dbCliZBX(svId);
	assertCreateTable(DBClientZabbix::getDBDomainId(svId),tableName);
}
#define assertCreateTableZBX(I,T) cut_trace(_assertCreateTableZBX(I,T))

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

} // testDBClientZabbix

