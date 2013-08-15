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
#include "ActorCollector.h"
#include "DBClientAction.h"

struct ActionManager::PrivateContext {
	static ActorCollector collector;
	DBClientAction dbAction;
	SeparatorCheckerWithCallback separator;
	bool inQuot;
	bool quotFinished;
	bool byBackSlash;
	string currWord;
	StringVector *argVect;

	PrivateContext(void)
	: separator(" '\\"),
	  inQuot(false),
	  quotFinished(false),
	  byBackSlash(false),
	  argVect(NULL)
	{
	}

	void resetParser(void)
	{
		inQuot = false;
		quotFinished = false;
		byBackSlash = false;
		currWord.clear();
		argVect = NULL;
	}

	void pushbackCurrWord(void)
	{
		if (currWord.empty())
			return;
		argVect->push_back(currWord);
		currWord.clear();
	}
};

ActorCollector ActionManager::PrivateContext::collector;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ActionManager::ActionManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  (' ', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\'', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\\', separatorCallback, m_ctx);
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
void ActionManager::separatorCallback(const char sep, PrivateContext *ctx)
{
	if (sep == ' ') {
		if (ctx->inQuot)
			ctx->currWord += ' ';
		else
			ctx->pushbackCurrWord();
		ctx->byBackSlash = false;
	} else if (sep == '\'') {
		if (ctx->byBackSlash) {
			ctx->currWord += "'";
		} else {
			if (ctx->inQuot)
				ctx->quotFinished = true;
			ctx->inQuot = !ctx->inQuot;
		}
		ctx->byBackSlash = false;
	} else if (sep == '\\') {
		if (ctx->byBackSlash)
			ctx->currWord += '\\';
		ctx->byBackSlash = !ctx->byBackSlash;
	}
}

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
	m_ctx->resetParser();
	m_ctx->argVect = &argVect;
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word = parsable.readWord(m_ctx->separator);
		m_ctx->currWord += word;
		m_ctx->byBackSlash = false;
	}
	m_ctx->pushbackCurrWord();
}

void ActionManager::execCommandAction(const ActionDef &actionDef)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_COMMAND,
	               "Invalid type: %d\n", actionDef.type);

	const gchar *workingDirectory = NULL;
	if (!actionDef.workingDir.empty())
		workingDirectory = actionDef.workingDir.c_str();

	StringVector argVect;
	makeExecArg(argVect, actionDef.path);
	// TODO: check the result of the parse

	const gchar *argv[argVect.size()+1];
	for (size_t i = 0; i < argVect.size(); i++)
		argv[i] = argVect[i].c_str();
	argv[argVect.size()] = NULL;

	GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GError *error = NULL;
	ActorInfo actorInfo;

	// We take the lock here to avoid the child spanwed below from
	// not being collected. If the child immediately exits
	// before the following 'm_ctx->collector.addActor(&childPid)' is
	// called, ActorCollector::checkExitProcess() possibly ignores it,
	// because the pid of the child isn't in the wait child set.
	m_ctx->collector.lock();
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)&argv, NULL,
	                flags, childSetup, userData, &actorInfo.pid, &error);
	if (!succeeded) {
		m_ctx->collector.unlock();
		string msg = StringUtils::sprintf(
		  "Failed to execute command: %s", error->message);
		g_error_free(error);
		MLPL_ERR("%s\n", msg.c_str());
		m_ctx->dbAction.logErrExecAction(actionDef, msg);
		return;
	}
	actorInfo.logId = m_ctx->dbAction.logStartExecAction(actionDef);
	m_ctx->collector.addActor(actorInfo);
	m_ctx->collector.unlock();
}
