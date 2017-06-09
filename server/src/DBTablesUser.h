/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include <list>
#include <map>
#include "DBTables.h"
#include "DataQueryOption.h"
#include "OperationPrivilege.h"
#include "HatoholError.h"

struct UserInfo {
	UserIdType id;
	std::string name;
	std::string password;
	OperationPrivilegeFlag flags;

	bool operator==(const UserInfo &u) {
		return id == u.id &&
		       name == u.name &&
		       password == u.password &&
		       flags == u.flags;
	}
};

typedef std::list<UserInfo>          UserInfoList;
typedef UserInfoList::iterator       UserInfoListIterator;
typedef UserInfoList::const_iterator UserInfoListConstIterator;

struct AccessInfo {
	AccessInfoIdType id;
	UserIdType userId;
	ServerIdType serverId;
	HostgroupIdType hostgroupId;
};

typedef std::list<AccessInfo>          AccessInfoList;
typedef AccessInfoList::iterator       AccessInfoIterator;
typedef AccessInfoList::const_iterator AccessInfoConstIterator;

typedef std::map<HostgroupIdType, AccessInfo *>      HostGrpAccessInfoMap;
typedef HostGrpAccessInfoMap::iterator        HostGrpAccessInfoMapIterator;
typedef HostGrpAccessInfoMap::const_iterator  HostGrpAccessInfoMapConstIterator;

struct ServerAccessInfoMap : std::map<ServerIdType, HostGrpAccessInfoMap *> {
	virtual ~ServerAccessInfoMap();
};
typedef ServerAccessInfoMap::iterator         ServerAccessInfoMapIterator;
typedef ServerAccessInfoMap::const_iterator   ServerAccessInfoMapConstIterator;

struct UserRoleInfo {
	UserRoleIdType id;
	std::string name;
	OperationPrivilegeFlag flags;
};

typedef std::list<UserRoleInfo>          UserRoleInfoList;
typedef UserRoleInfoList::iterator       UserRoleInfoListIterator;
typedef UserRoleInfoList::const_iterator UserRoleInfoListConstIterator;

class UserQueryOption : public DataQueryOption {
public:
	UserQueryOption(UserIdType userId = INVALID_USER_ID);
	UserQueryOption(DataQueryContext *dataQueryContext);
	virtual ~UserQueryOption();

	HatoholError setTargetName(const std::string &name);
	OperationPrivilegeFlag getPrivilegesFlag(void) const;
	void                   setPrivilegesFlag(const OperationPrivilegeFlag flags);
	void                   unsetPrivilegesFlag(void);
	void         queryOnlyMyself(void);

	virtual std::string getCondition(void) const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class AccessInfoQueryOption : public DataQueryOption {
public:
	AccessInfoQueryOption(UserIdType userId = INVALID_USER_ID);
	AccessInfoQueryOption(DataQueryContext *dataQueryContext);
	virtual ~AccessInfoQueryOption();

	virtual std::string getCondition(void) const override;

	void setTargetUserId(UserIdType userId);
	UserIdType getTargetUserId(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class UserRoleQueryOption : public DataQueryOption {
public:
	UserRoleQueryOption(UserIdType userId = INVALID_USER_ID);
	UserRoleQueryOption(DataQueryContext *dataQueryContext);
	virtual ~UserRoleQueryOption();

	void setTargetUserRoleId(UserRoleIdType userRoleId);
        UserRoleIdType getTargetUserRoleId(void) const;

	virtual std::string getCondition(void) const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class DBTablesUser : public DBTables {
public:
	static const int   USER_DB_VERSION;
	static const char *TABLE_NAME_USERS;
	static const char *TABLE_NAME_ACCESS_LIST;
	static const char *TABLE_NAME_USER_ROLES;
	static const size_t MAX_USER_NAME_LENGTH;
	static const size_t MAX_PASSWORD_LENGTH;
	static const size_t MAX_USER_ROLE_NAME_LENGTH;
	static void init(void);
	static void reset(void);
	static const SetupInfo &getConstSetupInfo(void);

	DBTablesUser(DBAgent &dbAgent);
	virtual ~DBTablesUser();

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

	HatoholError updateUserInfo(UserInfo &userInfo,
	                            const OperationPrivilege &privilege);

	HatoholError updateUserInfoFlags(OperationPrivilegeFlag &oldUserFlag,
	                                 OperationPrivilegeFlag &updateUserFlag,
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
	UserIdType getUserId(const std::string &user,
	                     const std::string &password);

	/**
	 * Add an access list element.
	 *
	 * @param accessInfo
	 * An AccessInfo instance that has parameters to be stored.
	 */
	HatoholError addAccessInfo(AccessInfo &accessInfo,
	                           const OperationPrivilege &privilege);
	HatoholError deleteAccessInfo(AccessInfoIdType id,
	                              const OperationPrivilege &privilege);

	/**
	 * Get a UserInfo for the specified user ID.
	 *
	 * @param userInfo
	 * A reference of a UserInfo structure to which obtained information
	 * is returned.
	 *
	 * @param userId A user ID to be queried.
	 *
	 * @return true if the specified user is found and the UserInfo
	 * structure is filled. Otherwise false is returned.
	 */
	bool getUserInfo(UserInfo &userInfo, const UserIdType userId);

	/**
	 * Get a list of UserInfo instances.
	 *
	 * @param userInfoList The found results are added to this reference.
	 * @param option
	 * A UserQueryOption instance.
	 * If OPPRVLG_GET_ALL_USERS is not set or queryOnlyMyself() is called,
	 * the search target is only the user set in this parameter.
	 * In addtion, setTargetName() is called and the name is set,
	 * this method searches only that.
	 */
	void getUserInfoList(UserInfoList &userInfoList,
	                     const UserQueryOption &option);

	/**
	 * Make a map that has the ServerAccessInfo instances for
	 * the user with userId.
	 *
	 * @param srvAccessInfoMap An ServerAccessInfoMap instance.
	 * @param option A AccessInfoQueryOption instance.
	 */
	HatoholError getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
	                              const AccessInfoQueryOption &option);
	static void destroyServerAccessInfoMap(
	  ServerAccessInfoMap &srvAccessInfoMap);

	/**
	 * Gather server IDs and host group IDs for the user with userId.
	 *
	 * @param srvHostGrpMap
	 * A ServerHostGrpSetMap instance in which the obtained data is stored.
	 * @param userId a user ID.
	 */
	void getServerHostGrpSetMap(ServerHostGrpSetMap &srvHostGrpSetMap,
	                            const UserIdType &userId);

	HatoholError addUserRoleInfo(UserRoleInfo &userRoleInfo,
	                             const OperationPrivilege &privilege);
	HatoholError updateUserRoleInfo(UserRoleInfo &userRoleInfo,
					const OperationPrivilege &privilege);
	HatoholError deleteUserRoleInfo(const UserRoleIdType userRoleId,
					const OperationPrivilege &privilege);
	void getUserRoleInfoList(UserRoleInfoList &userRoleInfoList,
	                         const UserRoleQueryOption &option);

	static HatoholError isValidUserName(const std::string &name);
	static HatoholError isValidPassword(const std::string &password);
	static HatoholError isValidFlags(const OperationPrivilegeFlag flags);
	static HatoholError isValidUserRoleName(const std::string &name);

	bool isAccessible(const ServerIdType &serverId,
	                  const OperationPrivilege &privilege,
	                  const bool &useTransaction = true);

	bool isAccessible(const ServerIdType &serverId,
	                  const HostgroupIdType &hostgroupId,
	                  const OperationPrivilege &privilege,
	                  const bool &useTransaction = true);

	void getUserIdSet(UserIdSet &userIdSet);

protected:
	static SetupInfo &getSetupInfo(void);
	void getUserInfoList(UserInfoList &userInfoList,
	                     const std::string &condition);
	HatoholError hasPrivilegeForUpdateUserInfo(
	  UserInfo &userInfo, const OperationPrivilege &privilege);

	bool isAccessible(
	  const OperationPrivilege &privilege, const std::string &condition,
	  const bool &useTransaction);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

