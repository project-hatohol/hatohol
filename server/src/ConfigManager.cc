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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <mutex>
#include <errno.h>
#include "ConfigManager.h"
#include "DBTablesConfig.h"
#include "Reaper.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

enum {
	CONF_MGR_ERROR_NULL,
	CONF_MGR_ERROR_INVALID_PORT,
	CONF_MGR_ERROR_INVALID_LOG_LEVEL,
	CONF_MGR_ERROR_INTERNAL,
	CONF_MGR_ERROR_INVALID_NUM_WORKERS,
};

const char *ConfigManager::HATOHOL_DB_DIR_ENV_VAR_NAME = "HATOHOL_DB_DIR";
static const char *DEFAULT_DATABASE_DIR = "/tmp";
static const size_t DEFAULT_NUM_PRESERVED_REPLICA_GENERATION = 3;

const char *ConfigManager::DEFAULT_PID_FILE_PATH = LOCALSTATEDIR "/run/hatohol.pid";

static int DEFAULT_MAX_NUM_RUNNING_COMMAND_ACTION = 10;

static gboolean parseFaceRestPort(
  const gchar *option_name, const gchar *value,
  gpointer data, GError **error)
{
	GQuark quark =
	  g_quark_from_static_string("config-manager-quark");
	CommandLineOptions *obj =
	  static_cast<CommandLineOptions *>(data);
	if (!value) {
		g_set_error(error, quark, CONF_MGR_ERROR_NULL,
		            "value is NULL.");
		return FALSE;
	}
	int port = atoi(value);
	if (!Utils::isValidPort(port, false)) {
		g_set_error(error, quark, CONF_MGR_ERROR_INVALID_PORT,
		            "value: %s, %d.", value, port);
		return FALSE;
	}

	obj->faceRestPort = port;
	return TRUE;
}

static void showVersion(void)
{
	printf("Hatohol version: %s, build date: %s %s\n",
	       VERSION, __DATE__, __TIME__);
}

static gboolean parseFaceRestNumWorkers(
  const gchar *option_name, const gchar *value,
  gpointer data, GError **error)
{
	GQuark quark =
	  g_quark_from_static_string("config-manager-quark");
	CommandLineOptions *obj =
	  static_cast<CommandLineOptions *>(data);
	if (!value) {
		g_set_error(error, quark, CONF_MGR_ERROR_NULL,
		            "value is NULL.");
		return FALSE;
	}

	int numWorkers = atoi(value);
	if (numWorkers <= 0) {
		g_set_error(error, quark, CONF_MGR_ERROR_INVALID_NUM_WORKERS,
		            "value: %s, %d.", value, numWorkers);
		return FALSE;
	}

	obj->faceRestNumWorkers = numWorkers;

	return TRUE;
}


// ---------------------------------------------------------------------------
// CommandLineOptions
// ---------------------------------------------------------------------------
CommandLineOptions::CommandLineOptions(void)
: pidFilePath(NULL),
  user(NULL),
  dbServer(NULL),
  dbName(NULL),
  dbUser(NULL),
  dbPassword(NULL),
  foreground(FALSE),
  testMode(FALSE),
  faceRestPort(-1),
  faceRestNumWorkers(0)
{
}

// ---------------------------------------------------------------------------
// ConfigManager::Impl
// ---------------------------------------------------------------------------
struct ConfigManager::Impl {
	static std::mutex     mutex;
	static ConfigManager *instance;
	string                confFilePath;
	string                databaseDirectory;
	string                actionCommandDirectory;
	string                residentYardDirectory;
	bool                  foreground;
	string                dbServerAddress;
	int                   dbServerPort;
	bool                  testMode;
	AtomicValue<int>      faceRestPort;
	string                user;
	string                pidFilePath;
	int                   faceRestNumWorkers;

	// methods
	Impl(void)
	: foreground(false),
	  dbServerAddress("localhost"),
	  dbServerPort(0),
	  testMode(false),
	  faceRestPort(0),
	  pidFilePath(DEFAULT_PID_FILE_PATH),
	  faceRestNumWorkers(0)
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
		Reaper<GKeyFile> keyFileReaper(keyFile, g_key_file_free);

		GError *error = NULL;
		gboolean succeeded =
		  g_key_file_load_from_file(keyFile, path.c_str(),
		                            G_KEY_FILE_NONE, &error);
		if (!succeeded) {
			Reaper<GError> errorFree(error, g_error_free);
			if (error->domain == G_FILE_ERROR &&
			    error->code == G_FILE_ERROR_NOENT) {
				MLPL_DBG("Not found: %s\n", path.c_str());
			} else {
				MLPL_ERR("Failed to load config file: %s (%s)\n",
					 path.c_str(), error->message);
			}
			return false;
		}

		loadConfigFileMySQLGroup(keyFile);
		loadConfigFileFaceRestGroup(keyFile);

		return true;
	}

	void reflectCommandLineOptions(const CommandLineOptions &cmdLineOpts)
	{
		if (cmdLineOpts.dbServer)
			parseDBServer(cmdLineOpts.dbServer);
		DBHatohol::setDefaultDBParams(cmdLineOpts.dbName,
		                              cmdLineOpts.dbUser,
		                              cmdLineOpts.dbPassword);
		if (cmdLineOpts.foreground)
			foreground = true;
		if (cmdLineOpts.testMode)
			testMode = true;
		if (cmdLineOpts.faceRestPort >= 0)
			faceRestPort = cmdLineOpts.faceRestPort;
		if (cmdLineOpts.pidFilePath)
			pidFilePath = cmdLineOpts.pidFilePath;
		if (cmdLineOpts.user)
			user = cmdLineOpts.user;
		if (cmdLineOpts.faceRestNumWorkers > 0)
			faceRestNumWorkers = cmdLineOpts.faceRestNumWorkers;
	}

private:
	void loadConfigFileMySQLGroup(GKeyFile *keyFile)
	{
		const gchar *group = "mysql";

		if (!g_key_file_has_group(keyFile, group))
			return;

		gchar *database =
			g_key_file_get_string(keyFile, group, "database", NULL);
		gchar *user =
			g_key_file_get_string(keyFile, group, "user", NULL);
		gchar *password =
			g_key_file_get_string(keyFile, group, "password", NULL);
		gchar *host =
			g_key_file_get_string(keyFile, group, "host", NULL);
		gint port =
			g_key_file_get_integer(keyFile, group, "port", NULL);

		if (host)
			dbServerAddress = host;
		if (port > 0)
			dbServerPort = port;
		DBHatohol::setDefaultDBParams(database, user, password,
		                              host, port);
		g_free(database);
		g_free(user);
		g_free(password);
		g_free(host);
	}

	void loadConfigFileFaceRestGroup(GKeyFile *keyFile)
	{
		const gchar *group = "FaceRest";

		if (!g_key_file_has_group(keyFile, group))
			return;

		gint num = g_key_file_get_integer(keyFile, group,
						  "workers", NULL);
		if (num > 0) {
			getInstance()->setFaceRestNumWorkers(num);
			MLPL_INFO("ConfigFile: [FaceRest] workers=%d\n", num);
		} else {
			MLPL_WARN("ConfigFile: [FaceRest] workers=%d: Invalid value. Ignored.\n", num);
		}
	}
};

mutex          ConfigManager::Impl::mutex;
ConfigManager *ConfigManager::Impl::instance = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
bool ConfigManager::parseCommandLine(gint *argc, gchar ***argv,
                                     CommandLineOptions *cmdLineOpts)
{
	gboolean showVersionFlag = false;
	GOptionEntry entries[] = {
		{"version",
		 'v', 0, G_OPTION_ARG_NONE,
		 &showVersionFlag, "Show the version", NULL},
		{"pid-file",
		 'p', 0, G_OPTION_ARG_STRING,
		 &cmdLineOpts->pidFilePath, "Pid file path", NULL},
		{"foreground",
		 'f', 0, G_OPTION_ARG_NONE,
		 &cmdLineOpts->foreground, "Run as a foreground process", NULL},
		{"test-mode",
		 't', 0, G_OPTION_ARG_NONE,
		 &cmdLineOpts->testMode, "Run in a test mode", NULL},
		{"db-server",
		 'c', 0, G_OPTION_ARG_STRING,
		 &cmdLineOpts->dbServer, "Database server", NULL},
		{"db-name",
		 'n', 0, G_OPTION_ARG_STRING,
		 &cmdLineOpts->dbName, "Database name", NULL},
		{"db-user",
		 'u', 0, G_OPTION_ARG_STRING,
		 &cmdLineOpts->dbUser, "Database user", NULL},
		{"db-password",
		 'w', 0, G_OPTION_ARG_STRING,
		 &cmdLineOpts->dbPassword, "Database password", NULL},
		{"face-rest-port",
		 'r', 0, G_OPTION_ARG_CALLBACK, (gpointer)parseFaceRestPort,
		 "Port of FaceRest", NULL},
		{"user",
		 'U', 0, G_OPTION_ARG_STRING, &cmdLineOpts->user,
		 "Run as another user", NULL},
		{"log-level",
		 'l', 0, G_OPTION_ARG_CALLBACK, (gpointer)parseLogLevel,
		 "Log level: DBG, INFO, WARN, ERR, CRIT, or BUG. ", NULL},
		{"face-rest-workers",
		 'T', 0, G_OPTION_ARG_CALLBACK, (gpointer)parseFaceRestNumWorkers,
		 "Number of FaceRest worker threads", NULL},
		{ NULL }
	};

	GOptionContext *optCtx = g_option_context_new(NULL);
	GOptionGroup *optGrp = g_option_group_new(
	  "ConfigManager", "ConfigManager group", "ConfigManager",
	  cmdLineOpts, NULL);
	g_option_context_set_main_group(optCtx, optGrp);
	g_option_context_add_main_entries(optCtx, entries, NULL);
	GError *error = NULL;
	if (!g_option_context_parse(optCtx, argc, argv, &error)) {
		MLPL_ERR("Failed to parse command line argment. (%s)\n",
		         error ? error->message : "Unknown reason");
		if (error)
			g_error_free(error);
		return false;
	}
	g_option_context_free(optCtx);

	if (showVersionFlag) {
		showVersion();
		return false;
	}

	// reflect options so that ConfigManager can return them
	// even before reset() is called.
	getInstance()->m_impl->reflectCommandLineOptions(*cmdLineOpts);
	return true;
}

void ConfigManager::reset(const CommandLineOptions *cmdLineOpts,
			  bool loadConfigFile)
{
	delete Impl::instance;
	Impl::instance = NULL;
	ConfigManager *confMgr = getInstance();
	if (loadConfigFile)
		confMgr->loadConfFile();

	confMgr->m_impl->actionCommandDirectory =
	  StringUtils::sprintf("%s/%s/action", LIBEXECDIR, PACKAGE);
	confMgr->m_impl->residentYardDirectory = string(PREFIX"/sbin");

	// override by the command line options if needed
	unique_ptr<const CommandLineOptions> localOpts;
	if (!cmdLineOpts) {
		localOpts = unique_ptr<const CommandLineOptions>(
		  new CommandLineOptions());
		cmdLineOpts = localOpts.get();
	}
	confMgr->m_impl->reflectCommandLineOptions(*cmdLineOpts);
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
	ThreadLocalDBCache cache;
	cache.getConfig().getTargetServers(monitoringServers, option);
}

const string &ConfigManager::getDatabaseDirectory(void) const
{
	return m_impl->databaseDirectory;
}

size_t ConfigManager::getNumberOfPreservedReplicaGeneration(void) const
{
	return DEFAULT_NUM_PRESERVED_REPLICA_GENERATION;
}

bool ConfigManager::isForegroundProcess(void) const
{
	return m_impl->foreground;
}

string ConfigManager::getDBServerAddress(void) const
{
	return m_impl->dbServerAddress;
}

int ConfigManager::getDBServerPort(void) const
{
	return m_impl->dbServerPort;
}

int ConfigManager::getMaxNumberOfRunningCommandAction(void)
{
	return DEFAULT_MAX_NUM_RUNNING_COMMAND_ACTION;
}

string ConfigManager::getActionCommandDirectory(void)
{
	lock_guard<mutex> lock(m_impl->mutex);
	return m_impl->actionCommandDirectory;
}

void ConfigManager::setActionCommandDirectory(const string &dir)
{
	lock_guard<mutex> lock(m_impl->mutex);
	m_impl->actionCommandDirectory = dir;
}

string ConfigManager::getResidentYardDirectory(void)
{
	lock_guard<mutex> lock(m_impl->mutex);
	return m_impl->residentYardDirectory;
}

void ConfigManager::setResidentYardDirectory(const string &dir)
{
	lock_guard<mutex> lock(m_impl->mutex);
	m_impl->residentYardDirectory = dir;
}

bool ConfigManager::isTestMode(void) const
{
	return m_impl->testMode;
}

int ConfigManager::getFaceRestPort(void) const
{
	return m_impl->faceRestPort;
}

void ConfigManager::setFaceRestPort(const int &port)
{
	m_impl->faceRestPort = port;
}

string ConfigManager::getPidFilePath(void) const
{
	return m_impl->pidFilePath;
}

string ConfigManager::getUser(void) const
{
	return m_impl->user;
}

int ConfigManager::getFaceRestNumWorkers(void) const
{
	return m_impl->faceRestNumWorkers;
}

void ConfigManager::setFaceRestNumWorkers(const int &num)
{
	m_impl->faceRestNumWorkers = num;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ConfigManager::loadConfFile(void)
{
	vector<string> confFiles;
	confFiles.push_back(m_impl->confFilePath);

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

gboolean ConfigManager::parseLogLevel(
  const gchar *option_name, const gchar *value,
  gpointer data, GError **error)
{
	GQuark quark =
	  g_quark_from_static_string("config-manager-quark");
	if (!value) {
		g_set_error(error, quark, CONF_MGR_ERROR_NULL,
		            "value is NULL.");
		return FALSE;
	}

	string level = value;
	bool validLevel = false;
	if (level == "DBG"  ||
	    level == "INFO" ||
	    level == "WARN" ||
	    level == "ERR"  ||
	    level == "CRIT" ||
	    level == "BUG") {
		validLevel = true;
	}

	if (!validLevel) {
		g_set_error(error, quark, CONF_MGR_ERROR_INVALID_LOG_LEVEL,
		            "Invalid log level: %s.", value);
		return FALSE;
	}

	if (setenv(Logger::LEVEL_ENV_VAR_NAME, level.c_str(), 1) == -1) {
		g_set_error(error, quark, CONF_MGR_ERROR_INTERNAL,
		            "Failed to call setenv: errno: %d", errno);
		return FALSE;
	}

	return TRUE;
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
