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
#include <stdarg.h>
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

static void _assertMakeExecArgs(const char *firstArg, ...)
{
	va_list ap;
	va_start(ap, firstArg);
	StringVector inArgVect;
	inArgVect.push_back(firstArg);
	while (true) {
		const char *word = va_arg(ap, const char *);
		if (!word)
			break;
		inArgVect.push_back(word);
	}

	string testCmd;
	for (size_t i = 0; i < inArgVect.size(); i++) {
		testCmd += inArgVect[i];
		testCmd += " ";
	}

	TestActionManager actMgr;
	StringVector argVect;
	actMgr.callMakeExecArg(argVect, testCmd);
	cppcut_assert_equal(inArgVect.size(), argVect.size());
	for (size_t i = 0; i < inArgVect.size(); i++) {
		cppcut_assert_equal(inArgVect[i], argVect[i],
		                    cut_message("index: %zd", i));
	}
	va_end(ap);
}
#define assertMakeExecArgs(FIRST, ...) \
cut_trace(_assertMakeExecArgs(FIRST, ##__VA_ARGS__))

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
	assertMakeExecArgs("ls", NULL);
}

void test_makeExecArgTwo(void)
{
	assertMakeExecArgs("ls", "-l", NULL);
}

void test_execCommandAction(void)
{
	PipeUtils readPipe, writePipe;
	cppcut_assert_equal(true, readPipe.makeFileInTmpAndOpenForRead());
	cppcut_assert_equal(true, writePipe.makeFileInTmpAndOpenForWrite());

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
