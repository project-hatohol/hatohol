#include <cppcutter.h>
#include <cutter.h>

#include "Asura.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBAgentTest.h"

namespace testDBClientAsura {

template<typename T> void _assertAddToDB(T *arg, void (*func)(T *))
{
	bool gotException = false;
	try {
		(*func)(arg);
	} catch (const AsuraException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

static void addTargetServer(MonitoringServerInfo *serverInfo)
{
	DBClientAsura dbAsura;
	dbAsura.addTargetServer(serverInfo);
}
#define assertAddServerToDB(X) \
cut_trace(_assertAddToDB<MonitoringServerInfo>(X, addTargetServer))

static string makeExpectedOutput(MonitoringServerInfo *serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s\n",
	                        serverInfo->id, serverInfo->type,
	                        serverInfo->hostName.c_str(),
	                        serverInfo->ipAddress.c_str(),
	                        serverInfo->nickname.c_str());
	return expectedOut;
}

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
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_OFFSET_ASURA);

	// create an instance (the database will be automatically created)
	DBClientAsura dbAsura;
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient";
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_OFFSET_ASURA,
	                                       statement);
	string expectedOut = StringUtils::sprintf("%d|%d\n",
	                                          DBClient::DB_VERSION,
	                                          DBClientAsura::DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableServers(void)
{
	const string tableName = "servers";
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_OFFSET_ASURA);
	DBClientAsura dbAsura;
	string command = "sqlite3 " + dbPath + " \".table\"";
	assertExist(tableName, executeCommand(command));

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_OFFSET_ASURA,
	                                       statement);
	string expectedOut = StringUtils::sprintf(""); // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_testAddTargetServer(void)
{
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_OFFSET_ASURA);

	// added a record
	MonitoringServerInfo *testInfo = serverInfo;
	assertAddServerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from servers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}


} // namespace testDBClientAsura
