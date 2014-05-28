/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <syscall.h>
#include <libsoup/soup.h>
#include <MutexLock.h>
#include "Hatohol.h"
#include "Utils.h"
#include "Helpers.h"
#include "HatoholThreadBase.h"
#include "EventSemaphore.h"
using namespace std;
using namespace mlpl;

namespace testUtils {

struct TestExecEvtLoop : public HatoholThreadBase {
	pid_t threadId;
	pid_t eventLoopThreadId;
	GMainContext *context;
	GMainLoop *loop;
	bool quitLoop;
	bool useFunctor;
	SyncType syncType;
	MutexLock mutex;

	TestExecEvtLoop(void)
	: threadId(0),
	  eventLoopThreadId(0),
	  context(NULL),
	  loop(NULL),
	  quitLoop(true),
	  useFunctor(false),
	  syncType(SYNC)
	{
		mutex.lock();
		context = g_main_context_default();
		cppcut_assert_not_null(context);

		loop = g_main_loop_new(context, TRUE);
		cppcut_assert_not_null(loop);
	}

	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		threadId = Utils::getThreadId();
		if (!useFunctor) {
			Utils::executeOnGLibEventLoop<TestExecEvtLoop>(
			  _idleTask, this, syncType, context);
		} else {
			Utils::executeOnGLibEventLoop<TestExecEvtLoop>(
			  *this, syncType, context);
		}
		mutex.unlock();
		return NULL;
	}

	static void _idleTask(TestExecEvtLoop *obj)
	{
		obj->idleTask();
	}

	void idleTask(void)
	{
		eventLoopThreadId = Utils::getThreadId();
		if (quitLoop)
			g_main_loop_quit(loop);
	}

	void operator()(void) {
		idleTask();
	}
};

static void _assertValidateJSMethodName(const string &name, bool expect)
{
	string errMsg;
	cppcut_assert_equal(
	  expect, Utils::validateJSMethodName(name, errMsg),
	  cut_message("%s", errMsg.c_str()));
}
#define assertValidateJSMethodName(N,E) \
cut_trace(_assertValidateJSMethodName(N,E))

static void _assertGetExtension(const string &path, const string &expected)
{
	string actualExt = Utils::getExtension(path);
	cppcut_assert_equal(expected, actualExt);
}
#define assertGetExtension(P, E) cut_trace(_assertGetExtension(P, E))

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	releaseDefaultContext();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getCurrTimeAsMicroSecond(void)
{
	const size_t allowedErrorInMicroSec = 100 * 1000; // 100 ms
	struct timeval tv;
	cppcut_assert_equal(0, gettimeofday(&tv, NULL),
	                    cut_message("errno: %d", errno));
	uint64_t currTime = Utils::getCurrTimeAsMicroSecond();
	uint64_t timeError = currTime;
	timeError -= tv.tv_sec * 1000 * 1000;
	timeError -= tv.tv_usec;
	cppcut_assert_equal(true, timeError < allowedErrorInMicroSec);
	if (isVerboseMode())
		cut_notify("timeError: %" PRIu64 " [us]", timeError);
}

void test_validateJSMethodName(void)
{
	assertValidateJSMethodName("IYHoooo_21", true);
}

void test_validateJSMethodNameEmpty(void)
{
	assertValidateJSMethodName("", false);
}

void test_validateJSMethodNameFromNumber(void)
{
	assertValidateJSMethodName("1foo", false);
}

void test_validateJSMethodNameWithSpace(void)
{
	assertValidateJSMethodName("o o", false);
}

void test_validateJSMethodNameWithDot(void)
{
	assertValidateJSMethodName("o.o", false);
}

void test_validateJSMethodNameWithExclamation(void)
{
	assertValidateJSMethodName("o!o", false);
}

void test_getExtension(void)
{
	string extension = "json";
	string path = "/hoge/foo/gator." + extension;
	assertGetExtension(path, extension);
}

void test_getExtensionLastDot(void)
{
	assertGetExtension("/hoge/foo/gator.", "");
}

void test_getExtensionNoDot(void)
{
	assertGetExtension("/hoge/foo/gator", "");
}

void test_getExtensionRoot(void)
{
	assertGetExtension("/", "");
}

void test_getExtensionNull(void)
{
	assertGetExtension("", "");
}

void test_getSelfExeDir(void)
{
	string actual = Utils::getSelfExeDir();
	string selfPath = executeCommand(
	  StringUtils::sprintf("readlink /proc/%d/exe", getpid()));
	string basename =
	   executeCommand(StringUtils::sprintf("basename `%s`",
	                                       selfPath.c_str()));
	static const size_t SIZE_DIR_SEPARATOR = 1;
	size_t expectLen =
	   selfPath.size() - basename.size() - SIZE_DIR_SEPARATOR;
	string expect(selfPath, 0, expectLen);
	cppcut_assert_equal(expect, actual);
}

void test_executeOnGlibEventLoop(void)
{
	acquireDefaultContext();
	TestExecEvtLoop thread;
	thread.start();
	g_main_loop_run(thread.loop);
	thread.mutex.lock(); // wait for mainThread() to exit.

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

void test_executeOnGlibEventLoopCalledFromSameContext(void)
{
	acquireDefaultContext();
	TestExecEvtLoop task;
	task.quitLoop = false;
	task.mainThread(NULL);
	cppcut_assert_equal(Utils::getThreadId(), task.threadId);
	cppcut_assert_equal(Utils::getThreadId(), task.eventLoopThreadId);
}

void test_executeOnGlibEventLoopForFunctor(void)
{
	acquireDefaultContext();
	TestExecEvtLoop thread;
	thread.useFunctor = true;
	thread.start();
	g_main_loop_run(thread.loop);
	thread.mutex.lock(); // wait for mainThread() to exit.

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

void test_executeOnGlibEventLoopFromSameContextForFunctor(void)
{
	acquireDefaultContext();
	TestExecEvtLoop task;
	task.useFunctor = true;
	task.quitLoop = false;
	task.mainThread(NULL);
	cppcut_assert_equal(Utils::getThreadId(), task.threadId);
	cppcut_assert_equal(Utils::getThreadId(), task.eventLoopThreadId);
}

void test_executeOnGlibEventLoopAsync(void)
{
	struct {
		static gboolean expired(gpointer data)
		{
			cut_fail("Timer expired.");
			return G_SOURCE_REMOVE;
		}
	} proc;

	acquireDefaultContext();

	TestExecEvtLoop thread;
	thread.start();
	thread.syncType = ASYNC;
	const size_t timeoutInMSec = 5000;
	guint timer_tag = g_timeout_add(timeoutInMSec, proc.expired, NULL);
	g_main_loop_run(thread.loop);
	g_source_remove(timer_tag);
	thread.mutex.lock(); // wait for mainThread() to exit.

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

void test_getUsingPortInformation(void)
{
	const int port = 12345;
	SoupServer *soupSv = soup_server_new(SOUP_SERVER_PORT, port, NULL);
	string info = Utils::getUsingPortInfo(port);
	g_object_unref(soupSv);
	MLPL_INFO("%s", info.c_str());

	StringVector lines;
	StringUtils::split(lines, info, '\n');
	cppcut_assert_equal(true, lines.size() >= 3);
	cppcut_assert_equal(
	  lines[0],
	  StringUtils::sprintf("Self PID: %d, exit status: 0", getpid()));
}

void test_removeGSource(void)
{
	struct Dummy {
		static gboolean func(gpointer data)
		{
			return G_SOURCE_REMOVE;
		}
	};
	const guint tag = g_idle_add(Dummy::func, NULL);
	cppcut_assert_equal(true, Utils::removeGSourceIfNeeded(tag));

	// Is absence of the event with tag + 1 guaranteed ?
	cppcut_assert_equal(false, Utils::removeGSourceIfNeeded(tag+1));
}

void test_removeGSourceWithInvalidTag(void)
{
	cppcut_assert_equal(true,
	                    Utils::removeGSourceIfNeeded(INVALID_EVENT_ID));
}

void test_watchFdInGLibMainLoop(void)
{
	struct Ctx {
		bool timedout;
		bool posted;
		bool called;
		TestExecEvtLoop evtloop;
		EventSemaphore sem;
		guint idleTag;
		guint timeoutTag;

		Ctx(void)
		: timedout(false),
		  posted(false),
		  called(false),
		  sem(0),
		  idleTag(INVALID_EVENT_ID),
		  timeoutTag(INVALID_EVENT_ID)
		{
		}

		static gboolean post(gpointer data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			obj->sem.post();
			obj->posted = true;
			return G_SOURCE_REMOVE;
		}

		static gboolean expired(gpointer data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			obj->timedout = true;
			return G_SOURCE_REMOVE;
		}

		static gboolean fdEvent(gpointer data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			obj->called = true;
			g_main_loop_quit(obj->evtloop.loop);
			return G_SOURCE_REMOVE;
		}
	} ctx;

	Utils::watchFdInGLibMainLoop(
	  ctx.sem.getEventFd(), G_IO_IN|G_IO_HUP|G_IO_ERR, ctx.fdEvent, &ctx);

	// callback for post()
	ctx.idleTag = g_idle_add(ctx.post, &ctx);

	// timer for checking timeout.
	const size_t timeoutInMSec = 5000;
	ctx.timeoutTag = g_timeout_add(timeoutInMSec, ctx.expired, &ctx);

	// run
	g_main_loop_run(ctx.evtloop.loop);
	cppcut_assert_equal(false, ctx.timedout);
	g_source_remove(ctx.timeoutTag);
	cppcut_assert_equal(true, ctx.posted);
	cppcut_assert_equal(true, ctx.called);
	cppcut_assert_equal(0, ctx.sem.wait());
}

} // namespace testUtils
