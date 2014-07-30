/*
 * Copyright (C) 2014 Project Hatohol
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

#include "DBClientHost.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_HOST_LIST = "host_list";

const int DBClientHost::DB_VERSION = 1;

static const ColumnDef COLUMN_DEF_HOST_LIST[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_LIST,              // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
},
};

enum {
	IDX_HOST_LIST,
	NUM_IDX_HOST_LIST
};

static const DBAgent::TableProfile tableProfileHostList(
  TABLE_NAME_HOST_LIST, COLUMN_DEF_HOST_LIST,
  sizeof(COLUMN_DEF_HOST_LIST), NUM_IDX_HOST_LIST);

struct DBClientHost::PrivateContext
{
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

static bool updateDB(DBAgent *dbAgent, int oldVer, void *data)
{
	return true;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientHost::init(void)
{
	//
	// set database info
	//
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		&tableProfileHostList,
	}
	};
	static const size_t NUM_TABLE_INFO =
	  sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
		&updateDB,
	};
	registerSetupInfo(DB_DOMAIN_ID_HOST, &DB_SETUP_FUNC_ARG);
}

DBClientHost::DBClientHost(void)
: DBCGroupRegular(DB_DOMAIN_ID_HOST),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientHost::~DBClientHost()
{
	delete m_ctx;
}

