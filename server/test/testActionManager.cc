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

#include <cppcutter.h>
#include <sys/types.h> 
#include "Hatohol.h"
#include "ActionManager.h"
#include "PipeUtils.h"

namespace testActionManagern {

class TestActionManager : public ActionManager
{
public:
	void callMakeExecArg(StringVector &vect, const string &command)
	{
		makeExecArg(vect, command);
	}

	void callExecCommandAction(const ActionDef &actionDef)
	{
		execCommandAction(actionDef);
	}
};

void setup(void)
{
	hatoholInit();
}

void teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_makeExecArg(void)
{
	string cmd = "ls";
	TestActionManager actMgr;
	StringVector argVect;
	actMgr.callMakeExecArg(argVect, "ls");
	cppcut_assert_equal((size_t)1, argVect.size());
	cppcut_assert_equal(cmd, argVect[0]);
}

void test_makeExecArgTwo(void)
{
	const char *args[] = {"ls", "-l"};
	const size_t numArgs = sizeof(args) / sizeof(const char *);
	TestActionManager actMgr;
	StringVector argVect;
	actMgr.callMakeExecArg(argVect, "ls");
	cppcut_assert_equal(numArgs, argVect.size());
	for (size_t i = 0; i < numArgs; i++)
		cppcut_assert_equal(string(args[i]), argVect[i]);
}

void test_execCommandAction(void)
{
	PipeUtils readPipe, writePipe;
	readPipe.makeFileInTmpAndOpenForRead();
	writePipe.makeFileInTmpAndOpenForWrite();

	ActionDef actDef;
	actDef.type = ACTION_COMMAND;
	actDef.command = StringUtils::sprintf(
	  "%s %s %s", cut_build_path("action-tp", NULL),
	  writePipe.getPath().c_str(), readPipe.getPath().c_str());

	TestActionManager actMgr;
	actMgr.callExecCommandAction(actDef);

	// connect to action-tp

	// check if the command is successfully executed
	DBClientAction dbAction;
}

} // namespace testActionManager
