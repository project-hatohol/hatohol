/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <glib.h>
#include <cutter.h>
#include <cppcutter.h>

#include "Logger.h"
#include "StringUtils.h"
#include "ParsableString.h"
#include "loggerTester.h"

using namespace std;
using namespace mlpl;

namespace testLogger {

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





static const char* LogHeaders [MLPL_NUM_LOG_LEVEL] = {
	"BUG", "CRIT", "ERR", "WARN", "INFO", "DBG",
};

#define DEF 1024
#define TIMEOUT 1
#define syslogPlacePattern 3

static void _assertSyslogOutput(const char *envMessage, const char *outMessage,
                             bool expectOut)
{

	LogLevel level = MLPL_LOG_INFO;
	const char *fileName = "test file";
	int lineNumber = 1;
	
	char consoleMessage[DEF];
	memset(consoleMessage, '\0', sizeof(consoleMessage));
	sprintf(consoleMessage, "[%s] <%s:%d> ", LogHeaders[level], fileName,
	        lineNumber);
	strcat(consoleMessage, envMessage);

	char syslogPlace[syslogPlacePattern][32] = {
		"/var/log/messages",    //CentOS
		"/var/log/syslog",      //ubuntu
		"/var/adm/messages"};   //Slackware
	
	int kindOfOS = 0;
	int fp;
	for (int i = 0; i < syslogPlacePattern; i++){
		if ((fp = open(syslogPlace[i], O_RDONLY)) != -1){
			kindOfOS = i;
			break;
		}
	}

	if (fp == -1){
		cut_fail("Error occur in test_syslogoutput. Failed to open syslog.");
	}

	char hoge[DEF];
        while (read(fp, hoge, DEF));
	
	
	int fd = inotify_init();
	inotify_add_watch(fd, syslogPlace[kindOfOS], 
				   IN_MODIFY|IN_ATTRIB|IN_DELETE_SELF|IN_MOVE_SELF);


	Logger::enableSyslogOutput();
	

	Logger::log(level, fileName, lineNumber,outMessage);

	time_t start = time(NULL);
	bool output = false;
	for (;;) {
		struct pollfd fds[1];
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		if (poll(fds, 1 ,(TIMEOUT - time(NULL) + start)*1000) > 0){
			char buf[DEF];
		        if (!read(fd, buf, sizeof(buf))){
				cut_fail("Error occur in test_syslogoutput. Failed to read buf.");
			}
		} else {
			break;
		}
		
		char syslogMessage[DEF];
		memset(syslogMessage, 0, sizeof(syslogMessage));

		if (!read(fp, syslogMessage, DEF)){
			cut_fail("Error occur in test_syslogoutput. Failed to read syslog.");
		}
		
		if (strstr(syslogMessage, consoleMessage) != NULL) {
			output = true;
			break;
		}
	}
	
	close(fp);
	close(fd);
 
	if (output != expectOut){
		cut_fail("Error occur in test_syslogoutput. Don't output expect message in syslog.");
	}
	
}
#define assertSyslogOutput(EM,OM,EXP) cut_trace(_assertSyslogOutput(EM,OM,EXP))



void test_syslogoutput(void)
{
	assertSyslogOutput("Test message", "Test message",  true);
	assertSyslogOutput("Test message", "Message of test",  false);
}

} // namespace testLogger
