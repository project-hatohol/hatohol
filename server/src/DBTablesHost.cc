/*
 * Copyright (C) 2014 Project Hatohol
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

#include "DBTablesHost.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_HOST_LIST = "host_list";
static const char *TABLE_NAME_HOST_INFO = "host_info";

const int DBTablesHost::TABLES_VERSION = 1;

const uint64_t NO_HYPERVISOR = -1;
const size_t MAX_HOST_NAME_LENGTH =  255;

static const ColumnDef COLUMN_DEF_HOST_LIST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
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

static const DBAgent::TableProfile tableProfileHostList =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOST_LIST,
			    COLUMN_DEF_HOST_LIST,
			    NUM_IDX_HOST_LIST);

// We manage multiple IP adresses and host naems for one host.
// So the following are defined in the independent table.
static const ColumnDef COLUMN_DEF_HOST_INFO[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
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
	"ip_addr",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	40,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
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

static const DBAgent::TableProfile tableProfileHostInfo =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOST_INFO,
			    COLUMN_DEF_HOST_INFO,
			    NUM_IDX_HOST_INFO);

struct DBTablesHost::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

static bool updateDB(DBAgent &dbAgent, const int &oldVer, void *data)
{
	return true;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesHost::reset(void)
{
	getSetupInfo().initialized = false;
}

DBTablesHost::DBTablesHost(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesHost::~DBTablesHost()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBTables::SetupInfo &DBTablesHost::getSetupInfo(void)
{
	//
	// set database info
	//
	static const TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileHostList,
	}, {
		&tableProfileHostInfo,
	}
	};
	static const size_t NUM_TABLE_INFO =
	  ARRAY_SIZE(TABLE_INFO);

	static SetupInfo SETUP_INFO = {
		DB_TABLES_ID_HOST,
		TABLES_VERSION,
		NUM_TABLE_INFO,
		TABLE_INFO,
		&updateDB,
	};
	return SETUP_INFO;
}

