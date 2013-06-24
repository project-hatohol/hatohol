/* Hatohol
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	void start(bool autoDeleteObject = false);
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

	static gpointer threadStarter(gpointer data);
};

#endif // HatoholThreadBase_h

