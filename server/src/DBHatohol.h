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

#ifndef DBHatohol_h
#define DBHatohol_h

#include "DB.h"

class DBTablesConfig;
class DBTablesHost;
class DBTablesUser;
class DBTablesAction;
class DBTablesMonitoring;

class DBHatohol : public DB {
public:
	static void reset(void);
	static void setDefaultDBParams(const std::string &dbName,
	                               const std::string &user = "",
	                               const std::string &password = "");

	DBHatohol(void);
	virtual ~DBHatohol();
	DBTablesConfig  &getDBTablesConfig(void);
	DBTablesHost    &getDBTablesHost(void);
	DBTablesUser    &getDBTablesUser(void);
	DBTablesAction  &getDBTablesAction(void);
	DBTablesMonitoring &getDBTablesMonitoring(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBHatohol_h
