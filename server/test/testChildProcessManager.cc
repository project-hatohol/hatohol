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
#include "Reaper.h"

using namespace std;
using namespace mlpl;

namespace testChildProcessManager {

static void _assertCreate(ChildProcessManager::CreateArg &arg)
{
	arg.args.push_back("/bin/cat");
	assertHatoholError(HTERR_OK,
	                   ChildProcessManager::getInstance()->create(arg));
	cppcut_assert_not_equal(0, arg.pid);
}
#define assertCreate(A) cut_trace(_assertCreate(A))

void cut_teardown(void)
{
	ChildProcessManager::getInstance()->reset();
}

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

void test_createWithEnv(void)
{
	ChildProcessManager::CreateArg arg;
	arg.envs.push_back("A=123");
	arg.envs.push_back("XYZ=^_^");
	assertCreate(arg);
	string path = StringUtils::sprintf("/proc/%d/environ", arg.pid);
	string expect;
	for (size_t i = 0; i < arg.envs.size(); i++) {
		expect += arg.envs[i].c_str();
		expect += '\0';
	}
	assertFileContent(expect, path);
}

void test_executedCb(void)
{
	struct Ctx : public ChildProcessManager::EventCallback {
		bool called;
		bool succeeded;

		Ctx(void)
		: called(false),
		  succeeded(false)
		{
		}

		virtual void onExecuted(const bool &_succeeded, GError *gerror) // override
		{
			called = true;
			succeeded = _succeeded;
		}
	};

	ChildProcessManager::CreateArg arg;
	Ctx *ctx = new Ctx();
	arg.eventCb = ctx;
	assertCreate(arg);
	cppcut_assert_equal(true, ctx->called);
	cppcut_assert_equal(true, ctx->succeeded);
}

void test_executedCbWithError(void)
{
	struct Ctx : public ChildProcessManager::EventCallback {
		bool called;
		virtual void onExecuted(const bool &succeeded, GError *gerror) // override
		{
			cppcut_assert_equal(false, succeeded);
			cppcut_assert_not_null(gerror);
			cppcut_assert_equal(G_SPAWN_ERROR, gerror->domain);
			cppcut_assert_equal((gint)G_SPAWN_ERROR_NOENT,
			                    gerror->code);
			called = true;
		}
	} *ctx = new Ctx();
	ctx->called = false;

	ChildProcessManager::CreateArg arg;
	arg.args.push_back("non-exisiting-command");
	arg.eventCb = ctx;
	assertHatoholError(HTERR_FAILED_TO_SPAWN,
	                   ChildProcessManager::getInstance()->create(arg));
	cppcut_assert_equal(true, ctx->called);
}

void test_collectedCb(void)
{
	struct Ctx : public ChildProcessManager::EventCallback {

		GMainLoopWithTimeout mainLoop;
		virtual void onCollected(const siginfo_t *siginfo) // override
		{
			mainLoop.quit();
		}
	} *ctx = new Ctx();

	ChildProcessManager::CreateArg arg;
	arg.eventCb = ctx;
	assertCreate(arg);
	cppcut_assert_equal(0, kill(arg.pid, SIGKILL));
	ctx->mainLoop.run();
}

void test_finalizedCb(void)
{
	struct Ctx : public ChildProcessManager::EventCallback {

		GMainLoopWithTimeout mainLoop;
		virtual void onFinalized(void) // override
		{
			mainLoop.quit();
		}
	} *ctx = new Ctx();

	ChildProcessManager::CreateArg arg;
	arg.eventCb = ctx;
	assertCreate(arg);
	cppcut_assert_equal(0, kill(arg.pid, SIGKILL));
	ctx->mainLoop.run();
}

} // namespace testChildProcessManager
