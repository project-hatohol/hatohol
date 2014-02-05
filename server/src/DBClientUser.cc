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

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "DBClientUser.h"
#include "DBClientConfig.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

const UserIdSet EMPTY_USER_ID_SET;
const AccessInfoIdSet EMPTY_ACCESS_INFO_ID_SET;
const UserRoleIdSet EMPTY_USER_ROLE_ID_SET;

// 1 -> 2:
//   * NUM_OPPRVLG:10 -> 16
// 2 -> 3:
//   * NUM_OPPRVLG:16 -> 19
//   * Add user_roles table
const int   DBClientUser::USER_DB_VERSION = 3;

const char *DBClientUser::DEFAULT_DB_NAME = DBClientConfig::DEFAULT_DB_NAME;
const char *DBClientUser::TABLE_NAME_USERS = "users";
const char *DBClientUser::TABLE_NAME_ACCESS_LIST = "access_list";
const char *DBClientUser::TABLE_NAME_USER_ROLES = "user_roles";
const size_t DBClientUser::MAX_USER_NAME_LENGTH = 128;
const size_t DBClientUser::MAX_PASSWORD_LENGTH = 128;
const size_t DBClientUser::MAX_USER_ROLE_NAME_LENGTH = 128;
static const char *TABLE_NAME_USERS = DBClientUser::TABLE_NAME_USERS;
static const char *TABLE_NAME_ACCESS_LIST =
  DBClientUser::TABLE_NAME_ACCESS_LIST;
static const char *TABLE_NAME_USER_ROLES =
  DBClientUser::TABLE_NAME_USER_ROLES;

static const ColumnDef COLUMN_DEF_USERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USERS,                  // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USERS,                  // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USERS,                  // tableName
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USERS,                  // tableName
	"flags",                           // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_USERS_ID,
	IDX_USERS_NAME,
	IDX_USERS_PASSWORD,
	IDX_USERS_FLAGS,
	NUM_IDX_USERS,
};

static DBAgent::TableProfile tableProfileUsers(
  DBClientUser::TABLE_NAME_USERS, COLUMN_DEF_USERS,
  sizeof(COLUMN_DEF_USERS), NUM_IDX_USERS);

static const ColumnDef COLUMN_DEF_ACCESS_LIST[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACCESS_LIST,            // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACCESS_LIST,            // tableName
	"user_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACCESS_LIST,            // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACCESS_LIST,            // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_ACCESS_LIST_ID,
	IDX_ACCESS_LIST_USER_ID,
	IDX_ACCESS_LIST_SERVER_ID,
	IDX_ACCESS_LIST_HOST_GROUP_ID,
	NUM_IDX_ACCESS_LIST,
};

static DBAgent::TableProfile tableProfileAccessList(
  DBClientUser::TABLE_NAME_ACCESS_LIST, COLUMN_DEF_ACCESS_LIST,
  sizeof(COLUMN_DEF_ACCESS_LIST), NUM_IDX_ACCESS_LIST);

static const ColumnDef COLUMN_DEF_USER_ROLES[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USER_ROLES,             // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USER_ROLES,             // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_USER_ROLES,             // tableName
	"flags",                           // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_USER_ROLES_ID,
	IDX_USER_ROLES_NAME,
	IDX_USER_ROLES_FLAGS,
	NUM_IDX_USER_ROLES,
};

static DBAgent::TableProfile tableProfileUserRoles(
  DBClientUser::TABLE_NAME_USER_ROLES, COLUMN_DEF_USER_ROLES,
  sizeof(COLUMN_DEF_USER_ROLES), NUM_IDX_USER_ROLES);

ServerAccessInfoMap::~ServerAccessInfoMap()
{
	for (ServerAccessInfoMapIterator it = begin(); it != end(); ++it) {
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		HostGrpAccessInfoMapIterator jt = hostGrpAccessInfoMap->begin();
		for (; jt != hostGrpAccessInfoMap->end(); ++jt)
			delete jt->second;
		delete hostGrpAccessInfoMap;
	}
}

struct DBClientUser::PrivateContext {
	static bool validUsernameChars[UINT8_MAX+1];
};

bool DBClientUser::PrivateContext::validUsernameChars[UINT8_MAX+1];

static void updateAdminPrivilege(DBAgent *dbAgent,
				 const OperationPrivilegeType old_NUM_OPPRVLG)
{
	static const OperationPrivilegeFlag oldAdminFlags =
		OperationPrivilege::makeFlag(old_NUM_OPPRVLG) - 1;
	DBAgent::UpdateArg arg(tableProfileUsers);
	arg.add(IDX_USERS_FLAGS, ALL_PRIVILEGES);
	arg.condition = StringUtils::sprintf(
	  "%s=%"FMT_OPPRVLG,
	  COLUMN_DEF_USERS[IDX_USERS_FLAGS].columnName, oldAdminFlags);
	dbAgent->update(arg);
}

static bool updateDB(DBAgent *dbAgent, int oldVer, void *data)
{
	static OperationPrivilegeType old_NUM_OPPRVLG;

	if (oldVer <= 1) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(10);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	if (oldVer <= 2) {
		old_NUM_OPPRVLG = static_cast<OperationPrivilegeType>(16);
		updateAdminPrivilege(dbAgent, old_NUM_OPPRVLG);
	}
	return true;
}

// ---------------------------------------------------------------------------
// UserQueryOption
// ---------------------------------------------------------------------------
struct UserQueryOption::PrivateContext {
	bool   onlyMyself;
	string targetName;

	PrivateContext(void)
	: onlyMyself(false)
	{
	}
};

UserQueryOption::UserQueryOption(UserIdType userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

UserQueryOption::~UserQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

HatoholError UserQueryOption::setTargetName(const string &name)
{
	HatoholError err = DBClientUser::isValidUserName(name);
	if (err != HTERR_OK)
		return err;
	m_ctx->targetName = name;
	return HatoholError(HTERR_OK);
}

void UserQueryOption::queryOnlyMyself(void)
{
	m_ctx->onlyMyself = true;
}

string UserQueryOption::getCondition(void) const
{
	UserIdType userId = getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return DBClientUser::getAlwaysFalseCondition();
	}

	string condition;
	if (!has(OPPRVLG_GET_ALL_USER) || m_ctx->onlyMyself) {
		condition = StringUtils::sprintf("%s=%"FMT_USER_ID"",
		  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);
	}

	if (!m_ctx->targetName.empty()) {
		// The validity of 'targetName' has been checked in
		// setTargetName().
		string nameCond =
		  StringUtils::sprintf("%s='%s'",
		    COLUMN_DEF_USERS[IDX_USERS_NAME].columnName,
		   m_ctx->targetName.c_str());
		if (condition.empty()) {
			condition = nameCond;
		} else {
			condition += " AND ";
			condition += nameCond;
		}
	}
	return condition;
}

// ---------------------------------------------------------------------------
// AccessInfoQueryOption
// ---------------------------------------------------------------------------
struct AccessInfoQueryOption::PrivateContext {
	UserIdType queryUserId;

	PrivateContext(void)
	: queryUserId(INVALID_USER_ID)
	{
	}
};

AccessInfoQueryOption::AccessInfoQueryOption(UserIdType userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

AccessInfoQueryOption::~AccessInfoQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}
string AccessInfoQueryOption::getCondition(void) const
{
	UserIdType userId = getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return DBClientUser::getAlwaysFalseCondition();
	}

	if (!has(OPPRVLG_GET_ALL_USER) && getUserId() != m_ctx->queryUserId) {
		return DBClientUser::getAlwaysFalseCondition();
	}

	return StringUtils::sprintf("%s=%"FMT_USER_ID"",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  getTargetUserId());
}

void AccessInfoQueryOption::setTargetUserId(UserIdType userId)
{
	m_ctx->queryUserId = userId;
}

UserIdType AccessInfoQueryOption::getTargetUserId(void) const
{
	return m_ctx->queryUserId;
}

// ---------------------------------------------------------------------------
// UserRoleQueryOption
// ---------------------------------------------------------------------------
struct UserRoleQueryOption::PrivateContext {
	UserRoleIdType targetUserRoleId;

	PrivateContext(void)
	: targetUserRoleId(INVALID_USER_ROLE_ID)
	{
	}
};

UserRoleQueryOption::UserRoleQueryOption(UserIdType userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

UserRoleQueryOption::~UserRoleQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

void UserRoleQueryOption::setTargetUserRoleId(UserRoleIdType userRoleId)
{
	m_ctx->targetUserRoleId = userRoleId;
}

UserRoleIdType UserRoleQueryOption::getTargetUserRoleId(void) const
{
	return m_ctx->targetUserRoleId;
}

string UserRoleQueryOption::getCondition(void) const
{
	string condition;

	if (m_ctx->targetUserRoleId != INVALID_USER_ROLE_ID)
		condition = StringUtils::sprintf("%s=%"FMT_USER_ROLE_ID"",
		  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
		  m_ctx->targetUserRoleId);

	return condition;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientUser::init(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_USERS,
		tableProfileUsers.numColumns,
		COLUMN_DEF_USERS,
	}, {
		TABLE_NAME_ACCESS_LIST,
		tableProfileAccessList.numColumns,
		COLUMN_DEF_ACCESS_LIST,
	}, {
		TABLE_NAME_USER_ROLES,
		tableProfileUserRoles.numColumns,
		COLUMN_DEF_USER_ROLES,
	},
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		USER_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
		&updateDB,
	};

	registerSetupInfo(
	  DB_DOMAIN_ID_USERS, DEFAULT_DB_NAME, &DB_SETUP_FUNC_ARG);

	// set valid characters for the user name
	for (uint8_t c = 'A'; c <= 'Z'; c++)
		PrivateContext::validUsernameChars[c] = true;
	for (uint8_t c = 'a'; c <= 'z'; c++)
		PrivateContext::validUsernameChars[c] = true;
	for (uint8_t c = '0'; c <= '9'; c++)
		PrivateContext::validUsernameChars[c] = true;
	PrivateContext::validUsernameChars['.'] = true;
	PrivateContext::validUsernameChars['-'] = true;
	PrivateContext::validUsernameChars['_'] = true;
	PrivateContext::validUsernameChars['@'] = true;
}

void DBClientUser::reset(void)
{
	// We share the connection information with CONFIG.
	DBConnectInfo connInfo = getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	setConnectInfo(DB_DOMAIN_ID_USERS, connInfo);
}

DBClientUser::DBClientUser(void)
: DBClient(DB_DOMAIN_ID_USERS),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientUser::~DBClientUser()
{
	if (m_ctx)
		delete m_ctx;
}

HatoholError DBClientUser::addUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_CREATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidPassword(userInfo.password);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userInfo.flags);
	if (err != HTERR_OK)
		return err;

	DBAgent::InsertArg arg(tableProfileUsers);
	arg.row->addNewItem(AUTO_INCREMENT_VALUE);
	arg.row->addNewItem(userInfo.name);
	arg.row->addNewItem(Utils::sha256(userInfo.password));
	arg.row->addNewItem(userInfo.flags);

	string dupCheckCond = StringUtils::sprintf("%s='%s'",
	  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName, userInfo.name.c_str());

	DBCLIENT_TRANSACTION_BEGIN() {
		if (isRecordExisting(TABLE_NAME_USERS, dupCheckCond)) {
			err = HTERR_USER_NAME_EXIST;
		} else {
			insert(arg);
			userInfo.id = getLastInsertId();
			err = HTERR_OK;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

HatoholError DBClientUser::hasPrivilegeForUpdateUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_UPDATE_USER))
		return HTERR_OK;
	if (userInfo.id != privilege.getUserId())
		return HTERR_NO_PRIVILEGE;
	if (userInfo.id == INVALID_USER_ID)
		return HTERR_NO_PRIVILEGE;

	UserInfo userInfoInDB;
	bool exist = getUserInfo(userInfoInDB, userInfo.id);
	if (!exist)
		return HTERR_NO_PRIVILEGE;
	userInfoInDB.password = userInfo.password;
	if (userInfoInDB == userInfo)
		return HTERR_OK;

	return HTERR_NO_PRIVILEGE;
}

HatoholError DBClientUser::updateUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	HatoholError err = hasPrivilegeForUpdateUserInfo(userInfo, privilege);
	if (err != HTERR_OK)
		return err;
	err = isValidUserName(userInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidPassword(userInfo.password);
	if (!userInfo.password.empty() && err != HTERR_OK)
		return err;
	err = isValidFlags(userInfo.flags);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileUsers);

	arg.add(IDX_USERS_NAME, userInfo.name);
	if (!userInfo.password.empty())
		arg.add(IDX_USERS_PASSWORD, Utils::sha256(userInfo.password));
	arg.add(IDX_USERS_FLAGS, userInfo.flags);

	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ID,
	  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userInfo.id);

	string dupCheckCond = StringUtils::sprintf(
	  "(%s='%s' and %s<>%" FMT_USER_ID ")",
	  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName,
	  userInfo.name.c_str(),
	  COLUMN_DEF_USERS[IDX_USERS_ID].columnName,
	  userInfo.id);

	DBCLIENT_TRANSACTION_BEGIN() {
		const char *tableName = arg.tableProfile.name;
		if (!isRecordExisting(tableName, arg.condition)) {
			err = HTERR_NOT_FOUND_USER_ID;
		} else if (isRecordExisting(tableName, dupCheckCond)) {
			err = HTERR_USER_NAME_EXIST;
		} else {
			update(arg);
			err = HTERR_OK;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

HatoholError DBClientUser::deleteUserInfo(
  const UserIdType userId, const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_DELETE_USER))
		return HTERR_NO_PRIVILEGE;

	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_USERS;
	const ColumnDef &colId = COLUMN_DEF_USERS[IDX_USERS_ID];
	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ID,
	                                     colId.columnName, userId);
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

UserIdType DBClientUser::getUserId(const string &user, const string &password)
{
	if (isValidUserName(user) != HTERR_OK)
		return INVALID_USER_ID;
	if (isValidPassword(password) != HTERR_OK)
		return INVALID_USER_ID;

	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_USERS;
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_ID]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_PASSWORD]);
	arg.condition = StringUtils::sprintf("%s='%s'",
	  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName, user.c_str());
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return INVALID_USER_ID;

	ItemGroupStream itemGroupStream(*grpList.begin());
	UserIdType userId;
	string truePasswd;
	itemGroupStream >> userId;
	itemGroupStream >> truePasswd;

	// comapare the passwords
	bool matched = (truePasswd == Utils::sha256(password));
	if (!matched)
		return INVALID_USER_ID;
	return userId;
}

HatoholError DBClientUser::addAccessInfo(AccessInfo &accessInfo,
					 const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	// check existing data
	DBAgentSelectExArg selectArg;
	selectArg.tableName = TABLE_NAME_ACCESS_LIST;
	selectArg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_ID]);
	selectArg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID]);
	selectArg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID]);
	selectArg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID]);
	selectArg.condition = StringUtils::sprintf(
	  "%s=%"FMT_USER_ID" AND %s=%"PRIu32" AND %s=%"PRIu64,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  accessInfo.userId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  accessInfo.serverId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  accessInfo.hostGroupId);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(selectArg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = selectArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	if (it != grpList.end()) {
		const ItemGroup *itemGroup = *it;
		accessInfo.id = *itemGroup->getItemAt(0);
		return HTERR_OK;
	}

	// add new data
	DBAgent::InsertArg arg(tableProfileAccessList);
	arg.row->addNewItem(AUTO_INCREMENT_VALUE);
	arg.row->addNewItem(accessInfo.userId);
	arg.row->addNewItem(accessInfo.serverId);
	arg.row->addNewItem(accessInfo.hostGroupId);

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		accessInfo.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

HatoholError DBClientUser::deleteAccessInfo(const AccessInfoIdType id,
					    const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_USER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	const ColumnDef &colId = COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_ID];
	arg.condition = StringUtils::sprintf("%s=%"FMT_ACCESS_INFO_ID,
	                                     colId.columnName, id);
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

bool DBClientUser::getUserInfo(UserInfo &userInfo, const UserIdType userId)
{
	UserInfoList userInfoList;
	string condition = StringUtils::sprintf("%s='%"FMT_USER_ID"'",
	  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);
	getUserInfoList(userInfoList, condition);
	if (userInfoList.empty())
		return false;
	HATOHOL_ASSERT(userInfoList.size() == 1, "userInfoList.size(): %zd\n",
	               userInfoList.size());
	userInfo = *userInfoList.begin();
	return true;
}

void DBClientUser::getUserInfoList(UserInfoList &userInfoList,
                                   const UserQueryOption &option)
{
	getUserInfoList(userInfoList, option.getCondition());
}

HatoholError DBClientUser::getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
					    const AccessInfoQueryOption &option)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID]);
	arg.condition = option.getCondition();

	if (isAlwaysFalseCondition(arg.condition))
		return HTERR_NO_PRIVILEGE;

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		AccessInfo *accessInfo = new AccessInfo();
		itemGroupStream >> accessInfo->id;
		itemGroupStream >> accessInfo->userId;
		itemGroupStream >> accessInfo->serverId;
		itemGroupStream >> accessInfo->hostGroupId;

		// insert data
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = NULL;
		ServerAccessInfoMapIterator it =
		  srvAccessInfoMap.find(accessInfo->serverId);
		if (it == srvAccessInfoMap.end()) {
			hostGrpAccessInfoMap = new HostGrpAccessInfoMap();
			srvAccessInfoMap[accessInfo->serverId] =
			   hostGrpAccessInfoMap;
		} else {
			hostGrpAccessInfoMap = it->second;
		}
		
		HostGrpAccessInfoMapIterator jt =
		  hostGrpAccessInfoMap->find(accessInfo->hostGroupId);
		if (jt != hostGrpAccessInfoMap->end()) {
			MLPL_WARN("Found duplicated serverId and hostGroupId: "
			          "%"FMT_SERVER_ID", %"PRIu64"\n",
			          accessInfo->serverId,
			          accessInfo->hostGroupId);
			delete accessInfo;
			continue;
		}
		(*hostGrpAccessInfoMap)[accessInfo->hostGroupId] = accessInfo;
	}

	return HTERR_OK;
}

void DBClientUser::destroyServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap)
{
	ServerAccessInfoMapIterator it = srvAccessInfoMap.begin();
	for (; it != srvAccessInfoMap.end(); it++) {
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		HostGrpAccessInfoMapIterator it2 = hostGrpAccessInfoMap->begin();
		for (; it2 != hostGrpAccessInfoMap->end(); it2++) {
			AccessInfo *info = it2->second;
			delete info;
		}
		delete hostGrpAccessInfoMap;
	}
	srvAccessInfoMap.clear();
}

void DBClientUser::getServerHostGrpSetMap(
  ServerHostGrpSetMap &srvHostGrpSetMap, const UserIdType &userId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID]);
	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ID"",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName, userId);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ServerIdType serverId;
		uint64_t hostGroupId;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> serverId;
		itemGroupStream >> hostGroupId;

		// insert data
		pair<HostGroupSetIterator, bool> result =
		  srvHostGrpSetMap[serverId].insert(hostGroupId);
		if (!result.second) {
			MLPL_WARN("Found duplicated serverId and hostGroupId: "
			          "%"FMT_SERVER_ID", %"PRIu64"\n",
			          serverId, hostGroupId);
			continue;
		}
	}
}

HatoholError DBClientUser::addUserRoleInfo(UserRoleInfo &userRoleInfo,
					   const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_CREATE_USER_ROLE))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserRoleName(userRoleInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userRoleInfo.flags);
	if (err != HTERR_OK)
		return err;
	if (userRoleInfo.flags == ALL_PRIVILEGES ||
	    userRoleInfo.flags == NONE_PRIVILEGE) {
		return HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST;
	}

	DBAgent::InsertArg arg(tableProfileUserRoles);
	arg.row->addNewItem(AUTO_INCREMENT_VALUE);
	arg.row->addNewItem(userRoleInfo.name);
	arg.row->addNewItem(userRoleInfo.flags);

	string dupCheckCond = StringUtils::sprintf(
	  "(%s='%s' or %s=%"FMT_OPPRVLG")",
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_NAME].columnName,
	  StringUtils::replace(userRoleInfo.name, "'", "''").c_str(),
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_FLAGS].columnName,
	  userRoleInfo.flags);

	DBCLIENT_TRANSACTION_BEGIN() {
		if (isRecordExisting(TABLE_NAME_USER_ROLES, dupCheckCond)) {
			err = HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST;
		} else {
			insert(arg);
			userRoleInfo.id = getLastInsertId();
			err = HTERR_OK;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

HatoholError DBClientUser::updateUserRoleInfo(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	HatoholError err;
	if (!privilege.has(OPPRVLG_UPDATE_ALL_USER_ROLE))
		return HatoholError(HTERR_NO_PRIVILEGE);
	err = isValidUserRoleName(userRoleInfo.name);
	if (err != HTERR_OK)
		return err;
	err = isValidFlags(userRoleInfo.flags);
	if (err != HTERR_OK)
		return err;
	if (userRoleInfo.flags == ALL_PRIVILEGES ||
	    userRoleInfo.flags == NONE_PRIVILEGE) {
		return HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST;
	}

	DBAgent::UpdateArg arg(tableProfileUserRoles);
	arg.add(IDX_USER_ROLES_NAME, userRoleInfo.name);
	arg.add(IDX_USER_ROLES_FLAGS, userRoleInfo.flags);

	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ROLE_ID,
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
	  userRoleInfo.id);

	string dupCheckCond = StringUtils::sprintf(
	  "((%s='%s' or %s=%" FMT_OPPRVLG ") and %s<>%" FMT_USER_ROLE_ID ")",
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_NAME].columnName,
	  StringUtils::replace(userRoleInfo.name, "'", "''").c_str(),
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_FLAGS].columnName,
	  userRoleInfo.flags,
	  COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID].columnName,
	  userRoleInfo.id);

	DBCLIENT_TRANSACTION_BEGIN() {
		const char *tableName = arg.tableProfile.name;
		if (!isRecordExisting(tableName, arg.condition)) {
			err = HTERR_NOT_FOUND_USER_ROLE_ID;
		} else if (isRecordExisting(tableName, dupCheckCond)) {
			err = HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST;
		} else {
			update(arg);
			err = HTERR_OK;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

HatoholError DBClientUser::deleteUserRoleInfo(
  const UserRoleIdType userRoleId, const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_DELETE_ALL_USER_ROLE))
		return HTERR_NO_PRIVILEGE;

	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_USER_ROLES;
	const ColumnDef &colId = COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID];
	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ROLE_ID,
	                                     colId.columnName, userRoleId);
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

void DBClientUser::getUserRoleInfoList(UserRoleInfoList &userRoleInfoList,
				       const UserRoleQueryOption &option)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_USER_ROLES;
	arg.pushColumn(COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_ID]);
	arg.pushColumn(COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_NAME]);
	arg.pushColumn(COLUMN_DEF_USER_ROLES[IDX_USER_ROLES_FLAGS]);
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		UserRoleInfo userRoleInfo;
		itemGroupStream >> userRoleInfo.id;
		itemGroupStream >> userRoleInfo.name;
		itemGroupStream >> userRoleInfo.flags;
		userRoleInfoList.push_back(userRoleInfo);
	}
}

HatoholError DBClientUser::isValidUserName(const string &name)
{
	if (name.empty())
		return HTERR_EMPTY_USER_NAME;
	if (name.size() > MAX_USER_NAME_LENGTH)
		return HTERR_TOO_LONG_USER_NAME;
	for (const char *p = name.c_str(); *p; p++) {
		uint8_t idx = *p;
		if (!PrivateContext::validUsernameChars[idx])
			return HTERR_INVALID_CHAR;
	}
	return HTERR_OK;
}

HatoholError DBClientUser::isValidPassword(const string &password)
{
	if (password.empty())
		return HTERR_EMPTY_PASSWORD;
	if (password.size() > MAX_PASSWORD_LENGTH)
		return HTERR_TOO_LONG_PASSWORD;
	return HTERR_OK;
}

HatoholError DBClientUser::isValidFlags(const OperationPrivilegeFlag flags)
{
	if (flags >= OperationPrivilege::makeFlag(NUM_OPPRVLG))
		return HTERR_INVALID_USER_FLAGS;
	return HTERR_OK;
}

HatoholError DBClientUser::isValidUserRoleName(const string &name)
{
	if (name.empty())
		return HTERR_EMPTY_USER_ROLE_NAME;
	if (name.size() > MAX_USER_ROLE_NAME_LENGTH)
		return HTERR_TOO_LONG_USER_ROLE_NAME;
	return HTERR_OK;
}

bool DBClientUser::isAccessible(const ServerIdType &serverId,
                                const OperationPrivilege &privilege,
                                const bool &useTransaction)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return false;
	}

	string condition = StringUtils::sprintf(
	  "%s=%"FMT_USER_ID" AND "
	  "(%s=%"FMT_SERVER_ID" OR %s=%"FMT_SERVER_ID") AND "
	  "%s=%"PRIu64,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName,
	  userId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  serverId,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID].columnName,
	  ALL_SERVERS,
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID].columnName,
	  ALL_HOST_GROUPS);

	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	arg.statements.push_back("count(*)");
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_INT);
	arg.condition = condition;

	if (useTransaction) {
		DBCLIENT_TRANSACTION_BEGIN() {
			select(arg);
		} DBCLIENT_TRANSACTION_END();
	} else {
		select(arg);
	}

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "No result");
	const ItemGroup *itemGroup = *grpList.begin();
	int count = *itemGroup->getItemAt(0);
	return count > 0;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientUser::getUserInfoList(UserInfoList &userInfoList,
                                   const string &condition)
{
	if (isAlwaysFalseCondition(condition))
		return;

	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_USERS;
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_ID]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_NAME]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_PASSWORD]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_FLAGS]);
	arg.condition = condition;

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		UserInfo userInfo;
		itemGroupStream >> userInfo.id;
		itemGroupStream >> userInfo.name;
		itemGroupStream >> userInfo.password;
		itemGroupStream >> userInfo.flags;
		userInfoList.push_back(userInfo);
	}
}

