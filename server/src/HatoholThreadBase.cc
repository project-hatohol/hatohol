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

#include <glib.h>
#include <glib-object.h>

#include <Logger.h>
#include <MutexLock.h>
#include <ReadWriteLock.h>
#include <Reaper.h>
using namespace mlpl;

#include <exception>
#include <stdexcept>

#include "Utils.h"
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "CacheServiceDBClient.h"

struct HatoholThreadBase::PrivateContext {
	GThread *thread;
	ReadWriteLock rwlock;
	ExceptionCallbackInfoList exceptionCbList;
	ExitCallbackInfoList      exitCbList;
	MutexLock                 mutexForThreadExit;

	// methods
	PrivateContext(void)
	: thread(NULL)
	{
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
	}

	void read_lock(void)
	{
		rwlock.readLock();
	}

	void read_unlock(void)
	{
		rwlock.unlock();
	}

	void write_lock(void)
	{
		rwlock.writeLock();
	}

	void write_unlock(void)
	{
		rwlock.unlock();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholThreadBase::HatoholThreadBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HatoholThreadBase::~HatoholThreadBase()
{
	stop();
	if (m_ctx)
		delete m_ctx;
}

void HatoholThreadBase::start(bool autoDeleteObject)
{
	HatoholThreadArg *arg = new HatoholThreadArg();
	arg->obj = this;
	arg->autoDeleteObject = autoDeleteObject;
	GError *error = NULL;
	m_ctx->thread =
#ifdef GLIB_VERSION_2_32
	  g_thread_try_new("HatoholThread", threadStarter, arg, &error);
#else
	  g_thread_create(threadStarter, arg, TRUE, &error);
#endif // GLIB_VERSION_2_32
	if (m_ctx->thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

void HatoholThreadBase::stop(void)
{
	m_ctx->mutexForThreadExit.lock();
	m_ctx->mutexForThreadExit.unlock();
}

void HatoholThreadBase::addExceptionCallback(ExceptionCallbackFunc func,
                                           void *data)
{
	ExceptionCallbackInfo exceptionInfo;
	exceptionInfo.func = func;
	exceptionInfo.data = data;

	m_ctx->write_lock();
	m_ctx->exceptionCbList.push_back(exceptionInfo);
	m_ctx->write_unlock();
}

void HatoholThreadBase::addExitCallback(ExitCallbackFunc func, void *data)
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
void HatoholThreadBase::doExceptionCallback(const exception &e)
{
	m_ctx->read_lock();
	ExceptionCallbackInfoListIterator it = m_ctx->exceptionCbList.begin();
	for (; it != m_ctx->exceptionCbList.end(); ++it) {
		ExceptionCallbackInfo &exceptionInfo = *it;
		(*exceptionInfo.func)(e, exceptionInfo.data);
	}
	m_ctx->read_unlock();
}

void HatoholThreadBase::doExitCallback(void)
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
static void callCacheServiceCleanup(void *unused)
{
	CacheServiceDBClient::cleanup();
}

gpointer HatoholThreadBase::threadStarter(gpointer data)
{
	// To call CacheServiceDBClient::cleanup() surely when this function
	// returns, even if it is due to an exception.
	Reaper<int> cacheServiceCleaner((int *)1, // dummy and not used
	                                callCacheServiceCleanup);

	gpointer ret = NULL;
	HatoholThreadArg *arg = static_cast<HatoholThreadArg *>(data);
	arg->obj->m_ctx->mutexForThreadExit.lock();
	try {
		ret = arg->obj->mainThread(arg);
	} catch (const HatoholException &e) {
		MLPL_ERR("Got Hatohol Exception: %s\n",
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
