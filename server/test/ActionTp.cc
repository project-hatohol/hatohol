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
#include <cstdlib>
#include <string>
#include <vector>
#include "ActionTp.h"
#include "NamedPipe.h"
#include "Logger.h"
#include "SmartBuffer.h"
#include "ResidentCommunicator.h"

using namespace std;
using namespace mlpl;

static const int TIMEOUT_MSEC = 5 * 1000;
static vector<string> g_argList;

struct Context : public ResidentPullHelper<Context> {
	bool exitFlag;
	int  exitCode;
	NamedPipe pipeRd, pipeWr;

	Context(void)
	: exitFlag(false),
	  exitCode(EXIT_FAILURE),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE)
	{
	}

	static gboolean pipeErrCb(GIOChannel *source,
	                          GIOCondition condition, gpointer data)
	{
		Context *ctx = static_cast<Context *>(data);
		MLPL_ERR("pipeError: %s\n",
		         Utils::getStringFromGIOCondition(condition).c_str());
		ctx->exitFlag = true;
		return FALSE;
	}


	void initPipes(const string &name)
	{
		if (!pipeRd.init(name, pipeErrCb, this)) {
			exitFlag = true;
			return;
		}
		if (!pipeWr.init(name, pipeErrCb, this)) {
			exitFlag = true;
			return;
		}
		initResidentPullHelper(&pipeRd, this);
	}
};

static bool cmdQuit(SmartBuffer &sbuf, size_t size, Context *ctx)
{
	MLPL_INFO("Quit\n");
	ResidentCommunicator comm;
	comm.setHeader(0, ACTTP_REPLAY_QUIT);
	comm.push(ctx->pipeWr); 
	ctx->exitFlag = true;
	ctx->exitCode = EXIT_SUCCESS;
	return false;
}

static bool cmdGetArgList(SmartBuffer &sbuf, size_t size, Context *ctx)
{
	// make a temporary buffer to store arguemnts
	SmartBuffer buf;

	// number of args
	uint16_t numArgs = g_argList.size();
	buf.addEx16(numArgs);

	// add argument strings
	for (size_t i = 0; i < numArgs; i++) {
		const string &arg = g_argList[i];

		// length of an argument (not including NULL terminator)
		uint16_t length = arg.size();
		buf.addEx16(length);

		// add string itself.
		buf.addEx(arg.c_str(), length);
	}
	size_t bufLength = buf.index();
	size_t bodyLength = ACTTP_ARG_LIST_SIZE_LEN + buf.index();

	// make packet
	ResidentCommunicator comm;
	comm.setHeader(bodyLength, ACTTP_REPLY_GET_ARG_LIST);
	SmartBuffer &pkt = comm.getBuffer();
	pkt.resetIndex();
	pkt.incIndex(RESIDENT_PROTO_HEADER_LEN);

	// write body
	pkt.add16(bufLength);
	buf.resetIndex();
	pkt.add(buf.getPointer<void>(), bufLength);

	// send packet
	comm.push(ctx->pipeWr); 

	return true; // continue
}

static void printUsage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("  $ ActionTp pipeName\n");
	printf("\n");
}

static void dispatch(GIOStatus stat, SmartBuffer &sbuf,
                     size_t size, Context *ctx)
{
	bool continueFlag = false;
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType == ACTTP_CODE_GET_ARG_LIST) {
		continueFlag = cmdGetArgList(sbuf, size, ctx);
	} else if (pktType == ACTTP_CODE_QUIT) {
		continueFlag = cmdQuit(sbuf, size, ctx);
	} else  {
		MLPL_ERR("Unexpected code: %d\n", pktType);
		ctx->exitFlag = true;
	}
	if (continueFlag)
		ctx->pullHeader(dispatch);
}

int main(int argc, char *argv[])
{
	Context ctx;
	if (argc < 2) {
		printUsage();
		return EXIT_FAILURE;
	}
	int pipePathIdx = 1;
	for (int i = 1; i < argc; i++) {
		g_argList.push_back(argv[i]);
	}

	string pipeName = argv[pipePathIdx++];
	ctx.initPipes(pipeName);

	// send BEGIN PACKET
	// NOTE: Exceptionally this command is sent from here as response.
	ResidentCommunicator comm;
	comm.setHeader(0, ACTTP_CODE_BEGIN);
	comm.push(ctx.pipeWr); 

	// set callback
	ctx.pullHeader(dispatch);

	// wait and execute commands
	while (!ctx.exitFlag)
		g_main_iteration(TRUE);

	// handle remaining events
	while (g_main_iteration(TRUE))
		;

	return ctx.exitCode;
}
