/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ConfigManager_h
#define ConfigManager_h

#include <glib.h>
#include <stdint.h>

#include "DBClientConfig.h"

static const int DB_DOMAIN_ID_CONFIG = 0x0010;
static const int DB_DOMAIN_ID_ASURA  = 0x0020;
static const int DB_DOMAIN_ID_ZABBIX = 0x1000;
static const size_t NUM_MAX_ZABBIX_SERVERS = 100;
// DBClintZabbix uses the number of domains by NUM_MAX_ZABBIX_SERVERS 
// So the domain ID is occupied
//   from DB_DOMAIN_ID_ZABBIX
//   to   DB_DOMAIN_ID_ZABBIX + NUM_MAX_ZABBIX_SERVERS - 1

class ConfigManager {
public:
	static const char *ASURA_DB_DIR_ENV_VAR_NAME;
	static ConfigManager *getInstance(void);

	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);
	const string &getDatabaseDirectory(void) const;
	size_t getNumberOfPreservedReplicaGeneration(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

	// Constructor and destructor
	ConfigManager(void);
	virtual ~ConfigManager();
};

#endif // ConfigManager_h
