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
#include "HatoholThreadBase.h"
#include "HatoholError.h"

struct ActorInfo;
typedef void (*ActorCollectedFunc)(const ActorInfo *actorInfo);

struct ActorInfo {
	pid_t    pid;
	uint64_t logId;
	bool     dontLog;
	std::string sessionId;

	// collectedCb is called with taking ActorCollector::lock().
	// postCollectedCb is called after calling ActorCollector::unlock().
	ActorCollectedFunc collectedCb;
	mutable ActorCollectedFunc postCollectedCb;
	mutable void              *collectedCbPriv;
	guint timerTag;
	
	// constructor and destructor
	ActorInfo(void);
	ActorInfo &operator=(const ActorInfo &actorInfo);
	virtual ~ActorInfo();
};

class ActorCollector
{
public:
	struct Profile {
		mlpl::StringVector args;
		mlpl::StringVector envs;
		std::string workingDirectory;

		/**
		 * Called in debut() when it is succeeded synchronously.
		 *
		 * @parameter pid
		 * A PID of a created process.
		 *
		 * @return
		 * A pointer of an ActorInfo instance that is typically 
		 * created in this callback. After this callback returns,
		 * the owner of it is taken over by ActorCollector.
		 */
		virtual ActorInfo *successCb(const pid_t &pid) = 0;

		virtual void postSuccessCb(void) = 0;
		virtual void errorCb(GError *error) = 0;
	};

	static void reset(void);

	/**
	 * Create a new actor.
	 *
	 * @param profile An actor's profile.
	 * @return A HatoholError instance.
	 */
	static HatoholError debut(Profile &profile);

	/**
	 * lock() has to be called before this function is used.
	 */
	static void addActor(ActorInfo *actorInfo);

	/**
	 * Check if ActorCollector is watching the process.
	 *
	 * @param pid A process ID of a checked process.
	 */
	static bool isWatching(pid_t pid);

	static void setDontLog(pid_t pid);

	static size_t getNumberOfWaitingActors(void);

protected:
	struct ActorContext;

	static void notifyChildSiginfo(
	  const siginfo_t *info, ActorContext &actorCtx);
	static void postCollectedProc(ActorContext &actorCtx);
	static void cleanupChildInfo(const pid_t &pid);

private:
	struct Impl;
};

