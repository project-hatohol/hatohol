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

#ifndef DBTablesHost_h
#define DBTablesHost_h

#include "DBTables.h"

class DBTablesHost : public DBTables {
public:
	static const int TABLES_VERSION;

	static void reset(void);

	DBTablesHost(DBAgent &dbAgent);
	virtual ~DBTablesHost();

protected:
	static SetupInfo &getSetupInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBTablesHost_h
