/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <memory>
#include "Params.h"
#include "SmartBuffer.h"
#include "DBTablesAction.h"
#include "ActorCollector.h"
#include "NamedPipe.h"
#include "StringUtils.h"

struct ResidentInfo;

class ActionManager
{
private:
	struct Impl;

public:
	static const char *NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC;
	static const size_t NUM_COMMNAD_ACTION_EVENT_ARG = 10;
	// 1: arg. magic (version)
	// 2: ActionId
	// 3: serverId
	// 4: hostId
	// 5: time: ex) 1378124964.8592
	// 6: eventId
	// 7: eventType
	// 8: triggerId
	// 9: triggerStatus
	// 10: triggerSeverity

	static const char *ENV_NAME_PATH_FOR_ACTION;
	static const char *ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION;
	static const char *ENV_NAME_SESSION_ID;

	struct ResidentNotifyInfo;
	static void reset(void);

	ActionManager(void);
	virtual ~ActionManager();
	void checkEvents(const EventInfoList &eventList);
	void reExecuteUnfinishedAction(void);

protected:
	/**
	 * A callback function that is called in spawn() after the child
	 * process is spawned.
	 *
	 * @param actorInfo
	 * An ActorInfo instance that has information about the spawned process.
	 * If the spawn fails, this parameter is set to NULL.
	 *
	 * @param actionDef
	 * An ActionDef reference that is passed to spawn().
	 *
	 * @param logId
	 * A log ID concerned with the spawned process.
	 *
	 * @param priv
	 * A pointer that is passed as a 'postprocPriv' on the call of spawn().
	 *
	 */
	typedef void (*SpawnPostproc)(ActorInfo *actorInfo,
	                              const ActionDef &actionDef,
	                              uint64_t logId, void *priv);

	static void setupPathForAction(std::string &path,
	                               std::string &ldLibraryPath);

	static gboolean residentReadErrCb(GIOChannel *source,
	                                  GIOCondition condition,
	                                  gpointer data);
	static gboolean residentWriteErrCb(GIOChannel *source,
	                                   GIOCondition condition,
	                                   gpointer data);
	static void launchedCb(GIOStatus stat, mlpl::SmartBuffer &buf,
	                       size_t size, ResidentNotifyInfo *notifyInfo);
	static void moduleLoadedCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
	                           size_t size, ResidentNotifyInfo *notifyInfo);
	static void gotNotifyEventAckCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
	                                size_t size,
	                                ResidentNotifyInfo *residentInfo);
	static void sendParameters(ResidentInfo *residentInfo);
	static gboolean commandActionTimeoutCb(gpointer data);
	static void residentActionTimeoutCb(NamedPipe *namedPipe,
	                                    gpointer data);
	bool shouldSkipByTime(const EventInfo &eventInfo);
	bool shouldSkipByLog(const EventInfo &eventInfo,
	                     DBTablesAction &dbAction);
	HatoholError runAction(const ActionDef &actionDef,
	                       const EventInfo &eventInfo,
	                       DBTablesAction &dbAction);

	/**
	 * Spawn an actor.
	 *
	 * @param actionDef An ActionDef instance.
	 * @param eventInfo An EventInfo instance.
	 * @param dbAgent   An DBTablesAction instance.
	 *
	 * @param argv
	 * An argument array for the command to be spawned. The first element
	 * is the command path itself. The last element should be NULL.
	 *
	 * @param postproc
	 * A callback function called after the child process is spawned.
	 * Because this function is executed after ActorCollect is locked,
	 * 'actorInfo', an argument of postproc, is never deleted in the time.
	 * Event if the spawn fails, this function is called back with
	 * actorInfo = NULL.
	 *
	 * @param postprocPriv
	 * A pointer that is passed to postproc.
	 *
	 * @return true if the spawn was successful. Otherwise false.
	 */
	static bool spawn(const ActionDef &actionDef,
	                  const EventInfo &eventInfo,
	                  DBTablesAction &dbAction, const gchar **argv,
	                  SpawnPostproc postproc, void *postprocPriv);

	/**
	 * execute a command-type action.
	 *
	 * @param actionDef
	 * An ActionDef instance concerned with the action.
	 *
	 * @param eventInfo
	 * An EventInfo instance concerned with the action.
	 *
	 * @param dbAgent
	 * An DBTablesAction instance.
	 *
	 * @param actorInfo
	 * If this parameter is not NULL, information about the executed
	 * actor such as a PID and a log ID is returned in it.
	 * If the execution failed, PID is set to 0.
	 */
	void execCommandAction(const ActionDef &actionDef,
	                       const EventInfo &eventInfo,
	                       DBTablesAction &dbAction,
	                       ActorInfo *actorInfo = NULL);

	static void execCommandActionCore(
	  const ActionDef &actionDef, const EventInfo &eventInfo,
	  DBTablesAction &dbAction, void *postprocCtx,
	  const mlpl::StringVector &argVect);
	
	static void addCommandDirectory(std::string &path);

	/**
	 * execute a resident-type action.
	 *
	 * @param actionDef
	 * An ActionDef instance concerned with the action.
	 *
	 * @param eventInfo
	 * An EventInfo instance concerned with the action.
	 *
	 * @param dbAgent
	 * An DBTablesAction instance.
	 *
	 * @param actorInfo
	 * If this parameter is not NULL, information about the executed
	 * actor such as a PID and a log ID is returned in it.
	 * If the execution failed, PID is set to 0.
	 */
	void execResidentAction(const ActionDef &actionDef,
	                        const EventInfo &eventInfo,
	                        DBTablesAction &dbAction,
	                        ActorInfo *actorInfo = NULL);

	ResidentInfo *launchResidentActionYard(const ActionDef &actionDef,
	                                       const EventInfo &eventInfo,
	                                       DBTablesAction &dbAction,
	                                       ActorInfo *actorInfoCopy);
	/**
	 * notify hatohol-resident-yard of an event only when it is idle and
	 * there is at least one element in residentInfo->notifyQueue.
	 * Othewise the request is processed later.
	 *
	 * @param residentInfo A residentInfo instance.
	 */
	void tryNotifyEvent(ResidentInfo *residentInfo);

	/**
	 * notify hatohol-resident-yard of an event at the top of notifyQueue.
	 * NOTE: This function is assumed to be called only from
	 * tryNotifyEvent().
	 *
	 * @param residentInfo A residentInfo instance.
	 *
	 * @param notifyInfo.
	 * A ResidentNotifyInfo instance. If the member: logId is
	 * INVALID_ACTION_LOG_ID, the new log record is created.
	 * Otherwise the record with it will be updated.
	 */
	void notifyEvent(ResidentInfo *residentInfo,
	                 ResidentNotifyInfo *notifyInfo);

	void execIncidentSenderAction(const ActionDef &actionDef,
				      const EventInfo &eventInfo,
				      DBTablesAction &dbAction);

	static void commandActorCollectedCb(const ActorInfo *actorInfo);
	static void commandActorPostCollectedCb(const ActorInfo *actorInfo);
	static void residentActorCollectedCb(const ActorInfo *actorInfo);
	static void residentActorPostCollectedCb(const ActorInfo *actorInfo);
	void closeResident(ResidentInfo *residentInfo);
	void closeResident(ResidentNotifyInfo *notifyInfo,
	                   ActionLogExecFailureCode failureCode);
	static void spawnPostprocCommandAction(ActorInfo *actorInfo,
	                                       const ActionDef &actionDef,
	                                       uint64_t logId, void *priv);
	static void spawnPostprocResidentAction(ActorInfo *actorInfo,
	                                        const ActionDef &actionDef,
	                                        uint64_t logId, void *priv);

	static void postProcSpawnFailure(
	  const ActionDef &actionDef, const EventInfo &eventInfo,
	  DBTablesAction &dbAction, ActorInfo *actorInfo,
	  uint64_t *logId, const GError *error, bool logUpdateFlag);

	void fillTriggerInfoInEventInfo(EventInfo &eventInfo);
	static size_t getNumberOfOnstageCommandActors(void);
	static std::string makeSessionIdEnv(const ActionDef &actionDef,
	                                    std::string &sessionId);

private:
	struct ActorProfile;
	std::unique_ptr<Impl> m_impl;
};

