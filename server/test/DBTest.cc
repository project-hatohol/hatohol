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

#include "DBTest.h"
#include "Helpers.h"

const char *TestDB::DB_NAME = "test_db";

DB::SetupContext TestDB::m_setupCtx(DB::DB_MYSQL);

// ---------------------------------------------------------------------------
// Public method
// ---------------------------------------------------------------------------
void TestDB::setup(void)
{
	const bool dbRecreate = true;
	makeTestMySQLDBIfNeeded(DB_NAME, dbRecreate);
	m_setupCtx.initialized = false;

	m_setupCtx.connectInfo = DBConnectInfo();
	m_setupCtx.connectInfo.dbName = DB_NAME;
}

TestDB::TestDB(void)
: DB(m_setupCtx)
{
}
