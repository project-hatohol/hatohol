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

#include <cstring>
#include "residentTest.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
#include "NamedPipe.h"
#include "Utils.h"
#include "Logger.h"
#include "StringUtils.h"
using namespace mlpl;

struct Context : public ResidentPullHelper<Context> {
	string pipename;
	NamedPipe pipeRd, pipeWr;
	ResidentNotifyEventArg notifyEvent;
	bool crashNotifyEvent;

	Context(void)
	: pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  crashNotifyEvent(false)
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

static void cmdCbGetEventInfo(SmartBuffer &sbuf, size_t size, Context *ctx)
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

static void testCmdCb(GIOStatus stat, SmartBuffer &sbuf, 
                      size_t size, Context *ctx)
{
	ResidentTestCmdType cmdType = static_cast<ResidentTestCmdType>(
	   ResidentCommunicator::getPacketType(sbuf)); 
	if (cmdType == RESIDENT_TEST_CMD_GET_EVENT_INFO) {
		cmdCbGetEventInfo(sbuf, size, ctx);
	} else {
		MLPL_ERR("Unknown expected cmd: %d\n", cmdType);
		exit(EXIT_FAILURE);
	}
}

static void crash(void)
{
	char *p = NULL;
	*p = 'a';
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
	return RESIDENT_MOD_NOTIFY_EVENT_ACK_OK;
}

struct ResidentModule RESIDENT_MODULE_SYMBOL = {
	RESIDENT_MODULE_VERSION, /* moduleVersion */
	init,                    // init
	notifyEvent,             // notifyEvent 
};

