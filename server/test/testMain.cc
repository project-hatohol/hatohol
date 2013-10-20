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

#include <cppcutter.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h> 
#include <unistd.h>

#include "Hatohol.h"
#include "Utils.h"
using namespace std;
using namespace mlpl;

namespace testMain {
struct FunctionArg {
	bool timedOut;
	bool isEndChildProcess;
	guint eventChildWatch;
	guint eventTimeout;
	GPid childPid;
	GMainLoop *loop;

	FunctionArg(void)
	: timedOut(false),
	  isEndChildProcess(false),
	  eventChildWatch(0),
	  eventTimeout(0),
	  childPid(0),
	  loop(NULL)
	{
	}
};

struct DaemonizeVariable {
	pid_t grandchildParentPid;
	string magicNumber;
	GPid childPid;
	pid_t grandchildPid;
	bool finishTest;
	string pidFilePath;

	DaemonizeVariable(void)
	: grandchildParentPid(0),
	  childPid(0),
	  grandchildPid(0),
	  finishTest(false)
	{
		pidFilePath = StringUtils::sprintf(
		  "/tmp/hatohol-test-daemonize-test-%d.pid", getpid());
	}

	void checkEnvironAndKillProcess(pid_t pid)
	{
		stringstream grandchildProcEnvironPath;
		ifstream grandchildEnvironFile;
		string env;
		grandchildProcEnvironPath << "/proc/" << pid << "/environ";
		grandchildEnvironFile.open(grandchildProcEnvironPath.str().c_str());
		while (getline(grandchildEnvironFile, env, '\0')) {
			if (env == magicNumber) {
				kill(pid, SIGTERM);
				return;
			}
		}
	}

	bool checkAllProcessID(void)
	{
		int result;
		struct dirent **nameList;

		result = scandir("/proc/", &nameList, NULL, alphasort);
		if (result == -1)
			cut_notify("Can't load /proc/ directory.");
		for (int i = 0; i < result ; ++i) {
			if (StringUtils::isNumber(nameList[i]->d_name)){
				int procPid = atoi(nameList[i]->d_name);
				if (procPid > 1)
					checkEnvironAndKillProcess(procPid);
				free(nameList[i]);
			}
		}
		free(nameList);

		return true;
	}

	~DaemonizeVariable(void)
	{
		if (finishTest) {
			kill(grandchildPid, SIGTERM);
			grandchildPid = 0;
		} else {
			checkAllProcessID();
		}
	}
};
DaemonizeVariable *g_daemonizeValue;

static pid_t getParentPid(pid_t pid, string &programName)
{
	stringstream procStatPath;
	procStatPath << "/proc/" << pid << "/stat";
	ifstream ifs(procStatPath.str().c_str());
	cppcut_assert_equal(
	  true, ifs.good(),
	  cut_message("path: %s", procStatPath.str().c_str()));

	pid_t mypid = 0;
	pid_t parentPid = 0;
	string status;
	ifs >> mypid;
	ifs >> programName;
	ifs >> status;
	ifs >> parentPid;

	programName = StringUtils::eraseChars(programName, "()");
	return parentPid;
}

void endChildProcess(GPid child_pid, gint status, gpointer data)
{
	FunctionArg *arg = (FunctionArg *) data;
	arg->isEndChildProcess = true;
	g_main_loop_quit(arg->loop);
}

gboolean timeOutChildProcess(gpointer data)
{
	FunctionArg *arg = (FunctionArg *) data;
	arg->timedOut = true;
	g_main_loop_quit(arg->loop);
	return FALSE;
}

gboolean closeChildProcess(gpointer data)
{
	FunctionArg *arg = (FunctionArg *) data;
	g_spawn_close_pid(arg->childPid);
	g_main_loop_quit(arg->loop);
	return FALSE;
}

bool childProcessLoop(GPid &childPid)
{
	const gboolean expected = TRUE;
	FunctionArg arg;

	arg.loop = g_main_loop_new(NULL, TRUE);
	arg.eventChildWatch = g_child_watch_add(childPid, endChildProcess, &arg);
	arg.eventTimeout = g_timeout_add_seconds(5, timeOutChildProcess, &arg);
	g_main_loop_run(arg.loop);
	g_main_loop_unref(arg.loop);
	if (!arg.timedOut) {
		g_spawn_close_pid(childPid);
		cppcut_assert_equal(expected, g_source_remove(arg.eventTimeout));
	} else {
		cppcut_assert_equal(expected, g_source_remove(arg.eventChildWatch));

		FunctionArg argForForceTerm;
		argForForceTerm.childPid = childPid;
		kill(childPid, SIGTERM);
		argForForceTerm.loop = g_main_loop_new(NULL, TRUE);
		argForForceTerm.eventChildWatch = g_child_watch_add(childPid, endChildProcess, &argForForceTerm);
		argForForceTerm.eventTimeout = g_timeout_add_seconds(5, closeChildProcess, &argForForceTerm);
		g_main_loop_run(argForForceTerm.loop);
		g_main_loop_unref(argForForceTerm.loop);
		if (!argForForceTerm.timedOut) {
			g_spawn_close_pid(childPid);
			cppcut_assert_equal(expected, g_source_remove(argForForceTerm.eventTimeout));
		} else {
			cppcut_assert_equal(expected, g_source_remove(argForForceTerm.eventChildWatch));
		}
	}

	cppcut_assert_equal(true, arg.isEndChildProcess);
	cppcut_assert_equal(false, arg.timedOut);
	return true;
}

bool parsePIDFile(int &grandchildPid, const string &grandChildPidFilePath)
{
	cut_assert_exist_path(grandChildPidFilePath.c_str());
	FILE *grandchildPidFile;
	grandchildPidFile = fopen(grandChildPidFilePath.c_str(), "r");
	cppcut_assert_not_null(grandchildPidFile);
	cppcut_assert_not_equal(EOF, fscanf(grandchildPidFile, "%d", &grandchildPid));
	cppcut_assert_equal(0, fclose(grandchildPidFile));

	return true;
}

static void parseStatFile(int &parentPid, int grandchildPid)
{
	string programName;
	parentPid = getParentPid(grandchildPid, programName);
}

bool parseEnvironFile(string makedMagicNumber, int grandchildPid)
{
	bool isMagicNumber;
	stringstream grandchildProcEnvironPath;
	ifstream grandchildEnvironFile;
	string env;
	grandchildProcEnvironPath << "/proc/" << grandchildPid << "/environ";
	grandchildEnvironFile.open(grandchildProcEnvironPath.str().c_str());
	while (getline(grandchildEnvironFile, env, '\0')) {
		isMagicNumber = env == makedMagicNumber;
		if (isMagicNumber)
			break;
	}
	cppcut_assert_equal(true, isMagicNumber);

	return true;
}

bool makeRandomNumber(string &magicNumber)
{
	srand((unsigned int)time(NULL));
	int randomNumber = rand();
	stringstream ss;
	ss << "MAGICNUMBER=" << randomNumber;
	magicNumber = ss.str();

	return true;
}

bool spawnChildProcess(string magicNumber, GPid &childPid, const string &pidFilePath)
{
	const gchar *argv[] = {
	  "../src/hatohol", "--config-db-server", "localhost",
	   "--pid-file-path", pidFilePath.c_str(), NULL};
	const gchar *envp[] = {"LD_LIBRARY_PATH=../src/.libs/", magicNumber.c_str(), NULL};
	gint stdOut, stdErr;
	GError *error;
	gboolean succeeded;

	succeeded = g_spawn_async_with_pipes(NULL,
			const_cast<gchar**>(argv),
			const_cast<gchar**>(envp),
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&childPid,
			NULL,
			&stdOut,
			&stdErr,
			&error);

	return succeeded == TRUE;
}

static pid_t getInitPid(int pid)
{
	// Ubuntu 13.10 runs init as a user session mode when X11 is used.
	// In that case, pid of the user mode 'init' is not 1.
	pid_t parentPid = pid;
	string programName;
	while (parentPid != 1) {
		parentPid = getParentPid(pid, programName);
		if (programName == "init")
			break;
		pid = parentPid;
	}
	return pid;
}

void cut_teardown(void)
{
	if (g_daemonizeValue != NULL)
		delete g_daemonizeValue;
}

void test_daemonize(void)
{
	DaemonizeVariable *value = new DaemonizeVariable();
	g_daemonizeValue = value;

	cppcut_assert_equal(true, makeRandomNumber(value->magicNumber));
	cppcut_assert_equal(true,
	   spawnChildProcess(value->magicNumber, value->childPid,
	                     value->pidFilePath));
	cppcut_assert_equal(true, childProcessLoop(value->childPid));
	cppcut_assert_equal(true, parsePIDFile(value->grandchildPid,
	                                       value->pidFilePath));
	parseStatFile(value->grandchildParentPid, value->grandchildPid);
	cppcut_assert_equal(getInitPid(getpid()), value->grandchildParentPid);
	cppcut_assert_equal(true, parseEnvironFile(value->magicNumber, value->grandchildPid));

	value->finishTest = true;
}
}

