/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <Logger.h>
#include <Mutex.h>
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

struct HatoholThreadBase::Impl {
	AtomicValue<GThread *>    thread;
	ReadWriteLock rwlock;
	ExitCallbackInfoList      exitCbList;
	SimpleSemaphore           semThreadStart;
	SimpleSemaphore           semThreadExit;
	Mutex                     mutexForReexecSleep;
	AtomicValue<bool>         exitRequested;

	// methods
	Impl(void)
	: thread(NULL),
	  semThreadStart(0),
	  exitRequested(false)
	{
		mutexForReexecSleep.lock();
	}

	virtual ~Impl()
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
: m_impl(new Impl())
{
}

HatoholThreadBase::~HatoholThreadBase()
{
	waitExit();
}

void HatoholThreadBase::start(bool autoDeleteObject, void *userData)
{
	HatoholThreadArg *arg = new HatoholThreadArg();
	arg->obj = this;
	arg->userData = userData;
	arg->autoDeleteObject = autoDeleteObject;
	GError *error = NULL;
	m_impl->thread =
#ifdef GLIB_VERSION_2_32
	  g_thread_try_new("HatoholThread", threadStarter, arg, &error);
#else
	  g_thread_create(threadStarter, arg, TRUE, &error);
#endif // GLIB_VERSION_2_32
	if (m_impl->thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
		g_error_free(error);
	}
	m_impl->waitThreadStarted();
}

void HatoholThreadBase::waitExit(void)
{
	m_impl->semThreadExit.wait();

	// Enable to call this method more than once
	m_impl->semThreadExit.post();
}

void HatoholThreadBase::addExitCallback(ExitCallbackFunc func, void *data)
{
	ExitCallbackInfo exitInfo;
	exitInfo.func = func;
	exitInfo.data = data;

	m_impl->write_lock();
	m_impl->exitCbList.push_back(exitInfo);
	m_impl->write_unlock();
}

bool HatoholThreadBase::isStarted(void) const
{
	return m_impl->thread;
}

bool HatoholThreadBase::isExitRequested(void) const
{
	return m_impl->exitRequested;
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
	m_impl->read_lock();
	ExitCallbackInfoListIterator it = m_impl->exitCbList.begin();
	for (; it != m_impl->exitCbList.end(); ++it) {
		ExitCallbackInfo &exitInfo = *it;
		(*exitInfo.func)(exitInfo.data);
	}
	m_impl->read_unlock();
}

int HatoholThreadBase::onCaughtException(const std::exception &e)
{
	return EXIT_THREAD;
}

void HatoholThreadBase::requestExit(void)
{
	m_impl->exitRequested = true;
}

void HatoholThreadBase::cancelReexecSleep(void)
{
	m_impl->mutexForReexecSleep.unlock();
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
	arg->obj->m_impl->thread = NULL;
	arg->obj->m_impl->semThreadExit.post();
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
	obj->m_impl->notifyThreadStarted();

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
		Mutex::Status status =
		  obj->m_impl->mutexForReexecSleep.timedlock(sleepTimeOrExit);

		// The status is MutextLock::OK, cancelReexecSleep() should
		// be called. In that case, we just return this method.
		if (status == Mutex::STAT_TIMEDOUT)
			goto begin;
	}
	return ret;
}
