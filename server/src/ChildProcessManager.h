/*
 * Copyright (C) 2014 Project Hatohol
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
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include "HatoholThreadBase.h"
#include "HatoholError.h"
#include "UsedCountable.h"

class ChildProcessManager : public HatoholThreadBase {
public:
	struct EventCallback : public UsedCountable {

		/**
		 * Called soon after an execution of a child process is done
		 * in ChildProcessManager::create().
		 * This callback is called in the critical section. So other
		 * callbacks such as onCollected() are never running at the
		 * same time.
		 *
		 * @param succeeded
		 * true if an exection has succeeded. Otherwise false.
		 *
		 * @param gerror
		 * A GError object. It may be NULL.
		 */
		virtual void onExecuted(const bool &succeeded,
		                        GError *gerror);

		/**
		 * Called when SIGCHLD of the child process concerned with
		 * this object is received.
		 * Note that this method is called only when the signal code
		 * is any of CLD_EXITED, CLD_KILLED, or CLD_DUMPED.
		 */
		virtual void onCollected(const siginfo_t *siginfo);

		/**
		 * Called at the end of process management for a child.
		 * This is called outside of the critical section.
		 */
		virtual void onFinalized(void);

		/**
		 * Called after ChildProcessManager::reset() sends a KILL
		 * signal to the child process.
		 * onCollected() and onFinalized() are no longer fired.
		 * So cleanup should be done in this method.
		 */
		virtual void onReset(void);

	protected:
		virtual ~EventCallback();
	};

	struct CreateArg {
		mlpl::StringVector args;
		mlpl::StringVector envs;
		std::string workingDirectory;
		GSpawnFlags flags;

		/**
		 * If eventCb is not NULL, eventCb->unref() is called in
		 * the destructor.
		 */
		EventCallback *eventCb;

		// Output paramters
		pid_t       pid;
		
		// Methods
		CreateArg(void);
		virtual ~CreateArg();
		void addFlag(const GSpawnFlags &flag);
	};

	/**
	 * Get the instance.
	 * Note: This class should be used as singleton.
	 */
	static ChildProcessManager *getInstance(void);

	void reset(void);

	/**
	 * Create a child process.
	 *
	 * @param arg Information about the child to be created.
	 * @return HatoholError instance.
	 */
	HatoholError create(CreateArg &arg);

protected:
	// This class is sigleton. So the construtor should not be public.
	ChildProcessManager(void);
	virtual ~ChildProcessManager() override;
	virtual gpointer mainThread(HatoholThreadArg *arg) override;

	bool isDead(const siginfo_t *siginfo);
	void collected(const siginfo_t *siginfo);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

