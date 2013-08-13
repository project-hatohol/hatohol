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
pid_t grandchild_pid;
GMainLoop *loop;
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
	const char *grandchild_pid_file_path = "/var/run/hatohol.pid";
	cut_assert_exist_path(grandchild_pid_file_path);
	FILE *grandchild_pid_file;
	grandchild_pid_file = fopen(grandchild_pid_file_path, "r");
	cppcut_assert_not_null(grandchild_pid_file);
	cppcut_assert_not_equal(EOF, fscanf(grandchild_pid_file, "%d", &grandchild_pid));

	stringstream ssStat;
	ssStat << "/proc/" << grandchild_pid << "/stat";
	string grandchild_proc_file_path = ssStat.str();
	cut_assert_exist_path(grandchild_proc_file_path.c_str());
	FILE *grandchild_proc_file;
	grandchild_proc_file = fopen(grandchild_proc_file_path.c_str(), "r");
	cppcut_assert_not_null(grandchild_proc_file);
	int grandchild_proc_pid,grandchild_proc_ppid;
	char comm[11];
	char state;
	cppcut_assert_equal(4, fscanf(grandchild_proc_file, "%d (%10s) %c %d ", &grandchild_proc_pid, comm, &state, &grandchild_proc_ppid));
	cppcut_assert_equal(1, grandchild_proc_ppid);

	stringstream grandchildProcEnvironPath;
	ifstream grandchildEnvironFile;
	string env;
	bool isMagicNumber;
	grandchildProcEnvironPath << "/proc/" << grandchild_pid << "/environ";
	grandchildEnvironFile.open(grandchildProcEnvironPath.str().c_str());
	while(getline(grandchildEnvironFile, env, '\0')) {
		isMagicNumber = checkMagicNumber(env);
		if(isMagicNumber)
			break;
	}
	cppcut_assert_equal(true, isMagicNumber);

	g_main_loop_quit(loop);
}

gboolean timeOutChildProcess(gpointer data)
{
	cut_fail("Timeout to be daemon.");
	return FALSE;
}

bool childProcessLoop(void)
{
	loop = g_main_loop_new(NULL, TRUE);
	g_child_watch_add(pid, endChildProcess, loop);
	g_timeout_add(100, timeOutChildProcess, NULL);
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
	if(grandchild_pid > 0)
		kill(grandchild_pid,SIGINT);
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
	childProcessLoop();
}
}

