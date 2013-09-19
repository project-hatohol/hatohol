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

#ifndef ConfigManager_h
#define ConfigManager_h

#include <glib.h>
#include <stdint.h>

#include "DBClientConfig.h"

class ConfigManager {
public:
	static const char *HATOHOL_DB_DIR_ENV_VAR_NAME;
	static ConfigManager *getInstance(void);
	static int ALLOW_ACTION_FOR_ALL_OLD_EVENTS;

	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);
	const string &getDatabaseDirectory(void) const;
	size_t getNumberOfPreservedReplicaGeneration(void) const;

	/**
	 * Get the time to ignore an action for old events.
	 * The events that are older than Tc - Ts shall be ignored, where
	 * Tc is the current time and Ts is a return value of this function.
	 *
	 * @return
	 * The allowed time in second. If this value is
	 * ALLOW_ACTION_FOR_ALL_OLD_EVENTS, the action shoall be invoked
	 * even for any old event.
	 *
	 */
	int getAllowedTimeOfActionForOldEvents(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	// Constructor and destructor
	ConfigManager(void);
	virtual ~ConfigManager();
};

#endif // ConfigManager_h
