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
#include <cppcutter.h>
#include "NamedPipe.h"
#include "SmartBuffer.h"
#include "Logger.h"
using namespace std;
using namespace mlpl;

namespace testNamedPipe {

static const gint INVALID_RESOURCE_ID = -1;

struct TestPushContext {

	NamedPipe pmrd, pmwr, psrd, pswr;
	GMainLoop *loop;
	gint timerId;
	size_t bufLen;
	bool hasError;

	TestPushContext(void)
	: pmrd(NamedPipe::END_TYPE_MASTER_READ),
	  pmwr(NamedPipe::END_TYPE_MASTER_WRITE),
	  psrd(NamedPipe::END_TYPE_SLAVE_READ),
	  pswr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  loop(NULL),
	  timerId(INVALID_RESOURCE_ID),
	  bufLen(0),
	  hasError(false)
	{
		loop = g_main_loop_new(NULL, TRUE);
	}

	virtual ~TestPushContext()
	{
		if (loop)
			g_main_loop_unref(loop);
		if (timerId != INVALID_RESOURCE_ID)
			g_source_remove(timerId);
	}
};
static TestPushContext *g_testPushCtx = NULL;

static gboolean
pushMasterRdCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	cppcut_assert_equal(condition, G_IO_IN);
	MLPL_BUG("TODO: assert data content: %s (%p)\n",
	         __PRETTY_FUNCTION__, ctx);
	ctx->hasError = true;
	g_main_loop_quit(ctx->loop);
	return FALSE;
}

static gboolean
pushMasterWrCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	cut_notify("Unexpectedly called: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	g_main_loop_quit(ctx->loop);
	ctx->hasError = true;
	return FALSE;
}

static gboolean
pushSlaveRdCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	cut_notify("Unexpectedly called: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	g_main_loop_quit(ctx->loop);
	ctx->hasError = true;
	return FALSE;
}

static gboolean
pushSlaveWrCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	cut_notify("Unexpectedly called: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	g_main_loop_quit(ctx->loop);
	ctx->hasError = true;
	return FALSE;
}

static gboolean timerHandler(gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	ctx->timerId = INVALID_RESOURCE_ID;
	g_main_loop_quit(ctx->loop);
	return FALSE;
}

void cut_teardown(void)
{
	if (g_testPushCtx) {
		delete g_testPushCtx;
		g_testPushCtx = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_push(void)
{
	g_testPushCtx = new TestPushContext();
	TestPushContext *ctx = g_testPushCtx;

	string name = "test_push";
	cppcut_assert_equal(true, ctx->pmrd.openPipe(name));
	cppcut_assert_equal(true, ctx->pmwr.openPipe(name));
	cppcut_assert_equal(true, ctx->psrd.openPipe(name));
	cppcut_assert_equal(true, ctx->pswr.openPipe(name));

	cppcut_assert_equal(
	  true, ctx->pmrd.createGIOChannel(pushMasterRdCb, ctx));
	cppcut_assert_equal(
	  true, ctx->pmwr.createGIOChannel(pushMasterWrCb, ctx));
	cppcut_assert_equal(
	  true, ctx->psrd.createGIOChannel(pushSlaveRdCb, ctx));
	cppcut_assert_equal(
	  true, ctx->pswr.createGIOChannel(pushSlaveWrCb, ctx));

	static const guint timeout = 5 * 1000; // ms
	ctx->bufLen = 10;
	SmartBuffer smbuf(ctx->bufLen);
	for (size_t i = 0; i < ctx->bufLen; i++)
		smbuf.add8(i);
	ctx->pswr.push(smbuf);
	ctx->timerId = g_timeout_add(timeout, timerHandler, ctx);
	g_main_loop_run(ctx->loop);
	cppcut_assert_not_equal(INVALID_RESOURCE_ID, ctx->timerId,
	                        cut_message("Timed out."));
	cppcut_assert_equal(false, ctx->hasError);
}

} // namespace testNamedPipe
