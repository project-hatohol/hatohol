/*
 * Copyright (C) 2013-2014 Project Hatohol
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

struct OptionValues {
	gchar    *pidFilePath;
	gchar    *dbServer;
	gboolean  foreground;
	gboolean  testMode;

	OptionValues(void)
	: pidFilePath(NULL),
	  dbServer(NULL),
	  foreground(FALSE),
	  testMode(FALSE)
	{
	}

	virtual ~OptionValues()
	{
		clear();
	}

	void clear(void)
	{
		g_free(pidFilePath);
		pidFilePath = NULL;

		g_free(dbServer);
		dbServer = NULL;

		foreground = FALSE;
		testMode   = FALSE;
	}
};
static OptionValues g_optionValues;

struct ConfigManager::PrivateContext {
	static Mutex          mutex;
	static ConfigManager *instance;
	string                confFilePath;
	string                databaseDirectory;
	string                actionCommandDirectory;
	string                residentYardDirectory;
	bool                  foreground;
	string                dbServerAddress;
	int                   dbServerPort;

	// methods
	PrivateContext(void)
	: foreground(false),
	  dbServerAddress("localhost"),
	  dbServerPort(0)
	{
	}

	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}

	void parseDBServer(const string &dbServer)
	{
		const size_t posColon = dbServer.find(":");
		if (posColon == string::npos) {
			dbServerAddress = dbServer;
			return;
		}
		if (posColon == dbServer.size() - 1) {
			MLPL_ERR("A column must not be the tail: %s\n",
			         dbServer.c_str());
			return;
		}
		dbServerAddress = string(dbServer, 0, posColon);
		dbServerPort = atoi(&dbServer.c_str()[posColon+1]);
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
			g_error_free(error);
			return false;
		}

		return true;
	}
};

Mutex          ConfigManager::PrivateContext::mutex;
ConfigManager *ConfigManager::PrivateContext::instance = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
bool ConfigManager::parseCommandLine(gint *argc, gchar ***argv)
{
	OptionValues *optVal = &g_optionValues;
	static GOptionEntry entries[] = {
		{"pid-file-path", 'p', 0, G_OPTION_ARG_STRING,
		 &optVal->pidFilePath, "Pid file path", NULL},
		{"foreground", 'f', 0, G_OPTION_ARG_NONE,
		 &optVal->foreground, "Run as a foreground process", NULL},
		{"test-mode", 't', 0, G_OPTION_ARG_NONE,
		 &optVal->testMode, "Run in a test mode", NULL},
		{"config-db-server", 'c', 0, G_OPTION_ARG_STRING,
		 &optVal->dbServer, "Database server", NULL},
		{ NULL }
	};

	GOptionContext *optCtx = g_option_context_new(NULL);
	g_option_context_add_main_entries(optCtx, entries, NULL);
	GError *error = NULL;
	if (!g_option_context_parse(optCtx, argc, argv, &error)) {
		MLPL_ERR("Failed to parse command line argment. (%s)\n",
		         error ? error->message : "Unknown reason");
		g_error_free(error);
		return false;
	}
	return true;
}

void ConfigManager::clearParseCommandLineResult(void)
{
	g_optionValues.clear();
}

void ConfigManager::reset(void)
{
	delete PrivateContext::instance;
	PrivateContext::instance = NULL;
	ConfigManager *confMgr = getInstance();
	confMgr->loadConfFile();

	confMgr->m_ctx->actionCommandDirectory =
	  StringUtils::sprintf("%s/%s/action", LIBEXECDIR, PACKAGE);
	confMgr->m_ctx->residentYardDirectory = string(PREFIX"/sbin");

	// override by the command line options if needed
	OptionValues *optVal = &g_optionValues;
	PrivateContext *ctx = confMgr->m_ctx;
	if (optVal->dbServer)
		ctx->parseDBServer(optVal->dbServer);
	if (optVal->foreground)
		ctx->foreground = true;
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

bool ConfigManager::isForegroundProcess(void) const
{
	return m_ctx->foreground;
}

string ConfigManager::getDBServerAddress(void) const
{
	return m_ctx->dbServerAddress;
}

int ConfigManager::getDBServerPort(void) const
{
	return m_ctx->dbServerPort;
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
	AutoMutex autoLock(&m_ctx->mutex);
	return m_ctx->actionCommandDirectory;
}

void ConfigManager::setActionCommandDirectory(const string &dir)
{
	AutoMutex autoLock(&m_ctx->mutex);
	m_ctx->actionCommandDirectory = dir;
}

string ConfigManager::getResidentYardDirectory(void)
{
	AutoMutex autoLock(&m_ctx->mutex);
	return m_ctx->residentYardDirectory;
}

void ConfigManager::setResidentYardDirectory(const string &dir)
{
	AutoMutex autoLock(&m_ctx->mutex);
	m_ctx->residentYardDirectory = dir;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ConfigManager::loadConfFile(void)
{
	vector<string> confFiles;
	confFiles.push_back(m_ctx->confFilePath);

	char *systemWideConfFile =
	   g_build_filename(SYSCONFDIR, PACKAGE_NAME, "hatohol.conf", NULL);
	confFiles.push_back(systemWideConfFile);
	g_free(systemWideConfFile);

	for (size_t i = 0; i < confFiles.size(); i++) {
		const bool succeeded = m_ctx->loadConfFile(confFiles[i]);
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
