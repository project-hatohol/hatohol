/* Asura
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

#ifndef AsuraThreadBase_h
#define AsuraThreadBase_h

#include <glib.h>
#include <Utils.h>

class AsuraThreadBase;

struct AsuraThreadArg {
	AsuraThreadBase *obj;
	bool autoDeleteObject;
};

class AsuraThreadBase {
public:
	typedef void (*ExceptionCallbackFunc)(const exception &e, void *data);

	AsuraThreadBase(void);
	virtual ~AsuraThreadBase();
	void start(bool autoDeleteObject = false);
	void *addExceptionCallback(ExceptionCallbackFunc func);

	/**
	 * Send a stop request. This function blocks until
	 * the thread is stopped.
	 */
	virtual void stop(void);

protected:
	// virtual methods
	virtual gpointer mainThread(AsuraThreadArg *arg) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	static gpointer threadStarter(gpointer data);
};

#endif // AsuraThreadBase_h

