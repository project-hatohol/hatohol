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

#include <cppcutter.h>
#include "DBTest.h"
#include "DBTables.h"
#include "DBAgentTest.h"
#include "Helpers.h"

namespace testDBTables {

const DBTablesId DB_TABLES_ID_TEST = 0x1234567;
const int DB_VERSION = 817;

class TestDBTables : public DBTables {
public:
	TestDBTables(DBAgent &dbAgent, DBTables::SetupInfo &setupInfo)
	: DBTables(dbAgent, setupInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createTable(void)
{
	static const DBTables::TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileTest,
	}, {
		&tableProfileTestAutoInc,
	},
	};

	static DBTables::SetupInfo SETUP_INFO = {
		DB_TABLES_ID_TEST,
		DB_VERSION,
		ARRAY_SIZE(TABLE_INFO),
		TABLE_INFO,
	};

	TestDB::setup();
	TestDB testDB;
	cppcut_assert_equal(false, SETUP_INFO.initialized);
	TestDBTables tables(testDB.getDBAgent(), SETUP_INFO);

	assertDBTablesVersion(testDB.getDBAgent(),
	                      DB_TABLES_ID_TEST, DB_VERSION);
	assertCreateTable(&testDB.getDBAgent(), tableProfileTest.name);
	assertCreateTable(&testDB.getDBAgent(), tableProfileTestAutoInc.name);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
}

void test_createIndex(void)
{
	// We make a copy to update TableProfile::indexDefArray
	DBAgent::TableProfile tableProfile = tableProfileTest;

	static const int columnIndexes[] = {
	  IDX_TEST_TABLE_AGE, IDX_TEST_TABLE_NAME, DBAgent::IndexDef::END};
	static const DBAgent::IndexDef indexDef[] = {
		{"index_age_name", columnIndexes, false},
		{NULL}
	};
	tableProfile.indexDefArray = indexDef;

	static const DBTables::TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfile,
	},
	};

	static DBTables::SetupInfo SETUP_INFO = {
		DB_TABLES_ID_TEST,
		DB_VERSION,
		ARRAY_SIZE(TABLE_INFO),
		TABLE_INFO,
	};
	cppcut_assert_equal(false, SETUP_INFO.initialized);

	TestDB::setup();
	TestDB testDB;
	TestDBTables tables(testDB.getDBAgent(), SETUP_INFO);
	assertExistIndex(testDB.getDBAgent(), tableProfile.name,
	                 "index_age_name", 2);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
}

void test_tableInitializer(void)
{
	struct Gizmo {
		bool called;
		Gizmo(void)
		: called(false)
		{
		}

		static void initializer(DBAgent &agent, void *data)
		{
			Gizmo *obj = static_cast<Gizmo *>(data);
			obj->called = true;
		}
	} gizmo;

	const DBTables::TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileTest,
		Gizmo::initializer,
		&gizmo,
	},
	};

	static DBTables::SetupInfo SETUP_INFO = {
		DB_TABLES_ID_TEST,
		DB_VERSION,
		ARRAY_SIZE(TABLE_INFO),
		TABLE_INFO,
	};
	cppcut_assert_equal(false, SETUP_INFO.initialized);

	TestDB::setup();
	TestDB testDB;
	TestDBTables tables(testDB.getDBAgent(), SETUP_INFO);
	cppcut_assert_equal(true, gizmo.called);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
}

void test_tableUpdater(void)
{
	struct Gizmo {
		bool called;
		int oldVer;
		bool updateSuccess;

		Gizmo(void)
		: called(false),
		  oldVer(0),
		  updateSuccess(true)
		{
		}

		static bool updater(DBAgent &agent, const int &oldVer,
		                    void *data)
		{
			Gizmo *obj = static_cast<Gizmo *>(data);
			obj->called = true;
			obj->oldVer = oldVer;
			return obj->updateSuccess;
		}
	} gizmo;

	const DBTables::TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileTest,
	},
	};

	static DBTables::SetupInfo SETUP_INFO = {
		DB_TABLES_ID_TEST,
		DB_VERSION,
		ARRAY_SIZE(TABLE_INFO),
		TABLE_INFO,
		Gizmo::updater,
		&gizmo,
		false,
	};
	cppcut_assert_equal(false, SETUP_INFO.initialized);

	TestDB::setup();
	TestDB testDB;

	// Once we create a table
	TestDBTables tables(testDB.getDBAgent(), SETUP_INFO);
	cppcut_assert_equal(true, SETUP_INFO.initialized);

	// Create again with the same version.
	SETUP_INFO.initialized = false;
	TestDBTables tables2(testDB.getDBAgent(), SETUP_INFO);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
	cppcut_assert_equal(false, gizmo.called);
	cppcut_assert_equal(0, gizmo.oldVer);

	// Create again with the upper version of DBTables.
	SETUP_INFO.version++;
	SETUP_INFO.initialized = false;
	TestDBTables tables3(testDB.getDBAgent(), SETUP_INFO);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
	cppcut_assert_equal(true, gizmo.called);
	cppcut_assert_equal(DB_VERSION, gizmo.oldVer);

	// A case that an update fails.
	SETUP_INFO.version++;
	SETUP_INFO.initialized = false;
	bool gotException = false;
	try {
		gizmo.updateSuccess = false;
		TestDBTables tables4(testDB.getDBAgent(), SETUP_INFO);
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
	cppcut_assert_equal(false, SETUP_INFO.initialized);
}

} //namespace testDBTables


