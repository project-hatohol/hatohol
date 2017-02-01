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

#pragma once
#include "DB.h"

class DBTablesConfig;
class DBTablesHost;
class DBTablesUser;
class DBTablesAction;
class DBTablesMonitoring;
class DBTablesLastInfo;

class DBHatohol : public DB {
public:
	static void reset(void);

	/**
	 * Set default DB parameters
	 *
	 * Each of the following parameters can be NULL. In that case,
	 * the paramter is  not updated (the previous value is used)
	 *
	 * @param dbName   A database name.
	 * @param user     A user name used to connect with a DB server.
	 * @param password A password used to connect with a DB server.
	 * @param host     A DB server or NULL for localhost.
	 * @param port     A DB port of zero for the default port.
	 */
	static void setDefaultDBParams(const char *dbName,
	                               const char *user = NULL,
	                               const char *password = NULL,
	                               const char *host = NULL,
	                               const int  &port = 0);

	DBHatohol(void);
	virtual ~DBHatohol();
	DBTablesConfig  &getDBTablesConfig(void);
	DBTablesHost    &getDBTablesHost(void);
	DBTablesUser    &getDBTablesUser(void);
	DBTablesAction  &getDBTablesAction(void);
	DBTablesMonitoring &getDBTablesMonitoring(void);
	DBTablesLastInfo &getDBTablesLastInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

