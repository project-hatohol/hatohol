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
#include <glib.h>
#include <Utils.h>

class HatoholThreadBase;

struct HatoholThreadArg {
	HatoholThreadBase *obj;
	bool autoDeleteObject;
	void *userData;
};

class HatoholThreadBase {
private:
	typedef void (*ExceptionCallbackFunc)(const exception &e, void *data);
	struct ExceptionCallbackInfo {
		ExceptionCallbackFunc func;
		void                 *data;
	};
	typedef list<ExceptionCallbackInfo> ExceptionCallbackInfoList;
	typedef ExceptionCallbackInfoList::iterator
	        ExceptionCallbackInfoListIterator;

	typedef void (*ExitCallbackFunc)(void *data);
	struct ExitCallbackInfo {
		ExitCallbackFunc func;
		void            *data;
	};
	typedef list<ExitCallbackInfo>         ExitCallbackInfoList;
	typedef ExitCallbackInfoList::iterator ExitCallbackInfoListIterator;

public:
	HatoholThreadBase(void);
	virtual ~HatoholThreadBase();
	void start(bool autoDeleteObject = false, void *userData = NULL);
	void addExceptionCallback(ExceptionCallbackFunc func, void *data);
	void addExitCallback(ExitCallbackFunc func, void *data);

	/**
	 * Send a stop request. This function blocks until
	 * the thread is stopped.
	 */
	virtual void stop(void);

protected:
	void doExceptionCallback(const exception &e);
	void doExitCallback(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	static void threadCleanup(HatoholThreadArg *arg);
	static gpointer threadStarter(gpointer data);
};

#endif // HatoholThreadBase_h

