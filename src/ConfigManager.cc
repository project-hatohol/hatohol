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

#include "ConfigManager.h"
#include "DBClientConfig.h"

static const char *DEFAULT_DATABASE_DIR = "/tmp";
static const size_t DEFAULT_NUM_PRESERVED_REPLICA_GENERATION = 3;

struct ConfigManager::PrivateContext {
	static GStaticMutex   mutex;
	static ConfigManager *instance;
	string                databaseDirectory;

	// methods
	static void lock(void)
	{
		g_static_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_static_mutex_unlock(&mutex);
	}
};

GStaticMutex   ConfigManager::PrivateContext::mutex = G_STATIC_MUTEX_INIT;
ConfigManager *ConfigManager::PrivateContext::instance = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ConfigManager *ConfigManager::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new ConfigManager();
	PrivateContext::unlock();

	return PrivateContext::instance;
}

void ConfigManager::addTargetServer(MonitoringServerInfo *monitoringServerInfo)
{
	DBClientConfig dbConfig;
	dbConfig.addTargetServer(monitoringServerInfo);
}

void ConfigManager::getTargetServers
  (MonitoringServerInfoList &monitoringServers)
{
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(monitoringServers);
}

const string &ConfigManager::getDatabaseDirectory(void) const
{
	return m_ctx->databaseDirectory;
}

size_t ConfigManager::getNumberOfPreservedReplicaGeneration(void) const
{
	return DEFAULT_NUM_PRESERVED_REPLICA_GENERATION;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->databaseDirectory = DEFAULT_DATABASE_DIR;
}

ConfigManager::~ConfigManager()
{
	if (m_ctx)
		delete m_ctx;
}
