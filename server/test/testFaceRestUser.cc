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

#include <cppcutter.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "CacheServiceDBClient.h"
#include "UnifiedDataStore.h"
#include "SessionManager.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestUser {

static JsonParserAgent *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	stopFaceRest();

	if (g_parser) {
		delete g_parser;
		g_parser = NULL;
	}
}

static void _assertUser(JsonParserAgent *parser, const UserInfo &userInfo,
                        uint32_t expectUserId = 0)
{
	if (expectUserId)
		assertValueInParser(parser, "userId", expectUserId);
	assertValueInParser(parser, "name", userInfo.name);
	assertValueInParser(parser, "flags", userInfo.flags);
}
#define assertUser(P,I,...) cut_trace(_assertUser(P,I,##__VA_ARGS__))

static void assertUserRolesMapInParser(JsonParserAgent *parser)
{
	assertStartObject(parser, "userRoles");

	string flagsStr =
	  StringUtils::sprintf("%" FMT_OPPRVLG, NONE_PRIVILEGE);
	assertStartObject(parser, flagsStr);
	assertValueInParser(parser, "name", string("Guest"));
	parser->endObject();

	flagsStr = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	assertStartObject(parser, flagsStr);
	assertValueInParser(parser, "name", string("Admin"));
	parser->endObject();

	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		flagsStr = StringUtils::toString(userRoleInfo.flags);
		assertStartObject(parser, flagsStr);
		assertValueInParser(parser, "name", userRoleInfo.name);
		parser->endObject();
	}
	parser->endObject();
}

static void _assertUsers(const string &path, const UserIdType &userId,
                         const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers", NumTestUserInfo);
	assertStartObject(g_parser, "users");
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		g_parser->startElement(i);
		const UserInfo &userInfo = testUserInfo[i];
		assertUser(g_parser, userInfo, i + 1);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertUserRolesMapInParser(g_parser);
}
#define assertUsers(P, U, ...) cut_trace(_assertUsers(P, U, ##__VA_ARGS__))

#define assertAddUser(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/user", ##__VA_ARGS__))

void _assertAddUserWithSetup(const StringMap &params,
                             const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	assertAddUser(params, userId, expectCode, NumTestUserInfo + 1);
}
#define assertAddUserWithSetup(P,C) cut_trace(_assertAddUserWithSetup(P,C))

#define assertUpdateUser(P, ...) \
cut_trace(_assertUpdateRecord(g_parser, P, "/user", ##__VA_ARGS__))

void _assertUpdateUserWithSetup(const StringMap &params,
                                uint32_t targetUserId,
                                const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	assertUpdateUser(params, targetUserId, userId, expectCode);
}
#define assertUpdateUserWithSetup(P,U,C) \
cut_trace(_assertUpdateUserWithSetup(P,U,C))

static void _assertUpdateAddUserMissing(
  const StringMap &parameters,
  const HatoholErrorCode expectErrorCode = HTERR_NOT_FOUND_PARAMETER)
{
	setupTestMode();
	startFaceRest();
	RequestArg arg("/test/user", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, expectErrorCode);
}
#define assertUpdateAddUserMissing(P,...) \
cut_trace(_assertUpdateAddUserMissing(P, ##__VA_ARGS__))

static void _assertUpdateOrAddUser(const string &name)
{
	setupTestMode();
	startFaceRest();

	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);

	StringMap parameters;
	parameters["name"] = name;
	parameters["password"] = "AR2c43fdsaf";
	parameters["flags"] = "0";

	RequestArg arg("/test/user", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_OK);

	// check the data in the DB
	string statement = StringUtils::sprintf(
	  "select name,password,flags from %s where name='%s'",
	  DBClientUser::TABLE_NAME_USERS, name.c_str());
	string expect = StringUtils::sprintf("%s|%s|%s",
	  parameters["name"].c_str(),
	  Utils::sha256( parameters["password"]).c_str(),
	  parameters["flags"].c_str());
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
}
#define assertUpdateOrAddUser(U) cut_trace(_assertUpdateOrAddUser(U))

static void _assertServerAccessInfo(JsonParserAgent *parser, HostGrpAccessInfoMap &expected)
{
	assertStartObject(parser, "allowedHostgroups");
	HostGrpAccessInfoMapIterator it = expected.begin();
	for (size_t i = 0; i < expected.size(); i++) {
		uint64_t hostgroupId = it->first;
		string idStr;
		if (hostgroupId == ALL_HOST_GROUPS)
			idStr = "-1";
		else
			idStr = StringUtils::sprintf("%" PRIu64, hostgroupId);
		assertStartObject(parser, idStr);
		AccessInfo *info = it->second;
		assertValueInParser(parser, "accessInfoId",
				    static_cast<uint64_t>(info->id));
		parser->endObject();
	}
	parser->endObject();
}
#define assertServerAccessInfo(P,I,...) cut_trace(_assertServerAccessInfo(P,I,##__VA_ARGS__))

static void _assertAllowedServers(const string &path, const UserIdType &userId,
                                  const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	ServerAccessInfoMap srvAccessInfoMap;
	makeServerAccessInfoMap(srvAccessInfoMap, userId);
	assertStartObject(g_parser, "allowedServers");
	ServerAccessInfoMapIterator it = srvAccessInfoMap.begin();
	for (; it != srvAccessInfoMap.end(); ++it) {
		const ServerIdType &serverId = it->first;
		string idStr;
		if (serverId == ALL_SERVERS)
			idStr = "-1";
		else
			idStr = StringUtils::toString(serverId);
		assertStartObject(g_parser, idStr);
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		assertServerAccessInfo(g_parser, *hostGrpAccessInfoMap);
		g_parser->endObject();
	}
	g_parser->endObject();
	DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
}
#define assertAllowedServers(P,...) cut_trace(_assertAllowedServers(P,##__VA_ARGS__))

#define assertAddAccessInfo(U, P, USER_ID, ...) \
cut_trace(_assertAddRecord(g_parser, P, U, USER_ID, ##__VA_ARGS__))

void _assertAddAccessInfoWithCond(
  const string &serverId, const string &hostgroupId,
  string expectHostgroupId = "")
{
	setupUserDB();

	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	const UserIdType targetUserId = 1;

	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info", targetUserId);
	StringMap params;
	params["serverId"] = serverId;
	params["hostgroupId"] = hostgroupId;
	assertAddAccessInfo(url, params, userId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_ACCESS_LIST;
	int expectedId = 1;
	if (expectHostgroupId.empty())
		expectHostgroupId = hostgroupId;
	string expect = StringUtils::sprintf(
	  "%" FMT_ACCESS_INFO_ID "|%" FMT_USER_ID "|%s|%s\n",
	  expectedId, targetUserId, serverId.c_str(),
	  expectHostgroupId.c_str());
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}
#define assertAddAccessInfoWithCond(SVID, HGRP_ID, ...) \
cut_trace(_assertAddAccessInfoWithCond(SVID, HGRP_ID, ##__VA_ARGS__))

static void _assertUserRole(JsonParserAgent *parser,
			    const UserRoleInfo &userRoleInfo,
			    uint32_t expectUserRoleId = 0)
{
	if (expectUserRoleId)
		assertValueInParser(parser, "userRoleId", expectUserRoleId);
	assertValueInParser(parser, "name", userRoleInfo.name);
	assertValueInParser(parser, "flags", userRoleInfo.flags);
}
#define assertUserRole(P,I,...) cut_trace(_assertUserRole(P,I,##__VA_ARGS__))

static void _assertUserRoles(const string &path,
			     const UserIdType &userId,
			     const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUserRoles", NumTestUserRoleInfo);
	assertStartObject(g_parser, "userRoles");
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		g_parser->startElement(i);
		const UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		assertUserRole(g_parser, userRoleInfo, i + 1);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertUserRoles(P, U, ...) \
  cut_trace(_assertUserRoles(P, U, ##__VA_ARGS__))

void _assertLogin(const string &user, const string &password)
{
	setupTestDBUser(true, true);
	startFaceRest();
	StringMap query;
	if (!user.empty())
		query["user"] = user;
	if (!password.empty())
		query["password"] = password;
	RequestArg arg("/login", "cbname");
	arg.parameters = query;
	g_parser = getResponseAsJsonParser(arg);
}
#define assertLogin(U,P) cut_trace(_assertLogin(U,P))

void test_login(void)
{
	const int targetIdx = 1;
	const UserInfo &userInfo = testUserInfo[targetIdx];
	assertLogin(userInfo.name, userInfo.password);
	assertErrorCode(g_parser);
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	cppcut_assert_equal(false, sessionId.empty());

	SessionManager *sessionMgr = SessionManager::getInstance();
	const SessionPtr session = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, session.hasData());
	cppcut_assert_equal(targetIdx + 1, session->userId);
	assertTimeIsNow(session->loginTime);
	assertTimeIsNow(session->lastAccessTime);
}

void test_loginFailure(void)
{
	assertLogin(testUserInfo[1].name, testUserInfo[0].password);
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoUserName(void)
{
	assertLogin("", testUserInfo[0].password);
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoPassword(void)
{
	assertLogin(testUserInfo[0].name, "");
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoUserNameAndPassword(void)
{
	assertLogin("", "");
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_logout(void)
{
	test_login();
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	delete g_parser;

	RequestArg arg("/logout", "cbname");
	arg.headers.push_back(makeSessionIdHeader(sessionId));
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	SessionManager *sessionMgr = SessionManager::getInstance();
	const SessionPtr session = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(false, session.hasData());
}

void test_getUser(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_GET_ALL_USER);
	assertUsers("/user", userId, "cbname");
}

void test_getUserMe(void)
{
	const UserInfo &user = testUserInfo[1];
	assertLogin(user.name, user.password);
	assertErrorCode(g_parser);
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	delete g_parser;
	g_parser = NULL;

	RequestArg arg("/user/me", "cbname");
	arg.headers.push_back(makeSessionIdHeader(sessionId));
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers", 1);
	assertStartObject(g_parser, "users");
	g_parser->startElement(0);
	assertUser(g_parser, user);
}

void test_addUser(void)
{
	OperationPrivilegeFlag flags = OPPRVLG_GET_ALL_USER;
	const string user = "y@ru0";
	const string password = "w(^_^)d";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertAddUserWithSetup(params, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USERS;
	statement += " order by id desc limit 1";
	const int expectedId = NumTestUserInfo + 1;
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  expectedId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUser(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  targetId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithoutFlags(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = testUserInfo[targetId - 1].flags;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  targetId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithInvalidFlags(void)
{
	const UserIdType targetId = 1;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	params["flags"] = "no-number";
	assertUpdateUserWithSetup(params, targetId, HTERR_INVALID_PARAMETER);
}

void test_updateUserWithoutPassword(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string expectedPassword = testUserInfo[targetId - 1].password;

	StringMap params;
	params["name"] = user;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  targetId, user.c_str(),
	  Utils::sha256(expectedPassword).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithoutUserId(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string expectedPassword = testUserInfo[targetId - 1].password;

	StringMap params;
	params["name"] = user;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, -1, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_addUserWithoutUser(void)
{
	StringMap params;
	params["password"] = "w(^_^)d";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserWithoutPassword(void)
{
	StringMap params;
	params["name"] = "y@ru0";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserWithoutFlags(void)
{
	StringMap params;
	params["name"] = "y@ru0";
	params["password"] = "w(^_^)d";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserInvalidUserName(void)
{
	StringMap params;
	params["name"] = "!^.^!"; // '!' and '^' are invalid characters
	params["password"] = "w(^_^)d";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_INVALID_CHAR);
}

void test_deleteUser(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const UserIdType userId = findUserWith(OPPRVLG_DELETE_USER);
	string url = StringUtils::sprintf("/user/%" FMT_USER_ID, userId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser);
	UserIdSet userIdSet;
	userIdSet.insert(userId);
	assertUsersInDB(userIdSet);
}

void test_deleteUserWithoutId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	RequestArg arg("/user", "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserWithNonNumericId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	RequestArg arg("/user/zoo", "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_updateOrAddUserNotInTestMode(void)
{
	setupUserDB();
	startFaceRest();
	RequestArg arg("/test/user", "cbname");
	arg.request = "POST";
	arg.userId = findUserWith(OPPRVLG_CREATE_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_TEST_MODE);
}

void test_updateOrAddUserMissingUser(void)
{
	StringMap parameters;
	parameters["password"] = "foo";
	parameters["flags"] = "0";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserMissingPassword(void)
{
	StringMap parameters;
	parameters["name"] = "ABC";
	parameters["flags"] = "2";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserMissingFlags(void)
{
	StringMap parameters;
	parameters["name"] = "ABC";
	parameters["password"] = "AR2c43fdsaf";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserAdd(void)
{
	const string name = "Tux";
	// make sure the name is not in test data.
	for (size_t i = 0; i < NumTestUserInfo; i++)
		cppcut_assert_not_equal(name, testUserInfo[i].name);
	assertUpdateOrAddUser(name);
}

void test_updateOrAddUserUpdate(void)
{
	const size_t targetIndex = 2;
	const UserInfo &userInfo = testUserInfo[targetIndex];
	assertUpdateOrAddUser(userInfo.name);
}

void test_getAccessInfo(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertAllowedServers("/user/1/access-info", 1, "cbname");
}

void test_getAccessInfoWithUserId3(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertAllowedServers("/user/3/access-info", 3, "cbname");
}

void test_addAccessInfo(void)
{
	const string serverId = "2";
	const string hostgroupId = "3";
	assertAddAccessInfoWithCond(serverId, hostgroupId);
}

void test_updateAccessInfo(void)
{
	setupUserDB();

	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	const UserIdType targetUserId = 1;
	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info/2", targetUserId);
	StringMap params;
	params["serverId"] = "2";
	params["hostgroupId"] = "-1";

	startFaceRest();

	RequestArg arg(url);
	arg.parameters = params;
	arg.request = "PUT";
	arg.userId = userId;
	getServerResponse(arg);
	cppcut_assert_equal(
	  static_cast<int>(SOUP_STATUS_METHOD_NOT_ALLOWED),
	  arg.httpStatusCode);
}

void test_addAccessInfoWithAllHostgroups(void)
{
	const string serverId = "2";
	const string hostgroupId =
	  StringUtils::sprintf("%" PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostgroupId);
}

void test_addAccessInfoWithAllHostgroupsNegativeValue(void)
{
	const string serverId = "2";
	const string hostgroupId = "-1";
	const string expectHostgroup =
	  StringUtils::sprintf("%" PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostgroupId, expectHostgroup);
}

void test_addAccessInfoWithExistingData(void)
{
	const bool dbRecreate = true;
	const bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	const UserIdType targetUserId = 1;
	const string serverId = "1";
	const string hostgroupId = "1";

	StringMap params;
	params["userId"] = StringUtils::sprintf("%" FMT_USER_ID, userId);
	params["serverId"] = serverId;
	params["hostgroupId"] = hostgroupId;

	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info", targetUserId);
	assertAddAccessInfo(url, params, userId, HTERR_OK, 2);

	AccessInfoIdSet accessInfoIdSet;
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_deleteAccessInfo(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const AccessInfoIdType targetId = 2;
	string url = StringUtils::sprintf(
	  "/user/1/access-info/%" FMT_ACCESS_INFO_ID,
	  targetId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_UPDATE_USER);
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser);
	AccessInfoIdSet accessInfoIdSet;
	accessInfoIdSet.insert(targetId);
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_getUserRole(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = false;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUserRole();
	assertUserRoles("/user-role", USER_ID_SYSTEM, "cbname");
}

#define assertAddUserRole(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/user-role", ##__VA_ARGS__))

void _assertAddUserRoleWithSetup(const StringMap &params,
				 const HatoholErrorCode &expectCode,
				 bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	UserIdType userId;
	if (operatorHasPrivilege)
		userId = findUserWith(OPPRVLG_CREATE_USER_ROLE);
	else
		userId = findUserWithout(OPPRVLG_CREATE_USER_ROLE);
	assertAddUserRole(params, userId, expectCode, NumTestUserRoleInfo + 1);
}
#define assertAddUserRoleWithSetup(P, C, ...) \
cut_trace(_assertAddUserRoleWithSetup(P, C, ##__VA_ARGS__))

void test_addUserRole(void)
{
	UserRoleInfo expectedUserRoleInfo;
	expectedUserRoleInfo.id = NumTestUserRoleInfo + 1;
	expectedUserRoleInfo.name = "maintainer";
	expectedUserRoleInfo.flags =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER);

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertAddUserRoleWithSetup(params, HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_addUserRoleWithoutPrivilege(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	bool operatorHasPrivilege = true;
	assertAddUserRoleWithSetup(params, HTERR_NO_PRIVILEGE,
				   !operatorHasPrivilege);

	assertUserRolesInDB();
}

void test_addUserRoleWithoutName(void)
{
	StringMap params;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	assertAddUserRoleWithSetup(params, HTERR_NOT_FOUND_PARAMETER);

	assertUserRolesInDB();
}

void test_addUserRoleWithoutFlags(void)
{
	StringMap params;
	params["name"] = "maintainer";
	assertAddUserRoleWithSetup(params, HTERR_NOT_FOUND_PARAMETER);

	assertUserRolesInDB();
}

void test_addUserRoleWithEmptyUserName(void)
{
	StringMap params;
	params["name"] = "";
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	assertAddUserRoleWithSetup(params, HTERR_EMPTY_USER_ROLE_NAME);

	assertUserRolesInDB();
}

void test_addUserRoleWithInvalidFlags(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, ALL_PRIVILEGES + 1);
	assertAddUserRoleWithSetup(params, HTERR_INVALID_PRIVILEGE_FLAGS);

	assertUserRolesInDB();
}

#define assertUpdateUserRole(P, ...) \
cut_trace(_assertUpdateRecord(g_parser, P, "/user-role", ##__VA_ARGS__))

void _assertUpdateUserRoleWithSetup(const StringMap &params,
				    uint32_t targetUserRoleId,
				    const HatoholErrorCode &expectCode,
				    bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	UserIdType userId;
	if (operatorHasPrivilege)
		userId = findUserWith(OPPRVLG_UPDATE_ALL_USER_ROLE);
	else
		userId = findUserWithout(OPPRVLG_UPDATE_ALL_USER_ROLE);
	assertUpdateUserRole(params, targetUserRoleId, userId, expectCode);
}
#define assertUpdateUserRoleWithSetup(P,T,E, ...) \
cut_trace(_assertUpdateUserRoleWithSetup(P,T,E, ##__VA_ARGS__))

void test_updateUserRole(void)
{
	UserRoleInfo expectedUserRoleInfo;
	expectedUserRoleInfo.id = 1;
	expectedUserRoleInfo.name = "ServerAdmin";
	expectedUserRoleInfo.flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutName(void)
{
	int targetIdx = 0;
	UserRoleInfo expectedUserRoleInfo = testUserRoleInfo[targetIdx];
	expectedUserRoleInfo.id = targetIdx + 1;
	expectedUserRoleInfo.flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutFlags(void)
{
	int targetIdx = 0;
	UserRoleInfo expectedUserRoleInfo = testUserRoleInfo[targetIdx];
	expectedUserRoleInfo.id = targetIdx + 1;
	expectedUserRoleInfo.name = "ServerAdmin";

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutPrivilege(void)
{
	UserRoleIdType targetUserRoleId = 1;
	OperationPrivilegeFlag flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["name"] = "ServerAdmin";
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	bool operatorHasPrivilege = true;
	assertUpdateUserRoleWithSetup(params, targetUserRoleId,
				      HTERR_NO_PRIVILEGE,
				      !operatorHasPrivilege);

	assertUserRolesInDB();
}

void _assertDeleteUserRoleWithSetup(
  string url = "",
  HatoholErrorCode expectedErrorCode = HTERR_OK,
  bool operatorHasPrivilege = true,
  const UserRoleIdType targetUserRoleId = 2)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	if (url.empty())
		url = StringUtils::sprintf("/user-role/%" FMT_USER_ROLE_ID,
					   targetUserRoleId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	if (operatorHasPrivilege)
		arg.userId = findUserWith(OPPRVLG_DELETE_ALL_USER_ROLE);
	else
		arg.userId = findUserWithout(OPPRVLG_DELETE_ALL_USER_ROLE);
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser, expectedErrorCode);
	UserRoleIdSet userRoleIdSet;
	if (expectedErrorCode == HTERR_OK)
		userRoleIdSet.insert(targetUserRoleId);
	// check the version
	assertUserRolesInDB(userRoleIdSet);
}
#define assertDeleteUserRoleWithSetup(...) \
cut_trace(_assertDeleteUserRoleWithSetup(__VA_ARGS__))

void test_deleteUserRole(void)
{
	assertDeleteUserRoleWithSetup();
}

void test_deleteUserRoleWithoutId(void)
{
	assertDeleteUserRoleWithSetup("/user-role",
				      HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserRoleWithNonNumericId(void)
{
	assertDeleteUserRoleWithSetup("/user-role/maintainer",
				      HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserRoleWithoutPrivilege(void)
{
	bool operatorHasPrivilege = true;
	assertDeleteUserRoleWithSetup(string(), // set automatically
				      HTERR_NO_PRIVILEGE,
				      !operatorHasPrivilege);
}

} // namespace testFaceRestUser
