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

#include <Mutex.h>
#include "DB.h"
#include "DBAgentMySQL.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_TABLES_VERSION = "_tables_version";
static const DBDomainId JUNK_DID = -1; // TODO: should be removed from DBAgent

static const ColumnDef COLUMN_DEF_TABLES_VERSION[] = {
{
	// This column has the schema version for '_dbclient' table.
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TABLES_VERSION,         // tableName
	"tables_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// This column has the schema version for the sub class's table.
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TABLES_VERSION,         // tableName
	"version",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const DBAgent::TableProfile tableProfileTablesVersion =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_TABLES_VERSION,
                            COLUMN_DEF_TABLES_VERSION,
                            DB::NUM_IDX_TABLES);

DB::SetupContext::SetupContext(const DBType &_dbType)
: dbType(_dbType),
  initialized(false)
{
}

struct DB::Impl {
	static const string  alwaysFalseCondition;
	unique_ptr<DBAgent>  dbAgent;

	struct SetupProc : DBAgent::TransactionProc {
		void operator ()(DBAgent &dbAgent) override
		{
			if (dbAgent.isTableExisting(TABLE_NAME_TABLES_VERSION))
				return;
			dbAgent.createTable(tableProfileTablesVersion);
		}
	};

	Impl(SetupContext &setupCtx)
	{
		if (setupCtx.dbType == DB_MYSQL) {
			createDBAgentMySQL(setupCtx.connectInfo);
		} else {
			HATOHOL_ASSERT(false,
			               "Unknown DBType: %d\n", setupCtx.dbType);
		}

		AutoMutex autoMutex(&setupCtx.lock);
		// NOTE: A flag 'setupCtx.initialized' is used to improve
		// performance. Event if it isn't used, the existence check
		// of TABLE_NAME_TABLE_VERSION is performed every time.

		if (!setupCtx.initialized) {
			SetupProc setup;
			dbAgent->transaction(setup);
			setupCtx.initialized = true;
		}
	}

	void createDBAgentMySQL(const DBConnectInfo &connectInfo)
	{
		HATOHOL_ASSERT(!dbAgent.get(), "dbAgent is NOT NULL");
		dbAgent = unique_ptr<DBAgent>(
		            new DBAgentMySQL(connectInfo.dbName.c_str(),
		                             connectInfo.getUser(),
		                             connectInfo.getPassword(),
		                             connectInfo.getHost(),
		                             connectInfo.port,
		                             JUNK_DID, true /*skipSetup*/));
	}
};
const string DB::Impl::alwaysFalseCondition = "0";

const std::string &DB::getAlwaysFalseCondition(void)
{
	return Impl::alwaysFalseCondition;
}

bool DB::isAlwaysFalseCondition(const std::string &condition)
{
	return Impl::alwaysFalseCondition == condition;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
const DBAgent::TableProfile &DB::getTableProfileTablesVersion(void)
{
	return tableProfileTablesVersion;
}

DB::~DB()
{
}

DBAgent &DB::getDBAgent(void)
{
	return *m_impl->dbAgent.get();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DB::DB(SetupContext &setupCtx)
: m_impl(new Impl(setupCtx))
{
}

