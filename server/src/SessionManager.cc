/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstdio>
#include <map>
#include <uuid/uuid.h>
#include "Logger.h"
#include "SessionManager.h"
#include <Mutex.h>
#include "ReadWriteLock.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------
Session::Session(void)
: userId(INVALID_USER_ID),
  loginTime(SmartTime::INIT_CURR_TIME),
  lastAccessTime(SmartTime::INIT_CURR_TIME),
  timerId(INVALID_EVENT_ID),
  sessionMgr(NULL)
{
}

Session::~Session()
{
	Utils::removeGSourceIfNeeded(timerId);
}

// ---------------------------------------------------------------------------
// SessionManager
// ---------------------------------------------------------------------------

// Ref. man uuid_unparse.
const size_t SessionManager::SESSION_ID_LEN = 36;

const size_t SessionManager::INITIAL_TIMEOUT = 10 * 60; // 10 min.
const size_t SessionManager::DEFAULT_TIMEOUT = -1;
const size_t SessionManager::NO_TIMEOUT = 0;
const char * SessionManager::ENV_NAME_TIMEOUT = "HATOHOL_SESSION_TIMEOUT";

struct SessionManager::Impl {
	static Mutex           initLock;
	static SessionManager *instance;
	static size_t defaultTimeout;

	ReadWriteLock rwlock;
	SessionIdMap  sessionIdMap;

	virtual ~Impl() {
		rwlock.writeLock();
		SessionIdMapIterator it = sessionIdMap.begin();
		for (; it != sessionIdMap.end(); ++it) {
			Session *session = it->second;
			clearSessionManagerIfNeeded(session);
			session->unref();

		}
		sessionIdMap.clear();
		rwlock.unlock();
	}

private:
	void clearSessionManagerIfNeeded(Session *session)
	{
		if (session->getUsedCount() == 1)
			return;

		// To avoid an invalid access. Or timeCb() will access
		// the deleted 'this' instance.
		session->sessionMgr = NULL;
	}
};

SessionManager *SessionManager::Impl::instance = NULL;
Mutex           SessionManager::Impl::initLock;
size_t SessionManager::Impl::defaultTimeout = INITIAL_TIMEOUT;


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SessionManager::reset(void)
{
	delete Impl::instance;
	Impl::instance = NULL;

	Impl::defaultTimeout = INITIAL_TIMEOUT;
	char *env = getenv(ENV_NAME_TIMEOUT);
	if (env) {
		size_t timeout = 0;
		if (sscanf(env, "%zd", &timeout) != 1) {
			MLPL_ERR("Invalid value: %s. Use the default timeout\n",
			         env);
		} else {
			Impl::defaultTimeout = timeout;
		}
		MLPL_INFO("Default session timeout: %zd (sec)\n",
		          Impl::defaultTimeout);
	}
}

SessionManager *SessionManager::getInstance(void)
{
	Impl::initLock.lock();
	if (!Impl::instance)
		Impl::instance = new SessionManager();
	Impl::initLock.unlock();
	return Impl::instance;
}

string SessionManager::create(const UserIdType &userId, const size_t &timeout)
{
	Session *session = new Session();
	session->userId = userId;
	session->id = generateSessionId();
	session->sessionMgr = this;
	if (timeout == DEFAULT_TIMEOUT)
		session->timeout =  m_impl->defaultTimeout;
	else
		session->timeout = timeout;
	updateTimer(session);
	return session->id;
}

SessionPtr SessionManager::getSession(const string &sessionId)
{
	Session *session = NULL;
	m_impl->rwlock.readLock();
	SessionIdMapIterator it = m_impl->sessionIdMap.find(sessionId);
	if (it != m_impl->sessionIdMap.end())
		session = it->second;

	// Making sessionPtr inside the lock is important. It icrements the
	// used counter. Even if timerCb() is called back on an other thread
	// soon after the following rwlock.unlock(), the instance itself
	// is not deleted.
	SessionPtr sessionPtr(session);

	m_impl->rwlock.unlock();

	// Update the timer on the GLib event loop so that updateTimer() and
	// timerCb() can be executed exclusively.
	if (session)
		Utils::executeOnGLibEventLoop<Session>(updateTimer, session);

	return sessionPtr;
}

bool SessionManager::remove(const string &sessionId)
{
	Session *session = NULL;
	m_impl->rwlock.writeLock();
	SessionIdMapIterator it = m_impl->sessionIdMap.find(sessionId);
	if (it != m_impl->sessionIdMap.end()) {
		session = it->second;
		m_impl->sessionIdMap.erase(it);
	}
	m_impl->rwlock.unlock();
	if (!session)
		return false;
	session->unref();
	return true;
}

const SessionIdMap &SessionManager::getSessionIdMap(void)
{
	m_impl->rwlock.readLock();
	return m_impl->sessionIdMap;
}

void SessionManager::releaseSessionIdMap(void)
{
	m_impl->rwlock.unlock();
}

const size_t SessionManager::getDefaultTimeout(void)
{
	return Impl::defaultTimeout;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SessionManager::SessionManager(void)
: m_impl(new Impl())
{
}

SessionManager::~SessionManager()
{
}

string SessionManager::generateSessionId(void)
{
	uuid_t sessionUuid;
	uuid_generate(sessionUuid);
	static const size_t uuidBufSize = SESSION_ID_LEN + 1; // + NULL term.
	char uuidBuf[uuidBufSize];
	uuid_unparse(sessionUuid, uuidBuf);
	string sessionId = uuidBuf;
	return sessionId;
}

void SessionManager::updateTimer(Session *session)
{
	Impl *impl = session->sessionMgr->m_impl.get();
	Utils::removeGSourceIfNeeded(session->timerId);

	if (session->timeout) {
		session->timerId = g_timeout_add(session->timeout * 1000,
		                                 timerCb, session);
	}
	session->lastAccessTime.setCurrTime();

	// If timerCb() is called between 'm_impl->rwlock.unlock()' and
	// this function running on another thread, the session is
	// removed from sessionIdMap. So we have to insert session
	// into sessionIdMap every time.
	impl->rwlock.writeLock();
	impl->sessionIdMap[session->id] = session;
	impl->rwlock.unlock();
};

gboolean SessionManager::timerCb(gpointer data)
{
	Session *session = static_cast<Session *>(data);
	SessionManager *sessionMgr = session->sessionMgr;
	if (!sessionMgr) {
		MLPL_BUG("Session Manager: NULL, %s\n", session->id.c_str());
		return G_SOURCE_REMOVE;
	}
	session->timerId = INVALID_EVENT_ID;

	// Make a copy, because session may be deleted in the follwoing
	// remove(). However, it is used only when the remove() fails, 
	// which is a rare event. So this is a kind of wasted way.
	string sessionId = session->id;

	if (!sessionMgr->remove(session->id))
		MLPL_BUG("Failed to remove session: %s\n", sessionId.c_str());
	return G_SOURCE_REMOVE;
}
