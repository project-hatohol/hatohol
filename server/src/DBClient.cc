/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include "DBClient.h"
#include "DBAgentFactory.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_DBCLIENT_VERSION = "_dbclient_version";

static const ColumnDef COLUMN_DEF_DBCLIENT_VERSION[] = {
{
	// This column has the schema version for '_dbclient' table.
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_DBCLIENT_VERSION,       // tableName
	"domain_id",                       // columnName
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
	TABLE_NAME_DBCLIENT_VERSION,       // tableName
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

enum {
	IDX_DBCLIENT_VERSION_DOMAIN_ID,
	IDX_DBCLIENT_VERSION_VERSION,
	NUM_IDX_DBCLIENT,
};

static DBAgent::TableProfile tableProfileDBClientVersion(
  TABLE_NAME_DBCLIENT_VERSION, COLUMN_DEF_DBCLIENT_VERSION,
  sizeof(COLUMN_DEF_DBCLIENT_VERSION), NUM_IDX_DBCLIENT);

// This structure instnace is created once every DB_DOMAIN_ID
struct DBClient::DBSetupContext {
	bool              initialized;
	MutexLock         mutex;
	string            dbName;
	const DBSetupFuncArg *dbSetupFuncArg;
	DBConnectInfo     connectInfo;

	DBSetupContext(void)
	: initialized(false),
	  dbSetupFuncArg(NULL)
	{
	}
};

struct DBClient::PrivateContext {

	DBAgent                 *dbAgent;

	static DBSetupContextMap dbSetupCtxMap;
	static ReadWriteLock     dbSetupCtxMapLock;

	static const string      alwaysFalseCondition;

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
		HATOHOL_ASSERT(it != dbSetupCtxMap.end(),
		               "Failed to find. domainId: %d\n", domainId);
		DBSetupContext *setupCtx = it->second;
		setupCtx->mutex.lock();
		dbSetupCtxMapLock.unlock();
		return setupCtx;
	}

	static void registerSetupInfo(DBDomainId domainId, const string &dbName,
	                              const DBSetupFuncArg *dbSetupFuncArg)
	{
		DBSetupContext *setupCtx = NULL;
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.find(domainId);
		if (it != dbSetupCtxMap.end()) {
			dbSetupCtxMapLock.unlock();
			return;
		}

		// make a new DBSetupContext if it doen't exist.
		if (!setupCtx) {
			setupCtx = new DBSetupContext();
			dbSetupCtxMap[domainId] = setupCtx;
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
  DBClient::PrivateContext::dbSetupCtxMap;
ReadWriteLock DBClient::PrivateContext::dbSetupCtxMapLock;
const string DBClient::PrivateContext::alwaysFalseCondition = "0";

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
		  domainId, dbSetupFunc, (void *)setupCtx);
		bool skipSetup = false;
		m_ctx->dbAgent =
		  DBAgentFactory::create(domainId, setupCtx->dbName,
		                         skipSetup, &setupCtx->connectInfo);
		setupCtx->initialized = true;
		setupCtx->mutex.unlock();
	} else {
		setupCtx->mutex.unlock();
		bool skipSetup = true;
		m_ctx->dbAgent =
		  DBAgentFactory::create(domainId, setupCtx->dbName,
		                         skipSetup, &setupCtx->connectInfo);
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

const std::string &DBClient::getAlwaysFalseCondition(void)
{
	return PrivateContext::alwaysFalseCondition;
}

bool DBClient::isAlwaysFalseCondition(const std::string &condition)
{
	return PrivateContext::alwaysFalseCondition == condition;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// static methods
void DBClient::registerSetupInfo(
  DBDomainId domainId, const string &dbName,
  const DBSetupFuncArg *dbSetupFuncArg)
{
	PrivateContext::registerSetupInfo(domainId, dbName, dbSetupFuncArg);
}

void DBClient::setConnectInfo(
  DBDomainId domainId, const DBConnectInfo &connectInfo)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	setupCtx->connectInfo = connectInfo;
	setupCtx->mutex.unlock();
}

void DBClient::createTable
  (DBAgent *dbAgent, const DBAgent::TableProfile &tableProfile,
   CreateTableInitializer initializer, void *data)
{
	dbAgent->createTable(tableProfile);
	if (initializer)
		(*initializer)(dbAgent, data);
}

void DBClient::insertDBClientVersion(DBAgent *dbAgent,
                                     const DBSetupFuncArg *setupFuncArg)
{
	// insert default value
	DBAgent::InsertArg arg(tableProfileDBClientVersion);
	arg.row->addNewItem(dbAgent->getDBDomainId());
	arg.row->addNewItem(setupFuncArg->version);
	dbAgent->insert(arg);
}

void DBClient::updateDBIfNeeded(DBDomainId domainId, DBAgent *dbAgent,
                                const DBSetupFuncArg *setupFuncArg)
{
	int dbVersion = getDBVersion(dbAgent);
	if (dbVersion != setupFuncArg->version) {
		MLPL_INFO("DBDomain %d is outdated, try to update it ...\n"
		          "\told-version: %"PRIu32"\n"
		          "\tnew-version: %"PRIu32"\n",
		          domainId, dbVersion, setupFuncArg->version);
		void *data = setupFuncArg->dbUpdaterData;
		HATOHOL_ASSERT(setupFuncArg->dbUpdater,
		             "Updater: NULL, expect/actual ver. %d/%d",
		             setupFuncArg->version, dbVersion);
		bool succeeded =
		  (*setupFuncArg->dbUpdater)(dbAgent, dbVersion, data);
		HATOHOL_ASSERT(succeeded,
		               "Failed to update DB, expect/actual ver. %d/%d",
		               setupFuncArg->version, dbVersion);
		setDBVersion(dbAgent, setupFuncArg->version);
		MLPL_INFO("Succeeded in updating DBDomain %"PRIu32"\n",
		          domainId);
	}
}

int DBClient::getDBVersion(DBAgent *dbAgent)
{
	DBAgent::SelectExArg arg(tableProfileDBClientVersion);
	arg.add(IDX_DBCLIENT_VERSION_VERSION);
	arg.condition = StringUtils::sprintf("%s=%d",
	  COLUMN_DEF_DBCLIENT_VERSION[IDX_DBCLIENT_VERSION_DOMAIN_ID].columnName,
	  dbAgent->getDBDomainId());
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

void DBClient::setDBVersion(DBAgent *dbAgent, int version)
{
	DBAgent::UpdateArg arg(tableProfileDBClientVersion);
	arg.add(IDX_DBCLIENT_VERSION_VERSION, version);
	arg.condition = StringUtils::sprintf("%s=%d",
	  COLUMN_DEF_DBCLIENT_VERSION[IDX_DBCLIENT_VERSION_DOMAIN_ID].columnName,
	  dbAgent->getDBDomainId());
	dbAgent->update(arg);
}

// non-static methods
void DBClient::dbSetupFunc(DBDomainId domainId, void *data)
{
	DBSetupContext *setupCtx = static_cast<DBSetupContext *>(data);
	const DBSetupFuncArg *setupFuncArg = setupCtx->dbSetupFuncArg;
	bool skipSetup = true;
	const DBConnectInfo *connectInfo = &setupCtx->connectInfo;
	auto_ptr<DBAgent> rawDBAgent(DBAgentFactory::create(domainId,
	                                                    setupCtx->dbName,
	                                                    skipSetup,
	                                                    connectInfo));
	if (!rawDBAgent->isTableExisting(TABLE_NAME_DBCLIENT_VERSION))
		createTable(rawDBAgent.get(), tableProfileDBClientVersion);

	// If the row that has the version of this DBClient doesn't exist,
	// we insert it.
	string condition = StringUtils::sprintf("%s=%d",
	  COLUMN_DEF_DBCLIENT_VERSION[IDX_DBCLIENT_VERSION_DOMAIN_ID].columnName,
	  domainId);
	if (!rawDBAgent->isRecordExisting(TABLE_NAME_DBCLIENT_VERSION,
	                                  condition))
		insertDBClientVersion(rawDBAgent.get(), setupFuncArg);
	else
		updateDBIfNeeded(domainId, rawDBAgent.get(), setupFuncArg);

	for (size_t i = 0; i < setupFuncArg->numTableInfo; i++) {
		const DBSetupTableInfo &tableInfo
		  = setupFuncArg->tableInfoArray[i];
		if (rawDBAgent->isTableExisting(tableInfo.profile->name))
			continue;
		createTable(rawDBAgent.get(), *tableInfo.profile);
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

void DBClient::insert(const DBAgent::InsertArg &insertArg)
{
	getDBAgent()->insert(insertArg);
}

void DBClient::update(const DBAgent::UpdateArg &updateArg)
{
	getDBAgent()->update(updateArg);
}

void DBClient::select(const DBAgent::SelectArg &selectArg)
{
	getDBAgent()->select(selectArg);
}

void DBClient::select(DBAgentSelectExArg &selectExArg)
{
	getDBAgent()->select(selectExArg);
}

void DBClient::select(const DBAgent::SelectExArg &selectExArg)
{
	getDBAgent()->select(selectExArg);
}

void DBClient::deleteRows(const DBAgent::DeleteArg &deleteArg)
{
	getDBAgent()->deleteRows(deleteArg);
}

void DBClient::addColumns(const DBAgent::AddColumnsArg &addColumnsArg)
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
  const ItemGroup *itemGroup, const DBAgent::TableProfile &tableProfile,
  size_t targetIndex)
{
	return getDBAgent()->updateIfExistElseInsert(itemGroup, tableProfile,
	                                             targetIndex);
}

