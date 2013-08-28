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

#ifndef ActionManager_h
#define ActionManager_h

#include "SmartBuffer.h"
#include "DBClientAction.h"
#include "ActorCollector.h"

struct ResidentInfo;

class ActionManager
{
private:
	struct PrivateContext;

public:
	struct ResidentNotifyInfo;
	static void reset(void);

	ActionManager(void);
	virtual ~ActionManager();
	void checkEvents(const EventInfoList &eventList);

protected:
	static void separatorCallback(const char sep, PrivateContext *ctx);
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
	void runAction(const ActionDef &actionDef, const EventInfo &eventInfo);
	void makeExecArg(StringVector &argVect, const string &cmd);
	bool spawn(const ActionDef &actionDef, ActorInfo *actorInfo,
	           const gchar **argv);

	/**
	 * execute a command-type action.
	 *
	 * @param actionDef
	 * An ActionDef instance concerned with the action.
	 *
	 * @param eventInfo
	 * An EventInfo instance concerned with the action.
	 *
	 * @param actorInfo
	 * If this parameter is not NULL, information about the executed
	 * actor such as a PID and a log ID is returned in it.
	 */
	void execCommandAction(const ActionDef &actionDef,
	                       const EventInfo &eventInfo,
	                       ActorInfo *actorInfo = NULL);

	/**
	 * execute a resident-type action.
	 *
	 * @param actionDef
	 * An ActionDef instance concerned with the action.
	 *
	 * @param eventInfo
	 * An EventInfo instance concerned with the action.
	 *
	 * @param actorInfo
	 * If this parameter is not NULL, information about the executed
	 * actor such as a PID and a log ID is returned in it.
	 */
	void execResidentAction(const ActionDef &actionDef,
	                        const EventInfo &eventInfo,
	                        ActorInfo *actorInfo = NULL);

	ResidentInfo *launchResidentActionYard(const ActionDef &actionDef,
	                                       const EventInfo &eventInfo,
	                                       ActorInfo *actorInfo);
	/**
	 * notify hatohol-resident-yard of a event only when it is idle and
	 * there is at least one element in residentInfo->notifyQueue.
	 * Othewise the request is processed later.
	 *
	 * @param residentInfo A residentInfo instance.
	 */
	void tryNotifyEvent(ResidentInfo *residentInfo);

	/**
	 * notify hatohol-resident-yaevent of a event at the top of notifyQueue.
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

	void closeResident(ResidentInfo *residentInfo);

private:
	PrivateContext *m_ctx;
};

#endif // ActionManager_h

