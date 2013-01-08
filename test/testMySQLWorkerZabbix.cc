#include <string>
#include <vector>
#include <map>
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

typedef map<size_t, string> NumberStringMap;
typedef NumberStringMap::iterator NumberStringMapIterator;

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

static void assertRecord(int numExpectedLines, NumberStringMap &nsmap)
{
	vector<string> lines;
	string stdOutStr(g_standardOutput);
	StringUtils::split(lines, stdOutStr, '\n');
	cut_assert_equal_int(numExpectedLines, lines.size());

	NumberStringMapIterator it = nsmap.begin();
	for (; it != nsmap.end(); ++it) {
		size_t lineNum = it->first;
		string &expected = it->second;
		cut_assert_equal_string(expected.c_str(),
		                        lines[lineNum].c_str());
	}
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
void test_atAtVersionComment(void)
{
	const char *cmd = "select @@version_comment";
	executeCommand(cmd);
	NumberStringMap nsmap;
	nsmap[1] = "ASURA";
	assertRecord(2, nsmap);
}

void test_selectNodes(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT n.* FROM nodes n WHERE n.nodetype=1 ORDER BY n.nodeid";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_selectConfig(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT c.* FROM config c "
	  "WHERE c.configid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(2, nsmap);
}

void test_selectUseridAutoLogoutLastAccess(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT u.userid,u.autologout,s.lastaccess FROM sessions s,users u "
	  "WHERE s.sessionid='9331b5c345021fa3879caa8922586199' AND "
	  "s.status=0 AND s.userid=u.userid AND "
	  "((s.lastaccess+u.autologout>1357641517) OR (u.autologout=0)) "
	  "AND u.userid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(2, nsmap);
}

} // namespace testMySQLWorkerZabbix
