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
gchar **argv = NULL;
GPid pid;
GError error;

void teardown(void)
{
	g_spawn_close_pid(pid);
}
void test_Daemonize(void)
{
	g_spawn_async_with_pipes(NULL,
			argv,
			NULL,
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&pid,
			NULL,
			NULL,
			NULL,
			NULL);
	cppcut_assert_not_null(pid);
}
}

