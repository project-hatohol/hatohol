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

const int DBTables::Version::VENDOR_BITS = 8;
const int DBTables::Version::MAJOR_BITS  = 8;
const int DBTables::Version::MINOR_BITS  = 12;

static const int VENDOR_MASK = 0x0ff00000;
static const int MAJOR_MASK  = 0x000ff000;
static const int MINOR_MASK  = 0x00000fff;

int DBTables::Version::getPackedVer(
  const int &vendor, const int &major, const int &minor)
{
	int shiftBits = 0;
	int ver = MINOR_MASK & minor;

	shiftBits += MINOR_BITS;
	ver += MAJOR_MASK & (major << shiftBits);

	shiftBits += MAJOR_BITS;
	ver += VENDOR_MASK & (vendor << shiftBits);

	return ver;
}

DBTables::Version::Version(void)
: vendorVer(0),
  majorVer(0),
  minorVer(0)
{
}

int DBTables::Version::getPackedVer(void) const
{
	return getPackedVer(vendorVer, majorVer, minorVer);
}

void DBTables::Version::setPackedVer(const int &packedVer)
{
	int shiftBits = 0;
	minorVer = MINOR_MASK & packedVer;

	shiftBits += MINOR_BITS;
	majorVer = (MAJOR_MASK & packedVer) >> shiftBits;

	shiftBits += MAJOR_BITS;
	vendorVer = (VENDOR_MASK & packedVer) >> shiftBits;
}

string DBTables::Version::toString(void) const
{
	return StringUtils::sprintf("%02d.%02d [%02d]",
	                            majorVer, minorVer, vendorVer);
}

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

	static bool getTablesVersion(
	  Version &ver, const SetupInfo &setupInfo, DBAgent &dbAgent)
	{
		const DBAgent::TableProfile &tableProf =
		  DB::getTableProfileTablesVersion();
		const ColumnDef *colDefs = tableProf.columnDefs;
		string condition = StringUtils::sprintf("%s=%d",
		  colDefs[DB::IDX_TABLES_VERSION_TABLES_ID].columnName,
		  setupInfo.tablesId);
		if (!dbAgent.isRecordExisting(tableProf.name, condition))
			return false;
		const int packedVer =
		  getPackedTablesVersion(setupInfo, tableProf, dbAgent);
		ver.setPackedVer(packedVer);
		return true;
	}

	void setupTables(SetupInfo &setupInfo)
	{
		const DBAgent::TableProfile &tableProf =
		  DB::getTableProfileTablesVersion();
		Version ver;
		if (!getTablesVersion(ver, setupInfo, dbAgent))
			insertTablesVersion(setupInfo, tableProf);
		else
			updateTablesVersionIfNeeded(setupInfo, tableProf, ver);

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
		arg.add(underlying_value(setupInfo.tablesId));
		arg.add(setupInfo.version);
		dbAgent.insert(arg);
	}

	void updateTablesVersionIfNeeded(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion,
	  const Version &versionInDB)
	{
		if (versionInDB.getPackedVer() == setupInfo.version)
			return;
		Version versionInProg;
		versionInProg.setPackedVer(setupInfo.version);

		HATOHOL_ASSERT(versionInDB.vendorVer == versionInProg.vendorVer,
		  "Invalid Vendor version: DB/Prog: %s/%s (tablesId: %d)",
		  versionInDB.toString().c_str(),
		  versionInProg.toString().c_str(),
		  setupInfo.tablesId);

		HATOHOL_ASSERT(versionInDB.majorVer == versionInProg.majorVer,
		  "Invalid Major version: DB/Prog: %s/%s (tablesId: %d)",
		  versionInDB.toString().c_str(),
		  versionInProg.toString().c_str(),
		  setupInfo.tablesId);

		MLPL_INFO("DBTables %d is outdated, try to update it ...\n"
		          "\told-version: %s\n"
		          "\tnew-version: %s\n",
		          setupInfo.tablesId,
		          versionInDB.toString().c_str(),
		          versionInProg.toString().c_str());
		void *data = setupInfo.updaterData;
		HATOHOL_ASSERT(setupInfo.updater,
		  "Updater: NULL, new/old ver. %s/%s (%d)",
		  versionInDB.toString().c_str(),
		  versionInProg.toString().c_str(),
		  setupInfo.tablesId);

		bool succeeded =
		  (*setupInfo.updater)(dbAgent, versionInDB, data);
		HATOHOL_ASSERT(succeeded,
		  "Failed to update DB, new/old ver. %s/%s (%d)",
		  versionInDB.toString().c_str(),
		  versionInProg.toString().c_str(),
		  setupInfo.tablesId);

		setTablesVersion(setupInfo, tableProfileTablesVersion);
		MLPL_INFO("Succeeded in updating DBTables %d\n",
		          setupInfo.tablesId);
	}

	static int getPackedTablesVersion(
	  const SetupInfo &setupInfo,
	  const DBAgent::TableProfile &tableProfileTablesVersion,
	  DBAgent &dbAgent)
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
void DBTables::checkMajorVersionMain(
  const SetupInfo &setupInfo, DBAgent &dbAgent)
{
	DBTables::Version versionInDB;
	if (!Impl::getTablesVersion(versionInDB, setupInfo, dbAgent))
		return;
	DBTables::Version versionInProg;
	versionInProg.setPackedVer(setupInfo.version);
	if (versionInDB.majorVer != versionInProg.majorVer) {
		THROW_HATOHOL_EXCEPTION(
		  "Invalid Major version: DB/Prog: %s/%s "
		  "(tablesId: %d)\n",
		  versionInDB.toString().c_str(),
		  versionInProg.toString().c_str(),
		  setupInfo.tablesId);
	}
}

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
