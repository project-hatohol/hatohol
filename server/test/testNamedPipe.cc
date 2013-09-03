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

struct TestContext;
struct ExpectedCbArg {
	// set by a caller
	TestContext *ctx;

	// set by a callback handler
	GIOCondition condition;

	ExpectedCbArg(TestContext *_ctx)
	: ctx(_ctx)
	{
	}
};

struct TestCallback {
	string name;
	TestContext *ctx;

	GIOFunc cb; // custom
	void   *cbArg;

	// methods;
	TestCallback(const string &_name, TestContext *_ctx);
	void init(NamedPipe &namedPipe, const string &pipeName);
	void setCb(GIOFunc _cb, void *_cbArg);
	gboolean defaultCb(GIOChannel *source, GIOCondition condition);
	static gboolean callbackGate
	  (GIOChannel *source, GIOCondition condition, gpointer data);
};

struct TestContext {

	string name;
	NamedPipe pipeMasterRd, pipeMasterWr, pipeSlaveRd, pipeSlaveWr;
	GMainLoop *loop;
	gint timerId;
	size_t bufLen;
	bool hasError;
	
	TestCallback masterRdCb, masterWrCb, slaveRdCb, slaveWrCb;
	size_t numExpectedCbCalled;
	size_t pullOnPullTimes;

	bool timeoutTestPass;

	// methods;
	TestContext(const string &_name);
	virtual ~TestContext();
	void init(void);

	static gboolean expectedCb
	  (GIOChannel *source, GIOCondition condition, gpointer data);
	static gboolean timerHandler(gpointer data);
};

//
// Methods of TestCallback
//
TestCallback::TestCallback(const string &_name, TestContext *_ctx)
: name(_name),
  ctx(_ctx),
  cb(NULL),
  cbArg(NULL)
{
}

void TestCallback::init(NamedPipe &namedPipe, const string &pipeName)
{
	cppcut_assert_equal(true, namedPipe.init(pipeName, callbackGate, this));
}

void TestCallback::setCb(GIOFunc _cb, void *_cbArg)
{
	cb = _cb;
	cbArg = _cbArg;
}

gboolean TestCallback::defaultCb(GIOChannel *source, GIOCondition condition)
{
	cut_notify("Unexpectedly called: %s\n", name.c_str());
	g_main_loop_quit(ctx->loop);
	ctx->hasError = true;
	return FALSE;
}

gboolean TestCallback::callbackGate
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	TestCallback *obj = static_cast<TestCallback *>(data);
	if (obj->cb)
		return (*obj->cb)(source, condition, obj->cbArg);
	cut_notify("Unexpectedly called: %s\n", __PRETTY_FUNCTION__);
	return obj->defaultCb(source, condition);
}


//
// Methods of TestContext
//
TestContext::TestContext(const string &_name)
: name(_name),
  pipeMasterRd(NamedPipe::END_TYPE_MASTER_READ),
  pipeMasterWr(NamedPipe::END_TYPE_MASTER_WRITE),
  pipeSlaveRd(NamedPipe::END_TYPE_SLAVE_READ),
  pipeSlaveWr(NamedPipe::END_TYPE_SLAVE_WRITE),
  loop(NULL),
  timerId(INVALID_RESOURCE_ID),
  bufLen(0),
  hasError(false),
  masterRdCb("MasterRD", this),
  masterWrCb("MasterWR", this),
  slaveRdCb("SlaveRD", this),
  slaveWrCb("SlaveWR", this),
  numExpectedCbCalled(0),
  pullOnPullTimes(0),
  timeoutTestPass(false)
{
}

TestContext::~TestContext()
{
	if (loop)
		g_main_loop_unref(loop);
	if (timerId != INVALID_RESOURCE_ID)
		g_source_remove(timerId);
}

void TestContext::init(void)
{
	loop = g_main_loop_new(NULL, TRUE);
	cppcut_assert_not_null(loop);

	masterRdCb.init(pipeMasterRd, name);
	masterWrCb.init(pipeMasterWr, name);
	slaveRdCb.init(pipeSlaveRd, name);
	slaveWrCb.init(pipeSlaveWr, name);

	// set timeout
	static const guint timeout = 5 * 1000; // ms
	timerId = g_timeout_add(timeout, timerHandler, this);
}

gboolean TestContext::expectedCb
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	ExpectedCbArg *arg = static_cast<ExpectedCbArg *>(data);
	TestContext *ctx = arg->ctx;
	ctx->numExpectedCbCalled++;
	arg->condition = condition;
	g_main_loop_quit(ctx->loop);
	return FALSE;
}

gboolean TestContext::timerHandler(gpointer data)
{
	TestContext *ctx = static_cast<TestContext *>(data);
	ctx->timerId = INVALID_RESOURCE_ID;
	g_main_loop_quit(ctx->loop);
	return FALSE;
}

//
// General variables and methods
//
static TestContext *g_testPushCtx = NULL;

static void pullCb(GIOStatus stat, SmartBuffer &buf, size_t size, void *priv)
{
	TestContext *ctx = static_cast<TestContext *>(priv);
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	cppcut_assert_equal(ctx->bufLen, size);
	cppcut_assert_equal((size_t)0, buf.index());
	uint8_t *ptr = buf.getPointer<uint8_t>();
	for (size_t i = 0; i < size; i++)
		cppcut_assert_equal((uint8_t)i, ptr[i]);
	g_main_loop_quit(ctx->loop);
}

static void pullOnPullCb
  (GIOStatus stat, SmartBuffer &buf, size_t size, void *priv)
{
	TestContext *ctx = static_cast<TestContext *>(priv);
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	cppcut_assert_equal((size_t)1, size);
	cppcut_assert_equal((size_t)0, buf.index());
	uint8_t val = buf.getValue<uint8_t>();
	cppcut_assert_equal((uint8_t)ctx->pullOnPullTimes, val);
	ctx->pullOnPullTimes++;
	if (ctx->pullOnPullTimes < ctx->bufLen)
		ctx->pipeMasterRd.pull(1, pullOnPullCb, ctx);
	else
		g_main_loop_quit(ctx->loop);
}

static void _assertRun(TestContext *ctx)
{
	g_main_loop_run(ctx->loop);
	cppcut_assert_not_equal(INVALID_RESOURCE_ID, ctx->timerId,
	                        cut_message("Timed out."));
	cppcut_assert_equal(false, ctx->hasError);
}
#define assertRun(CTX) cut_trace(_assertRun(CTX))

static void _assertCloseUnexpectedly(const string &name, bool doPull)
{
	g_testPushCtx = new TestContext("test_close_in_pull");
	TestContext *ctx = g_testPushCtx;
	ctx->init();

	// set callback handlers
	ExpectedCbArg masterRdCbArg(ctx);
	ExpectedCbArg slaveWrCbArg(ctx);
	ctx->masterRdCb.setCb(TestContext::expectedCb, &masterRdCbArg);
	ctx->slaveWrCb.setCb(TestContext::expectedCb, &slaveWrCbArg);

	if (doPull) {
		// register read callback
		ctx->bufLen = 10;
		ctx->pipeMasterRd.pull(ctx->bufLen, pullCb, ctx);
	}

	// close the write pipe
	int fd = ctx->pipeSlaveWr.getFd();
	close(fd);

	// run the event loop 2 times
	while (ctx->numExpectedCbCalled < 2)
		assertRun(ctx);

	// check the reason of the callback
	cppcut_assert_equal(G_IO_HUP, masterRdCbArg.condition);
	cppcut_assert_equal(G_IO_NVAL, slaveWrCbArg.condition);
}
#define assertCloseUnexpectedly(NAME, DO_PULL) \
cut_trace(_assertCloseUnexpectedly(NAME, DO_PULL))

static void pushData(TestContext *ctx)
{
	SmartBuffer smbuf(ctx->bufLen);
	for (size_t i = 0; i < ctx->bufLen; i++)
		smbuf.add8(i);
	ctx->pipeSlaveWr.push(smbuf);
}

static void pullData(TestContext *ctx)
{
	ctx->pipeMasterRd.pull(ctx->bufLen, pullCb, ctx);
}

static void timeoutTestCb(NamedPipe *namedPipe, void *data)
{
	TestContext *ctx = static_cast<TestContext *>(data);
	ctx->timeoutTestPass = true;
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
void test_pullPush(void)
{
	g_testPushCtx = new TestContext("test_push");
	TestContext *ctx = g_testPushCtx;
	ctx->init();

	ctx->bufLen = 10;
	pullData(ctx);
	pushData(ctx);

	// run the event loop
	assertRun(ctx);
}

void test_pushPull(void)
{
	g_testPushCtx = new TestContext("test_push");
	TestContext *ctx = g_testPushCtx;
	ctx->init();

	ctx->bufLen = 10;
	pushData(ctx);
	pullData(ctx);

	// run the event loop
	assertRun(ctx);
}

void test_pushPullOnPull(void)
{
	g_testPushCtx = new TestContext("test_pushPullOnPull");
	TestContext *ctx = g_testPushCtx;
	ctx->init();

	ctx->bufLen = 10;
	pushData(ctx);
	ctx->pipeMasterRd.pull(1, pullOnPullCb, ctx);

	// run the event loop
	assertRun(ctx);
}

void test_closeUnexpectedly(void)
{
	bool doPull = false;
	assertCloseUnexpectedly("test_close", doPull);
}

void test_closeUnexpectedlyInPull(void)
{
	bool doPull = true;
	assertCloseUnexpectedly("test_close_in_pull", doPull);
}

void test_timeoutPull(void)
{
	g_testPushCtx = new TestContext("test_timeout");
	TestContext *ctx = g_testPushCtx;
	ctx->init();
	ctx->bufLen = 10;
	ctx->pipeMasterRd.setPullTimeout(100, timeoutTestCb, ctx);
	pullData(ctx);
	assertRun(ctx);
	cppcut_assert_equal(true, ctx->timeoutTestPass);
}

void test_timeoutPullNotFire(void)
{
	g_testPushCtx = new TestContext("test_timeout");
	TestContext *ctx = g_testPushCtx;
	ctx->init();
	ctx->bufLen = 10;
	ctx->pipeMasterRd.setPullTimeout(1000, timeoutTestCb, ctx);
	pullData(ctx);
	pushData(ctx);
	assertRun(ctx);
	cppcut_assert_equal(false, ctx->timeoutTestPass);
}

void test_timeoutPush(void)
{
	g_testPushCtx = new TestContext("test_timeout");
	TestContext *ctx = g_testPushCtx;
	ctx->init();
	// We have to write data with a size that blocks the write operation.
	// Ref. Approx. 64kB is written (buffered) on Ubuntu 13.04 (64bit).
	ctx->bufLen = 1024 * 1024;
	ctx->pipeSlaveWr.setPushTimeout(100, timeoutTestCb, ctx);
	pushData(ctx);
	assertRun(ctx);
	cppcut_assert_equal(true, ctx->timeoutTestPass);
}

} // namespace testNamedPipe
