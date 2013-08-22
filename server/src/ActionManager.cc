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

#include <cstring>
#include <deque>
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"

using namespace std;

struct ResidentQueueInfo {
	// To be added
};

typedef deque<ResidentQueueInfo> ResidentQueue;

enum ResidentStatus {
	RESIDENT_STAT_INIT,
	RESIDENT_STAT_WAIT_LAUNCHED,
	RESIDENT_STAT_WAIT_PARAM_ACK,
};

struct ResidentInfo {
	ActionManager *actionManager;
	ResidentQueue queue;
	NamedPipe pipeRd, pipeWr;
	string pipeName;
	ResidentStatus status;
	string modulePath;

	ResidentInfo(ActionManager *actMgr, const ActionDef &actionDef)
	: actionManager(actMgr),
	  pipeRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  status(RESIDENT_STAT_INIT)
	{
		pipeName = StringUtils::sprintf("resident-%d", actionDef.id);
		modulePath = actionDef.path;
		HATOHOL_ASSERT(modulePath.size() < PATH_MAX,
		               "moudlePath: %zd, PATH_MAX: %u\n",
		               modulePath.size(), PATH_MAX);
	}

	bool init(GIOFunc funcRd, GIOFunc funcWr)
	{
		if(!pipeRd.init(pipeName, funcRd, this))
			return false;
		if (!pipeWr.init(pipeName, funcWr, this))
			return false;
		return true;
	}
};

typedef map<int, ResidentInfo *>     RunningResidentMap;
typedef RunningResidentMap::iterator RunningResidentMapIterator;

struct ActionManager::PrivateContext {
	static ActorCollector collector;
	DBClientAction dbAction;
	SeparatorCheckerWithCallback separator;
	bool inQuot;
	bool byBackSlash;
	string currWord;
	StringVector *argVect;

	MutexLock residentLock;
	RunningResidentMap runningResidentMap;

	PrivateContext(void)
	: separator(" '\\"),
	  inQuot(false),
	  byBackSlash(false),
	  argVect(NULL)
	{
	}

	void resetParser(StringVector *_argVect)
	{
		inQuot = false;
		byBackSlash = false;
		currWord.clear();
		argVect = _argVect;
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
		if (ctx->byBackSlash)
			ctx->currWord += "'";
		else
			ctx->inQuot = !ctx->inQuot;
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
		execResidentAction(actionDef);
	} else {
		HATOHOL_ASSERT(true, "Unknown type: %d\n", actionDef.type);
	}
}

void ActionManager::makeExecArg(StringVector &argVect, const string &cmd)
{
	m_ctx->resetParser(&argVect);
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word = parsable.readWord(m_ctx->separator);
		m_ctx->currWord += word;
		m_ctx->byBackSlash = false;
	}
	m_ctx->pushbackCurrWord();
}

bool ActionManager::spawn(const ActionDef &actionDef, ActorInfo *actorInfo,
                          const gchar **argv)
{
	const gchar *workingDirectory = NULL;
	if (!actionDef.workingDir.empty())
		workingDirectory = actionDef.workingDir.c_str();

	GSpawnFlags flags =
	  (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GError *error = NULL;

	// We take the lock here to avoid the child spanwed below from
	// not being collected. If the child immediately exits
	// before the following 'm_ctx->collector.addActor(&childPid)' is
	// called, ActorCollector::checkExitProcess() possibly ignores it,
	// because the pid of the child isn't in the wait child set.
	m_ctx->collector.lock();
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)argv, NULL,
	                flags, childSetup, userData, &actorInfo->pid, &error);
	if (!succeeded) {
		m_ctx->collector.unlock();
		string msg = StringUtils::sprintf(
		  "Failed to execute command: %s, action ID: %d",
		  error->message, actionDef.id);
		g_error_free(error);
		MLPL_ERR("%s\n", msg.c_str());
		m_ctx->dbAction.logStartExecAction
		  (actionDef, DBClientAction::ACTLOG_EXECFAIL_EXEC_FAILURE);
		return false;
	}
	actorInfo->logId = m_ctx->dbAction.logStartExecAction(actionDef);
	m_ctx->collector.addActor(*actorInfo);
	m_ctx->collector.unlock();

	return true;
}

void ActionManager::execCommandAction(const ActionDef &actionDef,
                                      ActorInfo *_actorInfo)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_COMMAND,
	               "Invalid type: %d\n", actionDef.type);
	StringVector argVect;
	makeExecArg(argVect, actionDef.path);
	// TODO: check the result of the parse

	const gchar *argv[argVect.size()+1];
	for (size_t i = 0; i < argVect.size(); i++)
		argv[i] = argVect[i].c_str();
	argv[argVect.size()] = NULL;

	ActorInfo actorInfo;
	if (!spawn(actionDef, &actorInfo, argv))
		return;
	if (_actorInfo)
		memcpy(_actorInfo, &actorInfo, sizeof(ActorInfo));
}

void ActionManager::execResidentAction(const ActionDef &actionDef,
                                       ActorInfo *_actorInfo)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_RESIDENT,
	               "Invalid type: %d\n", actionDef.type);
	m_ctx->residentLock.lock();
	RunningResidentMapIterator it =
	   m_ctx->runningResidentMap.find(actionDef.id);
	if (it == m_ctx->runningResidentMap.end()) {
		ResidentInfo *residentInfo =
		  launchResidentActionYard(actionDef, _actorInfo);
		if (residentInfo)
			m_ctx->runningResidentMap[actionDef.id] = residentInfo;
	} else {
		goToResidentYardEntrance(it->second, actionDef, _actorInfo);
	}
	m_ctx->residentLock.unlock();
}

gboolean ActionManager::residentReadErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(data);
	ActionManager *obj = residentInfo->actionManager;
	MLPL_ERR("Error: condition: %x\n", condition);
	obj->closeResident(residentInfo);
	return FALSE;
}

gboolean ActionManager::residentWriteErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(data);
	ActionManager *obj = residentInfo->actionManager;
	MLPL_ERR("Error: condition: %x\n", condition);
	obj->closeResident(residentInfo);
	return FALSE;
}

void ActionManager::launchedCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                               size_t size, void *priv)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(priv);
	ActionManager *obj = residentInfo->actionManager;
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		obj->closeResident(residentInfo);
		return;
	}

	// check the packet type
	sbuf.resetIndex();
	uint32_t bodyLen = *sbuf.getPointerAndIncIndex<uint32_t>();
	if (bodyLen != 0) {
		MLPL_ERR("Invalid body length: %"PRIu32", "
		         "expect: 0\n", bodyLen);
		obj->closeResident(residentInfo);
		return;
	}

	uint16_t pktType = *sbuf.getPointerAndIncIndex<uint16_t>();
	if (pktType != RESIDENT_PROTO_PKT_TYPE_LAUNCHED) {
		MLPL_ERR("Invalid packet type: %"PRIu16", "
		         "expect: %"PRIu16"\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		obj->closeResident(residentInfo);
	}

	// send module parameters
	sendParameters(residentInfo);
	residentInfo->status = RESIDENT_STAT_WAIT_PARAM_ACK;
}

void ActionManager::sendParameters(ResidentInfo *residentInfo)
{
	size_t bodyLen = RESIDENT_PROTO_PARAM_MODULE_PATH_LEN
	                 + residentInfo->modulePath.size();
	size_t pktLen = RESIDENT_PROTO_HEADER_LEN + bodyLen;

	SmartBuffer sbuf(pktLen);
	sbuf.add32(bodyLen);
	sbuf.add16(RESIDENT_PROTO_PKT_TYPE_PARAMETERS);
	sbuf.add16(residentInfo->modulePath.size());
	memcpy(sbuf.getPointer<void>(), residentInfo->modulePath.c_str(),
	       residentInfo->modulePath.size());
	residentInfo->pipeWr.push(sbuf);
}

//
// The folloing functions shall be called with the lock of m_ctx->residentLock
//
ResidentInfo *ActionManager::launchResidentActionYard
  (const ActionDef &actionDef, ActorInfo *actorInfo)
{
	// make a ResidentInfo instance.
	ResidentInfo *residentInfo = new ResidentInfo(this, actionDef);
	if (!residentInfo->init(residentReadErrCb, residentWriteErrCb)) {
		delete residentInfo;
		return NULL;
	}

	const gchar *argv[] = {
	  "hatohol-resident-yard",
	  residentInfo->pipeName.c_str(),
	  NULL};
	if (!spawn(actionDef, actorInfo, argv)) {
		delete residentInfo;
		return NULL;
	}

	residentInfo->status = RESIDENT_STAT_WAIT_LAUNCHED;
	residentInfo->pipeRd.pull(RESIDENT_PROTO_HEADER_LEN,
	                          launchedCb, residentInfo);
	return residentInfo;
}

void ActionManager::goToResidentYardEntrance(
  ResidentInfo *residentInfo, const ActionDef &actionDef, ActorInfo *actorInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	ResidentQueueInfo residentQueueInfo;
	residentInfo->queue.push_back(residentQueueInfo);
	if (residentInfo->queue.empty())
		notifyEvent(residentInfo, actionDef, actorInfo);

	// When the queue is not empty, the queued action will be started.
	// The trigger is the end notification of the current resident action.
}

void ActionManager::notifyEvent(
  ResidentInfo *residentInfo, const ActionDef &actionDef, ActorInfo *actorInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void ActionManager::closeResident(ResidentInfo *residentInfo)
{
	MLPL_BUG("Not implemented: %s (%p)\n",
	         __PRETTY_FUNCTION__, residentInfo);
}
