/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <memory>
#include "DBClient.h"
#include "DBAgentFactory.h"

int DBClient::DB_VERSION = 1;
static const char *TABLE_NAME_DBCLIENT = "_dbclient";

static const ColumnDef COLUMN_DEF_DBCLIENT[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_DBCLIENT,               // tableName
	"self_db_version",                 // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_DBCLIENT,               // tableName
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
static const size_t NUM_COLUMNS_DBCLIENT =
  sizeof(COLUMN_DEF_DBCLIENT) / sizeof(ColumnDef);

enum {
	IDX_DBCLIENT_SELF_DB_VERSION,
	IDX_DBCLIENT_VERSION,
	NUM_IDX_DBCLIENT,
};

struct DBClient::PrivateContext
{
	static GMutex mutex;
	DBAgent      *dbAgent;

	PrivateContext(void)
	: dbAgent(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		if (dbAgent)
			delete dbAgent;
	}

	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};
GMutex DBClient::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClient::DBClient(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClient::~DBClient()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// static methods
void DBClient::createTable
  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
   const ColumnDef *columnDefs, CreateTableInitializer initializer, void *data)
{
	DBAgentTableCreationArg arg;
	arg.tableName  = tableName;
	arg.numColumns = numColumns;
	arg.columnDefs = columnDefs;
	dbAgent->createTable(arg);
	if (initializer)
		(*initializer)(dbAgent, data);
}

void DBClient::tableInitializerDBClient(DBAgent *dbAgent, void *data)
{
	DBSetupFuncArg *setupFuncArg = static_cast<DBSetupFuncArg *>(data);

	// insert default value
	DBAgentInsertArg insArg;
	insArg.tableName = TABLE_NAME_DBCLIENT;
	insArg.numColumns = NUM_COLUMNS_DBCLIENT;
	insArg.columnDefs = COLUMN_DEF_DBCLIENT;
	insArg.row->add(new ItemInt(DB_VERSION), false);
	insArg.row->add(new ItemInt(setupFuncArg->version), false);
	dbAgent->insert(insArg);
}

void DBClient::updateDBIfNeeded(DBAgent *dbAgent, DBSetupFuncArg *setupFuncArg)
{
	// check the version for this class's database
	const ColumnDef *columnDef =
	 &COLUMN_DEF_DBCLIENT[IDX_DBCLIENT_SELF_DB_VERSION];
	int dbVersion = getDBVersion(dbAgent, columnDef);
	if (dbVersion != DBClient::DB_VERSION) {
		THROW_ASURA_EXCEPTION("Not implemented: %s\n",
		                      __PRETTY_FUNCTION__);
	}

	// check the version for this class's database
	columnDef = &COLUMN_DEF_DBCLIENT[IDX_DBCLIENT_VERSION];
	dbVersion = getDBVersion(dbAgent, columnDef);
	if (dbVersion != setupFuncArg->version) {
		void *data = setupFuncArg->dbUpdaterData;
		(*setupFuncArg->dbUpdater)(dbAgent, dbVersion, data);
	}
}

int DBClient::getDBVersion(DBAgent *dbAgent, const ColumnDef *columnDef)
{
	DBAgentSelectArg arg;
	arg.tableName = columnDef->tableName;
	arg.columnDefs = columnDef;
	arg.columnIndexes.push_back(0);
	dbAgent->select(arg);

	const ItemGroupList &itemGroupList = arg.dataTable->getItemGroupList();
	ASURA_ASSERT(
	  itemGroupList.size() == 1,
	  "itemGroupList.size(): %zd", itemGroupList.size());
	ItemGroupListConstIterator it = itemGroupList.begin();
	const ItemGroup *itemGroup = *it;
	ASURA_ASSERT(
	  itemGroup->getNumberOfItems() == 1,
	  "itemGroup->getNumberOfItems: %zd", itemGroup->getNumberOfItems());
	ItemInt *itemVersion =
	  dynamic_cast<ItemInt *>(itemGroup->getItemAt(0));
	ASURA_ASSERT(itemVersion != NULL, "type: itemVersion: %s\n",
	             DEMANGLED_TYPE_NAME(*itemVersion));
	return itemVersion->get();
}

// non-static methods
void DBClient::dbSetupFunc(DBDomainId domainId, void *data)
{
	DBSetupFuncArg *setupFuncArg = static_cast<DBSetupFuncArg *>(data);
	bool skipSetup = true;
	auto_ptr<DBAgent> rawDBAgent(DBAgentFactory::create(domainId,
	                                                    skipSetup));
	if (!rawDBAgent->isTableExisting(TABLE_NAME_DBCLIENT)) {
		createTable(rawDBAgent.get(),
		            TABLE_NAME_DBCLIENT, NUM_COLUMNS_DBCLIENT,
		            COLUMN_DEF_DBCLIENT, tableInitializerDBClient,
		            setupFuncArg);
	} else {
		updateDBIfNeeded(rawDBAgent.get(), setupFuncArg);
	}

	for (size_t i = 0; i < setupFuncArg->numTableInfo; i++) {
		const DBSetupTableInfo &tableInfo
		  = setupFuncArg->tableInfoArray[i];
		if (rawDBAgent->isTableExisting(tableInfo.name))
			continue;
		createTable(rawDBAgent.get(), tableInfo.name,
		            tableInfo.numColumns, tableInfo.columnDefs);
		if (!tableInfo.initializer)
			continue;
		(*tableInfo.initializer)(rawDBAgent.get(),
	                                 tableInfo.initializerData);
	}
}

void DBClient::setDBAgent(DBAgent *dbAgent)
{
	m_ctx->dbAgent = dbAgent;
}

DBAgent *DBClient::getDBAgent(void) const
{
	return m_ctx->dbAgent;
}

void DBClient::begin(void)
{
	getDBAgent()->begin();
}

void DBClient::rollback(void)
{
	getDBAgent()->rollback();
}

void DBClient::commit(void)
{
	getDBAgent()->commit();
}

void DBClient::insert(DBAgentInsertArg &insertArg)
{
	getDBAgent()->insert(insertArg);
}

void DBClient::update(DBAgentUpdateArg &updateArg)
{
	getDBAgent()->update(updateArg);
}

void DBClient::select(DBAgentSelectArg &selectArg)
{
	getDBAgent()->select(selectArg);
}

void DBClient::select(DBAgentSelectExArg &selectExArg)
{
	getDBAgent()->select(selectExArg);
}

void DBClient::deleteRows(DBAgentDeleteArg &deleteArg)
{
	getDBAgent()->deleteRows(deleteArg);
}
