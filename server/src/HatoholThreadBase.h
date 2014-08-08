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

#ifndef HatoholThreadBase_h
#define HatoholThreadBase_h

#include <list>
#include <memory>
#include <glib.h>
#include <Utils.h>

#include "Params.h"

class HatoholThreadBase;

struct HatoholThreadArg {
	HatoholThreadBase *obj;
	bool autoDeleteObject;
	void *userData;
};

class HatoholThreadBase {
private:
	typedef void (*ExitCallbackFunc)(void *data);
	struct ExitCallbackInfo {
		ExitCallbackFunc func;
		void            *data;
	};
	typedef std::list<ExitCallbackInfo>    ExitCallbackInfoList;
	typedef ExitCallbackInfoList::iterator ExitCallbackInfoListIterator;

public:
	static const int EXIT_THREAD = -1;

	HatoholThreadBase(void);
	virtual ~HatoholThreadBase();
	void start(bool autoDeleteObject = false, void *userData = NULL);
	void addExitCallback(ExitCallbackFunc func, void *data);
	bool isStarted(void) const;

	/**
	 * Get the flag if exitSync() or requestExit() is called.
	 *
	 * @return
	 * true once exitSync() or requestExit() is called. Otherwise false.
	 */
	bool isExitRequested(void) const;

	/**
	 * Request to exit and wait for its completion synchronously.
	 */
	virtual void exitSync(void);

	/**
	 * Wait for the exit of the thread.
	 */
	virtual void waitExit(void);

protected:
	void doExitCallback(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg) = 0;

	/**
	 * Called when an exception is caught.
	 *
	 * @param e An exception instance.
	 * @return
	 * The sleep time in msec if the return value is positive. After the
	 * sleep, mainThread() will be re-executed. If it's EXIT_THREAD,
	 * the main thread exits. If you cancel the sleep, you can call
	 * cancelReexecSleep().
	 * The default implementation of the this method returns EXIT_THREAD.
	 */
	virtual int onCaughtException(const std::exception &e);

	/**
	 * Set the internal exit flag. After this method is called,
	 * isExitRequested returns true.
	 */
	void requestExit(void);

	void cancelReexecSleep(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	static void threadCleanup(HatoholThreadArg *arg);
	static gpointer threadStarter(gpointer data);
};

#endif // HatoholThreadBase_h

