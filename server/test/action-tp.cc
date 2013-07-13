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
#include "PipeUtils.h"

using namespace std;

static void printUsage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("  $ action-tp pipeFileForRead pipeFileForWrite\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	PipeUtils readPipe, writePipe;
	if (argc < 3) {
		printUsage();
		return EXIT_FAILURE;
	}

	string readPipePath = argv[1];
	if (!readPipe.openForRead(readPipePath))
		return EXIT_FAILURE;

	string writePipePath = argv[2];
	if (!writePipe.openForWrite(writePipePath))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
