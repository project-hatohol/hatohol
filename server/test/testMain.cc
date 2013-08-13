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

#include "Hatohol.h"
#include "Utils.h"
using namespace std;

namespace testMain {
GPid pid;
pid_t grandchildPid = 0;
GMainLoop *loop;
guint eventTimeout = 0;
int randomNumber;

bool checkMagicNumber(string &actualEnvironment)
{
	stringstream ssMagicNumberString;
	ssMagicNumberString << "MAGICNUMBER=" << randomNumber;
	string MagicNumber = ssMagicNumberString.str();
	return actualEnvironment == MagicNumber;
}

void endChildProcess(GPid child_pid, gint status, gpointer data)
{
	const char *grandchildPidFilePath = "/var/run/hatohol.pid";
	cut_assert_exist_path(grandchildPidFilePath);
	FILE *grandchildPidFile;
	grandchildPidFile = fopen(grandchildPidFilePath, "r");
	cppcut_assert_not_null(grandchildPidFile);
	cppcut_assert_not_equal(EOF, fscanf(grandchildPidFile, "%d", &grandchildPid));

	stringstream ssStat;
	ssStat << "/proc/" << grandchildPid << "/stat";
	string grandchildProcFilePath = ssStat.str();
	cut_assert_exist_path(grandchildProcFilePath.c_str());
	FILE *grandchildProcFile;
	grandchildProcFile = fopen(grandchildProcFilePath.c_str(), "r");
	cppcut_assert_not_null(grandchildProcFile);
	int grandchildProcPid, grandchildProcPpid;
	char comm[11];
	char state;
	cppcut_assert_equal(4, fscanf(grandchildProcFile, "%d (%10s) %c %d ", &grandchildProcPid, comm, &state, &grandchildProcPpid));
	cppcut_assert_equal(1, grandchildProcPpid);

	stringstream grandchildProcEnvironPath;
	ifstream grandchildEnvironFile;
	string env;
	bool isMagicNumber;
	grandchildProcEnvironPath << "/proc/" << grandchildPid << "/environ";
	grandchildEnvironFile.open(grandchildProcEnvironPath.str().c_str());
	while (getline(grandchildEnvironFile, env, '\0')) {
		isMagicNumber = checkMagicNumber(env);
		if (isMagicNumber)
			break;
	}
	cppcut_assert_equal(true, isMagicNumber);

	g_main_loop_quit(loop);
}

gboolean timeOutChildProcess(gpointer data)
{
	cut_fail("Timeout to be daemon.");
	g_main_loop_run(loop);
	return FALSE;
}

bool childProcessLoop(void)
{
	loop = g_main_loop_new(NULL, TRUE);
	g_child_watch_add(pid, endChildProcess, loop);
	eventTimeout = g_timeout_add(100, timeOutChildProcess, NULL);
	g_main_loop_run(loop);

	return true;
}

void setup(void)
{
	srand((unsigned int)time(NULL));
	randomNumber = rand();
}

void teardown(void)
{
	g_spawn_close_pid(pid);
	if (grandchildPid > 1) {
		kill(grandchildPid, SIGTERM);
		grandchildPid = 0;
	}
	if (eventTimeout != 0) {
		gboolean expected = TRUE;
		cppcut_assert_equal(expected, g_source_remove(eventTimeout));
	}
}

void test_daemonize(void)
{
	stringstream ss;
	ss << "MAGICNUMBER=" << randomNumber;
	string magicNumber = ss.str();
	const gchar *argv[] = {"../src/hatohol", "--config-db-server", "localhost", NULL};
	const gchar *envp[] = {"LD_LIBRARY_PATH=../src/.libs/", magicNumber.c_str(), NULL};
	gint stdOut, stdErr;
	GError *error;
	gboolean succeeded;
	gboolean expected = TRUE;
	succeeded = g_spawn_async_with_pipes(NULL,
			const_cast<gchar**>(argv),
			const_cast<gchar**>(envp),
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&pid,
			NULL,
			&stdOut,
			&stdErr,
			&error);
	cppcut_assert_equal(expected, succeeded);
	cppcut_assert_equal(true, childProcessLoop());

}
}

