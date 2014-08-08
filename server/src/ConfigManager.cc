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

struct ConfigManager::Impl {
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

Mutex          ConfigManager::Impl::mutex;
ConfigManager *ConfigManager::Impl::instance = NULL;
string         ConfigManager::Impl::actionCommandDirectory;
string         ConfigManager::Impl::residentYardDirectory;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ConfigManager::reset(void)
{
	Impl::actionCommandDirectory =
	  StringUtils::sprintf("%s/%s/action", LIBEXECDIR, PACKAGE);
	Impl::residentYardDirectory = string(PREFIX"/sbin");
}

ConfigManager *ConfigManager::getInstance(void)
{
	if (Impl::instance)
		return Impl::instance;

	Impl::lock();
	if (!Impl::instance)
		Impl::instance = new ConfigManager();
	Impl::unlock();

	return Impl::instance;
}

void ConfigManager::getTargetServers
  (MonitoringServerInfoList &monitoringServers, ServerQueryOption &option)
{
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(monitoringServers, option);
}

const string &ConfigManager::getDatabaseDirectory(void) const
{
	return m_impl->databaseDirectory;
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
	Impl::lock();
	string dir = Impl::actionCommandDirectory;
	Impl::unlock();
	return dir;
}

void ConfigManager::setActionCommandDirectory(const string &dir)
{
	Impl::lock();
	Impl::actionCommandDirectory = dir;
	Impl::unlock();
}

string ConfigManager::getResidentYardDirectory(void)
{
	Impl::lock();
	string dir = Impl::residentYardDirectory;
	Impl::unlock();
	return dir;
}

void ConfigManager::setResidentYardDirectory(const string &dir)
{
	Impl::lock();
	Impl::residentYardDirectory = dir;
	Impl::unlock();
}


// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager(void)
: m_impl(NULL)
{
	m_impl = new Impl();
	const char *envDBDir = getenv(HATOHOL_DB_DIR_ENV_VAR_NAME);
	if (envDBDir)
		m_impl->databaseDirectory = envDBDir;
	else
		m_impl->databaseDirectory = DEFAULT_DATABASE_DIR;
}

ConfigManager::~ConfigManager()
{
	delete m_impl;
}
