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

const int   DBClientUser::USER_DB_VERSION = 1;
const char *DBClientUser::DEFAULT_DB_NAME = DBClientConfig::DEFAULT_DB_NAME;
const char *DBClientUser::TABLE_NAME_USERS = "users";
const char *DBClientUser::TABLE_NAME_ACCESS_LIST = "access_list";
const size_t DBClientUser::MAX_USER_NAME_LENGTH = 128;
const size_t DBClientUser::MAX_PASSWORD_LENGTH = 128;
static const char *TABLE_NAME_USERS = DBClientUser::TABLE_NAME_USERS;
static const char *TABLE_NAME_ACCESS_LIST =
  DBClientUser::TABLE_NAME_ACCESS_LIST;

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
static const size_t NUM_COLUMNS_USERS =
  sizeof(COLUMN_DEF_USERS) / sizeof(ColumnDef);

enum {
	IDX_USERS_ID,
	IDX_USERS_NAME,
	IDX_USERS_PASSWORD,
	IDX_USERS_FLAGS,
	NUM_IDX_USERS,
};


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
static const size_t NUM_COLUMNS_ACCESS_LIST =
  sizeof(COLUMN_DEF_ACCESS_LIST) / sizeof(ColumnDef);

enum {
	IDX_ACCESS_LIST_ID,
	IDX_ACCESS_LIST_USER_ID,
	IDX_ACCESS_LIST_SERVER_ID,
	IDX_ACCESS_LIST_HOST_GROUP_ID,
	NUM_IDX_ACCESS_LIST,
};

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

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientUser::init(void)
{
	HATOHOL_ASSERT(
	  NUM_COLUMNS_USERS == NUM_IDX_USERS,
	  "Invalid number of elements: NUM_COLUMNS_USERS (%zd), "
	  "NUM_IDX_USERS (%d)",
	  NUM_COLUMNS_USERS, NUM_IDX_USERS);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_ACCESS_LIST == NUM_IDX_ACCESS_LIST,
	  "Invalid number of elements: NUM_COLUMNS_ACCESS_LIST (%zd), "
	  "NUM_IDX_ACCESS_LIST (%d)",
	  NUM_COLUMNS_ACCESS_LIST, NUM_IDX_ACCESS_LIST);

	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_USERS,
		NUM_COLUMNS_USERS,
		COLUMN_DEF_USERS,
	}, {
		TABLE_NAME_ACCESS_LIST,
		NUM_COLUMNS_ACCESS_LIST,
		COLUMN_DEF_ACCESS_LIST,
	},
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

	static DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		USER_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
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

DBClientUserError DBClientUser::addUserInfo(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	DBClientUserError err;
	if (!privilege.has(OPPRVLG_CREATE_USER))
		return DBCUSRERR_NO_PRIVILEGE;
	err = isValidUserName(userInfo.name);
	if (err != DBCUSRERR_NO_ERROR)
		return err;
	err = isValidPassword(userInfo.password);
	if (err != DBCUSRERR_NO_ERROR)
		return err;
	err = isValidFlags(userInfo.flags);
	if (err != DBCUSRERR_NO_ERROR)
		return err;

	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_USERS;
	arg.numColumns = NUM_COLUMNS_USERS;
	arg.columnDefs = COLUMN_DEF_USERS;

	row->ADD_NEW_ITEM(Int, 0); // This is automaticall set (0 is dummy)
	row->ADD_NEW_ITEM(String, userInfo.name);
	row->ADD_NEW_ITEM(String, Utils::sha256(userInfo.password));
	row->ADD_NEW_ITEM(Uint64, userInfo.flags);
	arg.row = row;

	string dupCheckCond = StringUtils::sprintf("%s='%s'",
	  COLUMN_DEF_USERS[IDX_USERS_NAME].columnName, userInfo.name.c_str());

	DBCLIENT_TRANSACTION_BEGIN() {
		if (isRecordExisting(TABLE_NAME_USERS, dupCheckCond)) {
			err = DBCUSRERR_USER_NAME_EXIST;
		} else {
			insert(arg);
			userInfo.id = getLastInsertId();
			err = DBCUSRERR_NO_ERROR;
		}
	} DBCLIENT_TRANSACTION_END();
	return err;
}

UserIdType DBClientUser::getUserId(const string &user, const string &password)
{
	if (isValidUserName(user) != DBCUSRERR_NO_ERROR)
		return INVALID_USER_ID;
	if (isValidPassword(password) != DBCUSRERR_NO_ERROR)
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

	const ItemGroup *itemGroup = *grpList.begin();
	int idx = 0;

	// user ID
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemUserId);
	UserIdType userId = itemUserId->get();

	// password
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemString, itemPasswd);
	string truePasswd = itemPasswd->get();

	// comapare the passwords
	bool matched = (truePasswd == Utils::sha256(password));
	if (!matched)
		return INVALID_USER_ID;
	return userId;
}

void DBClientUser::addAccessInfo(AccessInfo &accessInfo)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	arg.numColumns = NUM_COLUMNS_ACCESS_LIST;
	arg.columnDefs = COLUMN_DEF_ACCESS_LIST;

	row->ADD_NEW_ITEM(Int, 0); // This is automatically set (0 is dummy)
	row->ADD_NEW_ITEM(Int, accessInfo.userId);
	row->ADD_NEW_ITEM(Int, accessInfo.serverId);
	row->ADD_NEW_ITEM(Uint64, accessInfo.hostGroupId);
	arg.row = row;

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		accessInfo.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
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
                                   DataQueryOption &option)
{
	UserIdType userId = option.getUserId();
	if (userId == INVALID_USER_ID) {
		MLPL_WARN("INVALID_USER_ID\n");
		return;
	}

	string condition;
	if (!option.has(OPPRVLG_GET_ALL_USERS)) {
		condition = StringUtils::sprintf("%s=%"FMT_USER_ID"",
		  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);
	}
	getUserInfoList(userInfoList, condition);
}

void DBClientUser::getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
                                    const UserIdType userId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACCESS_LIST;
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_HOST_GROUP_ID]);
	arg.condition = StringUtils::sprintf("%s=%"FMT_USER_ID"",
	  COLUMN_DEF_ACCESS_LIST[IDX_ACCESS_LIST_USER_ID].columnName, userId);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		const ItemGroup *itemGroup = *it;
		int idx = 0;
		AccessInfo *accessInfo = new AccessInfo();

		// ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemId);
		accessInfo->id = itemId->get();

		// user ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
		                  ItemInt, itemUserId);
		accessInfo->userId = itemUserId->get();

		// server ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
		                  ItemInt, itemServerId);
		accessInfo->serverId = itemServerId->get();

		// host group ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
		                  ItemUint64, itemHostGrpId);
		accessInfo->hostGroupId = itemHostGrpId->get();

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
			          "%"PRIu32 ", %" PRIu64"\n",
			          accessInfo->serverId,
			          accessInfo->hostGroupId);
			delete accessInfo;
			continue;
		}
		(*hostGrpAccessInfoMap)[accessInfo->hostGroupId] = accessInfo;
	}
}

void DBClientUser::getServerHostGrpSetMap(
  ServerHostGrpSetMap &srvHostGrpSetMap, const UserIdType userId)
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
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		const ItemGroup *itemGroup = *it;
		int idx = 0;

		// server ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
		                  ItemInt, itemServerId);
		uint32_t serverId = itemServerId->get();

		// host group ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
		                  ItemUint64, itemHostGrpId);
		uint64_t hostGroupId = itemHostGrpId->get();

		// insert data
		pair<HostGroupSetIterator, bool> result =
		  srvHostGrpSetMap[serverId].insert(hostGroupId);
		if (!result.second) {
			MLPL_WARN("Found duplicated serverId and hostGroupId: "
			          "%"PRIu32 ", %" PRIu64"\n",
			          serverId, hostGroupId);
			continue;
		}
	}
}

DBClientUserError DBClientUser::isValidUserName(const string &name)
{
	if (name.empty())
		return DBCUSRERR_EMPTY_USER_NAME;
	if (name.size() > MAX_USER_NAME_LENGTH)
		return DBCUSRERR_TOO_LONG_USER_NAME;
	for (const char *p = name.c_str(); *p; p++) {
		uint8_t idx = *p;
		if (!PrivateContext::validUsernameChars[idx])
			return DBCUSRERR_INVALID_CHAR;
	}
	return DBCUSRERR_NO_ERROR;
}

DBClientUserError DBClientUser::isValidPassword(const string &password)
{
	if (password.empty())
		return DBCUSRERR_EMPTY_PASSWORD;
	if (password.size() > MAX_PASSWORD_LENGTH)
		return DBCUSRERR_TOO_LONG_PASSWORD;
	return DBCUSRERR_NO_ERROR;
}

DBClientUserError DBClientUser::isValidFlags(const OperationPrivilegeFlag flags)
{
	if (flags > OperationPrivilege::makeFlag(NUM_OPPRVLG))
		return DBCUSRERR_INVALID_USER_FLAGS;
	return DBCUSRERR_NO_ERROR;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientUser::getUserInfoList(UserInfoList &userInfoList,
                                   const string &condition)
{
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
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		UserInfo userInfo;

		// user ID
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt,
		                  itemUserId);
		userInfo.id = itemUserId->get();

		// password
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemString,
		                  itemName);
		userInfo.name = itemName->get();

		// password
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemString,
		                  itemPasswd);
		userInfo.password = itemPasswd->get();

		// flags
		DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemUint64,
		                  itemFlags);
		userInfo.flags = itemFlags->get();

		userInfoList.push_back(userInfo);
	}
}

