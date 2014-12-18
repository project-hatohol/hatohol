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

#include "DBTest.h"
#include "Helpers.h"
#include "DBHatohol.h"

const char *TestDB::DB_NAME = "test_db";

// TODO: It's not unwanted to use a derivied class for the test of
// the base class. We should think a more smart way.
DB::SetupContext TestDB::m_setupCtx(typeid(DBHatohol));

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
