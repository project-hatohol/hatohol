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
	                       size_t size, void *priv);
	static void moduleLoadedCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
	                           size_t size, ResidentInfo *residentInfo);
	static void sendParameters(ResidentInfo *residentInfo);
	void runAction(const ActionDef &actionDef);
	void makeExecArg(StringVector &argVect, const string &cmd);
	bool spawn(const ActionDef &actionDef, ActorInfo *actorInfo,
	           const gchar **argv);
	void execCommandAction(const ActionDef &actionDef,
	                       ActorInfo *actorInfo = NULL);
	void execResidentAction(const ActionDef &actionDef,
	                        ActorInfo *actorInfo = NULL);
	ResidentInfo *launchResidentActionYard(const ActionDef &actionDef,
	                                       ActorInfo *actorInfo);
	void goToResidentYardEntrance(ResidentInfo *residentInfo,
	                              const ActionDef &actionDef,
	                              ActorInfo *actorInfo);
	void notifyEvent(ResidentInfo *residentInfo,
	                 const ActionDef &actionDef, ActorInfo *actorInfo);
	void closeResident(ResidentInfo *residentInfo);

private:
	PrivateContext *m_ctx;
};

#endif // ActionManager_h

