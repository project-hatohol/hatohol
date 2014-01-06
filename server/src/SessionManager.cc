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
	g_source_remove(timerId);
}

// ---------------------------------------------------------------------------
// SessionManager
// ---------------------------------------------------------------------------

// Ref. man uuid_unparse.
const size_t SessionManager::SESSION_ID_LEN = 36;

const size_t SessionManager::DEFAULT_TIMEOUT = 10 * 60 * 1000; // 10 min.

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
	if (timeout)
		session->timerId = g_timeout_add(timeout, timerCb, session);
	session->sessionMgr = this;
	m_ctx->rwlock.writeLock();
	m_ctx->sessionIdMap[session->id] = session;
	m_ctx->rwlock.unlock();
	return session->id;
}

SessionPtr SessionManager::getSession(const string &sessionId)
{
	Session *session = NULL;
	m_ctx->rwlock.readLock();
	SessionIdMapIterator it = m_ctx->sessionIdMap.find(sessionId);
	if (it != m_ctx->sessionIdMap.end())
		session = it->second;
	m_ctx->rwlock.unlock();
	return SessionPtr(session);
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
	// This is unsafe. Do unref on GLib event loop.
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

gboolean SessionManager::timerCb(gpointer data)
{
	Session *session = static_cast<Session *>(data);
	SessionManager *sessionMgr = session->sessionMgr;
	if (!sessionMgr->remove(session->id))
		MLPL_BUG("Failed to remove session: %s\n", session->id.c_str());
	return G_SOURCE_REMOVE;
}
