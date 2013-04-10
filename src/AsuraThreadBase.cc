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

struct AsuraThreadBase::PrivateContext {
	GThread *thread;
	GRWLock rwlock;
	ExceptionCallbackInfoList exceptionCbList;

	// methods
	PrivateContext(void)
	: thread(NULL)
	{
		g_rw_lock_init(&rwlock);
	}

	virtual ~PrivateContext()
	{
		if (thread)
			g_thread_unref(thread);

		g_rw_lock_clear(&rwlock);
	}

	void read_lock(void)
	{
		g_rw_lock_reader_lock(&rwlock);
	}

	void read_unlock(void)
	{
		g_rw_lock_reader_unlock(&rwlock);
	}

	void write_lock(void)
	{
		g_rw_lock_writer_lock(&rwlock);
	}

	void write_unlock(void)
	{
		g_rw_lock_writer_unlock(&rwlock);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
AsuraThreadBase::AsuraThreadBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

AsuraThreadBase::~AsuraThreadBase()
{
	if (m_ctx)
		delete m_ctx;
}

void AsuraThreadBase::start(bool autoDeleteObject)
{
	AsuraThreadArg *arg = new AsuraThreadArg();
	arg->obj = this;
	arg->autoDeleteObject = autoDeleteObject;
	GError *error = NULL;
	m_ctx->thread =
	   g_thread_try_new("AsuraThread", threadStarter, arg, &error);
	if (m_ctx->thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

void AsuraThreadBase::stop(void)
{
}

void AsuraThreadBase::addExceptionCallback(ExceptionCallbackFunc func,
                                           void *data)
{
	ExceptionCallbackInfo exceptionInfo;
	exceptionInfo.func = func;
	exceptionInfo.data = data;

	m_ctx->write_lock();
	m_ctx->exceptionCbList.push_back(exceptionInfo);
	m_ctx->write_unlock();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void AsuraThreadBase::doExceptionCallback(const exception &e)
{
	m_ctx->read_lock();
	ExceptionCallbackInfoListIterator it = m_ctx->exceptionCbList.begin();
	for (; it != m_ctx->exceptionCbList.end(); ++it) {
		ExceptionCallbackInfo &exceptionInfo = *it;
		(*exceptionInfo.func)(e, exceptionInfo.data);
	}
	m_ctx->read_unlock();
}

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
		MLPL_ERR("Got Asura Exception: %s\n",
		         e.getFancyMessage().c_str());
		arg->obj->doExceptionCallback(e);
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
		arg->obj->doExceptionCallback(e);
	}
	if (arg->autoDeleteObject)
		delete arg->obj;
	delete arg;
	return ret;
}
