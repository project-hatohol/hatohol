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

#include <MutexLock.h>
using namespace mlpl;

#include "ConfigManager.h"
#include "DBClientConfig.h"

const char *ConfigManager::HATOHOL_DB_DIR_ENV_VAR_NAME = "HATOHOL_DB_DIR";
static const char *DEFAULT_DATABASE_DIR = "/tmp";
static const size_t DEFAULT_NUM_PRESERVED_REPLICA_GENERATION = 3;

int ConfigManager::ALLOW_ACTION_FOR_ALL_OLD_EVENTS;
static int DEFAULT_ALLOWED_TIME_OF_ACTION_FOR_OLD_EVENTS
  = 60 * 60 * 24; // 24 hours

struct ConfigManager::PrivateContext {
	static MutexLock      mutex;
	static ConfigManager *instance;
	string                databaseDirectory;

	// methods
	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};

MutexLock      ConfigManager::PrivateContext::mutex;
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

int ConfigManager::getAllowedTimeOfActionForOldEvents(void)
{
	return DEFAULT_ALLOWED_TIME_OF_ACTION_FOR_OLD_EVENTS;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	const char *envDBDir = getenv(HATOHOL_DB_DIR_ENV_VAR_NAME);
	if (envDBDir)
		m_ctx->databaseDirectory = envDBDir;
	else
		m_ctx->databaseDirectory = DEFAULT_DATABASE_DIR;
}

ConfigManager::~ConfigManager()
{
	if (m_ctx)
		delete m_ctx;
}
