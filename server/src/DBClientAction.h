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

#ifndef DBClientAction_h
#define DBClientAction_h

#include <string>
#include "DBClientHatohol.h"
#include "DBClientConnectable.h"
#include "ConfigManager.h"

enum ActionType {
	ACTION_COMMAND,
	ACTION_RESIDENT,
};

struct ActionDef {
	ActionType  type;
	std::string workingDir;
	std::string command;
};

typedef list<ActionDef>               ActionDefList;
typedef ActionDefList::iterator       ActionDefListIterator;
typedef ActionDefList::const_iterator ActionDefListConstIterator;

struct ExitChildInfo {
	pid_t pid;
	int   status;
	int   exitCode;
};

class DBClientAction :
   public DBClientConnectable<DB_DOMAIN_ID_ACTION>
{
public:
	static int ACTION_DB_VERSION;
	static const char *DEFAULT_DB_NAME;

	static void init(void);
	DBClientAction(void);
	virtual ~DBClientAction();
	void getActionList(const EventInfo &eventInfo,
	                   ActionDefList &actionDefList);
	void logStartExecAction(const ActionDef &actionDef);
	void logEndExecAction(const ExitChildInfo &exitChildInfo);
	void logErrExecAction(const ActionDef &actionDef, const string &msg);

protected:

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

};

#endif // DBClientAction_h

