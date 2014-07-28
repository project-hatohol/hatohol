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

#ifndef MonitoringServerInfo_h
#define MonitoringServerInfo_h

#include <list>
#include <string>
#include "Params.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_UNKNOWN = -2,
	MONITORING_SYSTEM_FAKE    = -1, // mainly for test
	MONITORING_SYSTEM_ZABBIX,
	MONITORING_SYSTEM_NAGIOS,
	MONITORING_SYSTEM_HAPI_ZABBIX,
	MONITORING_SYSTEM_HAPI_NAGIOS,
	MONITORING_SYSTEM_HAPI_JSON,
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

	/**
	 * Ensure to return an human friendly name.
	 * 
	 * @return
	 * If nickname is set, it is returned. Otherwise, if hostName is set,
	 * it is returned. If both of them are empty, ipAddress is returned.
	 */
	std::string getDisplayName(void) const;

	static void initialize(MonitoringServerInfo &monitroingServerInfo);
};

typedef std::list<MonitoringServerInfo>    MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;
typedef MonitoringServerInfoList::const_iterator MonitoringServerInfoListConstIterator;

#endif // MonitoringServerInfo_h
