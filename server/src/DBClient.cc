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

#include <memory>

#include <MutexLock.h>
using namespace mlpl;

#include "DBClient.h"
#include "DBAgentFactory.h"

int DBClient::DBCLIENT_DB_VERSION = 1;
static const char *TABLE_NAME_DBCLIENT = "_dbclient";

static const ColumnDef COLUMN_DEF_DBCLIENT[] = {
{
	// This column has the schema version for '_dbclient' table.
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
	// This column has the schema version for the sub class's table.
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

// This structure instnace is created once every DB_DOMAIN_ID
struct DBClient::DBSetupContext {
	bool              initialized;
	MutexLock         mutex;
	string            dbName;
	DBSetupFuncArg   *dbSetupFuncArg;
	DBConnectInfo     connectInfo;

	DBSetupContext(void)
	: initialized(false),
	  dbSetupFuncArg(NULL)
	{
	}
};

struct DBClient::PrivateContext {

	DBAgent                 *dbAgent;

	// A DBSetupContext insntace is first stored in standbySetupCtxMap
	// when it is registered by registerSetupInfo(). It is moved to
	// dbSetupCtxMap when it is used. The purpose of this mechanism is
	// to reduce the time of the instance lookup, because there may
	// be instances that are registered but not actually used.
	static DBSetupContextMap standbySetupCtxMap;
	static DBSetupContextMap dbSetupCtxMap;
	static ReadWriteLock     dbSetupCtxMapLock;

	PrivateContext(void)
	: dbAgent(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		if (dbAgent)
			delete dbAgent;
	}

	static DBSetupContext *getDBSetupContext(DBDomainId domainId)
	{
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.find(domainId);
		DBSetupContext *setupCtx = NULL;
		if (it == dbSetupCtxMap.end()) {
			// search from the standby map.
			it = standbySetupCtxMap.find(domainId);
			HATOHOL_ASSERT(it != dbSetupCtxMap.end(),
			  "Failed to find. domainId: %d\n", domainId);

			// move the element to dbSetupCtxMap.
			setupCtx = it->second;
			standbySetupCtxMap.erase(it);
			dbSetupCtxMap[domainId] = setupCtx;
		} else {
			setupCtx = it->second;
		}
		setupCtx->mutex.lock();
		dbSetupCtxMapLock.unlock();
		return setupCtx;
	}

	static void registerSetupInfo(DBDomainId domainId, const string &dbName,
	                              DBSetupFuncArg *dbSetupFuncArg)
	{
		DBSetupContext *setupCtx = NULL;
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.find(domainId);
		if (it != dbSetupCtxMap.end()) {
			setupCtx = it->second;
		} else {
			// search from the standby map.
			it = standbySetupCtxMap.find(domainId);
			if (it != standbySetupCtxMap.end())
				setupCtx = it->second;
		}

		// make a new DBSetupContext if it is neither in dbSetupCtxMap
		// nor standbySetupCtxMap.
		if (!setupCtx) {
			setupCtx = new DBSetupContext();
			standbySetupCtxMap[domainId] = setupCtx;
		}

		setupCtx->dbName = dbName;
		setupCtx->dbSetupFuncArg = dbSetupFuncArg;
		dbSetupCtxMapLock.unlock();
	}

	static void clearInitializedFlag(void)
	{
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.begin();
		for (; it != dbSetupCtxMap.end(); ++it) {
			DBSetupContext *setupCtx = it->second;
			setupCtx->initialized = false;
			setupCtx->connectInfo.reset();
		}
		dbSetupCtxMapLock.unlock();
	}
};

DBClient::DBSetupContextMap
  DBClient::PrivateContext::standbySetupCtxMap;
DBClient::DBSetupContextMap
  DBClient::PrivateContext::dbSetupCtxMap;
ReadWriteLock DBClient::PrivateContext::dbSetupCtxMapLock;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

// static method
void DBClient::reset(void)
{
	// We assume that this function is called in the test.
	PrivateContext::clearInitializedFlag();
}

void DBClient::setDefaultDBParams(
  DBDomainId domainId,
  const string &dbName, const string &user, const string &password)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	setupCtx->connectInfo.dbName   = dbName;
	setupCtx->connectInfo.user     = user;
	setupCtx->connectInfo.password = password;
	setupCtx->mutex.unlock();

	bool usePassword = !setupCtx->connectInfo.password.empty();
	MLPL_INFO("Default DB: %s, User: %s, use passowrd: %s\n",
	          setupCtx->connectInfo.dbName.c_str(),
	          setupCtx->connectInfo.user.c_str(),
	          usePassword ? "yes" : "no");
}

DBConnectInfo DBClient::getDBConnectInfo(DBDomainId domainId)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	DBConnectInfo connInfo = setupCtx->connectInfo;
	setupCtx->mutex.unlock();
	return connInfo; // we return the copy
}

// non static method
DBClient::DBClient(DBDomainId domainId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	if (!setupCtx->initialized) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		DBAgent::addSetupFunction(
		  domainId, dbSetupFunc, (void *)setupCtx->dbSetupFuncArg);
		bool skipSetup = false;
		m_ctx->dbAgent = DBAgentFactory::create(domainId, skipSetup,
		                                        &setupCtx->connectInfo);
		setupCtx->initialized = true;
		setupCtx->mutex.unlock();
	} else {
		setupCtx->mutex.unlock();
		bool skipSetup = true;
		m_ctx->dbAgent = DBAgentFactory::create(domainId, skipSetup,
		                                        &setupCtx->connectInfo);
	}
}

DBClient::~DBClient()
{
	if (m_ctx)
		delete m_ctx;
}

DBAgent *DBClient::getDBAgent(void) const
{
	return m_ctx->dbAgent;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// static methods
void DBClient::registerSetupInfo(
  DBDomainId domainId, const string &dbName,
  DBSetupFuncArg *dbSetupFuncArg)
{
	PrivateContext::registerSetupInfo(domainId, dbName, dbSetupFuncArg);
}

void DBClient::setConnectInfo(
  DBDomainId domainId, const DBConnectInfo &connectInfo)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	setupCtx->connectInfo = connectInfo;
	setupCtx->dbSetupFuncArg->connectInfo = &setupCtx->connectInfo;
	setupCtx->mutex.unlock();
}

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
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, DBCLIENT_DB_VERSION);
	row->ADD_NEW_ITEM(Int, setupFuncArg->version);
	insArg.row = row;
	dbAgent->insert(insArg);
}

void DBClient::updateDBIfNeeded(DBAgent *dbAgent, DBSetupFuncArg *setupFuncArg)
{
	// check the version for this class's database
	const ColumnDef *columnDef =
	 &COLUMN_DEF_DBCLIENT[IDX_DBCLIENT_SELF_DB_VERSION];
	int dbVersion = getDBVersion(dbAgent, columnDef);
	if (dbVersion != DBCLIENT_DB_VERSION) {
		THROW_HATOHOL_EXCEPTION("Not implemented: %s\n",
		                      __PRETTY_FUNCTION__);
	}

	// check the version for the sub class's database
	columnDef = &COLUMN_DEF_DBCLIENT[IDX_DBCLIENT_VERSION];
	dbVersion = getDBVersion(dbAgent, columnDef);
	if (dbVersion != setupFuncArg->version) {
		void *data = setupFuncArg->dbUpdaterData;
		HATOHOL_ASSERT(setupFuncArg->dbUpdater,
		             "Updater: NULL, expect/actual ver. %d/%d",
		             setupFuncArg->version, dbVersion);
		bool succeeded =
		  (*setupFuncArg->dbUpdater)(dbAgent, dbVersion, data);
		HATOHOL_ASSERT(succeeded,
		               "Failed to update DB, expect/actual ver. %d/%d",
		               setupFuncArg->version, dbVersion);
		setDBVersion(dbAgent, columnDef, setupFuncArg->version);
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
	HATOHOL_ASSERT(
	  itemGroupList.size() == 1,
	  "itemGroupList.size(): %zd", itemGroupList.size());
	ItemGroupListConstIterator it = itemGroupList.begin();
	const ItemGroup *itemGroup = *it;
	HATOHOL_ASSERT(
	  itemGroup->getNumberOfItems() == 1,
	  "itemGroup->getNumberOfItems: %zd", itemGroup->getNumberOfItems());
	const ItemInt *itemVersion =
	  dynamic_cast<const ItemInt *>(itemGroup->getItemAt(0));
	HATOHOL_ASSERT(itemVersion != NULL, "type: itemVersion: %s\n",
	             DEMANGLED_TYPE_NAME(*itemVersion));
	return itemVersion->get();
}

void DBClient::setDBVersion(DBAgent *dbAgent, const ColumnDef *columnDef, int version)
{
	DBAgentUpdateArg arg;
	arg.tableName = columnDef->tableName;
	arg.columnDefs = columnDef;
	arg.columnIndexes.push_back(0);
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, version);
	arg.row = row;
	dbAgent->update(arg);
}

// non-static methods
void DBClient::dbSetupFunc(DBDomainId domainId, void *data)
{
	DBSetupFuncArg *setupFuncArg = static_cast<DBSetupFuncArg *>(data);
	bool skipSetup = true;
	const DBConnectInfo *connectInfo = setupFuncArg->connectInfo;
	auto_ptr<DBAgent> rawDBAgent(DBAgentFactory::create(domainId,
	                                                    skipSetup,
	                                                    connectInfo));
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

void DBClient::addColumns(DBAgentAddColumnsArg &addColumnsArg)
{
	getDBAgent()->addColumns(addColumnsArg);
}

bool DBClient::isRecordExisting(const string &tableName,
                                const string &condition)
{
	return getDBAgent()->isRecordExisting(tableName, condition);
}

uint64_t DBClient::getLastInsertId(void)
{
	return getDBAgent()->getLastInsertId();
}

bool DBClient::updateIfExistElseInsert(
  const ItemGroup *itemGroup, const string &tableName,
  size_t numColumns, const ColumnDef *columnDefs, size_t targetIndex)
{
	return getDBAgent()->updateIfExistElseInsert(
	         itemGroup, tableName, numColumns, columnDefs, targetIndex);
}

