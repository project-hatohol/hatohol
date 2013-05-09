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
#include <glib-object.h>

#include <Logger.h>
#include <MutexLock.h>
using namespace mlpl;

#include <exception>
#include <stdexcept>

#include "Utils.h"
#include "AsuraThreadBase.h"
#include "AsuraException.h"

struct AsuraThreadBase::PrivateContext {
	GThread *thread;
#ifdef GLIB_VERSION_2_32
	GRWLock rwlock;
#else
	GStaticMutex lock;
#endif // GLIB_VERSION_2_32
	ExceptionCallbackInfoList exceptionCbList;
	ExitCallbackInfoList      exitCbList;
	MutexLock                 mutexForThreadExit;

	// methods
	PrivateContext(void)
	: thread(NULL)
	{
#ifdef GLIB_VERSION_2_32
		g_rw_lock_init(&rwlock);
#else
		g_static_mutex_init(&lock);
#endif // GLIB_VERSION_2_32
	}

	virtual ~PrivateContext()
	{
		if (thread) {
#ifdef GLIB_VERSION_2_32
			g_thread_unref(thread);
#else
			// no need to code for freeing
#endif // GLIB_VERSION_2_32
		}

#ifdef GLIB_VERSION_2_32
		g_rw_lock_clear(&rwlock);
#else
		g_static_mutex_free(&lock);
#endif // GLIB_VERSION_2_32
	}

	void read_lock(void)
	{
#ifdef GLIB_VERSION_2_32
		g_rw_lock_reader_lock(&rwlock);
#else
		g_static_mutex_lock(&lock);
#endif // GLIB_VERSION_2_32
	}

	void read_unlock(void)
	{
#ifdef GLIB_VERSION_2_32
		g_rw_lock_reader_unlock(&rwlock);
#else
		g_static_mutex_unlock(&lock);
#endif // GLIB_VERSION_2_32
	}

	void write_lock(void)
	{
#ifdef GLIB_VERSION_2_32
		g_rw_lock_writer_lock(&rwlock);
#else
		g_static_mutex_lock(&lock);
#endif // GLIB_VERSION_2_32
	}

	void write_unlock(void)
	{
#ifdef GLIB_VERSION_2_32
		g_rw_lock_writer_unlock(&rwlock);
#else
		g_static_mutex_unlock(&lock);
#endif // GLIB_VERSION_2_32
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
#ifdef GLIB_VERSION_2_32
	  g_thread_try_new("AsuraThread", threadStarter, arg, &error);
#else
	  g_thread_create(threadStarter, arg, TRUE, &error);
#endif // GLIB_VERSION_2_32
	if (m_ctx->thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

void AsuraThreadBase::stop(void)
{
	m_ctx->mutexForThreadExit.lock();
	m_ctx->mutexForThreadExit.unlock();
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

void AsuraThreadBase::addExitCallback(ExitCallbackFunc func, void *data)
{
	ExitCallbackInfo exitInfo;
	exitInfo.func = func;
	exitInfo.data = data;

	m_ctx->write_lock();
	m_ctx->exitCbList.push_back(exitInfo);
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

void AsuraThreadBase::doExitCallback(void)
{
	m_ctx->read_lock();
	ExitCallbackInfoListIterator it = m_ctx->exitCbList.begin();
	for (; it != m_ctx->exitCbList.end(); ++it) {
		ExitCallbackInfo &exitInfo = *it;
		(*exitInfo.func)(exitInfo.data);
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
	arg->obj->m_ctx->mutexForThreadExit.lock();
	try {
		ret = arg->obj->mainThread(arg);
	} catch (const AsuraException &e) {
		MLPL_ERR("Got Asura Exception: %s\n",
		         e.getFancyMessage().c_str());
		arg->obj->doExceptionCallback(e);
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
		arg->obj->doExceptionCallback(e);
	} catch (...) {
		arg->obj->m_ctx->mutexForThreadExit.unlock();
		throw;
	}
	arg->obj->doExitCallback();
	arg->obj->m_ctx->mutexForThreadExit.unlock();
	if (arg->autoDeleteObject)
		delete arg->obj;
	delete arg;
	return ret;
}
