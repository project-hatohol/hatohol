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

#include <stdio.h>
#include <stdlib.h>
#include "Logger.h"
using namespace mlpl;

#include <string>
using namespace std;

#include "loggerTester.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "argc < 2\n");
		return EXIT_FAILURE;
	}
	string level = argv[1];
	if (level == "DBG")
		MLPL_DBG("%s\n", testString);
	else if (level == "INFO")
		MLPL_INFO("%s\n", testString);
	else if (level == "WARN")
		MLPL_WARN("%s\n", testString);
	else if (level == "ERR")
		MLPL_ERR("%s\n", testString);
	else if (level == "CRIT")
		MLPL_CRIT("%s\n", testString);
	else if (level == "BUG")
		MLPL_BUG("%s\n", testString);
	else {
		fprintf(stderr, "unknown level: %s\n", level.c_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
