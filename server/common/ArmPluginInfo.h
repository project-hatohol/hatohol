/*
 * Copyright (C) 2015 Project Hatohol
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
#include <string>
#include <vector>
#include <map>
#include "Params.h"
#include "MonitoringServerInfo.h"

struct ArmPluginInfo {
	int id;
	MonitoringSystemType type;
	std::string path;

	/**
	 * The broker URL such as "localhost:5672".
	 * If this value is empty, the default URL is used.
	 */
	std::string brokerUrl;

	/**
	 * If this parameter is empty, dynamically generated queue addresss
	 * is passed to the created plugin proess as an environment variable.
	 * If the plugin process is a passive type (not created by the
	 * Hatohol), the parameter should be set.
	 */
	std::string staticQueueAddress;

	/**
	 * Monitoring server ID of the server this ArmPlugin communicates with.
	 */
	ServerIdType serverId;

	std::string tlsCertificatePath;
	std::string tlsKeyPath;
	std::string tlsCACertificatePath;
	int tlsEnableVerify;

	/*
	 * UUID for HAPI2 plugins
	 */
	std::string uuid;

	static void initialize(ArmPluginInfo &armPluginInfo);

	bool isTLSVerifyEnabled(void) const;
};

typedef std::vector<ArmPluginInfo>        ArmPluginInfoVect;
typedef ArmPluginInfoVect::iterator       ArmPluginInfoVectIterator;
typedef ArmPluginInfoVect::const_iterator ArmPluginInfoVectConstIterator;

typedef std::map<MonitoringSystemType, ArmPluginInfo *> ArmPluginInfoMap;
typedef ArmPluginInfoMap::iterator          ArmPluginInfoMapIterator;
typedef ArmPluginInfoMap::const_iterator    ArmPluginInfoMapConstIterator;

