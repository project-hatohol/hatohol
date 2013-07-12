/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef DBClientConfig_h
#define DBClientConfig_h

#include <list>
#include "DBClient.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_ZABBIX,
	MONITORING_SYSTEM_NAGIOS,
};

struct MonitoringServerInfo {
	int                  id;
	MonitoringSystemType type;
	string               hostName;
	string               ipAddress;
	string               nickname;
	int                  port;
	int                  pollingIntervalSec;
	int                  retryIntervalSec;

	// The following variables are used in different purposes
	// depending on the MonitoringSystemType.
	//
	// [MONITORING_SYSTEM_ZABBIX]
	//   userName, passowrd: loggin for the ZABBIX server.
	//   dbName            : Not used
	//
	// [MONITORING_SYSTEM_NAGIOS]
	//   userName, passowrd: MySQL user name and the password.
	//   dbName            : MySQL database name.
	string               userName;
	string               password;
	string               dbName; // for naigos ndutils

	// methods

	/**
	 * return an appropriate host information for connection.
	 * @return
	 * If ipAddress is set, it is returned. Otherwise, if hostName is set,
	 * it is returned. If ipAddress and hostName are both not set, NULL
	 * is returned.
	 */
	const char *getHostAddress(void) const;
};

typedef list<MonitoringServerInfo>         MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;

class DBClientConfig : public DBClient {
public:
	static int CONFIG_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static void reset(bool deepReset = false);
	static bool parseCommandLineArgument(CommandLineArg &cmdArg);
	static const DBConnectInfo &getDBConnectInfo(void);

	/**
	 * set the default parameters to connect the DB.
	 * The parameters set by this function are used for the instance of
	 * DBClientConfig created with NULL as the first argument.
	 * The paramers are reset to the sysytem default when reset() is called.
	 *
	 * @param dbName A database name.
	 * @param user
	 * A user name. If this paramter is "" (empty string), the name of
	 * the current user is used.
	 * @param passowrd
	 * A password. If this parameter is "" (empty string), no passowrd
	 * is used.
	 */
	static void setDefaultDBParams(const string &dbName,
	                               const string &user = "",
	                               const string &password = "");

	DBClientConfig(const DBConnectInfo *connectInfo = NULL);
	virtual ~DBClientConfig();

	string  getDatabaseDir(void);
	void setDatabaseDir(const string &dir);
	bool isFaceMySQLEnabled(void);
	int  getFaceRestPort(void);
	void setFaceRestPort(int port);
	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);

protected:
	static void resetDBInitializedFlags(void);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static void initDefaultDBConnectInfoMaster(void);
	static void initDefaultDBConnectInfo(void);
	static bool parseDBServer(const string &dbServer,
	                          string &host, size_t &port);
	void prepareSetupFunction(const DBConnectInfo *connectInfo);
	const DBConnectInfo *getDefaultConnectInfo(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientConfig_h
