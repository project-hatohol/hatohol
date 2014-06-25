/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cppcutter.h>
#include "Helpers.h"
#include "HapProcess.h"
#include "HatoholArmPluginInterface.h"

namespace testHapProcess {

class TestHapProcess : public HapProcess {
public:
	TestHapProcess(int argc, char *argv[])
	: HapProcess(argc, argv)
	{
	}

	const HapCommandLineArg &callGetCommandLineArg(void) const
	{
		return getCommandLineArg();
	}

	const GError *callGetErrorOfCommandLineArg(void) const
	{
		return getErrorOfCommandLineArg();
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_commandlineArgDefault(void)
{
	int argc = 1;
	char *argv[argc];
	TestHapProcess hapProc(argc, argv);
	assertGError(hapProc.callGetErrorOfCommandLineArg());
	const HapCommandLineArg &cmdLineArg = hapProc.callGetCommandLineArg();
	cut_assert_equal_string(NULL, cmdLineArg.brokerUrl);
	cut_assert_equal_string(NULL, cmdLineArg.queueAddress);
}

void test_commandlineArgBrokerUrl(void)
{
	const char *argv[] = {"abc", "--broker-url=example.com:52211"};
	int argc = sizeof(argv) / sizeof(char *);
	TestHapProcess hapProc(argc, (char**)argv);
	assertGError(hapProc.callGetErrorOfCommandLineArg());
	const HapCommandLineArg &cmdLineArg = hapProc.callGetCommandLineArg();
	cut_assert_equal_string("example.com:52211", cmdLineArg.brokerUrl);
}

} // namespace testHapProcess
