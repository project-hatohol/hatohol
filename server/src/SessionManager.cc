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
  lastAccessTime(SmartTime::INIT_CURR_TIME)
{
}

Session::~Session()
{
}

// ---------------------------------------------------------------------------
// SessionManager
// ---------------------------------------------------------------------------

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

string SessionManager::create(const UserIdType &userId)
{
	Session *sessionInfo = new Session();
	sessionInfo->userId = userId;
	string sessionId = generateSessionId();
	m_ctx->rwlock.writeLock();
	m_ctx->sessionIdMap[sessionId] = sessionInfo;
	m_ctx->rwlock.unlock();
	return sessionId;
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
	static const size_t uuidBufSize = 37;
	char uuidBuf[uuidBufSize];
	uuid_unparse(sessionUuid, uuidBuf);
	string sessionId = uuidBuf;
	return sessionId;
}

