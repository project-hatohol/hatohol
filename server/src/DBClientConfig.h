/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include "DBClientHatohol.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_ZABBIX,
	MONITORING_SYSTEM_NAGIOS,
	NUM_MONITORING_SYSTEMS,
};

struct MonitoringServerInfo {
	ServerIdType         id;
	MonitoringSystemType type;
	std::string          hostName;
	std::string          ipAddress;
	std::string          nickname;
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
	std::string          userName;
	std::string          password;
	std::string          dbName; // for naigos ndutils

	// methods

	/**
	 * return an appropriate host information for connection.
	 *
	 * @param forURI
	 * Set true if you want to add square brackets ("[" and "]")
	 * automatically for IPv6 address. It's required to build an URI.
	 * e.g.) ::1 -> [::1]
         *       (for building an URI like http://[::1]:80/path)
	 * 
	 * @return
	 * If ipAddress is set, it is returned. Otherwise, if hostName is set,
	 * it is returned.
	 */
	std::string getHostAddress(bool forURI = false) const;
};

typedef std::list<MonitoringServerInfo>    MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;

class ServerQueryOption : public DataQueryOption {
public:
	ServerQueryOption(const UserIdType &userId = INVALID_USER_ID);
	ServerQueryOption(DataQueryContext *dataQueryContext);
	virtual ~ServerQueryOption();

	void setTargetServerId(const ServerIdType &serverId);

	// Overriding virtual methods
	virtual std::string getCondition(void) const;

protected:
	bool hasPrivilegeCondition(std::string &condition) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class DBClientConfig : public DBClient {
public:
	static int CONFIG_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static const char *DEFAULT_USER_NAME;
	static const char *DEFAULT_PASSWORD;
	static void init(const CommandLineArg &cmdArg);
	static void reset(void);

	DBClientConfig(void);
	virtual ~DBClientConfig();

	std::string getDatabaseDir(void);
	void setDatabaseDir(const std::string &dir);
	bool isFaceMySQLEnabled(void);
	int  getFaceRestPort(void);
	void setFaceRestPort(int port);
	bool isCopyOnDemandEnabled(void);

	HatoholError addTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege);

	HatoholError updateTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege);

	HatoholError deleteTargetServer(const ServerIdType &serverId,
	                                const OperationPrivilege &privilege);
	void getTargetServers(MonitoringServerInfoList &monitoringServers,
	                      ServerQueryOption &option);

	/**
	 * Get the ID set of accessible servers.
	 *
	 * @param serverIdSet
	 * The obtained IDs are inserted to this object.
	 * @param dataQueryContext A DataQueryContext instance.
	 */
	void getServerIdSet(ServerIdSet &serverIdSet,
	                    DataQueryContext *dataQueryContext);

protected:
	static bool parseCommandLineArgument(const CommandLineArg &cmdArg);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static bool parseDBServer(const std::string &dbServer,
	                          std::string &host, size_t &port);

	static bool canUpdateTargetServer(
	  MonitoringServerInfo *monitoringServerInfo,
	  const OperationPrivilege &privilege);

	static bool canDeleteTargetServer(
	  const ServerIdType &serverId, const OperationPrivilege &privilege);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

};

#endif // DBClientConfig_h
