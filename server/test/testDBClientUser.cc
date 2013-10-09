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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#define __STDC_LIMIT_MACROS 
#include <stdint.h>
#include <cppcutter.h>
#include <cutter.h>

#include "Helpers.h"
#include "DBClientUser.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "Hatohol.h"

namespace testDBClientUser {

static void _assertUserInfo(const UserInfo &expect, const UserInfo &actual)
{
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.name, actual.name);
	cppcut_assert_equal(Utils::sha256(expect.password), actual.password);
	cppcut_assert_equal(expect.flags, actual.flags);
}
#define assertUserInfo(E,A) cut_trace(_assertUserInfo(E,A))

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
		const HostGroupSet &hostGroupSet = it->second;
		num += hostGroupSet.size();
	}
	return num;
}

static void _assertAccessInfo(const AccessInfo &expect,
                              const AccessInfo &actual)
{
	cppcut_assert_equal(expect.userId, actual.userId);
	cppcut_assert_equal(expect.serverId, actual.serverId);
	cppcut_assert_equal(expect.hostGroupId, actual.hostGroupId);
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
		                    cut_message("Failed to lookup: %"PRIu32,
		                                expectAccessInfo.serverId));
		const HostGrpAccessInfoMap *hostGrpAccessInfoMap = jt->second;
		HostGrpAccessInfoMapConstIterator kt =
		   hostGrpAccessInfoMap->find(expectAccessInfo.hostGroupId);
		cppcut_assert_equal(true, kt != hostGrpAccessInfoMap->end(),
		                    cut_message("Failed to lookup: %"PRIu64,
		                                expectAccessInfo.hostGroupId));
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
		                    cut_message("Failed to lookup: %"PRIu32,
		                                expectAccessInfo.serverId));
		const HostGroupSet &hostGroupSet = jt->second;
		HostGroupSetConstIterator kt =
		   hostGroupSet.find(expectAccessInfo.hostGroupId);
		cppcut_assert_equal(true, kt != hostGroupSet.end(),
		                    cut_message("Failed to lookup: %"PRIu64,
		                                expectAccessInfo.hostGroupId));
	}
}
#define assertServerHostGrpSetMap(E,A) \
cut_trace(_assertServerHostGrpSetMap(E,A))

static void setupWithUserIdIndexMap(UserIdIndexMap &userIdIndexMap)
{
	loadTestDBAccessList();
	makeTestUserIdIndexMap(userIdIndexMap);
}

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

	// check the version
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USERS;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		const UserInfo &userInfo = testUserInfo[i];
		expect += StringUtils::sprintf("%zd|%s|%s|%"PRIu32"\n",
		  i+1, userInfo.name.c_str(),
		  Utils::sha256(userInfo.password).c_str(),
		  userInfo.flags);
	}
	assertDBContent(dbUser.getDBAgent(), statement, expect);
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

void test_addAccessList(void)
{
	loadTestDBAccessList();
	DBClientUser dbUser;

	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_ACCESS_LIST;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		const AccessInfo &accessInfo = testAccessInfo[i];
		expect += StringUtils::sprintf(
		  "%zd|%"FMT_USER_ID"|%d|%"PRIu64"\n",
		  i+1, accessInfo.userId,
		  accessInfo.serverId, accessInfo.hostGroupId);
	}
	assertDBContent(dbUser.getDBAgent(), statement, expect);
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

void test_getServerAccessInfoMap(void)
{
	DBClientUser dbUser;
	UserIdIndexMap userIdIndexMap;
	setupWithUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		ServerAccessInfoMap srvAccessInfoMap;
		UserIdType userId = it->first;
		dbUser.getAccessInfoMap(srvAccessInfoMap, userId);
		assertServerAccessInfoMap(it->second, srvAccessInfoMap);
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
	cppcut_assert_equal(true, DBClientUser::isValidUserName("CAPITAL"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("small"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("Camel"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("sna_ke"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("sna-ke"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("sna.ke"));
	cppcut_assert_equal(true, DBClientUser::isValidUserName("Ab9@ho.com"));
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
		cppcut_assert_equal(false, DBClientUser::isValidUserName(name),
		                    cut_message("index: %d", i));
	}
}

void test_isValidUserNameWithEmptyString(void)
{
	cppcut_assert_equal(false, DBClientUser::isValidUserName(""));
}

void test_isValidUserNameLongUserName(void)
{
	string name;
	for (size_t i = 0; i < DBClientUser::MAX_USER_NAME_LENGTH; i++)
		name += "A";
	cppcut_assert_equal(true, DBClientUser::isValidUserName(name));
	name += "A";
	cppcut_assert_equal(false, DBClientUser::isValidUserName(name));
}

void test_isValidPasswordWithLongPassowrd(void)
{
	string password;
	for (size_t i = 0; i < DBClientUser::MAX_PASSWORD_LENGTH; i++)
		password += "A";
	cppcut_assert_equal(true, DBClientUser::isValidPassword(password));
	password += "A";
	cppcut_assert_equal(false, DBClientUser::isValidPassword(password));
}

} // namespace testDBClientUser
