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

#ifndef DBClientUser_h
#define DBClientUser_h

#include <map>
#include "DBClient.h"

enum
{
	USER_FLAG_ADMIN = (1 << 0),
};

struct UserInfo {
	UserIdType id;
	string name;
	string password;
	uint32_t flags;
};
static const UserIdType INVALID_USER_ID = -1;
static const int USER_ID_ADMIN = 0;

struct AccessInfo {
	int id;
	UserIdType userId;
	uint32_t serverId;
	uint64_t hostGroupId;
};

typedef std::map<uint64_t, AccessInfo *> AccessInfoHGMap;
typedef AccessInfoHGMap::iterator        AccessInfoHGMapIterator;
typedef AccessInfoHGMap::const_iterator  AccessInfoHGMapConstIterator;

struct AccessInfoMap : std::map<uint32_t, AccessInfoHGMap *> {
	virtual ~AccessInfoMap();
};
typedef AccessInfoMap::iterator               AccessInfoMapIterator;;
typedef AccessInfoMap::const_iterator         AccessInfoMapConstIterator;;

class DBClientUser : public DBClient {
public:
	static const int   USER_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static const char *TABLE_NAME_USERS;
	static const char *TABLE_NAME_ACCESS_LIST;
	static void init(void);
	static void reset(void);

	DBClientUser(void);
	virtual ~DBClientUser();

	/**
	 * Add user information to DB.
	 *
	 * @param userInfo
	 * A UserInfo instance that has parameters to be stored.
	 */
	void addUserInfo(UserInfo &userInfo);

	/**
	 * Get the user Id.
	 *
	 * @param user A user name.
	 * @param password A password.
	 *
	 * @return
	 * A user ID if authentification is successed.
	 * Otherwise INVALID_USER_ID is returned.
	 */
	UserIdType getUserId(const string &user, const string &password);

	/**
	 * Add an access list element.
	 *
	 * @param accessInfo 
	 * An AccessInfo instance that has parameters to be stored.
	 */
	void addAccessInfo(AccessInfo &accessInfo);

	bool getUserInfo(UserInfo &userInfo, const UserIdType userId);

	/**
	 * Make a map that has the AccessInfo instances for user with userId.
	 *
	 * @param accessInfoMap An AccessInfoMap instance.
	 * @param userId a user ID.
	 */
	void getAccessInfoMap(AccessInfoMap &accessInfoMap,
	                      const UserIdType userId);

protected:

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientUser_h
