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

static size_t countAccessInfoMapElements(const AccessInfoMap &accessInfoMap)
{
	size_t num = 0;
	AccessInfoMapConstIterator it = accessInfoMap.begin();
	for (; it != accessInfoMap.end(); ++it) {
		const AccessInfoHGMap *accessInfoHGMap = it->second;
		num += accessInfoHGMap->size();
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

static void _assertAccessInfoMap(const set<int> &expectIdxSet,
                                 const AccessInfoMap &accessInfoMap)
{
	// check total number
	cppcut_assert_equal(expectIdxSet.size(),
	                    countAccessInfoMapElements(accessInfoMap));

	// check each element
	set<int>::const_iterator it = expectIdxSet.begin();
	for (; it != expectIdxSet.end(); ++it) {
		const AccessInfo &expectAccessInfo = testAccessInfo[*it];
		AccessInfoMapConstIterator jt = 
		  accessInfoMap.find(expectAccessInfo.serverId);
		cppcut_assert_equal(true, jt != accessInfoMap.end(),
		                    cut_message("Failed to lookup: %"PRIu32,
		                                expectAccessInfo.serverId));
		const AccessInfoHGMap *accessInfoHGMap = jt->second;
		AccessInfoHGMapConstIterator kt =
		   accessInfoHGMap->find(expectAccessInfo.hostGroupId);
		cppcut_assert_equal(true, kt != accessInfoHGMap->end(),
		                    cut_message("Failed to lookup: %"PRIu64,
		                                expectAccessInfo.hostGroupId));
		const AccessInfo *actualAccessInfo = kt->second;
		assertAccessInfo(expectAccessInfo, *actualAccessInfo);
	}
}
#define assertAccessInfoMap(E,A) cut_trace(_assertAccessInfoMap(E,A))

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

void test_getAccessInfoMap(void)
{
	loadTestDBAccessList();
	DBClientUser dbUser;

	typedef map<UserIdType, set<int> > UserIdIndexMap;
	typedef UserIdIndexMap::iterator   UserIdIndexMapIterator;
	UserIdIndexMap userIdIndexMap;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo &accessInfo = testAccessInfo[i];
		userIdIndexMap[accessInfo.userId].insert(i);
	}

	UserIdIndexMapIterator it = userIdIndexMap.begin();
	for (; it != userIdIndexMap.end(); ++it) {
		AccessInfoMap accessInfoMap;
		UserIdType userId = it->first;
		dbUser.getAccessInfoMap(accessInfoMap, userId);
		assertAccessInfoMap(it->second, accessInfoMap);
	}

}

} // namespace testDBClientUser
