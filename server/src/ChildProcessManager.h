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

class ChildProcessManager : public HatoholThreadBase {
public:
	struct EventCallback {
		virtual void onCollected(const siginfo_t *siginfo) {}
	};

	struct CreateArg {
		mlpl::StringVector args;
		std::string workingDirectory;
		GSpawnFlags flags;
		EventCallback *eventCb;

		// Output paramters
		pid_t       pid;
		
		// Methods
		CreateArg(void);
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


