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

static gboolean
pushMasterRdCb(GIOChannel *source, GIOCondition condition, gpointer data);
static gboolean
pushMasterWrCb(GIOChannel *source, GIOCondition condition, gpointer data);
static gboolean
pushSlaveRdCb(GIOChannel *source, GIOCondition condition, gpointer data);
static gboolean
pushSlaveWrCb(GIOChannel *source, GIOCondition condition, gpointer data);

struct TestPushContext {

	string name;
	NamedPipe pipeMasterRd, pipeMasterWr, pipeSlaveRd, pipeSlaveWr;
	GMainLoop *loop;
	gint timerId;
	size_t bufLen;
	bool hasError;

	TestPushContext(const string &_name)
	: name(_name),
	  pipeMasterRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeMasterWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  pipeSlaveRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeSlaveWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  loop(NULL),
	  timerId(INVALID_RESOURCE_ID),
	  bufLen(0),
	  hasError(false)
	{

	}

	virtual ~TestPushContext()
	{
		if (loop)
			g_main_loop_unref(loop);
		if (timerId != INVALID_RESOURCE_ID)
			g_source_remove(timerId);
	}

	void init(void)
	{
		loop = g_main_loop_new(NULL, TRUE);
		cppcut_assert_not_null(loop);
		cppcut_assert_equal(true, pipeMasterRd.openPipe(name));
		cppcut_assert_equal(true, pipeMasterWr.openPipe(name));
		cppcut_assert_equal(true, pipeSlaveRd.openPipe(name));
		cppcut_assert_equal(true, pipeSlaveWr.openPipe(name));

		cppcut_assert_equal(
		  true, pipeMasterRd.createGIOChannel(pushMasterRdCb, this));
		cppcut_assert_equal(
		  true, pipeMasterWr.createGIOChannel(pushMasterWrCb, this));
		cppcut_assert_equal(
		  true, pipeSlaveRd.createGIOChannel(pushSlaveRdCb, this));
		cppcut_assert_equal(
		  true, pipeSlaveWr.createGIOChannel(pushSlaveWrCb, this));
	}
};
static TestPushContext *g_testPushCtx = NULL;

static gboolean
pushMasterRdCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(data);
	cut_notify("Unexpectedly called: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	g_main_loop_quit(ctx->loop);
	ctx->hasError = true;
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

static void pullCb(GIOStatus stat, SmartBuffer &buf, size_t size, void *priv)
{
	TestPushContext *ctx = static_cast<TestPushContext *>(priv);
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	cppcut_assert_equal(ctx->bufLen, size);
	buf.resetIndex();
	uint8_t *ptr = buf.getPointer<uint8_t>();
	for (size_t i = 0; i < size; i++)
		cppcut_assert_equal((uint8_t)i, ptr[i]);
	g_main_loop_quit(ctx->loop);
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
	string name = "test_push";
	g_testPushCtx = new TestPushContext(name);
	TestPushContext *ctx = g_testPushCtx;
	ctx->init();

	// register read callback
	ctx->bufLen = 10;
	ctx->pipeMasterRd.pull(ctx->bufLen, pullCb, ctx);

	// send data
	SmartBuffer smbuf(ctx->bufLen);
	for (size_t i = 0; i < ctx->bufLen; i++)
		smbuf.add8(i);
	ctx->pipeSlaveWr.push(smbuf);

	// set timeout
	static const guint timeout = 5 * 1000; // ms
	ctx->timerId = g_timeout_add(timeout, timerHandler, ctx);

	// run the event loop
	g_main_loop_run(ctx->loop);
	cppcut_assert_not_equal(INVALID_RESOURCE_ID, ctx->timerId,
	                        cut_message("Timed out."));
	cppcut_assert_equal(false, ctx->hasError);
}

} // namespace testNamedPipe
