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
#include "PipeUtils.h"
#include "Logger.h"
#include "SmartBuffer.h"

using namespace std;
using namespace mlpl;

static const int TIMEOUT_MSEC = 5 * 1000;
static vector<string> g_argList;

typedef bool (*CommandRunner)
  (PipeUtils &readPipe, PipeUtils &writePipe, int &retCode);

bool cmdBegin(PipeUtils &readPipe, PipeUtils &writePipe, int &retCode)
{
	MLPL_ERR("This function must not be called: %s\n", __PRETTY_FUNCTION__);
	return false;
}

bool cmdQuit(PipeUtils &readPipe, PipeUtils &writePipe, int &retCode)
{
	MLPL_INFO("Quit\n");
	ActionTpCommHeader header;
	header.length = sizeof(header);
	header.code   = ACTTP_CODE_QUIT;
	header.flags  = ACTTP_FLAGS_RES;
	writePipe.send(header.length, &header);
	retCode = EXIT_SUCCESS;
	return false; // return false to exit program.
}

bool cmdGetArgList(PipeUtils &readPipe, PipeUtils &writePipe, int &retCode)
{
	SmartBuffer pkt;

	// header
	ActionTpCommHeader header;
	// header.length is set later
	header.code  = ACTTP_CODE_GET_ARG_LIST;
	header.flags = ACTTP_FLAGS_RES;
	pkt.addEx(&header, sizeof(header));

	// number of args
	uint16_t numArgs = g_argList.size();
	pkt.addEx16(numArgs);

	// add argument strings
	for (size_t i = 0; i < numArgs; i++) {
		const string &arg = g_argList[i];

		// length of an argument (not including NULL terminator)
		uint16_t length = arg.size();
		pkt.addEx16(length);

		// add string itself.
		pkt.addEx(arg.c_str(), length);
	}

	// set the length in the header
	size_t totalLength = pkt.index();
	pkt.resetIndex();
	ActionTpCommHeader *headerPtr = pkt.getPointer<ActionTpCommHeader>();
	headerPtr->length = totalLength;

	// send packet
	return writePipe.send(totalLength, headerPtr);
}

static CommandRunner commands[] = {
	cmdBegin,
	cmdQuit,
	cmdGetArgList,
};
static const size_t NUM_COMMANDS = sizeof(commands) / sizeof(CommandRunner);

static void printUsage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("  $ ActionTp [--resident] pipeFileForRead pipeFileForWrite\n");
	printf("\n");
}

static bool dispatch(PipeUtils &readPipe, PipeUtils &writePipe, int &retCode)
{
	// get command
	ActionTpCommHeader header;
	if (!readPipe.recv(sizeof(header), &header, TIMEOUT_MSEC)) {
		return false;
	}

	if (header.code >= NUM_COMMANDS) {
		MLPL_ERR("Unexpected code: %d\n", header.code);
		return false;
	}

	return (*commands[header.code])(readPipe, writePipe, retCode);
}

int main(int argc, char *argv[])
{
	PipeUtils readPipe, writePipe;
	if (argc < 3) {
		printUsage();
		return EXIT_FAILURE;
	}
	int pipePathIdx = 1;
	for (int i = 1; i < argc; i++) {
		g_argList.push_back(argv[i]);
	}

	string readPipePath = argv[pipePathIdx++];
	if (!readPipe.openForRead(readPipePath))
		return EXIT_FAILURE;

	string writePipePath = argv[pipePathIdx];
	if (!writePipe.openForWrite(writePipePath))
		return EXIT_FAILURE;

	// send BEGIN PACKET
	// NOTE: Exceptionally this command is sent from here as response.
	ActionTpCommHeader header;
	header.length = sizeof(header);
	header.code = ACTTP_CODE_BEGIN;
	header.flags = ACTTP_FLAGS_RES;
	if (!writePipe.send(header.length, &header))
		return EXIT_FAILURE;

	// wait and execute commands
	int retCode = EXIT_FAILURE;
	while (dispatch(readPipe, writePipe, retCode))
		;

	return retCode;
}
