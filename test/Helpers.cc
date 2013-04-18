#include <cppcutter.h>
#include <unistd.h>
#include "Helpers.h"
#include "DBClientZabbix.h"
#include "DBAgentSQLite3.h"

void _assertStringVector(StringVector &expected, StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}

void _assertExist(const string &target, const string &words)
{
	StringVector splitWords;
	string wordsStripCR = StringUtils::eraseChars(words, "\n");
	StringUtils::split(splitWords, wordsStripCR, ' ');
	for (size_t i = 0; i < splitWords.size(); i++) {
		if (splitWords[i] == target)
			return;
	}
	cut_fail("Not found: %s in %s", target.c_str(), words.c_str());
}

string executeCommand(const string &commandLine)
{
	gboolean ret;
	gchar *standardOutput = NULL;
	gchar *standardError = NULL;
	gint exitStatus;
	GError *error = NULL;
	string errorStr;
	string stdoutStr, stderrStr;

	ret = g_spawn_command_line_sync(commandLine.c_str(),
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
	return stdoutStr;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("ret: %d, exit status: %d\n<<stdout>>\n%s\n<<stderr>>\n%s\n"
	         "<<error->message>>\n%s",
	         ret, exitStatus, stdoutStr.c_str(), stderrStr.c_str(), 
	         errorStr.c_str());
	return "";
}

string getFixturesDir(void)
{
	char *cwd = get_current_dir_name();
	string dir = cwd;
	free(cwd);
	dir += G_DIR_SEPARATOR;
	dir += "fixtures";
	dir += G_DIR_SEPARATOR;
	return dir;
}

bool isVerboseMode(void)
{
	static bool checked = false;
	static bool verboseMode = false;
	if (!checked) {
		char *env = getenv("VERBOSE");
		if (env && env == string("1"))
			verboseMode = true;
		checked = true;
	}
	return verboseMode;
}

string deleteDBClientZabbixDB(int serverId)
{
	DBDomainId domainId = DBClientZabbix::getDBDomainId(serverId);
	string dbPath = DBAgentSQLite3::getDBPath(domainId);
	unlink(dbPath.c_str());
	cut_assert_not_exist_path(dbPath.c_str());
	return dbPath;
}

string execSqlite3ForDBClientZabbix(int serverId, const string &statement)
{
	DBDomainId domainId = DBClientZabbix::getDBDomainId(serverId);
	string dbPath = DBAgentSQLite3::getDBPath(domainId);
	cut_assert_exist_path(dbPath.c_str());
	string commandLine =
	  StringUtils::sprintf("sqlite3 %s \"%s\"",
	                       dbPath.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}
