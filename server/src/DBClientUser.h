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

#include <list>
#include <map>
#include "DBClient.h"
#include "DataQueryOption.h"
#include "OperationPrivilege.h"
#include "HatoholError.h"

struct UserInfo {
	UserIdType id;
	string name;
	string password;
	OperationPrivilegeFlag flags;
};

typedef std::list<UserInfo>          UserInfoList;
typedef UserInfoList::iterator       UserInfoListIterator;
typedef UserInfoList::const_iterator UserInfoListConstIterator;

struct AccessInfo {
	int id;
	UserIdType userId;
	uint32_t serverId;
	uint64_t hostGroupId;
};

typedef std::list<AccessInfo>          AccessInfoList;
typedef AccessInfoList::iterator       AccessInfoIterator;
typedef AccessInfoList::const_iterator AccessInfoConstIterator;

typedef std::map<uint64_t, AccessInfo *>      HostGrpAccessInfoMap;
typedef HostGrpAccessInfoMap::iterator        HostGrpAccessInfoMapIterator;
typedef HostGrpAccessInfoMap::const_iterator  HostGrpAccessInfoMapConstIterator;

struct ServerAccessInfoMap : std::map<uint32_t, HostGrpAccessInfoMap *> {
	virtual ~ServerAccessInfoMap();
};
typedef ServerAccessInfoMap::iterator         ServerAccessInfoMapIterator;
typedef ServerAccessInfoMap::const_iterator   ServerAccessInfoMapConstIterator;

typedef std::set<uint64_t>                  HostGroupSet;
typedef HostGroupSet::iterator              HostGroupSetIterator;
typedef HostGroupSet::const_iterator        HostGroupSetConstIterator;

typedef std::map<uint32_t, HostGroupSet>    ServerHostGrpSetMap;
typedef ServerHostGrpSetMap::iterator       ServerHostGrpSetMapIterator;
typedef ServerHostGrpSetMap::const_iterator ServerHostGrpSetMapConstIterator;

class DBClientUser : public DBClient {
public:
	static const int   USER_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static const char *TABLE_NAME_USERS;
	static const char *TABLE_NAME_ACCESS_LIST;
	static const size_t MAX_USER_NAME_LENGTH;
	static const size_t MAX_PASSWORD_LENGTH;
	static void init(void);
	static void reset(void);

	DBClientUser(void);
	virtual ~DBClientUser();

	/**
	 * Add user information to DB.
	 *
	 * @param userInfo
	 * A UserInfo instance that has parameters to be stored.
	 *
	 * @param privilege
	 * An OperationPrivilege instance.
	 *
	 * @return An error code.
	 */
	HatoholError addUserInfo(UserInfo &userInfo,
	                             const OperationPrivilege &privilege);

	HatoholError deleteUserInfo(const UserIdType userId,
	                                const OperationPrivilege &privilege);

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
	void getUserInfoList(UserInfoList &userInfoList,
	                     DataQueryOption &option);

	/**
	 * Make a map that has the ServerAccessInfo instances for
	 * the user with userId.
	 *
	 * @param srvAccessInfoMap An ServerAccessInfoMap instance.
	 * @param userId a user ID.
	 */
	void getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
	                      const UserIdType userId);

	/**
	 * Gather server IDs and host group IDs for the user with userId.
	 *
	 * @param srvHostGrpMap
	 * A ServerHostGrpSetMap instance in which the obtained data is stored.
	 * @param userId a user ID.
	 */
	void getServerHostGrpSetMap(ServerHostGrpSetMap &srvHostGrpSetMap,
	                            const UserIdType userId);

	static HatoholError isValidUserName(const string &name);
	static HatoholError isValidPassword(const string &password);
	static HatoholError isValidFlags(
	                           const OperationPrivilegeFlag flags);

protected:
	void getUserInfoList(UserInfoList &userInfoList,
	                     const string &condition);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientUser_h
