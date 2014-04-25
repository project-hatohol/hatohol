/*
 * Copyright (C) 2014 Project Hatohol
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
#include "ChildProcessManager.h"
#include "HatoholError.h"
#include "Helpers.h"

namespace testChildProcessManager {

static gboolean failureDueToTimedOut(gpointer data)
{
	cut_fail("Timed out");
	return G_SOURCE_REMOVE;
}

static void _assertCreate(ChildProcessManager::CreateArg &arg)
{
	arg.args.push_back("/bin/cat");
	assertHatoholError(HTERR_OK,
	                   ChildProcessManager::getInstance()->create(arg));
	cppcut_assert_not_equal(0, arg.pid);
}
#define assertCreate(A) cut_trace(_assertCreate(A))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createWithEmptyParameter(void)
{
	ChildProcessManager::CreateArg arg;
	assertHatoholError(HTERR_INVALID_ARGS,
	                   ChildProcessManager::getInstance()->create(arg));
}

void test_createWithInvalidPath(void)
{
	ChildProcessManager::CreateArg arg;
	arg.args.push_back("non-exisiting-command");
	assertHatoholError(HTERR_FAILED_TO_SPAWN,
	                   ChildProcessManager::getInstance()->create(arg));
}

void test_create(void)
{
	ChildProcessManager::CreateArg arg;
	assertCreate(arg);
}

void test_collectedCb(void)
{
	static const size_t timeout = 5000; // ms
	struct Ctx : public ChildProcessManager::EventCallback {
		guint tag;
		GMainLoop *loop;

		Ctx(void)
		: tag(0),
		  loop(NULL)
		{
		}


		virtual ~Ctx()
		{
			if (loop)
				g_main_loop_unref(loop);
			if (tag)
				g_source_remove(tag);
		}

		virtual void onCollected(const siginfo_t *siginfo) // override
		{
			g_main_loop_quit(loop);
		}
	} ctx;

	ChildProcessManager::CreateArg arg;
	arg.eventCb = &ctx;
	assertCreate(arg);
	cppcut_assert_equal(0, kill(arg.pid, SIGKILL));
	ctx.tag = g_timeout_add(timeout, failureDueToTimedOut, NULL);
	ctx.loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(ctx.loop);
}

} // namespace testChildProcessManager
