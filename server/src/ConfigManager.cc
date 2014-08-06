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
#include "Reaper.h"
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
	string                confFilePath;
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

	bool loadConfFile(const string &path)
	{
		if (path.empty())
			return false;

		GKeyFile *keyFile = g_key_file_new();
		if (!keyFile) {
			MLPL_CRIT("Failed to call g_key_file_new().\n");
			return false;
		}
		Reaper<GKeyFile> keyFileReaper(keyFile, g_key_file_unref);

		GError *error = NULL;
		gboolean succeeded =
		  g_key_file_load_from_file(keyFile, path.c_str(),
		                            G_KEY_FILE_NONE, &error);
		if (!succeeded) {
			MLPL_DBG("Failed to load config file: %s (%s)\n",
			         path.c_str(),
			         error ? error->message : "Unknown reason");
			return false;
		}

		return true;
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
	delete Impl::instance;
	Impl::instance = NULL;
	ConfigManager *confMgr = getInstance();
	confMgr->loadConfFile();

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
// Protected methods
// ---------------------------------------------------------------------------
void ConfigManager::loadConfFile(void)
{
	vector<string> confFiles;
	confFiles.push_back(m_impl->confFilePath.c_str());

	char *systemWideConfFile =
	   g_build_filename(SYSCONFDIR, PACKAGE_NAME, "hatohol.conf", NULL);
	confFiles.push_back(systemWideConfFile);
	g_free(systemWideConfFile);

	for (size_t i = 0; i < confFiles.size(); i++) {
		const bool succeeded = m_impl->loadConfFile(confFiles[i]);
		if (!succeeded)
			continue;
		MLPL_INFO("Use configuration file: %s\n",
		          confFiles[i].c_str());
		break;
	}
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager(void)
: m_impl(new Impl())
{
	const char *envDBDir = getenv(HATOHOL_DB_DIR_ENV_VAR_NAME);
	if (envDBDir)
		m_impl->databaseDirectory = envDBDir;
	else
		m_impl->databaseDirectory = DEFAULT_DATABASE_DIR;
}

ConfigManager::~ConfigManager()
{
}
