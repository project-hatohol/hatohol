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

#include <glib.h>

#include <Logger.h>
using namespace mlpl;

#include <exception>
#include <stdexcept>

#include "Utils.h"
#include "AsuraThreadBase.h"
#include "AsuraException.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
AsuraThreadBase::AsuraThreadBase(void)
: m_thread(NULL)
{
}

AsuraThreadBase::~AsuraThreadBase()
{
	if (m_thread)
		g_thread_unref(m_thread);
}

void AsuraThreadBase::start(bool autoDeleteObject)
{
	AsuraThreadArg *arg = new AsuraThreadArg();;
	arg->obj = this;
	arg->autoDeleteObject = autoDeleteObject;
	GError *error = NULL;
	m_thread = g_thread_try_new("AsuraThread", threadStarter, arg, &error);
	if (m_thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer AsuraThreadBase::threadStarter(gpointer data)
{
	gpointer ret = NULL;
	AsuraThreadArg *arg = static_cast<AsuraThreadArg *>(data);
	try {
		ret = arg->obj->mainThread(arg);
	} catch (const AsuraException &e) {
		MLPL_ERR("Got Asura Exception: %s\n", e.what());
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
	}
	if (arg->autoDeleteObject)
		delete arg->obj;
	delete arg;
	return ret;
}
