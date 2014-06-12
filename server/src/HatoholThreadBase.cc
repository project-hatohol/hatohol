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
#include <exception>
#include <stdexcept>
#include <AtomicValue.h>
#include <SimpleSemaphore.h>
#include "Utils.h"
#include "HatoholThreadBase.h"
#include "HatoholException.h"
using namespace std;
using namespace mlpl;

struct HatoholThreadBase::PrivateContext {
	AtomicValue<GThread *>    thread;
	ReadWriteLock rwlock;
	ExitCallbackInfoList      exitCbList;
	SimpleSemaphore           semThreadStart;
	SimpleSemaphore           semThreadExit;
	MutexLock                 mutexForReexecSleep;
	AtomicValue<bool>         exitRequested;

	// methods
	PrivateContext(void)
	: thread(NULL),
	  semThreadStart(0),
	  exitRequested(false)
	{
		mutexForReexecSleep.lock();
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

	void waitThreadStarted(void)
	{
		semThreadStart.wait();
	}

	void notifyThreadStarted(void)
	{
		semThreadExit.wait();
		semThreadStart.post();
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
	waitExit();
	if (m_ctx)
		delete m_ctx;
}

void HatoholThreadBase::start(bool autoDeleteObject, void *userData)
{
	HatoholThreadArg *arg = new HatoholThreadArg();
	arg->obj = this;
	arg->userData = userData;
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
		g_error_free(error);
	}
	m_ctx->waitThreadStarted();
}

void HatoholThreadBase::waitExit(void)
{
	m_ctx->semThreadExit.wait();

	// Enable to call this method more than once
	m_ctx->semThreadExit.post();
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

bool HatoholThreadBase::isStarted(void) const
{
	return m_ctx->thread;
}

bool HatoholThreadBase::isExitRequested(void) const
{
	return m_ctx->exitRequested;
}

void HatoholThreadBase::exitSync(void)
{
	requestExit();
	cancelReexecSleep();
	waitExit();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
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

int HatoholThreadBase::onCaughtException(const std::exception &e)
{
	return EXIT_THREAD;
}

void HatoholThreadBase::requestExit(void)
{
	m_ctx->exitRequested = true;
}

void HatoholThreadBase::cancelReexecSleep(void)
{
	m_ctx->mutexForReexecSleep.unlock();
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
void hatoholThreadCleanup(HatoholThreadArg *arg) __attribute__((weak));

void hatoholThreadCleanup(HatoholThreadArg *arg)
{
	// We do nothing here. Instead, the function with the same name is in
	// HatoholServer.cc in order to call CacheServiceDBClient::cleanup()
	// only when this class is used from Hatohol Server.
}

void HatoholThreadBase::threadCleanup(HatoholThreadArg *arg)
{
	arg->obj->doExitCallback();
	hatoholThreadCleanup(arg);
	arg->obj->m_ctx->thread = NULL;
	arg->obj->m_ctx->semThreadExit.post();
	if (arg->autoDeleteObject)
		delete arg->obj;
	delete arg;
}

gpointer HatoholThreadBase::threadStarter(gpointer data)
{
	gpointer ret = NULL;
	HatoholThreadArg *arg = static_cast<HatoholThreadArg *>(data);

	// threadCleanup() is called when the this function returns,
	// even if it is due to an exception.
	Reaper<HatoholThreadArg> threadCleaner(arg, threadCleanup);

	HatoholThreadBase *obj = arg->obj;
	obj->m_ctx->notifyThreadStarted();

begin:
	int sleepTimeOrExit = EXIT_THREAD;
	try {
		ret = obj->mainThread(arg);
	} catch (const HatoholException &e) {
		MLPL_ERR("Got Hatohol Exception: %s\n",
		         e.getFancyMessage().c_str());
		sleepTimeOrExit = obj->onCaughtException(e);
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
		sleepTimeOrExit = obj->onCaughtException(e);
	} catch (...) {
		throw;
	}
	if (sleepTimeOrExit >= 0) {
		MutexLock::Status status =
		  obj->m_ctx->mutexForReexecSleep.timedlock(sleepTimeOrExit);

		// The status is MutextLock::OK, cancelReexecSleep() should
		// be called. In that case, we just return this method.
		if (status == MutexLock::STAT_TIMEDOUT)
			goto begin;
	}
	return ret;
}
