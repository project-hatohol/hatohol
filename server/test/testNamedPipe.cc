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
#include <unistd.h>
#include <cppcutter.h>
#include "NamedPipe.h"
#include "SmartBuffer.h"
#include "Logger.h"
using namespace std;
using namespace mlpl;

namespace testNamedPipe {

static const gint INVALID_RESOURCE_ID = -1;

struct TestPushContext {

	string name;
	NamedPipe pipeMasterRd, pipeMasterWr, pipeSlaveRd, pipeSlaveWr;
	GMainLoop *loop;
	gint timerId;
	size_t bufLen;
	bool hasError;
	GIOFunc masterRdCb, masterWrCb, slaveRdCb, slaveWrCb;
	size_t numExpectedCbCalled;

	TestPushContext(const string &_name)
	: name(_name),
	  pipeMasterRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeMasterWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  pipeSlaveRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeSlaveWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  loop(NULL),
	  timerId(INVALID_RESOURCE_ID),
	  bufLen(0),
	  hasError(false),
	  masterRdCb(NULL),
	  masterWrCb(NULL),
	  slaveRdCb(NULL),
	  slaveWrCb(NULL),
	  numExpectedCbCalled(0)
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
		  true, pipeMasterRd.createGIOChannel(defaultMasterRdCb, this));
		cppcut_assert_equal(
		  true, pipeMasterWr.createGIOChannel(defaultMasterWrCb, this));
		cppcut_assert_equal(
		  true, pipeSlaveRd.createGIOChannel(defaultSlaveRdCb, this));
		cppcut_assert_equal(
		  true, pipeSlaveWr.createGIOChannel(defaultSlaveWrCb, this));

		// set timeout
		static const guint timeout = 5 * 1000; // ms
		timerId = g_timeout_add(timeout, timerHandler, this);
	}

	static gboolean defaultCb
	  (GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		g_main_loop_quit(ctx->loop);
		ctx->hasError = true;
		return FALSE;
	}

	static gboolean defaultMasterRdCb
	  (GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		if (ctx->masterRdCb)
			return (*ctx->masterRdCb)(source, condition, data);
		cut_notify("Unexpectedly called: %s\n", __PRETTY_FUNCTION__);
		return defaultCb(source, condition, data);
	}

	static gboolean defaultMasterWrCb
	  (GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		if (ctx->masterWrCb)
			return (*ctx->masterWrCb)(source, condition, data);
		cut_notify("Unexpectedly called: %s\n", __PRETTY_FUNCTION__);
		return defaultCb(source, condition, data);
	}

	static gboolean defaultSlaveRdCb
	  (GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		if (ctx->slaveRdCb)
			return (*ctx->slaveRdCb)(source, condition, data);
		cut_notify("Unexpectedly called: %s\n", __PRETTY_FUNCTION__);
		return defaultCb(source, condition, data);
	}

	static gboolean defaultSlaveWrCb
	  (GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		if (ctx->slaveWrCb)
			return (*ctx->slaveWrCb)(source, condition, data);
		cut_notify("Unexpectedly called: %s\n", __PRETTY_FUNCTION__);
		return defaultCb(source, condition, data);
	}

	static gboolean
	expectedCb(GIOChannel *source, GIOCondition condition, gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		ctx->numExpectedCbCalled++;
		g_main_loop_quit(ctx->loop);
		return FALSE;
	}

	static gboolean timerHandler(gpointer data)
	{
		TestPushContext *ctx = static_cast<TestPushContext *>(data);
		ctx->timerId = INVALID_RESOURCE_ID;
		g_main_loop_quit(ctx->loop);
		return FALSE;
	}
};
static TestPushContext *g_testPushCtx = NULL;

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
void test_pushPull(void)
{
	g_testPushCtx = new TestPushContext("test_push");
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

	// run the event loop
	g_main_loop_run(ctx->loop);
	cppcut_assert_not_equal(INVALID_RESOURCE_ID, ctx->timerId,
	                        cut_message("Timed out."));
	cppcut_assert_equal(false, ctx->hasError);
}

void test_closeUnexpectedly(void)
{
	g_testPushCtx = new TestPushContext("test_close");
	TestPushContext *ctx = g_testPushCtx;
	ctx->init();

	// set callback handlers
	ctx->masterRdCb = TestPushContext::expectedCb;
	ctx->slaveWrCb = TestPushContext::expectedCb;

	// close the write pipe
	int fd = ctx->pipeSlaveWr.getFd();
	close(fd);

	// run the event loop 2 times
	while (ctx->numExpectedCbCalled < 2) {
		g_main_loop_run(ctx->loop);
		cppcut_assert_not_equal(INVALID_RESOURCE_ID, ctx->timerId,
		                        cut_message("Timed out."));
		cppcut_assert_equal(false, ctx->hasError);
	}

	// TODO: check the error type.
}

} // namespace testNamedPipe
