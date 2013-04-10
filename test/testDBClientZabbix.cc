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

static void _assertCreateTable(int svId, const string &tableName)
{
	string dbPath = deleteDBClientZabbixDB(svId);
	DBClientZabbix dbCliZBX(svId);
	string command = "sqlite3 " + dbPath + " \".table\"";
	string output = executeCommand(command);
	assertExist(tableName, output);
}
#define assertCreateTable(I,T) cut_trace(_assertCreateTable(I,T))

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
}

void test_createTableSystem(void)
{
	int svId = TEST_ZABBIX_SERVER_ID + 1;
	assertCreateTable(svId, "system");

	// check content
	string dbPath = DBClientZabbix::getDBPath(svId);
	string cmd = StringUtils::sprintf("sqlite3 %s \"select * from %s\"",
	                                  dbPath.c_str(), "system");
	string output = executeCommand(cmd);
	string expectedOut =
	   StringUtils::sprintf("%d|-1\n", DBClientZabbix::DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableReplicaGeneration(void)
{
	assertCreateTable(TEST_ZABBIX_SERVER_ID + 2, "replica_generation");
}

void test_createTableTriggersRaw2_0(void)
{
	assertCreateTable(TEST_ZABBIX_SERVER_ID + 3, "triggers_raw_2_0");
}

} // testDBClientZabbix

