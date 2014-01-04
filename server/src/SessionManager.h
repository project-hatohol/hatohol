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

#ifndef SessionManager_h
#define SessionManager_h

#include <string>
#include <map>
#include "Params.h"
#include "SmartTime.h"

struct Session {
	UserIdType userId;
	mlpl::SmartTime loginTime;
	mlpl::SmartTime lastAccessTime;

	// constructor
	Session(void);
	~Session();
};

// Key: session ID, value: user ID
typedef std::map<std::string, Session *>           SessionIdMap;
typedef std::map<std::string, Session *>::iterator SessionIdMapIterator;
typedef std::map<std::string, Session *>::const_iterator
   SessionIdMapConstIterator;

class SessionManager {
public:
	static SessionManager *getInstance(void);

	/**
	 * Create a new session ID.
	 *
	 * @param userId A user ID
	 * @return       A newly created session ID.
	 */
	std::string create(const UserIdType &userId);

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

protected:
	SessionManager(void);
	virtual ~SessionManager();

	static std::string generateSessionId(void);

protected:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // SessionManager_h
