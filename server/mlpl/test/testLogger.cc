/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <glib.h>
#include <cutter.h>
#include <cppcutter.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <gcutter.h>

#include "Logger.h"
#include "StringUtils.h"
#include "ParsableString.h"
#include "loggerTester.h"
#include "SmartTime.h"

using namespace std;
using namespace mlpl;

namespace testLogger {

class testLogger : public Logger {
public:
	static string callCreateHeader(LogLevel level, const char *fileName,
	                               int lineNumber, string extraInfoString)
	{
		return createHeader(level, fileName, lineNumber, extraInfoString);
	}

	static string callCreateExtraInfoString(void)
	{
		return createExtraInfoString();
	}

	static void callAddProcessId(string &testString)
	{
		addProcessId(testString);
	}

	static void callAddThreadId(string &testString)
	{
		addThreadId(testString);
	}

	static void callAddCurrentTime(string &testString)
	{
		addCurrentTime(testString);
	}

	static void callSetExtraInfoFlag(const char *extraInfoArg)
	{
		setExtraInfoFlag(extraInfoArg);
	}
};

static gchar *g_standardOutput = NULL;
static gchar *g_standardError = NULL;
static GError *g_error = NULL;
static gint g_exitStatus;
static gboolean g_spawnRet;

static void failAndShowSpawnResult(void)
{
	const char *err_msg = "";
	if (g_error)
		err_msg = g_error->message; 

	cut_fail(
	  "ret: %d, exitStatus: %d, msg: %s\n"
	  "<<stdout>>\n"
	  "%s\n"
	  "<<stderr>>\n"
	  "%s\n",
	  g_spawnRet, g_exitStatus, err_msg, g_standardOutput, g_standardError);
}

static void assertSpawnResult(void)
{
	if (g_spawnRet != TRUE)
		goto err;
	if (g_exitStatus != EXIT_SUCCESS)
		goto err;
	cut_assert(true);
	return;
err:
	failAndShowSpawnResult();
}

static vector<string> split(const string str, const char delim)
{
	istringstream iss(str);
	string snippet;
	vector<string> afterSplitStrings;
	while(getline(iss, snippet, delim))
		afterSplitStrings.push_back(snippet);

	return afterSplitStrings;
}

static long stringToLong(string stringInteger)
{
	long longInteger;
	istringstream iss(stringInteger);
	iss >> longInteger;

	return longInteger;
}

static void _assertLogOutput(const char *envLevel, const char *outLevel,
                             bool expectOut)
{
	cppcut_assert_equal(0, setenv(Logger::LEVEL_ENV_VAR_NAME, envLevel, 1));
	const gchar *testDir = cut_get_test_directory();
	testDir = testDir ? testDir : ".";
	const gchar *commandPath = cut_build_path(testDir, "loggerTestee",
						   NULL);
	string commandLine = commandPath + string(" ") + string(outLevel);
	g_spawnRet = g_spawn_command_line_sync(commandLine.c_str(),
	                                       &g_standardOutput,
	                                       &g_standardError,
	                                       &g_exitStatus, &g_error);
	cut_trace(assertSpawnResult());
	if (!expectOut) {
		cppcut_assert_equal(true, string(g_standardError).empty());
		return;
	}

	ParsableString pstr(g_standardError);
	string word = pstr.readWord(ParsableString::SEPARATOR_SPACE);
	string expectStr = StringUtils::sprintf("[%s]", outLevel);
	cppcut_assert_equal(expectStr, word);

	word = pstr.readWord(ParsableString::SEPARATOR_SPACE);
	word = pstr.readWord(ParsableString::SEPARATOR_SPACE);

	expectStr = testString;
	expectStr += "\n";
	cppcut_assert_equal(expectStr, word);
}
#define assertLogOutput(EL,OL,EXP) cut_trace(_assertLogOutput(EL,OL,EXP))

static void _assertWaitSyslogUpdate(int fd, int timeout, bool &timedOut)
{
	static const size_t INOTIFY_EVT_BUF_SIZE =
	  sizeof(struct inotify_event) + NAME_MAX + 1;
	struct pollfd fds[1];
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	int ret = poll(fds, 1, timeout);
	if (ret == 0) {
		timedOut = true;
		return;
	}
	cppcut_assert_not_equal(-1, ret, cut_message("%s", g_strerror(errno)));

	char buf[INOTIFY_EVT_BUF_SIZE];
	ssize_t readRet = read(fd, buf, sizeof(buf));
	cppcut_assert_not_equal((ssize_t)-1, readRet,
	                        cut_message("%s", g_strerror(errno)));
}
#define assertWaitSyslogUpdate(F,T,O) cut_trace(_assertWaitSyslogUpdate(F,T,O))

static void _assertSyslogOutput(const char *envMessage, const char *outMessage,
                                bool shouldLog)
{
	static const char* LogHeaders[MLPL_NUM_LOG_LEVEL] = {
		"BUG", "CRIT", "ERR", "WARN", "INFO", "DBG",
	};

	LogLevel level = MLPL_LOG_INFO;
	const char *fileName = "test file";
	int lineNumber = 1;
	string expectedMsg =
	   StringUtils::sprintf("[%s] <%s:%d> ",
	                        LogHeaders[level], fileName, lineNumber);
	expectedMsg += envMessage;

	const char *syslogPathCandidates[] = {
		"/var/log/syslog",      //ubuntu
		"/var/log/messages",    //CentOS
	};
	const size_t numSyslogPathCandidates =
	   sizeof(syslogPathCandidates) / sizeof(const char *);
	
	const char *syslogPath = NULL;
	ifstream syslogFileStream;
	for (size_t i = 0; i < numSyslogPathCandidates; i++){
		syslogPath = syslogPathCandidates[i];
		syslogFileStream.open(syslogPath, ios::in);
		if (syslogFileStream.good())
			break;
	}
	cppcut_assert_equal(true, syslogFileStream.good(),
	                    cut_message("Failed to find a syslog file: %s: "
	                                "errno: %d.", syslogPath, errno));
	syslogFileStream.seekg(0, ios_base::end);

	int fd = inotify_init();
	inotify_add_watch(fd, syslogPath,
	                  IN_MODIFY|IN_ATTRIB|IN_DELETE_SELF|IN_MOVE_SELF);
	if (shouldLog)
		Logger::enableSyslogOutput();
	else
		Logger::disableSyslogOutput();
	Logger::log(level, fileName, lineNumber, "%s", outMessage);

	static const int TIMEOUT = 5 * 1000; // millisecond
	int expireClock = time(NULL) * 1000 + TIMEOUT;
	bool found = false;
	while (!found) {
		bool timedOut = false;
		int timeout = expireClock - time(NULL) * 1000;
		if (timeout <= 0)
			break;
		assertWaitSyslogUpdate(fd, timeout, timedOut);
		if (timedOut)
			break;

		auto tryFindMessage = [&]() {
			string line;
			while (getline(syslogFileStream, line)) {
				if (line.find(expectedMsg, 0) != string::npos)
					return true;
			}
			return false;
		};
		found = tryFindMessage();
	}
	close(fd);
	cppcut_assert_equal(shouldLog, found);
}
#define assertSyslogOutput(EM,OM,EXP) cut_trace(_assertSyslogOutput(EM,OM,EXP))

static void _assertCreateHeader(void)
{
	string processAndThreadIdStr = StringUtils::sprintf("%d T:%ld", getpid(),
	                                                    syscall(SYS_gettid));

	string actHeader = ":" + processAndThreadIdStr
	                    + " [ERR] <test_file.py:18> ";
	string testHeader = testLogger::callCreateHeader(MLPL_LOG_ERR,
	                                                 "test_file.py", 18,
	                                   testLogger::callCreateExtraInfoString());

	vector<string> afterSplitStrings = split(testHeader, 'P');
	cppcut_assert_equal(2, (int)afterSplitStrings.size());

	testHeader = afterSplitStrings[1];
	cppcut_assert_equal(actHeader, testHeader);
}
#define assertCreateHeader() cut_trace(_assertCreateHeader())

static void _assertCreateExtraInfo(const char *extraInfoArg)
{
	string testString = testLogger::callCreateExtraInfoString();
	if (strchr(extraInfoArg, 'C') != NULL)
		cppcut_assert_not_null(strstr(testString.c_str(),"["));
	if (strchr(extraInfoArg, 'P') != NULL)
		cppcut_assert_not_null(strstr(testString.c_str(),"P:"));
	if (strchr(extraInfoArg, 'T') != NULL)
		cppcut_assert_not_null(strstr(testString.c_str(),"T:"));
}
#define assertCreateExtraInfo(EXIA) cut_trace(_assertCreateExtraInfo(EXIA))

static void _assertAddProcessId(void)
{
	string testString;
	string actString = StringUtils::sprintf("P:%d ", getpid());

	testLogger::callAddProcessId(testString);

	cppcut_assert_equal(actString, testString);
}
#define assertAddProcessId() cut_trace(_assertAddProcessId())

static void _assertAddThreadId(void)
{
	string testString;
	string actString = StringUtils::sprintf("T:%ld ", syscall(SYS_gettid));

	testLogger::callAddThreadId(testString);

	cppcut_assert_equal(actString, testString);
}
#define assertAddThreadId() cut_trace(_assertAddThreadId())

static void _assertAddCurrentTime(void)
{
	SmartTime startTime = SmartTime::getCurrTime();

	string testString;
	testLogger::callAddCurrentTime(testString);
	// 'addCurrentTime()' makes a string such as "[sec.nsec]".
	// 'timespec' uses only the sec and nsec.
	// The following statements delete "[", "]" and ".".
	testString.erase(testString.begin());
	testString.erase(--testString.end());
	vector<string> SplitStrings = split(testString, '.');
	timespec testTimespec = {stringToLong(SplitStrings[0]),
	                         stringToLong(SplitStrings[1])};
	SmartTime headerTime = SmartTime(testTimespec);

	SmartTime stopTime = SmartTime::getCurrTime();

	cppcut_assert_equal(true, startTime < headerTime);
	cppcut_assert_equal(true, headerTime < stopTime);
}
#define assertAddCurrentTime() cut_trace(_assertAddCurrentTime())

void cut_teardown(void)
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

// ---------------------------------------------------------------------------
// test cases
// ---------------------------------------------------------------------------
void test_envLevelDBG(void)
{
	assertLogOutput("DBG", "DBG",  true);
	assertLogOutput("DBG", "INFO", true);
	assertLogOutput("DBG", "WARN", true);
	assertLogOutput("DBG", "ERR",  true);
	assertLogOutput("DBG", "CRIT", true);
	assertLogOutput("DBG", "BUG",  true);
}

void test_envLevelINFO(void)
{
	assertLogOutput("INFO", "DBG",  false);
	assertLogOutput("INFO", "INFO", true);
	assertLogOutput("INFO", "WARN", true);
	assertLogOutput("INFO", "ERR",  true);
	assertLogOutput("INFO", "CRIT", true);
	assertLogOutput("INFO", "BUG",  true);
}

void test_envLevelWARN(void)
{
	assertLogOutput("WARN", "DBG",  false);
	assertLogOutput("WARN", "INFO", false);
	assertLogOutput("WARN", "WARN", true);
	assertLogOutput("WARN", "ERR",  true);
	assertLogOutput("WARN", "CRIT", true);
	assertLogOutput("WARN", "BUG",  true);
}

void test_envLevelERR(void)
{
	assertLogOutput("ERR", "DBG",  false);
	assertLogOutput("ERR", "INFO", false);
	assertLogOutput("ERR", "WARN", false);
	assertLogOutput("ERR", "ERR",  true);
	assertLogOutput("ERR", "CRIT", true);
	assertLogOutput("ERR", "BUG",  true);
}

void test_envLevelCRIT(void)
{
	assertLogOutput("CRIT", "DBG",  false);
	assertLogOutput("CRIT", "INFO", false);
	assertLogOutput("CRIT", "WARN", false);
	assertLogOutput("CRIT", "ERR",  false);
	assertLogOutput("CRIT", "CRIT", true);
	assertLogOutput("CRIT", "BUG",  true);
}

void test_envLevelBUG(void)
{
	assertLogOutput("BUG", "DBG",  false);
	assertLogOutput("BUG", "INFO", false);
	assertLogOutput("BUG", "WARN", false);
	assertLogOutput("BUG", "ERR",  false);
	assertLogOutput("BUG", "CRIT", false);
	assertLogOutput("BUG", "BUG",  true);
}

void test_syslogoutput(void)
{
	assertSyslogOutput("Test message", "Test message",  true);
}

void test_syslogoutputDisable(void)
{
	assertSyslogOutput("Test message", "Message of test",  false);
}

void test_createHeader(void)
{
	assertCreateHeader();
}

void data_createExtraInfo(void)
{
	gcut_add_datum("Pattern PTC",
	               "pattern", G_TYPE_STRING, "PTC",
	               NULL);
	gcut_add_datum("Pattern PT",
	               "pattern", G_TYPE_STRING, "PT",
	                NULL);
	gcut_add_datum("Pattern TC",
	               "pattern", G_TYPE_STRING, "TC",
	                NULL);
	gcut_add_datum("Pattern PC",
	               "pattern", G_TYPE_STRING, "PC",
	               NULL);
	gcut_add_datum("Pattern P",
	               "pattern", G_TYPE_STRING, "P",
	               NULL);
	gcut_add_datum("Pattern T",
	               "pattern", G_TYPE_STRING, "T",
	               NULL);
	gcut_add_datum("Pattern C",
	               "pattern", G_TYPE_STRING, "C",
	               NULL);
	gcut_add_datum("Pattern null",
	               "pattern", G_TYPE_STRING, "",
	               NULL);
}

void test_createExtraInfo(gconstpointer data)
{
	const char *pattern = gcut_data_get_string(data, "pattern");
	testLogger::callSetExtraInfoFlag(pattern);
	assertCreateExtraInfo(pattern);
}

void test_addProcessId(void)
{
	assertAddProcessId();
}

void test_addThreadId(void)
{
	assertAddThreadId();
}

void test_addCurrentTime(void)
{
	assertAddCurrentTime();
}

} // namespace testLogger
