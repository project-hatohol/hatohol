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
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_commandlineArgDefault(void)
{
	int argc = 0;
	char *argv[argc];
	TestHapProcess hapProc(argc, argv);
	const HapCommandLineArg &cmdLineArg = hapProc.callGetCommandLineArg();
	cut_assert_equal_string(NULL, cmdLineArg.brokerUrl);
	cut_assert_equal_string(NULL, cmdLineArg.queueAddress);
}

} // namespace testHapProcess
