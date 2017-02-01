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

#pragma once
#include <string>
#include <memory>
#include <map>
#include "Params.h"
#include "SmartTime.h"
#include "UsedCountablePtr.h"
#include "UsedCountable.h"
#include "Mutex.h"

class SessionManager;
struct Session : public UsedCountable {
	UserIdType userId;
	std::string id;
	mlpl::SmartTime loginTime;
	mlpl::SmartTime lastAccessTime;
	size_t timeout;
	guint timerId;
	mlpl::Mutex lock;
	SessionManager *sessionMgr;

	// constructor
	Session(void);

	/**
	 * Cancel the timer. This method is suppose to be called with
	 * lock taken.
	 */
	void cancelTimer(void);

protected:
	virtual ~Session(); // makes delete impossible. Use unref().
};

// Key: session ID, value: user ID
typedef std::map<std::string, Session *>           SessionIdMap;
typedef std::map<std::string, Session *>::iterator SessionIdMapIterator;
typedef std::map<std::string, Session *>::const_iterator
   SessionIdMapConstIterator;

typedef UsedCountablePtr<Session> SessionPtr;

class SessionManager {
public:
	static const size_t SESSION_ID_LEN;
	static const size_t INITIAL_TIMEOUT;
	static const size_t DEFAULT_TIMEOUT;
	static const size_t NO_TIMEOUT;
	static const char * ENV_NAME_TIMEOUT;

	static void reset(void);
	static SessionManager *getInstance(void);

	/**
	 * Create a new session ID.
	 *
	 * @param userId A user ID
	 * @param timeout
	 * A timeout of the session in second. If this parameter is
	 * NO_TIMEOUT, the session is never timed out.
	 *
	 * @return       A newly created session ID.
	 */
	std::string create(const UserIdType &userId,
	                   const size_t &timeout = DEFAULT_TIMEOUT);

	/**
	 * Get the session instance associated with the given sessionId.
	 *
	 * @param A session ID.
	 * @return
	 * A SessionPtr instance is returned if the session is successfully
	 * obtained. The instance is wrapped in a smart pointer. You don't have
	 * to call unref() explicitly. If the sessionId is invalid,
	 * SessionPtr::hasData() will return false.
	 */
	SessionPtr getSession(const std::string &sessionId);

	/**
	 * Remove the session instance associated with the given sessionId.
	 *
	 * @param A session ID.
	 * @return true If the session is found and successfully removed.
	 * Otherwise false will be returned.
	 */
	bool remove(const std::string &sessionId);

	/**
	 * Get a reference of the seesion ID map.
	 *
	 * After the returned map is used, the caller must call
	 * releaseSessionIdMap(). Until it is called, some operations
	 * of this class are blocked.
	 *
	 * @return a reference of the session ID map.
	 */
	const SessionIdMap &getSessionIdMap(void);
	void releaseSessionIdMap(void);

	static const size_t getDefaultTimeout(void);

protected:
	SessionManager(void);
	virtual ~SessionManager();

	static std::string generateSessionId(void);
	static void updateTimer(Session *session);
	static gboolean timerCb(gpointer data);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

