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

#include <Mutex.h>
#include "DB.h"
#include "DBAgentMySQL.h"
#include "HatoholException.h"
#include "DBAgentFactory.h"

using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_TABLES_VERSION = "_tables_version";

static const ColumnDef COLUMN_DEF_TABLES_VERSION[] = {
{
	"tables_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
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

DB::SetupContext::SetupContext(const type_info &_dbClassType)
: dbClassType(_dbClassType),
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
	: dbAgent(DBAgentFactory::create(setupCtx.dbClassType,
	                                 setupCtx.connectInfo))
	{
		if (setupCtx.initialized)
			return;

		AutoMutex autoMutex(&setupCtx.lock);
		// NOTE: A flag 'setupCtx.initialized' is used to improve
		// performance. Event if it isn't used, the existence check
		// of TABLE_NAME_TABLE_VERSION is performed every time.

		if (!setupCtx.initialized) {
			SetupProc setup;
			dbAgent->runTransaction(setup);
			setupCtx.initialized = true;
		}
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

