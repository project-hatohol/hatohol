/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef ChildProcessManager_h
#define ChildProcessManager_h

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
		 */
		virtual void onExecuted(const bool &succeeded);

		virtual void onCollected(const siginfo_t *siginfo);

	protected:
		virtual ~EventCallback();
	};

	struct CreateArg {
		mlpl::StringVector args;
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
	virtual ~ChildProcessManager(); // override
	virtual gpointer mainThread(HatoholThreadArg *arg); // override

	void collected(const siginfo_t *siginfo);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ChildProcessManager_h


