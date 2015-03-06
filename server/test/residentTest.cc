/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include <unistd.h>
#include "residentTest.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
#include "NamedPipe.h"
#include "Helpers.h"
#include "Utils.h"
#include "Logger.h"
#include "StringUtils.h"
using namespace std;
using namespace mlpl;

struct Context : public ResidentPullHelper<Context> {
	string pipename;
	NamedPipe pipeRd, pipeWr;
	size_t countNotified;
	ResidentNotifyEventArg notifyEvent;
	bool crashNotifyEvent;
	bool blockReplyNotfiyEvent;
	bool sendEventInfo;

	Context(void)
	: pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  countNotified(0),
	  crashNotifyEvent(false),
	  blockReplyNotfiyEvent(false),
	  sendEventInfo(false)
	{
		memset(&notifyEvent, 0, sizeof(notifyEvent));
	}

	static gboolean pipeErrCb(GIOChannel *source,
	                          GIOCondition condition, gpointer data)
	{
		MLPL_ERR("pipeError: %s\n",
		         Utils::getStringFromGIOCondition(condition).c_str());
		return FALSE;
	}

	bool initPipes(void)
	{
		if (!pipeRd.init(pipename, pipeErrCb, this))
			return false;
		if (!pipeWr.init(pipename, pipeErrCb, this))
			return false;
		initResidentPullHelper(&pipeRd, this);
		return true;
	}
};

static void sendEventInfo(Context *ctx)
{
	ResidentCommunicator comm;
	comm.setHeader(RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN,
	               RESIDENT_TEST_REPLY_GET_EVENT_INFO);
	SmartBuffer &wrBuf = comm.getBuffer();
	memcpy(wrBuf.getPointer<void>(), &ctx->notifyEvent,
	       RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN);
	wrBuf.incIndex(RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN);
	comm.push(ctx->pipeWr); 
}

static void cmdCbGetEventInfo(SmartBuffer &sbuf, size_t size, Context *ctx)
{
	if (ctx->countNotified > 0) {
		sendEventInfo(ctx);
		return;
	}
	ctx->sendEventInfo = true;
}

static void testCmdCb(GIOStatus stat, SmartBuffer &sbuf, 
                      size_t size, Context *ctx)
{
	ResidentTestCmdType cmdType = static_cast<ResidentTestCmdType>(
	   ResidentCommunicator::getPacketType(sbuf)); 
	if (cmdType == RESIDENT_TEST_CMD_GET_EVENT_INFO) {
		cmdCbGetEventInfo(sbuf, size, ctx);
	} else if (cmdType == RESIDENT_TEST_CMD_UNBLOCK_REPLY_NOTIFY_EVENT) {
		ctx->blockReplyNotfiyEvent = false;
	} else {
		MLPL_ERR("Unknown expected cmd: %d\n", cmdType);
		exit(EXIT_FAILURE);
	}
	ctx->pullHeader(testCmdCb);
}

static void stall(void)
{
	while (true)
		sleep(1);
}

// --------------------------------------------------------------------------
// Module handlers
// --------------------------------------------------------------------------
Context ctx;

static uint32_t init(const char *arg)
{
	// parse arg
	StringVector argVect;
	StringUtils::split(argVect, arg, ' ');
	for (size_t i = 0; i < argVect.size(); i++) {
		const string &str = argVect[i];
		if (str == "--pipename") {
			if (i == argVect.size() - 1) {
				MLPL_BUG("No argument for --pipename");
				return RESIDENT_MOD_INIT_ERROR;
			}
			i++;
			ctx.pipename = argVect[i];
		} else if (str == "--crash-init") {
			crash();
		} else if (str == "--crash-notify-event") {
			ctx.crashNotifyEvent = true;
		} else if (str == "--stall-init") {
			stall();
		} else if (str == "--block-reply-notify-event") {
			ctx.blockReplyNotfiyEvent = true;
		}
		
	}

	// init pipe if needed
	if (!ctx.pipename.empty()) {
		if (!ctx.initPipes())
			return RESIDENT_MOD_INIT_ERROR;
		ctx.pullHeader(testCmdCb);
	}

	return RESIDENT_MOD_INIT_OK;
}

static uint32_t notifyEvent(ResidentNotifyEventArg *arg)
{
	if (ctx.crashNotifyEvent)
		crash();
	ctx.notifyEvent = *arg;
	ctx.notifyEvent.hostIdInServer = TEST_HOST_ID_REPLY_MAGIC_CODE;
	ctx.notifyEvent.triggerId      = TEST_TRIGGER_ID_REPLY_MAGIC_CODE;
	ctx.countNotified++;
	if (ctx.sendEventInfo) {
		sendEventInfo(&ctx);
		ctx.sendEventInfo = false;
	}
	while (ctx.blockReplyNotfiyEvent)
		g_main_context_iteration(NULL, TRUE);
	return RESIDENT_MOD_NOTIFY_EVENT_ACK_OK;
}

struct ResidentModule RESIDENT_MODULE_SYMBOL = {
	RESIDENT_MODULE_VERSION, /* moduleVersion */
	init,                    // init
	notifyEvent,             // notifyEvent 
};

