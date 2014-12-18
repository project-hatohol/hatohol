/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef ConfigManager_h
#define ConfigManager_h

#include <glib.h>
#include <stdint.h>
#include "DBTablesConfig.h"

struct CommandLineOptions {
	gchar    *pidFilePath;
	gchar    *dbServer;
	gchar    *dbName;
	gchar    *dbUser;
	gchar    *dbPassword;
	gboolean  foreground;
	gboolean  testMode;
	gboolean  enableCopyOnDemand;
	gboolean  disableCopyOnDemand;
	gint      faceRestPort;

	CommandLineOptions(void);
};

class ConfigManager {
public:
	enum ConfigState {
		DISABLE,
		ENABLE,
		UNKNOWN,
	};

	static const char *HATOHOL_DB_DIR_ENV_VAR_NAME;
	static ConfigManager *getInstance(void);
	static int ALLOW_ACTION_FOR_ALL_OLD_EVENTS;
	static const char *DEFAULT_PID_FILE_PATH;

	/**
	 * Parse the argument.
	 *
	 * @param argc A pointer to argc.
	 * @param argv A pointer to argv.
	 * @param cmdLineOpt
	 * The parsed result is stored in this instance.
	 *
	 * @return true if the parse succees. Otherwise false is returned.
	 */
	static bool parseCommandLine(gint *argc, gchar ***argv,
	                             CommandLineOptions *cmdLineOpt);

	static void reset(const CommandLineOptions *cmdLineOpt = NULL);

	void getTargetServers(MonitoringServerInfoList &monitoringServers,
	                      ServerQueryOption &option);
	const std::string &getDatabaseDirectory(void) const;
	size_t getNumberOfPreservedReplicaGeneration(void) const;

	bool isForegroundProcess(void) const;
	std::string getDBServerAddress(void) const;
	int getDBServerPort(void) const;

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

	int getMaxNumberOfRunningCommandAction(void);

	std::string getActionCommandDirectory(void);
	void setActionCommandDirectory(const std::string &dir);
	std::string getResidentYardDirectory(void);
	void setResidentYardDirectory(const std::string &dir);

	bool isTestMode(void) const;

	/**
	 * Get the flag for copy-on-demand of items.
	 *
	 * @retrun
	 * If --enable-copy-on-deman is specified, ENABLE is returned.
	 * If --disable-copy-on-deman is specified, DISABLE is returned.
	 * Otherwise, UNKNOWN is returned.
	 */
	ConfigState getCopyOnDemand(void) const;

	/**
	 * Get the port for FaceRest.
	 *
	 * @retrun
	 * If --face-rest-port <PORT> is specified, it is returned.
	 * Otherwise, 0 is returned.
	 */
	int getFaceRestPort(void) const;

	void setFaceRestPort(const int &port);

	std::string getPidFilePath(void) const;

protected:
	void loadConfFile(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	// Constructor and destructor
	ConfigManager(void);
	virtual ~ConfigManager();
};

#endif // ConfigManager_h
