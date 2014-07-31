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
static const char *TABLE_NAME_HOST_INFO = "host_info";

const int DBClientHost::DB_VERSION = 1;

const uint64_t NO_HYPERVISOR = -1;
const size_t MAX_HOST_NAME_LENGTH =  255;

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
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_LIST,              // tableName
	"hypervisor_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
},
};

enum {
	IDX_HOST_LIST_ID,
	IDX_HOST_LIST_HYPERVISOR_ID,
	NUM_IDX_HOST_LIST
};

static const DBAgent::TableProfile tableProfileHostList(
  TABLE_NAME_HOST_LIST, COLUMN_DEF_HOST_LIST,
  sizeof(COLUMN_DEF_HOST_LIST), NUM_IDX_HOST_LIST);

// We manage multiple IP adresses and host naems for one host.
// So the following are defined in the independent table.
static const ColumnDef COLUMN_DEF_HOST_INFO[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_INFO,              // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_INFO,              // tableName
	"host_list_id",                    // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	// Both IPv4 and IPv6 can be saved
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_INFO,              // tableName
	"ip_addr",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	40,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOST_INFO,              // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	MAX_HOST_NAME_LENGTH,              // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
},
};

enum {
	IDX_HOST_INFO_ID,
	IDX_HOST_INFO_HOST_LIST_ID,
	IDX_HOST_INFO_IP_ADDR,
	IDX_HOST_INFO_NAME,
	NUM_IDX_HOST_INFO
};

static const DBAgent::TableProfile tableProfileHostInfo(
  TABLE_NAME_HOST_INFO, COLUMN_DEF_HOST_INFO,
  sizeof(COLUMN_DEF_HOST_INFO), NUM_IDX_HOST_INFO);

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
	}, {
		&tableProfileHostInfo,
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

