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

#include "DBTables.h"
#include "DB.h"
#include "ItemGroupStream.h"

using namespace std;
using namespace mlpl;

struct DBTables::Impl {
	DBAgent &dbAgent;

	Impl(DBAgent &_dbAgent, SetupInfo &setupInfo)
	: dbAgent(_dbAgent)
	{
		struct SetupProc : DBAgent::TransactionProc {
			Impl      *impl;
			SetupInfo &setupInfo;

			SetupProc(Impl *_impl, SetupInfo &_setupInfo)
			: impl(_impl),
			  setupInfo(_setupInfo)
			{
			}

			void operator ()(DBAgent &dbAgent) override
			{
				impl->setupTables(setupInfo);
			}
		};

		AutoMutex(&setupInfo.lock);
		if (!setupInfo.initialized) {
			SetupProc setup(this, setupInfo);
			dbAgent.runTransaction(setup);
			setupInfo.initialized = true;
		}
	}

	void setupTables(SetupInfo &setupInfo)
	{
		const DBAgent::TableProfile &tableProf =
		  DB::getTableProfileTablesVersion();
		const ColumnDef *colDefs = tableProf.columnDefs;
		string condition = StringUtils::sprintf("%s=%d",
		  colDefs[DB::IDX_TABLES_VERSION_TABLES_ID].columnName,
		  setupInfo.tablesId);
		if (!dbAgent.isRecordExisting(tableProf.name, condition))
			insertTablesVersion(setupInfo, tableProf);
		else
			updateTablesVersionIfNeeded(setupInfo, tableProf);

		// Create tables
		vector<const TableSetupInfo *> createdTableInfoVect;
		for (size_t i = 0; i < setupInfo.numTableInfo; i++) {
			const TableSetupInfo &tableInfo =
			  setupInfo.tableInfoArray[i];
			if (dbAgent.isTableExisting(tableInfo.profile->name))
				continue;
			dbAgent.createTable(*tableInfo.profile);
			createdTableInfoVect.push_back(&tableInfo);
		}

		// Create and drop indexes if needed
		for (size_t i = 0; i < setupInfo.numTableInfo; i++) {
			const TableSetupInfo &tableInfo =
			  setupInfo.tableInfoArray[i];
			dbAgent.fixupIndexes(*tableInfo.profile);
		}

		// Call initalizers only for the created tables
		for (size_t i = 0; i < createdTableInfoVect.size(); i++) {
			const TableSetupInfo &tableInfo =
			   *createdTableInfoVect[i];
			if (!tableInfo.initializer)
				continue;
			(*tableInfo.initializer)(dbAgent,
			                         tableInfo.initializerData);
		}
	}

	void insertTablesVersion(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion)
	{
		DBAgent::InsertArg arg(tableProfileTablesVersion);
		arg.add(setupInfo.tablesId);
		arg.add(setupInfo.version);
		dbAgent.insert(arg);
	}

	void updateTablesVersionIfNeeded(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion)
	{
		const int versionInDB =
		  getTablesVersion(setupInfo, tableProfileTablesVersion);
		if (versionInDB == setupInfo.version)
			return;

		MLPL_INFO("DBTables %d is outdated, try to update it ...\n"
		          "\told-version: %" PRIu32 "\n"
		          "\tnew-version: %" PRIu32 "\n",
		          setupInfo.tablesId, versionInDB, setupInfo.version);
		void *data = setupInfo.updaterData;
		HATOHOL_ASSERT(setupInfo.updater,
		             "Updater: NULL, new/old ver. %d/%d",
		             setupInfo.version, versionInDB);
		bool succeeded =
		  (*setupInfo.updater)(dbAgent, versionInDB, data);
		HATOHOL_ASSERT(succeeded,
		               "Failed to update DB, new/old ver. %d/%d",
		               setupInfo.version, versionInDB);
		setTablesVersion(setupInfo, tableProfileTablesVersion);
		MLPL_INFO("Succeeded in updating DBTables %" PRIu32 "\n",
		          setupInfo.tablesId);
	}

	int getTablesVersion(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion)
	{
		DBAgent::SelectExArg arg(tableProfileTablesVersion);
		arg.add(DB::IDX_TABLES_VERSION_VERSION);
		const ColumnDef *colDefs = tableProfileTablesVersion.columnDefs;
		arg.condition = StringUtils::sprintf("%s=%d",
		  colDefs[DB::IDX_TABLES_VERSION_TABLES_ID].columnName,
		  setupInfo.tablesId);
		dbAgent.select(arg);

		const ItemGroupList &itemGroupList =
		   arg.dataTable->getItemGroupList();
		HATOHOL_ASSERT(itemGroupList.size() == 1,
		  "itemGroupList.size(): %zd", itemGroupList.size());
		ItemGroupStream itemGroupStream(*itemGroupList.begin());
		return itemGroupStream.read<int>();
	}

	void setTablesVersion(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion)
	{
		DBAgent::UpdateArg arg(tableProfileTablesVersion);
		arg.add(DB::IDX_TABLES_VERSION_VERSION, setupInfo.version);
		const ColumnDef *colDefs = tableProfileTablesVersion.columnDefs;
		arg.condition = StringUtils::sprintf("%s=%d",
		  colDefs[DB::IDX_TABLES_VERSION_TABLES_ID].columnName,
		  setupInfo.tablesId);
		dbAgent.update(arg);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBTables::DBTables(DBAgent &dbAgent, SetupInfo &setupInfo)
: m_impl(new Impl(dbAgent, setupInfo))
{
}

DBTables::~DBTables()
{
}

DBAgent &DBTables::getDBAgent(void)
{
	return m_impl->dbAgent;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBTables::begin(void)
{
	getDBAgent().begin();
}

void DBTables::rollback(void)
{
	getDBAgent().rollback();
}

void DBTables::commit(void)
{
	getDBAgent().commit();
}

void DBTables::insert(const DBAgent::InsertArg &insertArg)
{
	getDBAgent().insert(insertArg);
}

void DBTables::update(const DBAgent::UpdateArg &updateArg)
{
	getDBAgent().update(updateArg);
}

void DBTables::select(const DBAgent::SelectArg &selectArg)
{
	getDBAgent().select(selectArg);
}

void DBTables::select(const DBAgent::SelectExArg &selectExArg)
{
	getDBAgent().select(selectExArg);
}

void DBTables::deleteRows(const DBAgent::DeleteArg &deleteArg)
{
	getDBAgent().deleteRows(deleteArg);
}

void DBTables::addColumns(const DBAgent::AddColumnsArg &addColumnsArg)
{
	getDBAgent().addColumns(addColumnsArg);
}

bool DBTables::isRecordExisting(const string &tableName,
                                const string &condition)
{
	return getDBAgent().isRecordExisting(tableName, condition);
}

uint64_t DBTables::getLastInsertId(void)
{
	return getDBAgent().getLastInsertId();
}

bool DBTables::updateIfExistElseInsert(
  const ItemGroup *itemGroup, const DBAgent::TableProfile &tableProfile,
  size_t targetIndex)
{
	return getDBAgent().updateIfExistElseInsert(itemGroup, tableProfile,
	                                             targetIndex);
}
