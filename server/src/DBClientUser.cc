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

static const char *TABLE_NAME_USERS = "users";

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
	0,                                 // flags
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
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
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
}
};
static const size_t NUM_COLUMNS_USERS =
  sizeof(COLUMN_DEF_USERS) / sizeof(ColumnDef);

enum {
	IDX_USERS_ID,
	IDX_USERS_NAME,
	IDX_USERS_PASSWORD,
	NUM_IDX_USERS,
};

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

	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_USERS,
		NUM_COLUMNS_USERS,
		COLUMN_DEF_USERS,
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

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
