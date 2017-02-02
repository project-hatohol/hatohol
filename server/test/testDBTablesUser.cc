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

#include <stdint.h>
#include <cppcutter.h>
#include <cutter.h>
#include <gcutter.h>
#include "Helpers.h"
#include "DBTablesUser.h"
#include "DBTablesTest.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

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


namespace testDBTablesUser {

#define DECLARE_DBTABLES_USER(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesUser &VAR_NAME = _dbHatohol.getDBTablesUser();

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
	                     DBTablesUser::TABLE_NAME_USERS, userInfo.id);
	string expect =
	  StringUtils::sprintf(
	    "%" FMT_USER_ID "|%s|%s|%" PRIu64 "\n",
	    userInfo.id, userInfo.name.c_str(),
	    Utils::sha256(userInfo.password).c_str(), userInfo.flags);
	DECLARE_DBTABLES_USER(dbUser);
	assertDBContent(&dbUser.getDBAgent(), statement, expect);
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
		cppcut_assert_equal(
		  true, kt != hostGrpAccessInfoMap->end(),
		  cut_message("Failed to lookup: %" FMT_HOST_GROUP_ID,
		              expectAccessInfo.hostgroupId.c_str()));
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
		cppcut_assert_equal(
		  true, kt != hostgroupIdSet.end(),
		  cut_message("Failed to lookup: %" FMT_HOST_GROUP_ID,
		              expectAccessInfo.hostgroupId.c_str()));
	}
}
#define assertServerHostGrpSetMap(E,A) \
cut_trace(_assertServerHostGrpSetMap(E,A))

static void _assertGetUserInfo(const OperationPrivilegeFlag flags,
                               const size_t expectedNumUser,
                               const bool expectAllUsers)
{
	UserInfoList userInfoList;
	UserQueryOption option;
	const UserInfo userInfo = findFirstTestUserInfoByFlag(flags);
	option.setUserId(userInfo.id);
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	loadTestDBAccessList();

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();

	// search the User ID and Server ID
	ServerIdType serverId = 0;
	UserIdType userId = INVALID_USER_ID;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		const AccessInfo &accessInfo = testAccessInfo[i];
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
	cppcut_assert_equal(true, dbUser.isAccessible(serverId, privilege));

	// invalid user
	privilege.setUserId(INVALID_USER_ID);
	cppcut_assert_equal(false, dbUser.isAccessible(serverId, privilege));
}
#define assertIsAccessible(...) cut_trace(_assertIsAccessible(__VA_ARGS__))

static UserInfo setupForUpdate(size_t targetIndex = 1)
{
	UserInfo userInfo = testUserInfo[targetIndex];
	userInfo.id = targetIndex + 1;
	userInfo.password = ">=_=<3";
	userInfo.flags = 0;
	return userInfo;
}

static void setupWithUserIdIndexMap(UserIdIndexMap &userIdIndexMap)
{
	loadTestDBAccessList();

	makeTestUserIdIndexMap(userIdIndexMap);
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBUser();
}

void cut_teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DECLARE_DBTABLES_USER(dbUser);
	assertDBTablesVersion(
	  dbUser.getDBAgent(),
	  DBTablesId::USER, DBTablesUser::USER_DB_VERSION);
}

void test_addUser(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	cppcut_assert_not_null(&dbUser); // To avoid the waring (unsed)
	assertUsersInDB();
}

void test_addUserDuplicate(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserInfo userInfo = testUserInfo[1];
	assertHatoholError(HTERR_USER_NAME_EXIST,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithLongName(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	const size_t limitLength = DBTablesUser::MAX_USER_NAME_LENGTH;
	UserInfo userInfo = testUserInfo[1];
	for (size_t i = 0; userInfo.name.size() <= limitLength; i++)
		userInfo.name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_NAME,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithInvalidFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserInfo userInfo = testUserInfo[1];
	userInfo.name = "Crasher";
	userInfo.flags = OperationPrivilege::ALL_PRIVILEGES + 1;
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_addUserWithoutPrivilege(void)
{
	OperationPrivilegeFlag flags = OperationPrivilege::ALL_PRIVILEGES;
	flags &= ~(1 << OPPRVLG_CREATE_USER);
	OperationPrivilege privilege(flags);
	DECLARE_DBTABLES_USER(dbUser);
	UserInfo userInfo = testUserInfo[1];
	userInfo.name = "UniqueName-9srne7tskrgo";
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbUser.addUserInfo(userInfo, privilege));
	assertUsersInDB();
}

void test_updateUser(void)
{
	UserInfo userInfo = setupForUpdate();
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilege
	   privilege(OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER));
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	assertUserInfoInDB(userInfo);
}

void test_updateUserWithExistingUserName(void)
{
	size_t targetIdx = 1;
	UserInfo userInfo = testUserInfo[targetIdx];
	string expectedName = testUserInfo[targetIdx].name;
	string expectedPassword = testUserInfo[targetIdx].password;
	userInfo.id = targetIdx + 1;
	userInfo.name = testUserInfo[targetIdx + 1].name;
	userInfo.password.clear();
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
	UserInfo userInfo = setupForUpdate();
	userInfo.id = NumTestUserInfo + 5;
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	HatoholError err = dbUser.updateUserInfo(userInfo, privilege);
	assertHatoholError(HTERR_NOT_FOUND_TARGET_RECORD, err);
}

void test_updateUserInfoFlags(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	const size_t targetIndex1 = 5;
	const size_t targetUserId1 = targetIndex1 + 1;
	UserInfo expectedUserInfo1 = testUserInfo[targetIndex1];
	expectedUserInfo1.id = targetUserId1;
	const size_t targetIndex2 = 9;
	const size_t targetUserId2 = targetIndex2 + 1;
	UserInfo expectedUserInfo2 = testUserInfo[targetIndex2];
	expectedUserInfo2.id = targetUserId2;
	OperationPrivilegeFlag oldUserInfoFlag =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER);
	OperationPrivilegeFlag updateUserInfoFlag =
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_SERVER);
	expectedUserInfo1.flags = updateUserInfoFlag;
	expectedUserInfo2.flags = updateUserInfoFlag;
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE) |
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_USER));

	HatoholError err =
	  dbUser.updateUserInfoFlags(oldUserInfoFlag, updateUserInfoFlag, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the users info
	assertUserInfoInDB(expectedUserInfo1);
	assertUserInfoInDB(expectedUserInfo2);
}

void test_updateUserInfoFlagsWithInsufficientPrivileges(void)
{
	OperationPrivilegeFlag oldUserInfoFlag =
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_SERVER);
	OperationPrivilegeFlag updateUserInfoFlag =
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_SERVER);
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));

	// updateUserInfoFlags requests OPPRVLG_UPDATE_ALL_USER_ROLE and
	// OPPRVLG_UPDATE_USER privileges.
	// Users have only OPPRVLG_UPDATE_ALL_USER_ROLE privilege is
	// insufficient to update UserInfoRole flag and UserInfo flag together!
	HatoholError err =
	  dbUser.updateUserInfoFlags(oldUserInfoFlag, updateUserInfoFlag, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);
}

void test_deleteUser(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	const UserIdType targetId = 2;
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);
	assertHatoholError(HTERR_OK, err);

	// check the version
	UserIdSet userIdSet;
	userIdSet.insert(targetId);
	assertUsersInDB(userIdSet);
}

void test_deleteMyself(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	const UserIdType targetId = privilege.getUserId();
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);
	assertHatoholError(HTERR_DELETE_MYSELF, err);
}

void test_deleteUserWithoutPrivilege(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	const UserIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	// check the version
	assertUsersInDB();
}

void test_getUserId(void)
{
	const int targetIdx = 1;
	DECLARE_DBTABLES_USER(dbUser);
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(targetIdx+1, userId);
}

void test_getUserIdWrongUserPassword(void)
{
	const int targetIdx = 1;
	DECLARE_DBTABLES_USER(dbUser);
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx-1].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithSQLInjection(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	string name = "a' OR 1 OR name='a";
	string password = testUserInfo[0].password;
	UserIdType userId = dbUser.getUserId(name, password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithEmptyUsername(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	UserIdType userId = dbUser.getUserId("", "foo");
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_getUserIdWithEmptyPasword(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	UserIdType userId = dbUser.getUserId("fff", "");
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

void test_addAccessList(void)
{
	loadTestDBAccessList();

	DECLARE_DBTABLES_USER(dbUser);
	cppcut_assert_not_null(&dbUser); // To avoid the waring (unsed)
	assertAccessInfoInDB();
}

void test_addAccessListWithoutUpdateUserPrivilege(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	HatoholError err;
	OperationPrivilege privilege;
	AccessInfo accessInfo = testAccessInfo[0];
	err = dbUser.addAccessInfo(accessInfo, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	AccessInfoIdSet accessInfoIdSet;
	for (size_t i = 0; i < NumTestAccessInfo; i++)
		accessInfoIdSet.insert(i + 1);
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_deleteAccessInfo(void)
{
	loadTestDBAccessList();

	DECLARE_DBTABLES_USER(dbUser);
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

	DECLARE_DBTABLES_USER(dbUser);
	const AccessInfoIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteAccessInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertAccessInfoInDB();
}

void test_getUserInfo(void)
{
	DECLARE_DBTABLES_USER(dbUser);

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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
	UserQueryOption option;
	UserIdType nonexistId = NumTestUserInfo + 5;
	option.setUserId(nonexistId);
	UserInfoList userInfoList;
	dbUser.getUserInfoList(userInfoList, option);
	cppcut_assert_equal(true, userInfoList.empty());
}

void test_getUserInfoListWithMyNameAsTargetName(void)
{
	assertGetUserInfoListWithTargetName(
	  OperationPrivilege::ALL_PRIVILEGES, 1);
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
	assertGetUserInfoListWithTargetName(
	  OperationPrivilege::ALL_PRIVILEGES, expectNumList, !searchMySelf);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
		DBTablesUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByOwner(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	UserIdIndexMap userIdIndexMap;
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
		DBTablesUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByNonOwner(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	UserIdIndexMap userIdIndexMap;
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
		DBTablesUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerAccessInfoMapByAdminUser(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	UserIdIndexMap userIdIndexMap;
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
		DBTablesUser::destroyServerAccessInfoMap(srvAccessInfoMap);
	}
}

void test_getServerHostGrpSetMap(void)
{
	DECLARE_DBTABLES_USER(dbUser);
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
	                    DBTablesUser::isValidUserName("CAPITAL"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("small"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("Camel"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("sna_ke"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("sna-ke"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("sna.ke"));
	assertHatoholError(HTERR_OK,
	                    DBTablesUser::isValidUserName("Ab9@ho.com"));
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
		                   DBTablesUser::isValidUserName(name));
	}
}

void test_isValidUserNameWithEmptyString(void)
{
	assertHatoholError(HTERR_EMPTY_USER_NAME,
	                   DBTablesUser::isValidUserName(""));
}

void test_isValidUserNameLongUserName(void)
{
	string name;
	for (size_t i = 0; i < DBTablesUser::MAX_USER_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_OK, DBTablesUser::isValidUserName(name));
	name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_NAME,
	                   DBTablesUser::isValidUserName(name));
}

void test_isValidPasswordWithLongPassword(void)
{
	string password;
	for (size_t i = 0; i < DBTablesUser::MAX_PASSWORD_LENGTH; i++)
		password += "A";
	assertHatoholError(HTERR_OK, DBTablesUser::isValidPassword(password));
	password += "A";
	assertHatoholError(HTERR_TOO_LONG_PASSWORD,
	                   DBTablesUser::isValidPassword(password));
}

void test_isValidPasswordWithEmptyPassword(void)
{
	assertHatoholError(HTERR_EMPTY_PASSWORD,
	                    DBTablesUser::isValidPassword(""));
}

void test_isValidFlagsAllValidBits(void)
{
	OperationPrivilegeFlag flags =
	  OperationPrivilege::makeFlag(NUM_OPPRVLG) - 1;
	assertHatoholError(HTERR_OK, DBTablesUser::isValidFlags(flags));
}

void test_isValidFlagsExceededBit(void)
{
	OperationPrivilegeFlag flags =
	  OperationPrivilege::makeFlag(NUM_OPPRVLG);
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   DBTablesUser::isValidFlags(flags));
}

void test_isValidFlagsNone(void)
{
	OperationPrivilegeFlag flags = 0;
	assertHatoholError(HTERR_OK, DBTablesUser::isValidFlags(flags));
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
	loadTestDBAccessList();

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();

	// search for nonexisting User ID and Server ID
	ServerIdType serverId = 0;
	UserIdType userId = INVALID_USER_ID;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		const AccessInfo &accessInfo = testAccessInfo[i];
		userId = accessInfo.userId;
		if ((ServerIdType)accessInfo.serverId >= serverId)
			serverId = accessInfo.serverId + 1;
		serverId = accessInfo.serverId;
		break;
	}
	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(false, dbUser.isAccessible(serverId, privilege));
}

void data_isAccessibleWithHostgroup(void)
{
	set<UserIdType> userIdSet;

	struct {
		static void add(const UserIdType &userId,
		                const ServerIdType &serverId,
		                const HostgroupIdType &hostgroupId,
		                const bool &expect)
		{
			string serverIdStr =
			  serverId == ALL_SERVERS ?  "ALL" :
			    StringUtils::sprintf("%" FMT_SERVER_ID, serverId);

			string testName = StringUtils::sprintf(
			  "%s-U%" FMT_USER_ID "-S%s-H%s",
			  expect ? "T" : "F", userId,
			  serverIdStr.c_str(), hostgroupId.c_str());

			gcut_add_datum(testName.c_str(),
			  "userId",   G_TYPE_INT, userId,
			  "serverId", G_TYPE_INT, serverId,
			  "hostgroupId",   G_TYPE_STRING, hostgroupId.c_str(),
			  "expect",   G_TYPE_BOOLEAN, expect,
			  NULL);
		}
	} dataAdder;

	// Tests with listed paramters
	for (size_t i = 0; i < NumTestAccessInfo; ++i) {
		const AccessInfo *accessInfo = &testAccessInfo[i];
		dataAdder.add(accessInfo->userId, accessInfo->serverId,
		              accessInfo->hostgroupId, true);
		userIdSet.insert(accessInfo->userId);
	}

	// Tests with parameters that is not listed in the test data.
	UserIdSetConstIterator it = userIdSet.begin();
	for (; it != userIdSet.end(); ++it) {
		const UserIdType &userId = *it;
		ServerAccessInfoMap srvAccessInfoMap;
		makeServerAccessInfoMap(srvAccessInfoMap, userId);
		ServerAccessInfoMapConstIterator svaccIt =
		  srvAccessInfoMap.begin();

		bool hasAllServers = false;
		ServerIdType maxServerId = 0;
		for (; svaccIt != srvAccessInfoMap.end(); ++svaccIt) {
			const ServerIdType &serverId = svaccIt->first;
			if (serverId == ALL_SERVERS)
				hasAllServers = true;
			else if (serverId > maxServerId)
				maxServerId = serverId;
		}
		if (hasAllServers) {
			// We pass bogus non-existent server and hostgroup ID.
			dataAdder.add(userId, 12345, "67890", true);
			continue;
		}

		svaccIt = srvAccessInfoMap.begin();
		for (; svaccIt != srvAccessInfoMap.end(); ++svaccIt) {
			const ServerIdType &serverId = svaccIt->first;
			const HostGrpAccessInfoMap *grpMap = svaccIt->second;
			HostGrpAccessInfoMapConstIterator hgrpIt =
			  grpMap->begin();
			bool hasAllHostgroups = false;
			HostgroupIdType maxHostgroupId;
			for (; hgrpIt != grpMap->end(); ++hgrpIt) {
				const HostgroupIdType &hostgroupId = hgrpIt->first;
				if (hostgroupId == ALL_HOST_GROUPS)
					hasAllHostgroups = true;
				else if (hostgroupId > maxHostgroupId)
					maxHostgroupId = hostgroupId;
			}
			// We pass bogus non-existent hostgroup ID.
			if (hasAllHostgroups) {
				dataAdder.add(userId, serverId, "67890", true);
			} else {
				dataAdder.add(userId, serverId,
				              maxHostgroupId + "1", false);
			}
		}
	}
}

void test_isAccessibleWithHostgroup(gconstpointer data)
{
	loadTestDBAccessList();

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();

	const ServerIdType serverId = gcut_data_get_int(data, "serverId");
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");
	const UserIdType userId = gcut_data_get_int(data, "userId");
	const bool expect       = gcut_data_get_boolean(data, "expect");
	OperationPrivilege privilege;
	privilege.setUserId(userId);
	cppcut_assert_equal(
	  expect, dbUser.isAccessible(serverId, hostgroupId, privilege));
}

void test_constructorOfUserQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(UserQueryOption);
}

void test_constructorOfAccessInfoQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(AccessInfoQueryOption);
}

} // namespace testDBTablesUser

namespace testDBTablesUserWithoutLoadingUsers {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

void test_getUserIdFromEmptyDB(void)
{
	const int targetIdx = 1;
	DECLARE_DBTABLES_USER(dbUser);
	UserIdType userId = dbUser.getUserId(testUserInfo[targetIdx].name,
	                                     testUserInfo[targetIdx].password);
	cppcut_assert_equal(INVALID_USER_ID, userId);
}

} // namespace testDBTablesUserWithoutLoadingUsers

namespace testDBTablesUserRole {

static UserRoleInfo setupUserRoleInfoForUpdate(size_t targetIndex = 1)
{
	UserRoleInfo userRoleInfo = testUserRoleInfo[targetIndex];
	userRoleInfo.id = targetIndex + 1;
	userRoleInfo.name = ">=_=<3";
	userRoleInfo.flags =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER);
	return userRoleInfo;
}

static void _assertGetUserRoleInfo(
  const OperationPrivilegeFlag flags,
  UserRoleIdType targetUserRoleId = INVALID_USER_ROLE_ID)
{
	UserRoleInfoList userRoleInfoList;
	UserRoleQueryOption option;
	const UserInfo userInfo = findFirstTestUserInfoByFlag(flags);
	option.setUserId(userInfo.id);
	if (targetUserRoleId != INVALID_USER_ROLE_ID)
		option.setTargetUserRoleId(targetUserRoleId);
	DECLARE_DBTABLES_USER(dbUser);
	dbUser.getUserRoleInfoList(userRoleInfoList, option);
	string expected, actual;
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleInfo userRoleInfo = testUserRoleInfo[i];
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


void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBUser();
	loadTestDBUserRole();
}

void test_constructorOfUserRoleQueryOptionFromDataQueryContext(void)
{
	assertQueryOptionFromDataQueryContext(UserRoleQueryOption);
}

void test_addUserRole(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	cppcut_assert_not_null(&dbUser); // To avoid the waring (unsed)
	assertUserRolesInDB();
}

void test_addUserRoleWithDuplicatedName(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.flags
	  = OperationPrivilege::makeFlag(OPPRVLG_DELETE_ACTION);
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithDuplicatedFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithAllPrivilegeFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	userRoleInfo.flags = OperationPrivilege::ALL_PRIVILEGES;
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithNonePrivilegeFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	userRoleInfo.name = "Unique name kea#osemrnscs+";
	userRoleInfo.flags = OperationPrivilege::NONE_PRIVILEGE;
	assertHatoholError(HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithLongName(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	const size_t limitLength = DBTablesUser::MAX_USER_ROLE_NAME_LENGTH;
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	for (size_t i = 0; userRoleInfo.name.size() <= limitLength; i++)
		userRoleInfo.name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_ROLE_NAME,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithInvalidFlags(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = 0;
	userRoleInfo.name = "Crasher";
	userRoleInfo.flags = OperationPrivilege::ALL_PRIVILEGES + 1;
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithEmptyUserName(void)
{
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo;
	userRoleInfo.id = 0;
	userRoleInfo.flags = OperationPrivilege::ALL_PRIVILEGES;
	assertHatoholError(HTERR_EMPTY_USER_ROLE_NAME,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_addUserRoleWithoutPrivilege(void)
{
	OperationPrivilegeFlag flags = OperationPrivilege::ALL_PRIVILEGES;
	flags &= ~(1 << OPPRVLG_CREATE_USER_ROLE);
	OperationPrivilege privilege(flags);
	DECLARE_DBTABLES_USER(dbUser);
	UserRoleInfo userRoleInfo = testUserRoleInfo[1];
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbUser.addUserRoleInfo(userRoleInfo, privilege));
	assertUserRolesInDB();
}

void test_isValidUserRoleName(void)
{
	string name;
	for (size_t i = 0; i < DBTablesUser::MAX_USER_ROLE_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_OK, DBTablesUser::isValidUserRoleName(name));
}

void test_isValidUserRoleNameWithLongName(void)
{
	string name;
	for (size_t i = 0; i <= DBTablesUser::MAX_USER_ROLE_NAME_LENGTH; i++)
		name += "A";
	assertHatoholError(HTERR_TOO_LONG_USER_ROLE_NAME,
	                   DBTablesUser::isValidUserRoleName(name));
}

void test_getUserRoleInfoList(void)
{
	assertGetUserRoleInfo(OperationPrivilege::ALL_PRIVILEGES);
}

void test_getUserRoleInfoListWithTargetId(void)
{
	UserRoleIdType targetUserRoleId = 2;
	assertGetUserRoleInfo(OperationPrivilege::ALL_PRIVILEGES,
			      targetUserRoleId);
}

void test_getUserRoleInfoListWithNoPrivilege(void)
{
	const OperationPrivilegeFlag noPrivilege = 0;
	assertGetUserRoleInfo(noPrivilege);
}

void test_updateUserRole(void)
{
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate();
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_OK, err);

	assertUserRoleInfoInDB(userRoleInfo);
}

void test_updateUserRoleWithoutPrivilege(void)
{
	UserRoleInfo userRoleInfo = setupUserRoleInfoForUpdate();
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilegeFlag flags = OperationPrivilege::ALL_PRIVILEGES;
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
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	userRoleInfo.flags = OperationPrivilege::ALL_PRIVILEGES;
	DECLARE_DBTABLES_USER(dbUser);
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
	userRoleInfo.flags = OperationPrivilege::NONE_PRIVILEGE;
	DECLARE_DBTABLES_USER(dbUser);
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
	DECLARE_DBTABLES_USER(dbUser);
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
	userRoleInfo.flags = OperationPrivilege::ALL_PRIVILEGES + 1;
	DECLARE_DBTABLES_USER(dbUser);
	OperationPrivilege privilege(
	  OperationPrivilege::makeFlag(OPPRVLG_UPDATE_ALL_USER_ROLE));
	HatoholError err = dbUser.updateUserRoleInfo(userRoleInfo, privilege);
	assertHatoholError(HTERR_INVALID_PRIVILEGE_FLAGS, err);

	assertUserRolesInDB();
}

void test_deleteUserRole(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	const UserRoleIdType targetId = 2;
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	HatoholError err = dbUser.deleteUserRoleInfo(targetId, privilege);
	assertHatoholError(HTERR_OK, err);

	UserRoleIdSet userRoleIdSet;
	userRoleIdSet.insert(targetId);
	assertUserRolesInDB(userRoleIdSet);
}

void test_deleteUserRoleWithoutPrivilege(void)
{
	DECLARE_DBTABLES_USER(dbUser);
	const UserRoleIdType targetId = 2;
	OperationPrivilege privilege;
	HatoholError err = dbUser.deleteUserRoleInfo(targetId, privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertUserRolesInDB();
}

} // namespace testDBTablesUserRole
