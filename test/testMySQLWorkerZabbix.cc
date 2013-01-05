#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include <glib.h>
#include "SQLProcessor.h"

namespace testMySQLWorkerZabbix {

gchar *g_standardOutput = NULL;
gchar *g_standardError = NULL;
GError *g_error = NULL;

void setup()
{
}

void teardown()
{
	if (g_standardOutput) {
		g_free(g_standardOutput);
		g_standardOutput = NULL;
	}
	if (g_standardError) {
		g_free(g_standardError);
		g_standardError = NULL;
	}
	if (g_error) {
		g_error_free(g_error);
		g_error = NULL;
	}
}

static void printOutput(void)
{
	cut_notify("<<stdout>>\n%s\n<<stderr>>\n%s",
	           g_standardOutput, g_standardError);
}

static void assertRecord(const char *expected)
{
	vector<string> lines;
	string stdOutStr(g_standardOutput);
	StringUtils::split(lines, stdOutStr, '\n');
	cut_assert_equal_int(2, lines.size());
	cut_assert_equal_string(expected, lines[1].c_str());
}

static void executeCommand(const char *cmd)
{
	const gchar *working_directory = NULL;
	const char *portStr = getenv("TEST_MYSQL_PORT");
	if (!portStr)
		portStr = "3306";
	const gchar *argv[] = {
	  "mysql", "-h", "127.0.0.1", "-P", NULL /* portStr */, "-B",
	  "-e", NULL /* cmd */, NULL};
	argv[4] = portStr;
	argv[7] = cmd;
	gchar *envp = NULL;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GSpawnChildSetupFunc child_setup = NULL;
	gpointer user_data = NULL;
	gint exitStatus;
	gboolean ret = g_spawn_sync(working_directory,
	                            (gchar **)argv, (gchar **)envp, flags,
	                            child_setup, user_data,
	                            &g_standardOutput, &g_standardError,
	                            &exitStatus, &g_error);
	if (ret != TRUE)
		printOutput();
	cppcut_assert_equal(TRUE, ret);
	if (0 != exitStatus)
		printOutput();
	cppcut_assert_equal(0, exitStatus);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_connect(void)
{
	const char *cmd = {"select @@version_comment"};
	executeCommand(cmd);
	assertRecord("ASURA");
}

} // namespace testMySQLWorkerZabbix
