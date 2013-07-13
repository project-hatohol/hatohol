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
