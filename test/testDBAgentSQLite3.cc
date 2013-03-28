#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "StringUtils.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"

namespace testDBAgentSQLite3 {

static string dbPath = "/tmp/testDBAgentSQLite3.db";

static MonitoringServerInfo serverInfo = 
{
	1,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"pochi.dog.com",          // hostname
	"192.168.0.5",            // ip_address
	"POCHI",                  // nickname
};

static void deleteDB(void)
{
	unlink(dbPath.c_str());
	cut_assert_not_exist_path(dbPath.c_str());
}

static int getDBVersion(void)
{
	gboolean ret;
	gchar *standardOutput = NULL;
	gchar *standardError = NULL;
	gint exitStatus;
	GError *error = NULL;
	string errorStr;
	string stdoutStr, stderrStr;
	int version;
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select version from system\"",
	               dbPath.c_str());

	ret = g_spawn_command_line_sync(cmd.c_str(),
	                                &standardOutput, &standardError,
	                                &exitStatus, &error);
	if (standardOutput) {
		stdoutStr = standardOutput;
		g_free(standardOutput);
	}
	if (standardError) {
		stderrStr = standardError;
		g_free(standardError);
	}
	if (!ret)
		goto err;
	if (exitStatus != 0)
		goto err;
	version = atoi(stdoutStr.c_str());
	return version;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("ret: %d, exit status: %d\n<<stdout>>\n%s\n<<stderr>>\n%s\n"
	         "<<error->message>>\n%s",
	         ret, exitStatus, stdoutStr.c_str(), stderrStr.c_str(), 
	         errorStr.c_str());
}

static string getFixturesDir(void)
{
	char *cwd = get_current_dir_name();
	string dir = cwd;
	free(cwd);
	dir += G_DIR_SEPARATOR;
	dir += "fixtures";
	dir += G_DIR_SEPARATOR;
	return dir;
}

#define DEFINE_DBAGENT_WITH_INIT(DB_NAME, OBJ_NAME) \
string _path = getFixturesDir() + DB_NAME; \
DBAgentSQLite3::init(_path); \
DBAgentSQLite3 OBJ_NAME; \

void setup(void)
{
	deleteDB();
	DBAgentSQLite3::init(dbPath);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createSystemTable(void)
{
	{
		// we use a block to call destructor of dbAgent for
		// closing a DB.
		DBAgentSQLite3 dbAgent;
	}
	cut_assert_exist_path(dbPath.c_str());
	cppcut_assert_equal(DBAgentSQLite3::DB_VERSION, getDBVersion());
}

void test_testIsTableExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(true, dbAgent.isTableExisting("foo"));
}

void test_testIsTableExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(false, dbAgent.isTableExisting("NotExistTable"));
}

void test_testIsRecordxisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectTrueCondition = "id=1";
	cppcut_assert_equal
	  (true, dbAgent.isRecordExisting("foo", expectTrueCondition));
}

void test_testIsRecordExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectFalseCondition = "id=100";
	cppcut_assert_equal
	  (false, dbAgent.isRecordExisting("foo", expectFalseCondition));
}

void test_testAddTargetServer(void)
{
	DBAgentSQLite3 dbAgent;
	bool gotException = false;
	try {
		dbAgent.addTargetServer(&serverInfo);
	} catch (const AsuraException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

} // testDBAgentSQLite3

