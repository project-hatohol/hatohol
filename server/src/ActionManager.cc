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

#include "ActionManager.h"
#include "DBClientAction.h"

struct ActionManager::PrivateContext {
	DBClientAction dbAction;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ActionManager::ActionManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActionManager::~ActionManager()
{
	if (m_ctx)
		delete m_ctx;
}

void ActionManager::checkEvents(const EventInfoList &eventList)
{
	EventInfoListConstIterator it = eventList.begin();
	for (; it != eventList.end(); ++it) {
		ActionDefList actionDefList;
		m_ctx->dbAction.getActionList(*it, actionDefList);
		ActionDefListIterator actIt = actionDefList.begin();
		for (; actIt != actionDefList.end(); ++actIt)
			runAction(*actIt);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActionManager::runAction(const ActionDef &actionDef)
{
	if (actionDef.type == ACTION_COMMAND) {
		execCommandAction(actionDef);
	} else if (actionDef.type == ACTION_RESIDENT) {
		MLPL_BUG("Not implemented: runAction: ACTION_RESIDENT\n");
	} else {
		HATOHOL_ASSERT(true, "Unknown type: %d\n", actionDef.type);
	}
}

void ActionManager::makeExecArg(StringVector &argVect, const string &cmd)
{
	MLPL_WARN("Not fully implemented: %s\n", __PRETTY_FUNCTION__);
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word =
		  parsable.readWord(ParsableString::SEPARATOR_SPACE);
		argVect.push_back(word);
	}
}

void ActionManager::execCommandAction(const ActionDef &actionDef)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_COMMAND,
	               "Invalid type: %d\n", actionDef.type);

	const gchar *workingDirectory = NULL;
	if (!actionDef.workingDir.empty())
		workingDirectory = actionDef.workingDir.c_str();

	StringVector argVect;
	makeExecArg(argVect, actionDef.command);
	// TODO: check the result of the parse

	const gchar *argv[argVect.size()+1];
	for (size_t i = 0; i < argVect.size(); i++)
		argv[i] = argVect[i].c_str();
	argv[argVect.size()] = NULL;

	GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GPid childPid;
	GError *error = NULL;
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)&argv, NULL,
	                flags, childSetup, userData, &childPid, &error);
	if (!succeeded) {
		string msg = StringUtils::sprintf(
		  "Failed to execute command: %s", error->message);
		g_error_free(error);
		MLPL_ERR("%s\n", msg.c_str());
		m_ctx->dbAction.logErrExecAction(actionDef, msg);
		return;
	}
	m_ctx->dbAction.logStartExecAction(actionDef);
}
