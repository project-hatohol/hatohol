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
#include <list>
#include <string>
#include "Params.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_UNKNOWN = -2,
	MONITORING_SYSTEM_FAKE    = -1, // mainly for test
	// Some types are no longer used. But we keep the number of
	// the remaining ones for compatibility.
	__MONITORING_SYSTEM_UNUSED_0, // MONITORING_SYSTEM_ZABBIX,
	__MONITORING_SYSTEM_UNUSED_1, // MONITORING_SYSTEM_NAGIOS,
	__MONITORING_SYSTEM_UNUSED_2, // MONITORING_SYSTEM_HAPI_ZABBIX,
	__MONITORING_SYSTEM_UNUSED_3, // MONITORING_SYSTEM_HAPI_NAGIOS,
	MONITORING_SYSTEM_HAPI_JSON,
	MONITORING_SYSTEM_INCIDENT_TRACKER,
	__MONITORING_SYSTEM_UNUSED_6, // MONITORING_SYSTEM_HAPI_CEILOMETER,
	MONITORING_SYSTEM_HAPI2,
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

	static const int MIN_POLLING_INTERVAL_SEC;
	static const int MAX_POLLING_INTERVAL_SEC;
	static const int MIN_RETRY_INTERVAL_SEC;
	static const int MAX_RETRY_INTERVAL_SEC;

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
	std::string          baseURL; // for User specified monitoring server URL
	std::string          extendedInfo;

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

