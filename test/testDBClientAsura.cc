#include <cppcutter.h>
#include <cutter.h>

#include "Asura.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"
#include "Helpers.h"

namespace testDBClientAsura {

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

} // namespace testDBClientAsura
