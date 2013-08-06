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

#include "Hatohol.h"
#include "Utils.h"

namespace testMain {
GPid pid;
GMainLoop *loop;

void testEndChildProcess(GPid child_pid, gint status, gpointer data)
{
	cut_fail("When this call, child process of hatohol is die.");
	g_main_loop_quit(loop);
}

bool testMainLoop(void)
{
	loop = g_main_loop_new(NULL, TRUE);
	g_child_watch_add(pid, (GChildWatchFunc)testEndChildProcess, loop);
	g_main_loop_run(loop);

	return true;
}

void teardown(void)
{
	g_spawn_close_pid(pid);
}
void test_daemonize(void)
{
	const gchar *argv[] = {"../src/hatohol", "--config-db-server", "localhost", NULL};
	gint stdOut, stdErr;
	GError *error;
	gboolean succeeded;
	gboolean expected = TRUE;
	succeeded = g_spawn_async_with_pipes(NULL,
			const_cast<gchar**>(argv),
			NULL,
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&pid,
			NULL,
			&stdOut,
			&stdErr,
			&error);
	cppcut_assert_equal(expected, succeeded);
	testMainLoop();
}
}

