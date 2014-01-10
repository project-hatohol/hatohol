/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstdio>
#include <map>
#include <uuid/uuid.h>
#include "SessionManager.h"
#include "MutexLock.h"
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
const size_t SessionManager::NO_TIMEOUT = 0;
const char * SessionManager::ENV_NAME_TIMEOUT = "HATOHOL_SESSION_TIMEOUT";

size_t SessionManager::m_defaultTimeout = INITIAL_TIMEOUT;

struct SessionManager::PrivateContext {
	static MutexLock initLock;
	static SessionManager *instance;

	ReadWriteLock rwlock;
	SessionIdMap  sessionIdMap;

	virtual ~PrivateContext() {
		rwlock.writeLock();
		SessionIdMapIterator it = sessionIdMap.begin();
		for (; it != sessionIdMap.end(); ++it) {
			Session *session = it->second;
			session->unref();
		}
		sessionIdMap.clear();
		rwlock.unlock();
	}
};

SessionManager *SessionManager::PrivateContext::instance = NULL;
MutexLock SessionManager::PrivateContext::initLock;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SessionManager::reset(void)
{
	if (PrivateContext::instance) {
		delete PrivateContext::instance;
		PrivateContext::instance = NULL;
	}

	m_defaultTimeout = INITIAL_TIMEOUT;
	char *env = getenv(ENV_NAME_TIMEOUT);
	if (env) {
		size_t timeout = 0;
		if (sscanf(env, "%zd", &timeout) != 1) {
			MLPL_ERR("Invalid value: %s. Use the default timeout\n",
			         env);
		} else {
			m_defaultTimeout = timeout;
		}
		MLPL_INFO("Default session timeout: %zd (sec)\n",
		          m_defaultTimeout);
	}
}

SessionManager *SessionManager::getInstance(void)
{
	PrivateContext::initLock.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new SessionManager();
	PrivateContext::initLock.unlock();
	return PrivateContext::instance;
}

string SessionManager::create(const UserIdType &userId, const size_t &timeout)
{
	Session *session = new Session();
	session->userId = userId;
	session->id = generateSessionId();
	session->sessionMgr = this;
	session->timeout = timeout;
	updateTimer(session);
	return session->id;
}

SessionPtr SessionManager::getSession(const string &sessionId)
{
	Session *session = NULL;
	m_ctx->rwlock.readLock();
	SessionIdMapIterator it = m_ctx->sessionIdMap.find(sessionId);
	if (it != m_ctx->sessionIdMap.end())
		session = it->second;

	// Making sessionPtr inside the lock is important. It icrements the
	// used counter. Even if timerCb() is called back on an other thread
	// soon after the following rwlock.unlock(), the instance itself
	// is not deleted.
	SessionPtr sessionPtr(session);

	m_ctx->rwlock.unlock();

	// Update the timer on the GLib event loop so that updateTimer() and
	// timerCb() can be executed exclusively.
	if (session)
		Utils::executeOnGLibEventLoop<Session>(updateTimer, session);

	return sessionPtr;
}

bool SessionManager::remove(const string &sessionId)
{
	Session *session = NULL;
	m_ctx->rwlock.writeLock();
	SessionIdMapIterator it = m_ctx->sessionIdMap.find(sessionId);
	if (it != m_ctx->sessionIdMap.end()) {
		session = it->second;
		m_ctx->sessionIdMap.erase(it);
	}
	m_ctx->rwlock.unlock();
	if (!session)
		return false;
	session->unref();
	return true;
}

const SessionIdMap &SessionManager::getSessionIdMap(void)
{
	m_ctx->rwlock.readLock();
	return m_ctx->sessionIdMap;
}

void SessionManager::releaseSessionIdMap(void)
{
	m_ctx->rwlock.unlock();
}

const size_t SessionManager::getDefaultTimeout(void)
{
	return m_defaultTimeout;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SessionManager::SessionManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

SessionManager::~SessionManager()
{
	if (m_ctx)
		delete m_ctx;
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
	PrivateContext *ctx = session->sessionMgr->m_ctx;
	Utils::removeGSourceIfNeeded(session->timerId);

	if (session->timeout) {
		session->timerId = g_timeout_add(session->timeout,
		                                 timerCb, session);
	}
	session->lastAccessTime.setCurrTime();

	// If timerCb() is called between 'm_ctx->rwlock.unlock()' and
	// this function running on another thread, the session is
	// removed from sessionIdMap. So we have to insert session
	// into sessionIdMap every time.
	ctx->rwlock.writeLock();
	ctx->sessionIdMap[session->id] = session;
	ctx->rwlock.unlock();
};

gboolean SessionManager::timerCb(gpointer data)
{
	Session *session = static_cast<Session *>(data);
	SessionManager *sessionMgr = session->sessionMgr;
	if (!sessionMgr->remove(session->id))
		MLPL_BUG("Failed to remove session: %s\n", session->id.c_str());
	session->timerId = INVALID_EVENT_ID;
	return G_SOURCE_REMOVE;
}
