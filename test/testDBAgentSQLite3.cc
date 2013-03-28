#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "StringUtils.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"

namespace testDBAgentSQLite3 {

static string dbPath = "/tmp/testDBAgentSQLite3.db";

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
	return dir;
}

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
	const string testDBName = "FooTable.db";
	string path = getFixturesDir() + testDBName;
	DBAgentSQLite3::init(path);
	DBAgentSQLite3 dbAgent;
	cppcut_assert_equal(true, dbAgent.isTableExisting("foo"));
}

} // testDBAgentSQLite3

