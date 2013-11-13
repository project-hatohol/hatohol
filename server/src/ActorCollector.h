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

#ifndef ActorCollector_h
#define ActorCollector_h

#include "HatoholThreadBase.h"

struct ActorInfo;
typedef void (*ActorCollectedFunc)(const ActorInfo *actorInfo);

struct ActorInfo {
	pid_t    pid;
	uint64_t logId;
	bool     dontLog;

	// collectedCb is called with taking ActorCollector::lock().
	// postCollectedCb is called after calling ActorCollector::unlock().
	ActorCollectedFunc collectedCb;
	mutable ActorCollectedFunc postCollectedCb;
	mutable void              *collectedCbPriv;
	guint timerTag;
	
	// constructor and destructor
	ActorInfo(void);
	virtual ~ActorInfo();
};

class ActorCollector : public HatoholThreadBase
{
public:
	static void init(void);
	static void reset(void);
	static void resetOnCollectorThread(void);
	static void quit(void);
	static void lock(void);
	static void unlock(void);

	/**
	 * lock() has to be called before this function is used.
	 */
	static void addActor(ActorInfo *actorInfo);

	ActorCollector(void);
	virtual ~ActorCollector();

	/**
	 * Check if ActorCollector is watching the process.
	 *
	 * @param pid A process ID of a checked process.
	 */
	static bool isWatching(pid_t pid);

	static void setDontLog(pid_t pid);

	static size_t getNumberOfWaitingActors(void);

protected:
	static void registerSIGCHLD(void);
	static void incWaitingActor(void);

	// we redefine start() as protected so that HatoholThreadBase::start()
	// cannot be called from other classes.
	void start(void);
	void notifyChildSiginfo(siginfo_t *info);

	// overriden virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ActorCollector_h

