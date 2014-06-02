/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <stdint.h>
#include <cppcutter.h>
#include <cutter.h>
#include "Helpers.h"
#include "DBClientUser.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "CacheServiceDBClient.h"
using namespace std;
using namespace mlpl;

namespace testDBClientUser {

static void _assertUserInfo(const UserInfo &expect, const UserInfo &actual)
{
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.name, actual.name);
	cppcut_assert_equal(Utils::sha256(expect.password), actual.password);
	cppcut_assert_equal(expect.flags, actual.flags);
}
#define assertUserInfo(E,A) cut_trace(_assertUserInfo(E,A))

void _assertUserInfoInDB(UserInfo &userInfo) 
{
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, userInfo.id);
	string expect =
	  StringUtils::sprintf(
	    "%" FMT_USER_ID "|%s|%s|%" PRIu64 "\n",
	    userInfo.id, userInfo.name.c_str(),
	    Utils::sha256(userInfo.password).c_str(), userInfo.flags);
	DBClientUser dbUser;
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}
#define assertUserInfoInDB(I) cut_trace(_assertUserInfoInDB(I))

static size_t countServerAccessInfoMapElements(
  const ServerAccessInfoMap &srvServerAccessInfoMap)
{
	size_t num = 0;
	ServerAccessInfoMapConstIterator it =
	  srvServerAccessInfoMap.begin();
	for (; it != srvServerAccessInfoMap.end(); ++it) {
		const HostGrpAccessInfoMap
		  *hostGrpAccessInfoMap = it->second;
		num += hostGrpAccessInfoMap->size();
	}
	return num;
}

static size_t countServerHostGrpSetMapElements(
  const ServerHostGrpSetMap &srvHostGrpSetMap)
{
	size_t num = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const HostgroupIdSet &hostgroupIdSet = it->second;
		num += hostgroupIdSet.size();
	}
	return num;
}

static void _assertAccessInfo(const AccessInfo &expect,
                              const AccessInfo &actual)
{
	cppcut_assert_equal(expect.userId, actual.userId);
	cppcut_assert_equal(expect.serverId, actual.serverId);
	cppcut_assert_equal(expect.hostgroupId, actual.hostgroupId);
}
#define assertAccessInfo(E,A) cut_trace(_assertAccessInfo(E,A))

static void _assertServerAccessInfoMap(
  const set<int> &expectIdxSet, const ServerAccessInfoMap &srvAccessInfoMap)
{
	// check total number
	cppcut_assert_equal(expectIdxSet.size(),
	                    countServerAccessInfoMapElements(srvAccessInfoMap));

	// check each element
	set<int>::const_iterator it = expectIdxSet.begin();
	for (; it != expectIdxSet.end(); ++it) {
		const AccessInfo &expectAccessInfo = testAccessInfo[*it];
		ServerAccessInfoMapConstIterator jt = 
		  srvAccessInfoMap.find(expectAccessInfo.serverId);
		cppcut_assert_equal(true, jt != srvAccessInfoMap.end(),
		                    cut_message("Failed to lookup: %" PRIu32,
		                                expectAccessInfo.serverId));
		const HostGrpAccessInfoMap *hostGrpAccessInfoMap = jt->second;
		HostGrpAccessInfoMapConstIterator kt =
		   hostGrpAccessInfoMap->find(expectAccessInfo.hostgroupId);
		cppcut_assert_equal(true, kt != hostGrpAccessInfoMap->end(),
		                    cut_message("Failed to lookup: %" PRIu64,
		                                expectAccessInfo.hostgroupId));
		const AccessInfo *actualAccessInfo = kt->second;
		assertAccessInfo(expectAccessInfo, *actualAccessInfo);
	}
}
#define assertServerAccessInfoMap(E,A) cut_trace(_assertServerAccessInfoMap(E,A))

static void _assertServerHostGrpSetMap(
  const set<int> &expectIdxSet, const ServerHostGrpSetMap &srvHostGrpSetMap)
{
	// check total number
	cppcut_assert_equal(expectIdxSet.size(),
	                    countServerHostGrpSetMapElements(srvHostGrpSetMap));

	// check each element
	set<int>::const_iterator it = expectIdxSet.begin();
	for (; it != expectIdxSet.end(); ++it) {
		const AccessInfo &expectAccessInfo = testAccessInfo[*it];
		ServerHostGrpSetMapConstIterator jt = 
		  srvHostGrpSetMap.find(expectAccessInfo.serverId);
		cppcut_assert_equal(true, jt != srvHostGrpSetMap.end(),
		                    cut_message("Failed to lookup: %" PRIu32,
		                                expectAccessInfo.serverId));
		const HostgroupIdSet &hostgroupIdSet = jt->second;
		HostgroupIdSetConstIterator kt =
		   hostgroupIdSet.find(expectAccessInfo.hostgroupId);
		cppcut_assert_equal(true, kt != hostgroupIdSet.end(),
		                    cut_message("Failed to lookup: %" PRIu64,
		                                expectAccessInfo.hostgroupId));
	}
}
#define assertServerHostGrpSetMap(E,A) \
cut_trace(_assertServerHostGrpSetMap(E,A))

static const UserInfo findFirstTestUserInfoByFlag(
  const OperationPrivilegeFlag flags)
{
	UserInfo retUserInfo;
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		const UserInfo &userInfo = testUserInfo[i];
		if (userInfo.flags == flags) {
			retUserInfo = userInfo;
			retUserInfo.id = i + 1;
			return retUserInfo;
		}
	}
	cut_fail("Failed to find the user with flags: %" FMT_OPPRVLG, flags);
	return retUserInfo;
}

static void _assertGetUserInfo(const OperationPrivilegeFlag flags,
                               const size_t expectedNumUser,
                               const bool expectAllUsers)
{
	loadTestDBUser();
	UserInfoList userInfoList;
	UserQueryOption option;
	const UserInfo userInfo = findFirstTestUserInfoByFlag(flags);
	option.setUserId(userInfo.id);
	DBClientUser dbUser;
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(expectedNumUser, userInfoList.size());
	UserInfoListIterator it = userInfoList.begin();
	if (expectAllUsers) {
		for (size_t i = 0; i < NumTestUserInfo; i++, ++it) {
			UserInfo &userInfo = *it;
			userInfo.id = i + 1;
			assertUserInfo(testUserInfo[i], userInfo);
		}
	} else {
		assertUserInfo(userInfo, *it);
	}
}
#define assertGetUserInfo(F,N,A) cut_trace(_assertGetUserInfo(F,N,A))

void _assertGetUserInfoListWithTargetName(
  const OperationPrivilegeFlag callerFlags,
  const size_t expectNumList, const bool searchMySelf = true)
{
	loadTestDBUser();

	// search the user who has all privileges.
	size_t index = 1;
	for (; index < NumTestUserInfo; index++) {
		if (testUserInfo[index].flags == callerFlags)
			break;
	}
	cppcut_assert_not_equal(index, NumTestUserInfo);
	const size_t userId = index + 1;
	if (!searchMySelf)
		index -= 1;
	const UserInfo &targetUserInfo = testUserInfo[index];

	UserQueryOption option(userId);
	option.setTargetName(targetUserInfo.name);

	UserInfoList userInfoList;
	DBClientUser dbUser;
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(expectNumList, userInfoList.size());
	if (expectNumList == 0)
		return;
	assertUserInfo(targetUserInfo, *userInfoList.begin());
}
#define assertGetUserInfoListWithTargetName(F,N, ...) \
cut_trace(_assertGetUserInfoListWithTargetName(F,N, ##__VA_ARGS__))

void _assertIsAccessible(const bool useAllServers = false)
{
	loadTestDBUser();
	loadTestDBAccessList();
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();

	// search the User ID and Server ID
	ServerIdType serverId = 0;
	UserIdType userId = INVALID_USER_ID;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo &accessInfo = testAccessInfo[i];
		if (accessInfo.hostgroupId != ALL_HOST_GROUPS)
			continue;

		if (useAllServers) {
			if (accessInfo.serverId != ALL_SERVERS)
				continue;
		} else {
			if (accessInfo.serverId == ALL_SERVERS)
				continue;
		}
		userId = accessInfo.userId;
		serverId = accessInfo.serverId;
		break;
	}

	cppcut_assert_not_equal(INVALID_USER_ID, userId);

	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(true, dbUser->isAccessible(serverId, privilege));

	// invalid user
	privilege.setUserId(INVALID_USER_ID);
	cppcut_assert_equal(false, dbUser->isAccessible(serverId, privilege));
}
#define assertIsAccessible(...) cut_trace(_assertIsAccessible(__VA_ARGS__))

static void _assertGetUserRoleInfo(
  const OperationPrivilegeFlag flags,
  UserRoleIdType targetUserRoleId = INVALID_USER_ROLE_ID)
{
	loadTestDBUser();
	loadTestDBUserRole();
	UserRoleInfoList userRoleInfoList;
	UserRoleQueryOption option;
	const UserInfo userInfo = findFirstTestUserInfoByFlag(flags);
	option.setUserId(userInfo.id);
	if (targetUserRoleId != INVALID_USER_ROLE_ID)
		option.setTargetUserRoleId(targetUserRoleId);
	DBClientUser dbUser;
	dbUser.getUserRoleInfoList(userRoleInfoList, option);
	string expected, actual;
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		userRoleInfo.id = i + 1;
		if (targetUserRoleId == INVALID_USER_ROLE_ID ||
		    userRoleInfo.id == targetUserRoleId)
		{
			expected += makeUserRoleInfoOutput(userRoleInfo);
		}
	}
	UserRoleInfoListIterator it = userRoleInfoList.begin();
	for (; it != userRoleInfoList.end(); ++it) {
		UserRoleInfo &userRoleInfo = *it;
		actual += makeUserRoleInfoOutput(userRoleInfo);
	}
	cppcut_assert_equal(expected, actual);
}
#define assertGetUserRoleInfo(F, ...) \
  cut_trace(_assertGetUserRoleInfo(F, ##__VA_ARGS__))

static UserInfo setupForUpdate(size_t targetIndex = 1)
{
	loadTestDBUser();
	UserInfo userInfo = testUserInfo[targetIndex];
	userInfo.id = targetIndex + 1;
	userInfo.password = ">=_=<3";
	userInfo.flags = 0;
	return userInfo;
}

static UserRoleInfo setupUserRoleInfoForUpdate(size_t targetIndex = 1)
{
	loadTestDBUserRole();
	UserRoleInfo userRoleInfo = testUserRoleInfo[targetIndex];
	userRoleInfo.id = targetIndex + 1;
	userRoleInfo.name = ">=_=<3";
	userRoleInfo.flags =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER);
	return userRoleInfo;
}

static void setupWithUserIdIndexMap(UserIdIndexMap &userIdIndexMap)
{
	loadTestDBAccessList();
	makeTestUserIdIndexMap(userIdIndexMap);
}

template <class T> static void _assertQueryOptionFromDataQueryContext(void)
{
	DataQueryContextPtr dqCtxPtr =
	  DataQueryContextPtr(new DataQueryContext(USER_ID_SYSTEM), false);
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
	{
		T option(dqCtxPtr);
		cppcut_assert_equal((DataQueryContext *)dqCtxPtr,
		                    &option.getDataQueryContext());
		cppcut_assert_equal(2, dqCtxPtr->getUsedCount());
	}
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
}
#define assertQueryOptionFromDataQueryContext(T) \
cut_trace(_assertQueryOptionFromDataQueryContext<T>())

void cut_setup(void)
{
	hatoholInit();
	setupTestDBUser();
}

void cut_teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientUser dbUser;
	cppcut_assert_equal(DB_DOMAIN_ID_USERS,
	                    dbUser.getDBAgent()->getDBDomainId());
}

void test_createDB(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DBClientUser dbUser;

	// check the version
	string statement = "select * from _dbclient_version";
	string expect =
	  StringUtils::sprintf(
	    "%d|%d\n", DB_DOMAIN_ID_USERS, DBClientUser::USER_DB_VERSION);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_addUser(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	assertUsersInDB();
}

void test_addUserDuplicate(void)
{
	loadTestDBUser();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserInfo &userInfo = testUserInfo[1];
	assertHatoholError(HTERR_USER_NAME_EXIST,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithLongName(void)
{
	loadTestDBUser();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	const size_t limitLength = DBClientUser::MAX_USER_NAME_LENGTH;
	UserInfo userInfo = testUserInfo[1];
	for (size_t i = 0; userInfo.name.size() <= limitLength; i++)
		userInfo.name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_NAME,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithInvalidFlags(void)
{
	loadTestDBUser();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserInfo userInfo = testUserInfo[1];
	userInfo.name = "Crasher";
	userInfo.flags = ALL_PRIVILEGES + 1;
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithoutPrivilege(void)
{
	loadTestDBUser();
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	flags &= ~(1 << OPPRVLG_CREATE_USER);
	OperationPrivilege privilege(flags);
	DBClientUser dbUser;
	UserInfo userInfo = testUserInfo[1];
	userInfo.name = "UniqueName-9srne7tskrgo";
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_updateUser(void)
{
	UserInfo userInfo = setupForUpdate();
	DBClientUser dbUser;
	OperationPrivilege
	   privilege(OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER));
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	assertUserInfoInDB(userInfo);
}

void test_updateUserWithExistingUserName(void)
{
	loadTestDBUser();
	size_t targetIdx = 1;
	UserInfo userInfo = testUserInfo[targetIdx];
	string expectedName = testUserInfo[targetIdx].name;
	string expectedPassword = testUserInfo[targetIdx].password;
	userInfo.id = targetIdx + 1;
	userInfo.name = testUserInfo[targetIdx + 1].name;
	userInfo.password.clear();
	DBClientUser dbUser;
	OperationPrivilege
	   privilege(OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER));
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_USER_NAME_EXIST, err);

	// check the version
	userInfo.name = expectedName;
	userInfo.password = expectedPassword;
	assertUserInfoInDB(userInfo);
}

void test_updateUserWithEmptyPassword(void)
{
	UserIdType targetIndex = 1;
	UserInfo userInfo = setupForUpdate(targetIndex);
	string expectedPassword = testUserInfo[targetIndex].password;
	userInfo.password.clear();
	DBClientUser dbUser;
	OperationPrivilege
	   privilege(OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER));
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	userInfo.password = expectedPassword;
	assertUserInfoInDB(userInfo);
}

void test_updateUserWithoutPrivilege(void)
{
	DBClientUser dbUser;
	const size_t targetIndex = 1;
	UserInfo expectedUserInfo = testUserInfo[targetIndex];
	expectedUserInfo.id = targetIndex + 1;
	UserInfo userInfo = setupForUpdate(targetIndex);
	OperationPrivilege privilege;
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	// check the version
	assertUserInfoInDB(expectedUserInfo);
}

void test_updateUserPasswordByOwner(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	const size_t targetIndex = 0;
	const size_t targetUserId = targetIndex + 1;
	UserInfo expectedUserInfo = testUserInfo[targetIndex];
	expectedUserInfo.id = targetUserId;
	expectedUserInfo.password = "OwnerCanUpdatePassword";
	UserInfo userInfo = expectedUserInfo;
	OperationPrivilege privilege;
	privilege.setUserId(targetUserId);
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	assertUserInfoInDB(expectedUserInfo);
}

void test_updateUserPasswordByNonOwner(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	const size_t targetIndex = 0;
	const size_t targetUserId = targetIndex + 1;
	const size_t operatorUserId = 3;
	UserInfo expectedUserInfo = testUserInfo[targetIndex];
	expectedUserInfo.id = targetUserId;
	UserInfo userInfo = expectedUserInfo;
	userInfo.password = "NonOwnerCanUpdatePassword";
	OperationPrivilege privilege;
	privilege.setUserId(operatorUserId);
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	// check the version
	assertUserInfoInDB(expectedUserInfo);
}

void test_updateNonExistUser(void)
{
	DBClientUser dbUser;
	UserInfo userInfo = setupForUpdate();
	userInfo.id = NumTestUserInfo + 5;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_NOT_FOUND_TARGET_RECORD, err);
}

void test_deleteUser(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	const UserIdType targetId = 2;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	UserIdSet userIdSet;
	userIdSet.insert(targetId);
	assertUsersInDB(userIdSet);
}

void test_deleteUserWithoutPrivilege(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	const UserIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	// check the version
	assertUsersInDB();
}

void test_getUserId(void)
{
	loadTestDBUser();
	const int targetIdx = 1;
	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(targetIdx+1, userId);
}

void test_getUserIdWrongUserPassword(void)
{
	loadTestDBUser();
	const int targetIdx = 1;
	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx-1].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithSQLInjection(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	string name = "a' OR 1 OR name='a";
	string password = testUserInfo[0].password;
	UserIdType userId = dbUser.getUserId(name, password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdFromEmptyDB(void)
{
	const int targetIdx = 1;
	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithEmptyUsername(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId("", "foo");
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithEmptyPasword(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId("fff", "");
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_addAccessList(void)
{
	loadTestDBAccessList();
	DBClientUser dbUser;
	assertAccessInfoInDB();
}

void test_addAccessListWithoutUpdateUserPrivilege(void)
{
	DBClientUser dbUser;
	HatoholError err;
	OperationPrivilege privilege;
	err = dbUser.addAccessInfo(testAccessInfo[0], privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	AccessInfoIdSet accessInfoIdSet;
	for (size_t i = 0; i < NumTestAccessInfo; i++)
		accessInfoIdSet.insert(i + 1);
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_deleteAccessInfo(void)
{
	loadTestDBAccessList();
	DBClientUser dbUser;
	const AccessInfoIdType targetId = 2;
	OperationPrivilege privilege;
	privilege.setFlags((OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER)));
	HatoholError err = dbUser.deleteAccessInfo(targetId, privilege);
	assertHatoholError(HTERR_OK, err);

	AccessInfoIdSet accessInfoIdSet;
	accessInfoIdSet.insert(targetId);
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_deleteAccessWithoutUpdateUserPrivilege(void)
{
	loadTestDBAccessList();
	DBClientUser dbUser;
	const AccessInfoIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteAccessInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertAccessInfoInDB();
}

void test_getUserInfo(void)
{
	loadTestDBUser();
	DBClientUser dbUser;

	const size_t targetIdx = 2;
	const UserIdType targetUserId = targetIdx + 1;
	const UserInfo expectUserInfo = testUserInfo[targetIdx];
	UserInfo userInfo;
	cppcut_assert_equal(
	  true, dbUser.getUserInfo(userInfo, targetUserId));
	assertUserInfo(expectUserInfo, userInfo);
}

void test_getUserInfoWithNonExistId(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	UserInfo userInfo;
	UserIdType nonexistId = NumTestUserInfo + 5;
	cppcut_assert_equal(false, dbUser.getUserInfo(userInfo, nonexistId));
}

void test_getUserInfoListByNormalUser(void)
{
	OperationPrivilegeFlag noPrivilege = 0;
	const size_t expectedNumUsers = 1;
	const bool expectAllUsers = true;
	assertGetUserInfo(noPrivilege, expectedNumUsers, !expectAllUsers);
}

void test_getUserInfoListByUserWithGetAllUsersFlag(void)
{
	OperationPrivilegeFlag canGetAllUsers =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_USER);
	const size_t expectedNumUsers = NumTestUserInfo;
	const bool expectAllUsers = true;
	assertGetUserInfo(canGetAllUsers, expectedNumUsers, expectAllUsers);
}

void test_getUserInfoListWithNonExistUser(void)
{
	DBClientUser dbUser;
	UserQueryOption option;
	UserIdType nonexistId = NumTestUserInfo + 5;
	option.setUserId(nonexistId);
	UserInfoList userInfoList;
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(true, userInfoList.empty());
}

void test_getUserInfoListWithMyNameAsTargetName(void)
{
	assertGetUserInfoListWithTargetName(ALL_PRIVILEGES, 1);
}

void test_getUserInfoListWithMyNameAsTargetNameWithoutPrivileges(void)
{
	OperationPrivilegeFlag noPrivilege = 0;
	const size_t expectNumList = 1;
	assertGetUserInfoListWithTargetName(noPrivilege, expectNumList);
}

void test_getUserInfoListWithOtherName(void)
{
	const size_t expectNumList = 1;
	const bool searchMySelf = true;
	assertGetUserInfoListWithTargetName(ALL_PRIVILEGES, expectNumList,
					    !searchMySelf);
}

void test_getUserInfoListWithOtherNameWithoutPrivileges(void)
{
	OperationPrivilegeFlag noPrivilege = 0;
	const size_t expectNumList = 0;
	const bool searchMySelf = true;
	assertGetUserInfoListWithTargetName(noPrivilege, expectNumList,
					    !searchMySelf);
}

void test_getUserInfoListOnlyMyself(void)
{
	loadTestDBUser();
	DBClientUser dbUser;
	UserQueryOption option;
	option.queryOnlyMyself();
	static const UserIdType userId = 2;
	option.setUserId(userId);
	UserInfoList userInfoList;
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal((size_t)1, userInfoList.size());
	assertUserInfo(testUserInfo[userId-1], *userInfoList.begin());
}

void test_getServerAccessInfoMap(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerAccessInfoMap srvAccessInfoMap;
		AccessInfoQueryOption option(USER_ID_SYSTEM);
		option.setTargetUserId(it->first);
		HatoholError error = dbUser.getAccessInfoMap(srvAccessInfoMap,
							     option);
		cppcut_assert_equal(HTERR_OK, error.getCode());
		assertServerAccessInfoMap(it->second, srvAccessInfoMap);
		DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByOwner(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	loadTestDBUser();
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerAccessInfoMap srvAccessInfoMap;
		AccessInfoQueryOption option(it->first);
		option.setTargetUserId(it->first);
		HatoholError error = dbUser.getAccessInfoMap(srvAccessInfoMap,
							     option);
		cppcut_assert_equal(HTERR_OK, error.getCode());
		assertServerAccessInfoMap(it->second, srvAccessInfoMap);
		DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByNonOwner(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	loadTestDBUser();
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerAccessInfoMap srvAccessInfoMap;
		AccessInfoQueryOption option;
		UserIdType guestUserA = 1, guestUserB = 3, user;
		user = (it->first == guestUserA) ? guestUserB : guestUserA;
		option.setUserId(user);
		option.setTargetUserId(it->first);
		HatoholError error = dbUser.getAccessInfoMap(srvAccessInfoMap,
							     option);
		cppcut_assert_equal(HTERR_NO_PRIVILEGE, error.getCode());
		cut_assert_true(srvAccessInfoMap.empty());
		DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByAdminUser(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	loadTestDBUser();
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerAccessInfoMap srvAccessInfoMap;
		AccessInfoQueryOption option;
		UserIdType adminUser = 2;
		option.setUserId(adminUser);
		option.setTargetUserId(it->first);
		HatoholError error = dbUser.getAccessInfoMap(srvAccessInfoMap,
							     option);
		cppcut_assert_equal(HTERR_OK, error.getCode());
		assertServerAccessInfoMap(it->second, srvAccessInfoMap);
		DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerHostGrpSetMap(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerHostGrpSetMap srvHostGrpSetMap;
		UserIdType userId = it->first;
		dbUser.getServerHostGrpSetMap(srvHostGrpSetMap, userId);
		assertServerHostGrpSetMap(it->second, srvHostGrpSetMap);
	}
}

void test_isValidUserName(void)
{
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("CAPITAL"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("small"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("Camel"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("sna_ke"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("sna-ke"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("sna.ke"));
	assertHatoholError(HTERR_OK,
	                    DBClientUser::isValidUserName("Ab9@ho.com"));
}

void test_isValidUserNameWithInvalidChars(void)
{
	for (int i = 1; i <= UINT8_MAX; i++) {
		if (i >= 'A' && i <= 'Z')
			continue;
		if (i >= 'a' && i <= 'z')
			continue;
		if (i >= '0' && i <= '9')
			continue;
		if (i == '.' || i == '-' || i == '_' || i == '@')
			continue;
		string name = StringUtils::sprintf("AB%cxy", i);
		assertHatoholError(HTERR_INVALID_CHAR,
		                   DBClientUser::isValidUserName(name));
	}
}

void test_isValidUserNameWithEmptyString(void)
{
	assertHatoholError(HTERR_EMPTY_USER_NAME,
	                   DBClientUser::isValidUserName(""));
}

void test_isValidUserNameLongUserName(void)
{
	string name;
	for (size_t i = 0; i < DBClientUser::MAX_USER_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_OK, DBClientUser::isValidUserName(name));
	name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_NAME,
	                   DBClientUser::isValidUserName(name));
}

void test_isValidPasswordWithLongPassword(void)
{
	string password;
	for (size_t i = 0; i < DBClientUser::MAX_PASSWORD_LENGTH; i++)
		password += "A";
	assertHatoholError(HTERR_OK, DBClientUser::isValidPassword(password));
	password += "A";
	assertHatoholError(HTERR_TOO_LONG_PASSWORD,
	                   DBClientUser::isValidPassword(password));
}

void test_isValidPasswordWithEmptyPassword(void)
{
	assertHatoholError(HTERR_EMPTY_PASSWORD,
	                    DBClientUser::isValidPassword(""));
}

void test_isValidFlagsAllValidBits(void)
{
	OperationPrivilegeFlag flags =
	  OperationPrivilege::makeFlag(NUM_OPPRVLG) - 1;
	assertHatoholError(HTERR_OK, DBClientUser::isValidFlags(flags));
}

void test_isValidFlagsExceededBit(void)
{
	OperationPrivilegeFlag flags =
	  OperationPrivilege::makeFlag(NUM_OPPRVLG);
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   DBClientUser::isValidFlags(flags));
}

void test_isValidFlagsNone(void)
{
	OperationPrivilegeFlag flags = 0;
	assertHatoholError(HTERR_OK, DBClientUser::isValidFlags(flags));
}

void test_isAccessible(void)
{
	assertIsAccessible();
}

void test_isAccessibleAllServers(void)
{
	const bool useAllServers = true;
	assertIsAccessible(useAllServers);
}

void test_isAccessibleFalse(void)
{
	loadTestDBUser();
	loadTestDBAccessList();
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();

	// search for nonexisting User ID and Server ID
	ServerIdType serverId = 0;
	UserIdType userId = INVALID_USER_ID;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo &accessInfo = testAccessInfo[i];
		userId = accessInfo.userId;
		if ((ServerIdType)accessInfo.serverId >= serverId)
			serverId = accessInfo.serverId + 1;
		serverId = accessInfo.serverId;
		break;
	}
	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(false, dbUser->isAccessible(serverId, privilege));
}

void test_constructorOfUserQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(UserQueryOption);
}

void test_constructorOfAccessInfoQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(AccessInfoQueryOption);
}

void test_constructorOfUserRoleQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(UserRoleQueryOption);
}

void test_addUserRole(void)
{
	loadTestDBUserRole();
	DBClientUser dbUser;
	assertUserRolesInDB();
}

void test_addUserRoleWithDuplicatedName(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.flags
	  = OperationPrivilege::makeFlag(OPPRVLG_DELETE_ACTION);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithDuplicatedFlags(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithAllPrivilegeFlags(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	userRoleInfo.flags = ALL_PRIVILEGES;
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithNonePrivilegeFlags(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	userRoleInfo.flags = NONE_PRIVILEGE;
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithLongName(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	const size_t limitLength = DBClientUser::MAX_USER_ROLE_NAME_LENGTH;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	for (size_t i = 0; userRoleInfo.name.size() <= limitLength; i++)
		userRoleInfo.name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_ROLE_NAME,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithInvalidFlags(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = 0;
	userRoleInfo.name = "Crasher";
	userRoleInfo.flags = ALL_PRIVILEGES + 1;
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithEmptyUserName(void)
{
	loadTestDBUserRole();
	OperationPrivilege privilege(ALL_PRIVILEGES);
	DBClientUser dbUser;
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = 0;
	userRoleInfo.flags = ALL_PRIVILEGES;
	assertHatoholError(HTERR_EMPTY_USER_ROLE_NAME,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithoutPrivilege(void)
{
	loadTestDBUserRole();
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	flags &= ~(1 << OPPRVLG_CREATE_USER_ROLE);
	OperationPrivilege privilege(flags);
	DBClientUser dbUser;
	UserRoleInfo &userRoleInfo = testUserRoleInfo[1];
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_isValidUserRoleName(void)
{
	string name;
	for (size_t i = 0; i < DBClientUser::MAX_USER_ROLE_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_OK, DBClientUser::isValidUserRoleName(name));
}

void test_isValidUserRoleNameWithLongName(void)
{
	string name;
	for (size_t i = 0; i <= DBClientUser::MAX_USER_ROLE_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_ROLE_NAME,
	                   DBClientUser::isValidUserRoleName(name));
}

void test_getUserRoleInfoList(void)
{
	assertGetUserRoleInfo(ALL_PRIVILEGES);
}

void test_getUserRoleInfoListWithTargetId(void)
{
	UserRoleIdType targetUserRoleId = 2;
	assertGetUserRoleInfo(ALL_PRIVILEGES, targetUserRoleId);
}

void test_getUserRoleInfoListWithNoPrivilege(void)
{
	const OperationPrivilegeFlag noPrivilege = 0;
	assertGetUserRoleInfo(noPrivilege);
}

void test_updateUserRole(void)
{
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate();
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	assertUserRoleInfoInDB(userRoleInfo);
}

void test_updateUserRoleWithoutPrivilege(void)
{
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate();
	DBClientUser dbUser;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	flags &= ~OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE);
	OperationPrivilege privilege(flags);
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithExistingName(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.name = testUserRoleInfo[targetIndex + 1].name;
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithExistingFlags(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.flags = testUserRoleInfo[targetIndex + 1].flags;
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithAllPrivilegeFlags(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.flags = ALL_PRIVILEGES;
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithNonePrivilegeFlags(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.flags = NONE_PRIVILEGE;
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithEmptyName(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.name = "";
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_EMPTY_USER_ROLE_NAME, err);

	assertUserRolesInDB();
}

void test_updateUserRoleWithInvalidFlags(void)
{
	size_t targetIndex = 1;
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate(targetIndex);
	userRoleInfo.flags = ALL_PRIVILEGES + 1;
	DBClientUser dbUser;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS, err);

	assertUserRolesInDB();
}

void test_deleteUserRole(void)
{
	loadTestDBUserRole();
	DBClientUser dbUser;
	const UserRoleIdType targetId = 2;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	HatoholError err = dbUser.deleteUserRoleInfo(targetId, privilege);
	assertHatoholError(HTERR_OK, err);

	UserRoleIdSet userRoleIdSet;
	userRoleIdSet.insert(targetId);
	assertUserRolesInDB(userRoleIdSet);
}

void test_deleteUserRoleWithoutPrivilege(void)
{
	loadTestDBUserRole();
	DBClientUser dbUser;
	const UserRoleIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteUserRoleInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertUserRolesInDB();
}

} // namespace testDBClientUser
