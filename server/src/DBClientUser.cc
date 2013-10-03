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

#include "DBClientUser.h"
#include "DBClientConfig.h"

const int   DBClientUser::USER_DB_VERSION = 1;
const char *DBClientUser::DEFAULT_DB_NAME = DBClientConfig::DEFAULT_DB_NAME;
const char *DBClientUser::TABLE_NAME_USERS = "users";
const char *DBClientUser::TABLE_NAME_ACCESS_LIST = "access_list";
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
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
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

AccessInfoMap::~AccessInfoMap()
{
	AccessInfoMapIterator it = begin();
	for (; it != end(); ++it) {
		AccessInfoHGMap *accessInfoHGMap = it->second;
		AccessInfoHGMapIterator jt = accessInfoHGMap->begin();
		for (; jt != accessInfoHGMap->end(); ++jt)
			delete jt->second;
		delete accessInfoHGMap;
	}
}

struct DBClientUser::PrivateContext {
};

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
}

void DBClientUser::reset(void)
{
	DBConnectInfo connInfo = getDBConnectInfo(DB_DOMAIN_ID_USERS);
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

void DBClientUser::addUserInfo(UserInfo &userInfo)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_USERS;
	arg.numColumns = NUM_COLUMNS_USERS;
	arg.columnDefs = COLUMN_DEF_USERS;

	row->ADD_NEW_ITEM(Int, 0); // This is automaticall set (0 is dummy)
	row->ADD_NEW_ITEM(String, userInfo.name);
	row->ADD_NEW_ITEM(String, Utils::sha256(userInfo.password));
	row->ADD_NEW_ITEM(Int, userInfo.flags);
	arg.row = row;

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		userInfo.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
}

UserIdType DBClientUser::getUserId(const string &user, const string &password)
{
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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_USERS;
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_ID]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_NAME]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_PASSWORD]);
	arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_FLAGS]);
	arg.condition = StringUtils::sprintf("%s='%"FMT_USER_ID"'",
	  COLUMN_DEF_USERS[IDX_USERS_ID].columnName, userId);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return false;

	const ItemGroup *itemGroup = *grpList.begin();
	int idx = 0;

	// user ID
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemUserId);
	userInfo.id = itemUserId->get();

	// password
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemString, itemName);
	userInfo.name = itemName->get();

	// password
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemString, itemPasswd);
	userInfo.password = itemPasswd->get();

	// flags
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemFlags);
	userInfo.flags = itemFlags->get();

	return true;
}

void DBClientUser::getAccessInfoMap(AccessInfoMap &accessInfoMap,
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
		AccessInfoHGMap *accessInfoHGMap = NULL;
		AccessInfoMapIterator it =
		  accessInfoMap.find(accessInfo->serverId);
		if (it == accessInfoMap.end()) {
			accessInfoHGMap = new AccessInfoHGMap();
			accessInfoMap[accessInfo->serverId] = accessInfoHGMap;
		} else {
			accessInfoHGMap = it->second;
		}
		
		AccessInfoHGMapIterator jt =
		  accessInfoHGMap->find(accessInfo->hostGroupId);
		if (jt != accessInfoHGMap->end()) {
			MLPL_WARN("Found duplicated: serverId and hostGroupId: "
			          "%"PRIu32 ", %" PRIu64"\n",
			          accessInfo->serverId,
			          accessInfo->hostGroupId);
			delete accessInfo;
			continue;
		}
		(*accessInfoHGMap)[accessInfo->hostGroupId] = accessInfo;
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
