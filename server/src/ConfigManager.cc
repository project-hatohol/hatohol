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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Mutex.h>
#include "ConfigManager.h"
#include "DBClientConfig.h"
using namespace std;
using namespace mlpl;

const char *ConfigManager::HATOHOL_DB_DIR_ENV_VAR_NAME = "HATOHOL_DB_DIR";
static const char *DEFAULT_DATABASE_DIR = "/tmp";
static const size_t DEFAULT_NUM_PRESERVED_REPLICA_GENERATION = 3;

int ConfigManager::ALLOW_ACTION_FOR_ALL_OLD_EVENTS;
static int DEFAULT_ALLOWED_TIME_OF_ACTION_FOR_OLD_EVENTS
  = 60 * 60 * 24; // 24 hours

static int DEFAULT_MAX_NUM_RUNNING_COMMAND_ACTION = 10;

struct ConfigManager::PrivateContext {
	static Mutex          mutex;
	static ConfigManager *instance;
	string                databaseDirectory;
	static string         actionCommandDirectory;
	static string         residentYardDirectory;

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

Mutex          ConfigManager::PrivateContext::mutex;
ConfigManager *ConfigManager::PrivateContext::instance = NULL;
string         ConfigManager::PrivateContext::actionCommandDirectory;
string         ConfigManager::PrivateContext::residentYardDirectory;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ConfigManager::reset(void)
{
	PrivateContext::actionCommandDirectory =
	  StringUtils::sprintf("%s/%s/action", LIBEXECDIR, PACKAGE);
	PrivateContext::residentYardDirectory = string(PREFIX"/sbin");
}

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

void ConfigManager::getTargetServers
  (MonitoringServerInfoList &monitoringServers, ServerQueryOption &option)
{
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(monitoringServers, option);
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

int ConfigManager::getMaxNumberOfRunningCommandAction(void)
{
	return DEFAULT_MAX_NUM_RUNNING_COMMAND_ACTION;
}

string ConfigManager::getActionCommandDirectory(void)
{
	PrivateContext::lock();
	string dir = PrivateContext::actionCommandDirectory;
	PrivateContext::unlock();
	return dir;
}

void ConfigManager::setActionCommandDirectory(const string &dir)
{
	PrivateContext::lock();
	PrivateContext::actionCommandDirectory = dir;
	PrivateContext::unlock();
}

string ConfigManager::getResidentYardDirectory(void)
{
	PrivateContext::lock();
	string dir = PrivateContext::residentYardDirectory;
	PrivateContext::unlock();
	return dir;
}

void ConfigManager::setResidentYardDirectory(const string &dir)
{
	PrivateContext::lock();
	PrivateContext::residentYardDirectory = dir;
	PrivateContext::unlock();
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
	delete m_ctx;
}
