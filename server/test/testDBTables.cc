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

using namespace std;

namespace testDBTables {

const DBTablesId DB_TABLES_ID_TEST = static_cast<DBTablesId>(0x1234567);
const int DB_VERSION = 817;

static const int g_columnIndexes[] = {
	IDX_TEST_TABLE_AGE, IDX_TEST_TABLE_NAME, DBAgent::IndexDef::END
};

static const DBAgent::IndexDef g_indexDef[] = {
	{"index_age_name", g_columnIndexes, false},
	{NULL}
};

static DBTables::SetupInfo *g_setupInfo = NULL;

struct TestDBKit {

	struct Init {
		Init(void)
		{
			TestDB::setup();
		}
	} __init;

	TestDB                   db;
	DBAgent::TableProfile    tableProfile;
	DBTables::TableSetupInfo tableInfo[1];
	DBTables::SetupInfo      setupInfo;

	TestDBKit(void)
	: tableProfile(tableProfileTest)
	{
		tableProfile.indexDefArray = g_indexDef;

		memset(&tableInfo[0], 0, sizeof(DBTables::TableSetupInfo));
		tableInfo[0].profile = &tableProfile;

		initSetupInfo();
		g_setupInfo = &setupInfo;
	}

	void initSetupInfo(void)
	{
		setupInfo.tablesId       = DB_TABLES_ID_TEST;
		setupInfo.version        = DB_VERSION;
		setupInfo.numTableInfo   = ARRAY_SIZE(tableInfo);
		setupInfo.tableInfoArray = tableInfo;
		setupInfo.updater        = NULL;
		setupInfo.updaterData    = NULL;
		setupInfo.initialized    = false;
	}
};

class TestDBTables : public DBTables {
public:
	TestDBTables(DBAgent &dbAgent, DBTables::SetupInfo &setupInfo)
	: DBTables(dbAgent, setupInfo)
	{
	}

	static SetupInfo &getConstSetupInfo(void)
	{
		cppcut_assert_not_null(g_setupInfo);
		return *g_setupInfo;
	}
};

void cut_setup(void)
{
	g_setupInfo = NULL;
}

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
	TestDBKit dbKit;
	DBTables::SetupInfo &setupInfo = dbKit.setupInfo;
	cppcut_assert_equal(false, setupInfo.initialized);

	TestDB &testDB = dbKit.db;
	TestDBTables tables(testDB.getDBAgent(), setupInfo);
	assertExistIndex(testDB.getDBAgent(), dbKit.tableProfile.name,
	                 "index_age_name", 2);
	cppcut_assert_equal(true, setupInfo.initialized);
}

void test_checkMajorVersion(void)
{
	struct VersionChecker {
		TestDBKit    dbKit;

		void operator ()(void)
		{
			DBTables::checkMajorVersion<TestDBTables>(
			  dbKit.db.getDBAgent());
		}
	};

	// First we expect the method successfully finishes.
	VersionChecker verChecker;
	TestDBTables   tables(verChecker.dbKit.db.getDBAgent(),
	                      verChecker.dbKit.setupInfo);
	verChecker();

	// Next we bump the minor version up.
	verChecker.dbKit.setupInfo.version =
	  DBTables::Version::getPackedVer(0, 0, DB_VERSION + 1);
	verChecker();

	// Then we bump the major version up.
	bool gotException = false;
	try {
		verChecker.dbKit.setupInfo.version =
		  DBTables::Version::getPackedVer(0, 1, 0);
		verChecker();
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
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
		DBTables::Version oldVer;
		bool updateSuccess;

		Gizmo(void)
		: called(false),
		  updateSuccess(true)
		{
		}

		static bool updater(
		  DBAgent &agent, const DBTables::Version &oldPackedVer,
		  void *data)
		{
			Gizmo *obj = static_cast<Gizmo *>(data);
			obj->called = true;
			obj->oldVer = oldPackedVer;
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
	cppcut_assert_equal(0, gizmo.oldVer.getPackedVer());

	// Create again with the upper version of DBTables.
	SETUP_INFO.version++;
	SETUP_INFO.initialized = false;
	TestDBTables tables3(testDB.getDBAgent(), SETUP_INFO);
	cppcut_assert_equal(true, SETUP_INFO.initialized);
	cppcut_assert_equal(true, gizmo.called);
	cppcut_assert_equal(DB_VERSION, gizmo.oldVer.getPackedVer());

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

namespace testDBTablesVersion {

void test_constructor(void)
{
	DBTables::Version ver;
	cppcut_assert_equal(0, ver.vendorVer);
	cppcut_assert_equal(0, ver.majorVer);
	cppcut_assert_equal(0, ver.minorVer);
}

void test_staticGetPackedVer(void)
{
	cppcut_assert_equal(
	  0x01234567, DBTables::Version::getPackedVer(0x12, 0x34, 0x567));
}

void test_getPackedVer(void)
{
	DBTables::Version ver;
	ver.vendorVer = 0xab;
	ver.majorVer  = 0xcd;
	ver.minorVer  = 0xef1;
	cppcut_assert_equal(0x0abcdef1, ver.getPackedVer());
}

void test_setPackedVer(void)
{
	DBTables::Version ver;
	ver.setPackedVer(0x0fedcba9);
	cppcut_assert_equal(0xfe, ver.vendorVer);
	cppcut_assert_equal(0xdc, ver.majorVer);
	cppcut_assert_equal(0xba9, ver.minorVer);
}

void test_toString(void)
{
	DBTables::Version ver;
	ver.setPackedVer(0x02468ace);
	cppcut_assert_equal(string("104.2766 [36]"), ver.toString());
}

} //namespace testDBTablesVersion


