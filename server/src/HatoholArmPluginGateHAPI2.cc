/*
 * Copyright (C) 2015 Project Hatohol
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

#include <StringUtils.h>
#include <JSONParser.h>
#include <HatoholArmPluginInterfaceHAPI2.h>
#include "HatoholArmPluginGateHAPI2.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"
#include "ChildProcessManager.h"
#include <libsoup/soup.h>
#include <Reaper.h>
#include "ArmUtils.h"
#include "SelfMonitor.h"

using namespace std;
using namespace mlpl;

struct JSONRPCError {
	StringList errors;
	void addError(const char *format,
		      ...) __attribute__((__format__ (__printf__, 2, 3)))
	{
	       va_list ap;
	       va_start(ap, format);
	       using namespace mlpl::StringUtils;
	       errors.push_back(vsprintf(format, ap));
	       va_end(ap);
	}

	bool hasErrors(void) {
		return !errors.empty();
	}

	const StringList &getErrors(void) {
		return errors;
	}
};

#define PARSE_AS_MANDATORY(MEMBER, VALUE, RPCERR)			\
if (!parser.read(MEMBER, VALUE)) {					\
	MLPL_ERR("Failed to parse '%s' member.\n", MEMBER);		\
	string errorMessage =						\
		"Failed to parse mandatory member:";			\
	RPCERR.addError("%s '%s' does not exist.",			\
			errorMessage.c_str(), MEMBER);			\
}

#define CHECK_MANDATORY_PARAMS_EXISTENCE(PARAMS, RPCERR)		\
if (!parser.isMember(PARAMS)) {						\
	MLPL_ERR("Failed to parse '%s'.\n", PARAMS);			\
	string errorMessage = "Failed to parse:";			\
	RPCERR.addError("%s '%s' does not exist.",			\
			errorMessage.c_str(), PARAMS);			\
}

#define CHECK_MANDATORY_ARRAY_EXISTENCE(MEMBER, RPCERR)			\
if (JSONParser::VALUE_TYPE_ARRAY != parser.getValueType(MEMBER)) {	\
	MLPL_ERR("Failed to parse mandatory object '%s'.\n", MEMBER);	\
	string errorMessage =						\
		"Failed to parse mandatory object: ";			\
	RPCERR.addError("%s '%s' does not exist.",			\
			errorMessage.c_str(), MEMBER);			\
	return false;							\
}

#define CHECK_MANDATORY_ARRAY_EXISTENCE_INNER_LOOP(MEMBER, RPCERR)	\
if (JSONParser::VALUE_TYPE_ARRAY != parser.getValueType(MEMBER)) {	\
	MLPL_ERR("Failed to parse mandatory object '%s'.\n", MEMBER);	\
	string errorMessage =						\
		"Failed to parse mandatory object: ";			\
	RPCERR.addError("%s '%s' does not exist.",			\
			errorMessage.c_str(), MEMBER);			\
	parser.endElement();						\
	break;								\
}

static void updateSelfMonitor(SelfMonitorPtr monitor, const bool &hasError)
{
	monitor->update(hasError ? TRIGGER_STATUS_PROBLEM : TRIGGER_STATUS_OK);
}

struct HatoholArmPluginGateHAPI2::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo m_serverInfo;

	HatoholArmPluginGateHAPI2 &m_hapi2;
	ArmPluginInfo m_pluginInfo;
	ArmFake m_armFake;
	ArmStatus m_armStatus;
	string m_pluginProcessName;
	string m_pluginControlScriptPath;
	set<string> m_supportedProcedureNameSet;
	HostInfoCache hostInfoCache;
	map<string, Closure0 *> m_fetchClosureMap;
	map<string, Closure1<HistoryInfoVect> *> m_fetchHistoryClosureMap;
	SelfMonitorPtr monitorPluginInternal;
	SelfMonitorPtr monitorParseError;
	SelfMonitorPtr monitorGateInternal;
	SelfMonitorPtr monitorBrokerConn;

	Impl(const MonitoringServerInfo &_serverInfo,
	     HatoholArmPluginGateHAPI2 &hapi2)
	: m_serverInfo(_serverInfo),
	  m_hapi2(hapi2),
	  m_armFake(m_serverInfo),
	  m_armStatus(),
	  hostInfoCache(&_serverInfo.id),
	  monitorPluginInternal(new SelfMonitor(
	    _serverInfo.id, FAILED_HAP_INTERNAL_ERROR_TRIGGER_ID,
	    HatoholError::getMessage(HTERR_HAP_INTERNAL_ERROR),
	    TRIGGER_SEVERITY_CRITICAL)),
	  monitorParseError(new SelfMonitor(
	    _serverInfo.id, FAILED_PARSER_JSON_DATA_TRIGGER_ID,
	    HatoholError::getMessage(HTERR_INVALID_OBJECT_PASSED_BY_HAP2),
	    TRIGGER_SEVERITY_CRITICAL)),
	  monitorGateInternal(new SelfMonitor(
	    _serverInfo.id, FAILED_INTERNAL_ERROR_TRIGGER_ID,
	    HatoholError::getMessage(HTERR_INTERNAL_ERROR),
	    TRIGGER_SEVERITY_CRITICAL)),
	  monitorBrokerConn(new SelfMonitor(
	    _serverInfo.id, FAILED_CONNECT_BROKER_TRIGGER_ID,
	    HatoholError::getMessage(HTERR_FAILED_CONNECT_BROKER),
	    TRIGGER_SEVERITY_CRITICAL))
	{
		ArmPluginInfo::initialize(m_pluginInfo);
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		const ServerIdType &serverId = m_serverInfo.id;
		if (!dbConfig.getArmPluginInfo(m_pluginInfo, serverId)) {
			MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
				 serverId);
			return;
		}
		if (m_pluginInfo.staticQueueAddress.empty()) {
			m_pluginInfo.staticQueueAddress =
			  StringUtils::sprintf("hap2.%" FMT_SERVER_ID,
					       m_serverInfo.id);
		}
		m_hapi2.setArmPluginInfo(m_pluginInfo);
		setupSelfMonitors();
	}

	~Impl()
	{
		for (auto pair: m_fetchClosureMap) {
			Closure0 *closure = pair.second;
			if (closure)
				(*closure)();
			delete closure;
		}
		for (auto pair: m_fetchHistoryClosureMap) {
			Closure1<HistoryInfoVect> *closure = pair.second;
			HistoryInfoVect historyInfoVect;
			if (closure)
				(*closure)(historyInfoVect);
			delete closure;
		}
		m_armStatus.setRunningStatus(false);
	}

	void setupSelfMonitors(void)
	{
		auto ignore = [](SelfMonitor &m, ...) {};
		auto log = [](SelfMonitor &m, ...) {
			 m.saveEvent();
		};
		auto check = [&](SelfMonitor &monitor,
		                 const TriggerStatusType &prevStatus,
		                 const TriggerStatusType &currStatus) {
			if (monitor.getLastKnownStatus() != currStatus)
				log(monitor, prevStatus, currStatus);
		};
		const SelfMonitor::EventGenerator
		  mtx[NUM_TRIGGER_STATUS][NUM_TRIGGER_STATUS] = {
			{
				ignore, /* OK -> OK */
				log,    /* OK -> PROBLEM */
				ignore, /* OK -> UNKNOWN */
			}, {
				log,    /* PROBLEM -> OK */
				log,    /* PROBLEM -> PROBLEM */
				ignore, /* PROBLEM -> UNKNOWN */
			}, {
				check,  /* UNKNOWN -> OK */
				check,  /* UNKNOWN -> PROBLEM */
				ignore, /* UNKNOWN -> UNKNOWN */
			}
		};

		auto setAllStatusComb = [&](SelfMonitorPtr m) {
			for (int p = 0; p < NUM_TRIGGER_STATUS; p++) {
				for (int c = 0; c < NUM_TRIGGER_STATUS; c++) {
					m->setEventGenerator(
					  static_cast<TriggerStatusType>(p),
					  static_cast<TriggerStatusType>(c),
					  mtx[p][c]);
				}
			}
		};

		SelfMonitorPtr monitors[] = {
			monitorParseError,
			monitorGateInternal,
			monitorBrokerConn,
		};
		for (auto &m : monitors)
			setAllStatusComb(m);

		setupMonitorXPluginInternal();
	}

	void setupMonitorXPluginInternal(void)
	{
		monitorBrokerConn->addNotificationListener(monitorPluginInternal);
		auto handler = [](SelfMonitor &monitor,
		                  const SelfMonitor &srcMonitor,
		                  const TriggerStatusType &prevStatus,
		                  const TriggerStatusType &currStatus) {
			monitor.setWorkable(currStatus == TRIGGER_STATUS_OK);
		};
		monitorPluginInternal->setNotificationHandler(handler);
	}

	void start(void)
	{
		m_armStatus.setRunningStatus(true);
		callExchangeProfile();
	}

	bool isEstablished(void)
	{
		return m_hapi2.getEstablished();
	}

	bool parseExchangeProfileParams(JSONParser &parser, JSONRPCError &errObj)
	{
		m_supportedProcedureNameSet.clear();
		CHECK_MANDATORY_PARAMS_EXISTENCE("procedures", errObj);
		parser.startObject("procedures");
		size_t num = parser.countElements();
		for (size_t i = 0; i < num; i++) {
			string supportedProcedureName;
			parser.read(i, supportedProcedureName);
			m_supportedProcedureNameSet.insert(supportedProcedureName);
		}
		parser.endObject(); // procedures

		PARSE_AS_MANDATORY("name", m_pluginProcessName, errObj);
		MLPL_INFO("HAP Process connecting done. "
			  "Connected HAP process name: \"%s\"\n",
			  m_pluginProcessName.c_str());
		return true;
	}

	struct ExchangeProfileCallback : public ProcedureCallback {
		Impl &m_impl;
		ExchangeProfileCallback(Impl &impl)
		: m_impl(impl)
		{
		}

		void setArmInfoStatus(JSONRPCError errObj) {
			ArmStatus &status = m_impl.m_armStatus;
			ArmInfo armInfo = status.getArmInfo();
			if (errObj.hasErrors()) {
				armInfo.stat = ARM_WORK_STAT_FAILURE;
			} else {
				armInfo.stat = ARM_WORK_STAT_OK;
			}
			status.setArmInfo(armInfo);
		}

		virtual void onGotResponse(JSONParser &parser) override
		{
			bool isErr = parser.isMember("error");
			updateSelfMonitor(m_impl.monitorParseError, isErr);
			if (isErr) {
				string errorMessage;
				parser.startObject("error");
				parser.read("message", errorMessage);
				parser.endObject();
				MLPL_WARN("Received an error on calling "
					  "exchangeProfile: %s\n",
					  errorMessage.c_str());
				return;
			}

			JSONRPCError errObj;
			parser.startObject("result");
			m_impl.parseExchangeProfileParams(parser, errObj);
			parser.endObject();

			setArmInfoStatus(errObj);

			isErr = errObj.hasErrors();
			updateSelfMonitor(m_impl.monitorParseError, isErr);
			if (isErr)
				return;

			m_impl.m_hapi2.setEstablished(true);
		}

		virtual void onTimeout(void) override
		{
			MLPL_WARN("exchangeProfile has been timed out.\n");
		}
	};

	void callExchangeProfile(void) {
		JSONBuilder builder;
		builder.startObject();
		builder.add("jsonrpc", "2.0");
		builder.add("method", HAPI2_EXCHANGE_PROFILE);
		builder.startObject("params");
		builder.add("name", PACKAGE_NAME);
		builder.startArray("procedures");
		for (auto procedureDef : m_hapi2.getProcedureDefList()) {
			if (procedureDef.owner == HAP)
				continue;
			builder.add(procedureDef.name);
		}
		builder.endArray(); // procedures
		builder.endObject(); // params
		std::mt19937 random = m_hapi2.getRandomEngine();
		int64_t id = random();
		builder.add("id", id);
		builder.endObject();
		ProcedureCallback *callback =
		  new Impl::ExchangeProfileCallback(*this);
		ProcedureCallbackPtr callbackPtr(callback, false);
		m_hapi2.send(builder.generate(), id, callback);
	}

	void callUpdateMonitoringServerInfoNotification(
	  const MonitoringServerInfo monitoringServerInfo)
	{
		JSONBuilder builder;
		builder.startObject();
		builder.add("jsonrpc", "2.0");
		builder.add("method", HAPI2_UPDATE_MONITORING_SERVER_INFO);
		builder.startObject("params");
		builder.add("serverId", monitoringServerInfo.id);
		builder.add("url", monitoringServerInfo.baseURL);
		builder.add("type", m_pluginInfo.uuid);
		builder.add("nickName", monitoringServerInfo.nickname);
		builder.add("userName", monitoringServerInfo.userName);
		builder.add("password", monitoringServerInfo.password);
		builder.add("pollingIntervalSec", monitoringServerInfo.pollingIntervalSec);
		builder.add("retryIntervalSec", monitoringServerInfo.retryIntervalSec);
		builder.add("extendedInfo", monitoringServerInfo.extendedInfo);
		builder.endObject(); // params
		builder.endObject();
		m_hapi2.send(builder.generate());
	}

	void callUpdateMonitoringServerInfoNotificationIfNeeded(void)
	{
		MonitoringServerInfo monitoringServer;
		HatoholError err = getMonitoringServerInfo(monitoringServer);
		if (err == HTERR_OK &&
		    isMonitoringServerInfoChanged(monitoringServer)) {
			callUpdateMonitoringServerInfoNotification(monitoringServer);
		}
	}

	void queueFetchCallback(const string &fetchId, Closure0 *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchClosureMap[fetchId] = closure;
	}

	void runFetchCallback(const string &fetchId)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchClosureMap.find(fetchId);
		if (it != m_fetchClosureMap.end()) {
			Closure0 *closure = it->second;
			if (closure)
				(*closure)();
			m_fetchClosureMap.erase(it);
			delete closure;
		}
	}

	void queueFetchHistoryCallback(const string &fetchId,
				       Closure1<HistoryInfoVect> *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchHistoryClosureMap[fetchId] = closure;
	}

	void runFetchHistoryCallback(const string &fetchId,
				     const HistoryInfoVect &historyInfoVect)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchHistoryClosureMap.find(fetchId);
		if (it != m_fetchHistoryClosureMap.end()) {
			Closure1<HistoryInfoVect> *closure = it->second;
			(*closure)(historyInfoVect);
			m_fetchHistoryClosureMap.erase(it);
			delete closure;
		}
	}

	bool hasProcedure(const string procedureName)
	{
		return m_supportedProcedureNameSet.find(procedureName) !=
			m_supportedProcedureNameSet.end();
	}

	struct FetchProcedureCallback : public ProcedureCallback {
		Impl &m_impl;
		const string m_fetchId;
		const string m_methodName;
		FetchProcedureCallback(Impl &impl,
				       const string &fetchId,
				       const string &methodName)
		: m_impl(impl), m_fetchId(fetchId), m_methodName(methodName)
		{
		}

		bool isSucceeded(JSONParser &parser)
		{
			if (parser.isMember("error"))
				return false;

			string result;
			parser.read("result", result);
			return result == "SUCCESS";
		}

		void flush(void)
		{
			if (m_methodName == HAPI2_FETCH_HISTORY) {
				HistoryInfoVect historyInfoVect;
				m_impl.runFetchHistoryCallback(m_fetchId,
							       historyInfoVect);
			} else {
				m_impl.runFetchCallback(m_fetchId);
			}
		}

		virtual void onGotResponse(JSONParser &parser) override
		{
			if (isSucceeded(parser)) {
				// The callback function will be executed on
				// put* or update* procedures.
				return;
			}

			// The fetch* procedure has not been accepted by the
			// plugin. The closure for it should be expired
			// immediately.
			flush();
			MLPL_WARN("Failed to call %s\n", m_methodName.c_str());
		}

		virtual void onTimeout(void) override
		{
			flush();
			MLPL_WARN("%s has been timed out.\n", m_methodName.c_str());
		}
	};

	const string &getPluginControlScriptPath()
	{
		if (!m_pluginControlScriptPath.empty())
			return m_pluginControlScriptPath;

		if (m_pluginInfo.path.empty())
			return m_pluginControlScriptPath;

		if (g_path_is_absolute(m_pluginInfo.path.c_str())) {
			m_pluginControlScriptPath = m_pluginInfo.path;
			return m_pluginInfo.path;
		}

		m_pluginControlScriptPath =
		  LIBEXECDIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "hap2";
		m_pluginControlScriptPath += G_DIR_SEPARATOR;
		m_pluginControlScriptPath += m_pluginInfo.path;
		return m_pluginControlScriptPath;
	}

	bool isPluginControlScriptAvailable(void)
	{
		const string scriptPath = getPluginControlScriptPath();
		if (scriptPath.empty())
			return false;
		bool executable = g_file_test(scriptPath.c_str(),
					      G_FILE_TEST_IS_EXECUTABLE);
		if (!executable)
			MLPL_WARN("The plugin control script isn't executable:"
				  " %s\n", scriptPath.c_str());
		return executable;
	}

	bool startPlugin(void)
	{
		if (isPluginControlScriptAvailable())
			return runPluginControlScript("start");
		else
			return false;
	}

	bool stopPlugin(void)
	{
		if (isPluginControlScriptAvailable())
			return runPluginControlScript("stop");
		else
			return false;
	}

	void setChildProcessEnv(StringVector &envs)
	{
		const char *ldlibpath = getenv("LD_LIBRARY_PATH");
		if (ldlibpath) {
			envs.push_back(StringUtils::sprintf(
			  "LD_LIBRARY_PATH=%s",
			  ldlibpath));
		}

		const char *loggerLevel = getenv(Logger::LEVEL_ENV_VAR_NAME);
		if (loggerLevel) {
			envs.push_back(StringUtils::sprintf(
			  "%s=%s",
			  Logger::LEVEL_ENV_VAR_NAME, loggerLevel));
		}

		envs.push_back(StringUtils::sprintf(
		  "HAPI_MONITORING_SERVER_ID=%s",
		  StringUtils::toString(m_pluginInfo.serverId).c_str()));

		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_URL=%s",
		  m_pluginInfo.brokerUrl.c_str()));

		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_QUEUE=%s",
		  m_pluginInfo.staticQueueAddress.c_str()));

		SoupURI *uri = soup_uri_new(m_pluginInfo.brokerUrl.c_str());
		if (!uri)
			return;
		Reaper<SoupURI> uriReaper(uri, soup_uri_free);

		if (!SOUP_URI_IS_VALID(uri))
			return;

		const char *scheme = soup_uri_get_scheme(uri);
		if (!scheme || string(scheme) != "amqp")
			return;

		const char *host = soup_uri_get_host(uri);
		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_HOST=%s",
		  host ? host : ""));

		const int port = soup_uri_get_port(uri);
		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_PORT=%s",
		  port ? StringUtils::toString(port).c_str() : ""));

		const char *user = soup_uri_get_user(uri);
		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_USER=%s",
		  user ? user : ""));

		const char *password = soup_uri_get_password(uri);
		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_PASSWORD=%s",
		  password ? password : ""));

		const char *vhost = soup_uri_get_path(uri);
		while (vhost && *vhost == '/')
			vhost++;
		envs.push_back(StringUtils::sprintf(
		  "HAPI_AMQP_VHOST=%s",
		  vhost ? vhost : ""));
	}

	bool runPluginControlScript(const string command)
	{
		const string scriptPath = getPluginControlScriptPath();

		// argv
		const gchar *argv[3];
		argv[0] = scriptPath.c_str();
		argv[1] = command.c_str();
		argv[2] = NULL;

		// envp
		StringVector envs;
		setChildProcessEnv(envs);
		const gchar *envp[envs.size() + 1];
		for (size_t i = 0; i < envs.size(); i++)
			envp[i] = envs[i].c_str();
		envp[envs.size()] = NULL;

		GSpawnFlags flags = static_cast<GSpawnFlags>(0);
		gint exitStatus = 0;
		GError *error = NULL;
		gboolean succeeded =
		  g_spawn_sync(NULL, (gchar **)argv, (gchar **)envp,
			       flags, NULL, NULL, NULL, NULL,
			       &exitStatus, &error);
		if (!succeeded) {
			MLPL_ERR("Failed to %s: %s\n",
				 command.c_str(), scriptPath.c_str());
			// The error always exists in this case.
			MLPL_ERR("Reason: %s\n", error->message);
			g_error_free(error);
			return false;
		}

		MLPL_INFO("Succeeded to %s: %s\n",
			  command.c_str(), scriptPath.c_str());

		return true;
	}

	HatoholError getMonitoringServerInfo(
	  MonitoringServerInfo &monitoringServerInfo)
	{
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		ServerQueryOption option(USER_ID_SYSTEM);
		option.setTargetServerId(m_serverInfo.id);
		MonitoringServerInfoList monitoringServers;
		dataStore->getTargetServers(monitoringServers, option);

		if (monitoringServers.size() == 0) {
			MLPL_ERR("Target monitoring server is not found.\n");
			return HTERR_UNKNOWN_REASON;
		} else if (monitoringServers.size() > 1) {
			MLPL_ERR("Multiple monitoring servers is tied up.\n");
			return HTERR_UNKNOWN_REASON;
		}
		monitoringServerInfo = *monitoringServers.begin();
		return HTERR_OK;
	}

	bool isMonitoringServerInfoChanged(
	  const MonitoringServerInfo monitoringServer)
	{
		const MonitoringServerInfo &serverInfo = m_serverInfo;

		if (serverInfo.id != monitoringServer.id)
			return true;
		if (serverInfo.type != monitoringServer.type)
			return true;
		if (serverInfo.hostName != monitoringServer.hostName)
			return true;
		if (serverInfo.ipAddress != monitoringServer.ipAddress)
			return true;
		if (serverInfo.nickname != monitoringServer.nickname)
			return true;
		if (serverInfo.port != monitoringServer.port)
			return true;
		if (serverInfo.pollingIntervalSec !=
		    monitoringServer.pollingIntervalSec)
			return true;
		if (serverInfo.retryIntervalSec !=
		    monitoringServer.retryIntervalSec)
			return true;
		if (serverInfo.userName != monitoringServer.userName)
			return true;
		if (serverInfo.password != monitoringServer.password)
			return true;
		if (serverInfo.dbName != monitoringServer.dbName)
			return true;
		if (serverInfo.baseURL != monitoringServer.baseURL)
			return true;
		if (serverInfo.extendedInfo != monitoringServer.extendedInfo)
			return true;

		return false;
	}

	void upsertLastInfo(const string &lastInfoValue,
	                    const LastInfoType &type)
	{
		ThreadLocalDBCache cache;
		DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
		OperationPrivilege privilege(USER_ID_SYSTEM);
		const MonitoringServerInfo &serverInfo = m_serverInfo;
		LastInfoDef lastInfo;
		lastInfo.id = AUTO_INCREMENT_VALUE;
		lastInfo.dataType = type;
		lastInfo.value = lastInfoValue;
		lastInfo.serverId = serverInfo.id;
		const bool useTransaction = false;
		dbLastInfo.upsertLastInfo(lastInfo, privilege, useTransaction);
	}

	struct UpsertLastInfoHook : public DBAgent::TransactionHooks {
		Impl &impl;
		const LastInfoType type;
		string lastInfo;

		UpsertLastInfoHook(Impl &_impl, const LastInfoType &_type)
		: impl(_impl),
		  type(_type)
		{
		}

		virtual bool postAction(DBAgent &dbAgent) override
		{
			impl.upsertLastInfo(lastInfo, type);
			return true;
		}

		operator DBAgent::TransactionHooks *()
		{
			return lastInfo.empty() ? NULL : this;
		}
	};
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::HatoholArmPluginGateHAPI2(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: HatoholArmPluginInterfaceHAPI2(MODE_SERVER),
  m_impl(new Impl(serverInfo, *this))
{
	registerProcedureHandler(
	  HAPI2_EXCHANGE_PROFILE,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile);
	registerProcedureHandler(
	  HAPI2_MONITORING_SERVER_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo);
	registerProcedureHandler(
	  HAPI2_LAST_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerLastInfo);
	registerProcedureHandler(
	  HAPI2_PUT_ITEMS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutItems);
	registerProcedureHandler(
	  HAPI2_PUT_HISTORY,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHistory);
	registerProcedureHandler(
	  HAPI2_PUT_HOSTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHosts);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_GROUPS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroups);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroupMembership);
	registerProcedureHandler(
	  HAPI2_PUT_TRIGGERS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutTriggers);
	registerProcedureHandler(
	  HAPI2_PUT_EVENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutEvents);
	registerProcedureHandler(
	  HAPI2_PUT_HOST_PARENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHostParents);
	registerProcedureHandler(
	  HAPI2_PUT_ARM_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutArmInfo);

	if (autoStart)
		start();
}

void HatoholArmPluginGateHAPI2::start(void)
{
	updateSelfMonitor(m_impl->monitorGateInternal, false);
	HatoholArmPluginInterfaceHAPI2::start();
	m_impl->start();
	m_impl->startPlugin();
}

void HatoholArmPluginGateHAPI2::stop(void)
{
	m_impl->callUpdateMonitoringServerInfoNotificationIfNeeded();
	m_impl->stopPlugin();
	HatoholArmPluginInterfaceHAPI2::stop();
}

bool HatoholArmPluginGateHAPI2::isEstablished(void)
{
	return m_impl->isEstablished();
}

bool HatoholArmPluginGateHAPI2::parseTimeStamp(
  const string &timeStampString, timespec &timeStamp, const bool allowEmpty)
{
	timeStamp.tv_sec = 0;
	timeStamp.tv_nsec = 0;

	if (timeStampString.empty())
		return allowEmpty;

	StringVector list;
	StringUtils::split(list, timeStampString, '.', false);

	if (list.empty() || list.size() > 2)
		goto ERR;
	struct tm tm;
	if (!strptime(list[0].c_str(), "%Y%m%d%H%M%S", &tm))
		goto ERR;
	timeStamp.tv_sec = timegm(&tm); // as UTC

	if (list.size() == 1)
		return true;

	if (list[1].size() > 9)
		goto ERR;
	for (size_t i = 0; i < list[1].size(); i++) {
		unsigned int ch = list[1][i];
		if (ch < '0' || ch > '9')
			goto ERR;
	}
	for (size_t i = list[1].size(); i < 9; i++)
		list[1] += '0';
	timeStamp.tv_nsec = atol(list[1].c_str());

	return true;

 ERR:
	MLPL_ERR("Invalid timestamp format: %s\n",
		 timeStampString.c_str());
	return false;
}

static bool parseTimeStamp(
  JSONParser &parser, const string &member, timespec &timeStamp, JSONRPCError &errObj,
  const bool allowEmpty = false)
{
	string timeStampString;
	PARSE_AS_MANDATORY(member.c_str(), timeStampString, errObj);
	return HatoholArmPluginGateHAPI2::parseTimeStamp(timeStampString,
							 timeStamp,
							 allowEmpty);
}

bool HatoholArmPluginGateHAPI2::isFetchItemsSupported(void)
{
	return m_impl->hasProcedure(HAPI2_FETCH_ITEMS);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchItems(
  const LocalHostIdVector &hostIds, Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_ITEMS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_ITEMS);
	builder.startObject("params");
	if (!hostIds.empty()) {
		builder.startArray("hostIds");
		for (auto hostId : hostIds)
			builder.add(hostId);
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_ITEMS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

string buildTimeStamp(const time_t &timeValue)
{
	struct tm tm;;
	gmtime_r(&timeValue, &tm);
	char buf[32];
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tm);
	string timeString;
	timeString = buf;
	return timeString;
}

void HatoholArmPluginGateHAPI2::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_HISTORY)) {
		HistoryInfoVect historyInfoVect;
		if (closure)
			(*closure)(historyInfoVect);
		delete closure;
		return;
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_HISTORY);
	builder.startObject("params");
	builder.add("hostId", itemInfo.hostIdInServer);
	builder.add("itemId", itemInfo.id);
	builder.add("beginTime", buildTimeStamp(beginTime));
	builder.add("endTime", buildTimeStamp(endTime));
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchHistoryCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_HISTORY);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchTriggers(
  const LocalHostIdVector &hostIds, Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_TRIGGERS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_TRIGGERS);
	builder.startObject("params");
	if (!hostIds.empty()) {
		builder.startArray("hostIds");
		for (auto hostId : hostIds)
			builder.add(hostId);
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_TRIGGERS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchEvents(
  const std::string &lastInfo, const size_t count, const bool ascending,
  Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_EVENTS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_EVENTS);
	builder.startObject("params");
	builder.add("lastInfo", lastInfo);
	builder.add("count", count);
	builder.add("direction", ascending ? "ASC" : "DESC");
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_EVENTS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::~HatoholArmPluginGateHAPI2()
{
	stop();
}

const MonitoringServerInfo &
HatoholArmPluginGateHAPI2::getMonitoringServerInfo(void) const
{
	return m_impl->m_armFake.getServerInfo();
}

const ArmStatus &HatoholArmPluginGateHAPI2::getArmStatus(void) const
{
	return m_impl->m_armStatus;
}

string HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile(
  JSONParser &parser)
{
	m_impl->m_supportedProcedureNameSet.clear();
	JSONRPCError errObj;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");
	m_impl->parseExchangeProfileParams(parser, errObj);
	parser.endObject(); // params

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	setEstablished(true);

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	builder.add("name", PACKAGE_NAME);
	builder.startArray("procedures");
	for (auto procedureDef : getProcedureDefList()) {
		if (procedureDef.owner == HAP)
			continue;
		builder.add(procedureDef.name);
	}
	builder.endArray(); // procedures
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo(
  JSONParser &parser)
{
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	builder.add("serverId", serverInfo.id);
	builder.add("url", serverInfo.baseURL);
	builder.add("type", m_impl->m_pluginInfo.uuid);
	builder.add("nickName", serverInfo.nickname);
	builder.add("userName", serverInfo.userName);
	builder.add("password", serverInfo.password);
	builder.add("pollingIntervalSec", serverInfo.pollingIntervalSec);
	builder.add("retryIntervalSec", serverInfo.retryIntervalSec);
	builder.add("extendedInfo", serverInfo.extendedInfo);
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static const map<string, LastInfoType> LastInfoLabelMap = {
	{"host",                LAST_INFO_HOST},
	{"hostGroup",           LAST_INFO_HOST_GROUP},
	{"hostGroupMembership", LAST_INFO_HOST_GROUP_MEMBERSHIP},
	{"trigger",             LAST_INFO_TRIGGER},
	{"event",               LAST_INFO_EVENT},
	{"hostParent",          LAST_INFO_HOST_PARENT},
};

bool HatoholArmPluginGateHAPI2::convertLastInfoStrToType(
  const string &type, LastInfoType &lastInfoType)
{
	auto typeItr = LastInfoLabelMap.find(type);
	if (typeItr == LastInfoLabelMap.end()) {
		lastInfoType = LAST_INFO_ALL;
		return false;
	}
	lastInfoType = typeItr->second;
	return true;
}

static bool parseLastInfoParams(JSONParser &parser, LastInfoType &lastInfoType,
				JSONRPCError &errObj)
{
	string type;
	PARSE_AS_MANDATORY("params", type, errObj);

	return HatoholArmPluginGateHAPI2::convertLastInfoStrToType(type, lastInfoType);
}

string HatoholArmPluginGateHAPI2::procedureHandlerLastInfo(JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	LastInfoQueryOption option(USER_ID_SYSTEM);
	LastInfoType lastInfoType;
	JSONRPCError errObj;
	parseLastInfoParams(parser, lastInfoType, errObj);

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	option.setLastInfoType(lastInfoType);
	option.setTargetServerId(m_impl->m_serverInfo.id);
	LastInfoDefList lastInfoList;
	dbLastInfo.getLastInfoList(lastInfoList, option);
	string lastInfoValue =
	  lastInfoList.empty() ? "" : lastInfoList.begin()->value;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", lastInfoValue);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseItemParams(JSONParser &parser, ItemInfoList &itemInfoList,
			    const MonitoringServerInfo &serverInfo,
			    const HostInfoCache &hostInfoCache,
			    JSONRPCError &errObj)
{
	/**
	 * Get olny the first element from itemGroupName array.
	 *
	 * NOTE: This is tentative implementation. We will have to handle
	 *       all of the given item group names.
	 *
	 * @param name
	 * The first item group name. If the array is empty, an empty string
	 * is assigned.
	 * @return true if successful. Otherwise false.
	 */
	auto getItemGroupName = [&](string &name) {
		CHECK_MANDATORY_ARRAY_EXISTENCE("itemGroupName", errObj);
		parser.startObject("itemGroupName");
		size_t num = parser.countElements();
		if (num == 0)
			name.clear();
		else
			parser.read(0, name);
		// TODO: Don't ignore 2nd and the later itemGroupName.
		if (num > 1) {
			MLPL_WARN("Ignore 2nd and later itemGroups. This is "
			          "a limitation of the current version of "
			          "Hatohol (#1721).");
		}
		parser.endObject();
		return true;
	};

	CHECK_MANDATORY_ARRAY_EXISTENCE("items", errObj);
	parser.startObject("items");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse item contents.\n");
			errObj.addError("Failed to parse item array object.");
			parser.endObject(); // items
			return false;
		}

		ItemInfo itemInfo;
		itemInfo.id = AUTO_INCREMENT_VALUE;
		itemInfo.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("itemId", itemInfo.id, errObj);
		PARSE_AS_MANDATORY("hostId", itemInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("brief", itemInfo.brief, errObj);
		parseTimeStamp(parser, "lastValueTime", itemInfo.lastValueTime, errObj);
		PARSE_AS_MANDATORY("lastValue", itemInfo.lastValue, errObj);
		if (!getItemGroupName(itemInfo.itemGroupName))
			return false;
		PARSE_AS_MANDATORY("unit", itemInfo.unit, errObj);
		parser.endElement();
		HostInfoCache::Element cacheElem;
		const bool found =
			hostInfoCache.getName(itemInfo.hostIdInServer, cacheElem);
		if (!found) {
			MLPL_WARN(
			  "Host cache: not found. server: %" FMT_SERVER_ID ", "
			  "hostIdInServer: %" FMT_LOCAL_HOST_ID "\n",
			  serverInfo.id, itemInfo.hostIdInServer.c_str());
			cacheElem.hostId = INVALID_HOST_ID;
		}
		itemInfo.globalHostId = cacheElem.hostId;
		itemInfo.valueType = ITEM_INFO_VALUE_TYPE_UNKNOWN;
		itemInfo.delay = 0;
		itemInfoList.push_back(itemInfo);
	}
	parser.endObject(); // items
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutItems(JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList itemList;
	JSONRPCError errObj;
	string fetchId;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	const HostInfoCache &hostInfoCache = m_impl->hostInfoCache;
	parseItemParams(parser, itemList, serverInfo, hostInfoCache, errObj);
	if (parser.isMember("fetchId")) {
		parser.read("fetchId", fetchId);
	}

	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	dataStore->addItemList(itemList);

	if (!fetchId.empty()) {
		m_impl->runFetchCallback(fetchId);
	}

	// TODO: add error clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();

	return builder.generate();
}

static bool parseHistoryParams(JSONParser &parser, HistoryInfoVect &historyInfoVect,
			       const MonitoringServerInfo &serverInfo,
			       JSONRPCError &errObj)
{
	ItemIdType itemId = "";
	PARSE_AS_MANDATORY("itemId", itemId, errObj);
	CHECK_MANDATORY_ARRAY_EXISTENCE("samples", errObj);
	parser.startObject("samples");
	size_t num = parser.countElements();

	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse samples contents.\n");
			errObj.addError("Failed to parse samples array object.");
			parser.endObject(); // samples
			return false;
		}

		HistoryInfo historyInfo;
		historyInfo.itemId = itemId;
		historyInfo.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("value", historyInfo.value, errObj);
		parseTimeStamp(parser, "time", historyInfo.clock, errObj);
		parser.endElement();

		historyInfoVect.push_back(historyInfo);
	}
	parser.endObject(); // samples
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHistory(
  JSONParser &parser)
{
	HistoryInfoVect historyInfoVect;
	JSONRPCError errObj;
	string fetchId;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	parseHistoryParams(parser, historyInfoVect,
			   serverInfo, errObj);
	if (parser.isMember("fetchId")) {
		parser.read("fetchId", fetchId);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	if (!fetchId.empty()) {
		m_impl->runFetchHistoryCallback(fetchId, historyInfoVect);
	}

	// TODO: add error clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostsParams(JSONParser &parser, ServerHostDefVect &hostInfoVect,
			     const MonitoringServerInfo &serverInfo,
			     JSONRPCError &errObj)
{
	CHECK_MANDATORY_ARRAY_EXISTENCE("hosts", errObj);
	parser.startObject("hosts");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			errObj.addError("Failed to parse hosts array object.");
			parser.endObject(); // hosts
			return false;
		}

		ServerHostDef hostInfo;
		hostInfo.id = AUTO_INCREMENT_VALUE;
		hostInfo.hostId = AUTO_ASSIGNED_ID;
		hostInfo.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("hostId", hostInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("hostName", hostInfo.name, errObj);
		hostInfo.status = HOST_STAT_NORMAL;
		parser.endElement();

		hostInfoVect.push_back(hostInfo);
	}
	parser.endObject(); // hosts
	return true;
};

static bool parseUpdateType(JSONParser &parser, string &updateType,
			    JSONRPCError &errObj)
{
	PARSE_AS_MANDATORY("updateType", updateType, errObj);
	if (updateType == "ALL") {
		return true;
	} else if (updateType == "UPDATED") {
		return false;
	} else {
		MLPL_WARN("Invalid update type: %s\n", updateType.c_str());
		return false;
	}
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHosts(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect hostInfoVect;
	JSONRPCError errObj;
	Impl::UpsertLastInfoHook lastInfoUpserter(*m_impl, LAST_INFO_HOST);
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	parseHostsParams(parser, hostInfoVect, serverInfo, errObj);

	string updateType;
	bool checkInvalidHosts = parseUpdateType(parser, updateType, errObj);
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	// TODO: reflect error in response
	if (checkInvalidHosts) {
		dataStore->syncHosts(hostInfoVect, serverInfo.id,
	                             m_impl->hostInfoCache, lastInfoUpserter);
	} else {
		HostHostIdMap hostsMap;
		dataStore->upsertHosts(hostInfoVect, &hostsMap,
		                       lastInfoUpserter);
		m_impl->hostInfoCache.update(hostInfoVect, &hostsMap);
	}

	// TODO: add error clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostGroupsParams(JSONParser &parser,
				  HostgroupVect &hostgroupVect,
				  const MonitoringServerInfo &serverInfo,
				  JSONRPCError &errObj)
{
	CHECK_MANDATORY_ARRAY_EXISTENCE("hostGroups", errObj);
	parser.startObject("hostGroups");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			errObj.addError("Failed to parse hostGroups array object.");
			parser.endObject(); // hostGroups
			return false;
		}

		Hostgroup hostgroup;
		hostgroup.id = AUTO_INCREMENT_VALUE;
		hostgroup.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("groupId", hostgroup.idInServer, errObj);
		PARSE_AS_MANDATORY("groupName", hostgroup.name, errObj);
		parser.endElement();

		hostgroupVect.push_back(hostgroup);
	}
	parser.endObject(); // hostsGroups
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroups(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupVect hostgroupVect;
	JSONRPCError errObj;
	Impl::UpsertLastInfoHook lastInfoUpserter(*m_impl, LAST_INFO_HOST_GROUP);
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	parseHostGroupsParams(parser, hostgroupVect, serverInfo, errObj);
	string updateType;
	bool checkInvalidHostGroups = parseUpdateType(parser, updateType, errObj);
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	// TODO: reflect error in response
	if (checkInvalidHostGroups) {
		dataStore->syncHostgroups(hostgroupVect, serverInfo.id,
		                          lastInfoUpserter);
	} else {
		dataStore->upsertHostgroups(hostgroupVect, lastInfoUpserter);
	}

	// TODO: Add failure clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}


static bool parseHostGroupMembershipParams(
  JSONParser &parser,
  HostgroupMemberVect &hostgroupMemberVect,
  const MonitoringServerInfo &serverInfo,
  const HostInfoCache &hostInfoCache,
  JSONRPCError &errObj)
{
	CHECK_MANDATORY_ARRAY_EXISTENCE("hostGroupMembership", errObj);
	parser.startObject("hostGroupMembership");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			errObj.addError("Failed to parse hostGroupMembership array object.");
			parser.endObject(); // hostGroupMembership
			return false;
		}

		string hostIdInServer;
		PARSE_AS_MANDATORY("hostId", hostIdInServer, errObj);
		HostInfoCache::Element cacheElem;
		const bool found = hostInfoCache.getName(hostIdInServer,
							 cacheElem);
		if (!found) {
			MLPL_WARN(
			  "Host cache: not found. server: %" FMT_SERVER_ID ", "
			  "hostIdInServer: %" FMT_LOCAL_HOST_ID "\n",
			  serverInfo.id, hostIdInServer.c_str());
			cacheElem.hostId = INVALID_HOST_ID;
		}
		CHECK_MANDATORY_ARRAY_EXISTENCE_INNER_LOOP("groupIds", errObj);
		parser.startObject("groupIds");
		size_t groupIdNum = parser.countElements();
		for (size_t j = 0; j < groupIdNum; j++) {
			HostgroupMember hostgroupMember;
			hostgroupMember.id = AUTO_INCREMENT_VALUE;
			hostgroupMember.serverId = serverInfo.id;
			hostgroupMember.hostIdInServer = hostIdInServer;
			hostgroupMember.hostId = cacheElem.hostId;

			parser.read(j, hostgroupMember.hostgroupIdInServer);
			hostgroupMemberVect.push_back(hostgroupMember);
		}
		parser.endObject(); // groupIds
		parser.endElement();
	}
	parser.endObject(); // hostGroupsMembership
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostGroupMembership(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupMemberVect hostgroupMembershipVect;
	JSONRPCError errObj;
	Impl::UpsertLastInfoHook
	 lastInfoUpserter(*m_impl, LAST_INFO_HOST_GROUP_MEMBERSHIP);
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	const HostInfoCache &hostInfoCache = m_impl->hostInfoCache;
	parseHostGroupMembershipParams(parser,
				       hostgroupMembershipVect,
				       serverInfo, hostInfoCache, errObj);

	string updateType;
	bool checkInvalidHostGroupMembership =
		parseUpdateType(parser, updateType, errObj);
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	// TODO: reflect error in response
	if (checkInvalidHostGroupMembership) {
		dataStore->syncHostgroupMembers(hostgroupMembershipVect,
		                                serverInfo.id,
		                                lastInfoUpserter);
	} else {
		dataStore->upsertHostgroupMembers(hostgroupMembershipVect,
		                                  lastInfoUpserter);
	}

	// add error clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseTriggerStatus(JSONParser &parser, TriggerStatusType &status,
			       JSONRPCError &errObj, bool skipValidate)
{
	string statusString;
	if (skipValidate) {
		parser.read("status", statusString);
	} else {
		PARSE_AS_MANDATORY("status", statusString, errObj);
	}
	if (statusString == "OK") {
		status = TRIGGER_STATUS_OK;
	} else if (statusString == "NG") {
		status = TRIGGER_STATUS_PROBLEM;
	} else if (statusString == "UNKNOWN") {
		status = TRIGGER_STATUS_UNKNOWN;
	} else {
		MLPL_WARN("Unknown trigger status: %s\n", statusString.c_str());
		status = TRIGGER_STATUS_UNKNOWN;
	}
	return true;
}

static bool parseTriggerSeverity(JSONParser &parser,
				 TriggerSeverityType &severity,
				 JSONRPCError &errObj, bool skipValidate)
{
	string severityString;
	if (skipValidate) {
		parser.read("severity", severityString);
	} else {
		PARSE_AS_MANDATORY("severity", severityString, errObj);
	}
	if (severityString == "ALL") {
		severity = TRIGGER_SEVERITY_ALL;
	} else if (severityString == "UNKNOWN") {
		severity = TRIGGER_SEVERITY_UNKNOWN;
	} else if (severityString == "INFO") {
		severity = TRIGGER_SEVERITY_INFO;
	} else if (severityString == "WARNING") {
		severity = TRIGGER_SEVERITY_WARNING;
	} else if (severityString == "ERROR") {
		severity = TRIGGER_SEVERITY_ERROR;
	} else if (severityString == "CRITICAL") {
		severity = TRIGGER_SEVERITY_CRITICAL;
	} else if (severityString == "EMERGENCY") {
		severity = TRIGGER_SEVERITY_EMERGENCY;
	} else {
		MLPL_WARN("Unknown trigger severity: %s\n",
			  severityString.c_str());
		severity = TRIGGER_SEVERITY_UNKNOWN;
	}
	return true;
}

static HostIdType getHostInfoCacheWithAdhocRegistration(
  HostInfoCache &hostInfoCache, const ServerIdType &serverId,
  const LocalHostIdType &hostIdInServer, const string &name)
{
	HostInfoCache::Element cacheElem;
	const bool found = hostInfoCache.getName(hostIdInServer, cacheElem);
	if (!found) {
		hostInfoCache.registerAdHoc(hostIdInServer, name, cacheElem);
		MLPL_INFO(
		  "Host cache: add-hoc registration. "
		  "server: %" FMT_SERVER_ID ", "
		  "hostIdInServer: %" FMT_LOCAL_HOST_ID "\n",
		  serverId, hostIdInServer.c_str());
	}
	return cacheElem.hostId;
}

static bool parseTriggersParams(JSONParser &parser, TriggerInfoList &triggerInfoList,
				const MonitoringServerInfo &serverInfo,
				HostInfoCache &hostInfoCache,
				JSONRPCError &errObj)
{
	CHECK_MANDATORY_ARRAY_EXISTENCE("triggers", errObj);
	parser.startObject("triggers");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse triggers contents.\n");
			errObj.addError("Failed to parse triggers array object.");
			parser.endObject(); // triggers
			return false;
		}

		TriggerInfo triggerInfo;
		triggerInfo.id = AUTO_INCREMENT_VALUE;
		PARSE_AS_MANDATORY("triggerId",   triggerInfo.id, errObj);
		triggerInfo.serverId = serverInfo.id;
		parseTriggerStatus(parser, triggerInfo.status, errObj, false);
		parseTriggerSeverity(parser, triggerInfo.severity, errObj, false);
		parseTimeStamp(parser, "lastChangeTime", triggerInfo.lastChangeTime, errObj);
		PARSE_AS_MANDATORY("hostId",       triggerInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("hostName",     triggerInfo.hostName, errObj);
		PARSE_AS_MANDATORY("brief",        triggerInfo.brief, errObj);
		PARSE_AS_MANDATORY("extendedInfo", triggerInfo.extendedInfo, errObj);
		parser.endElement();

		triggerInfo.validity = TRIGGER_VALID;

		triggerInfo.globalHostId =
		    getHostInfoCacheWithAdhocRegistration(
		      hostInfoCache, serverInfo.id,
		      triggerInfo.hostIdInServer, triggerInfo.hostName);
		triggerInfoList.push_back(triggerInfo);
	}
	parser.endObject(); // triggers
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutTriggers(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerInfoList;
	JSONRPCError errObj;
	Impl::UpsertLastInfoHook lastInfoUpserter(*m_impl, LAST_INFO_TRIGGER);
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	parseTriggersParams(parser, triggerInfoList,
	                    serverInfo, m_impl->hostInfoCache, errObj);

	string updateType;
	bool checkInvalidTriggers = parseUpdateType(parser, updateType, errObj);
	string fetchId;
	if (parser.isMember("fetchId")) {
		parser.read("fetchId", fetchId);
	}
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	// TODO: reflect error in response
	if (checkInvalidTriggers) {
		dataStore->syncTriggers(triggerInfoList, serverInfo.id,
		                        lastInfoUpserter);
	} else {
		dataStore->addTriggers(triggerInfoList, lastInfoUpserter);
	}

	if (!fetchId.empty()) {
		m_impl->runFetchCallback(fetchId);
	}

	// add failure
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseEventType(JSONParser &parser, EventInfo &eventInfo,
			   JSONRPCError &errObj)
{
	string eventType;
	PARSE_AS_MANDATORY("type", eventType, errObj);

	if (eventType == "GOOD") {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else if (eventType == "BAD") {
		eventInfo.type = EVENT_TYPE_BAD;
	} else if (eventType == "UNKNOWN") {
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	} else if (eventType == "NOTIFICATION") {
		eventInfo.type = EVENT_TYPE_NOTIFICATION;
	} else {
		MLPL_WARN("Invalid event type: %s\n", eventType.c_str());
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	}
	return true;
};

static bool parseEventsParams(JSONParser &parser, EventInfoList &eventInfoList,
			      const MonitoringServerInfo &serverInfo,
			      HostInfoCache &hostInfoCache,
			      JSONRPCError &errObj)
{
	CHECK_MANDATORY_ARRAY_EXISTENCE("events", errObj);
	parser.startObject("events");
	size_t num = parser.countElements();
	constexpr const size_t numLimit = 1000;

	if (num > numLimit) {
		string errorMessage =
		  StringUtils::sprintf(
		    "Event Object is too large. "
		    "Object size limit(%zd) exceeded.\n",
		    numLimit);
		MLPL_ERR("%s", errorMessage.c_str());
		errObj.addError("%s", errorMessage.c_str());
		parser.endObject(); // events
		return false;
	}

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse events contents.\n");
			errObj.addError("Failed to parse events array object.");
			parser.endObject(); // events
			return false;
		}

		EventInfo eventInfo;
		eventInfo.unifiedId = AUTO_INCREMENT_VALUE;
		eventInfo.serverId = serverInfo.id;
		PARSE_AS_MANDATORY("eventId",  eventInfo.id, errObj);
		parseTimeStamp(parser, "time", eventInfo.time, errObj);
		parseEventType(parser, eventInfo, errObj);
		TriggerIdType triggerId;
		if (parser.read("triggerId", triggerId)) {
			eventInfo.triggerId = triggerId;
		} else {
			eventInfo.triggerId = DO_NOT_ASSOCIATE_TRIGGER_ID;
		}
		parseTriggerStatus(parser,         eventInfo.status, errObj, true);
		parseTriggerSeverity(parser,       eventInfo.severity, errObj, true);
		PARSE_AS_MANDATORY("hostId",       eventInfo.hostIdInServer, errObj);
		PARSE_AS_MANDATORY("hostName",     eventInfo.hostName, errObj);
		PARSE_AS_MANDATORY("brief",        eventInfo.brief, errObj);
		PARSE_AS_MANDATORY("extendedInfo", eventInfo.extendedInfo, errObj);
		parser.endElement();

		eventInfo.globalHostId =
		  getHostInfoCacheWithAdhocRegistration(
		    hostInfoCache, serverInfo.id,
		    eventInfo.hostIdInServer, eventInfo.hostName);
		eventInfoList.push_back(eventInfo);
	}
	parser.endObject(); // events
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutEvents(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventInfoList;
	JSONRPCError errObj;
	string fetchId;
	Impl::UpsertLastInfoHook lastInfoUpserter(*m_impl, LAST_INFO_EVENT);
	bool mayMoreFlag = false;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	parseEventsParams(parser, eventInfoList, serverInfo,
	                  m_impl->hostInfoCache, errObj);

	if (parser.isMember("fetchId")) {
		parser.read("fetchId", fetchId);
		if (parser.isMember("mayMoreFlag")) {
			parser.read("mayMoreFlag", mayMoreFlag);
		} else {
			MLPL_WARN("No mayMoreFlag while a fetchId is provided!\n");
		}
	}
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	dataStore->addEventList(eventInfoList, lastInfoUpserter);

	if (!mayMoreFlag)
		m_impl->runFetchCallback(fetchId);

	// TODO: add error clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostParentsParams(
  JSONParser &parser,
  VMInfoVect &vmInfoVect,
  JSONRPCError &errObj
)
{
	StringList errors;
	CHECK_MANDATORY_ARRAY_EXISTENCE("hostParents", errObj);
	parser.startObject("hostParents");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse hostParents contents.\n");
			errObj.addError("Failed to parse hostParents array object.");
			parser.endObject(); // hostParents
			return false;
		}

		VMInfo vmInfo;
		vmInfo.id = AUTO_INCREMENT_VALUE;
		string childHostId, parentHostId;
		PARSE_AS_MANDATORY("childHostId", childHostId, errObj);
		vmInfo.hostId = StringUtils::toUint64(childHostId);
		PARSE_AS_MANDATORY("parentHostId", parentHostId, errObj);
		vmInfo.hypervisorHostId = StringUtils::toUint64(parentHostId);
		parser.endElement();

		vmInfoVect.push_back(vmInfo);
	}
	parser.endObject(); // hostsParents
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHostParents(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	VMInfoVect vmInfoVect;
	JSONRPCError errObj;
	Impl::UpsertLastInfoHook lastInfoUpserter(*m_impl,
	                                          LAST_INFO_HOST_PARENT);
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	parseHostParentsParams(parser, vmInfoVect, errObj);

	string updateType;
	bool checkInvalidHostParents = parseUpdateType(parser, updateType, errObj);
	MLPL_BUG("Take into account the result: checkInvalidHostParents: %d.",
	         checkInvalidHostParents);
	if (parser.isMember("lastInfo")) {
		parser.read("lastInfo", lastInfoUpserter.lastInfo);
	}
	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	dbHost.upsertVMInfoVect(vmInfoVect, lastInfoUpserter);

	// TODO: make failure clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseArmInfoParams(JSONParser &parser, ArmInfo &armInfo,
			       JSONRPCError &errObj)
{
	string status;
	PARSE_AS_MANDATORY("lastStatus", status, errObj);
	if (status == "INIT") {
		armInfo.stat = ARM_WORK_STAT_INIT;
	} else if (status == "OK") {
		armInfo.stat = ARM_WORK_STAT_OK;
	} else if (status == "NG") {
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	} else {
		MLPL_WARN("Invalid status: %s\n", status.c_str());
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	}
	PARSE_AS_MANDATORY("failureReason", armInfo.failureComment, errObj);
	timespec successTime, failureTime;
	const bool allowEmpty = true;
	parseTimeStamp(parser, "lastSuccessTime", successTime, errObj, allowEmpty);
	parseTimeStamp(parser, "lastFailureTime", failureTime, errObj, allowEmpty);
	SmartTime lastSuccessTime(successTime);
	SmartTime lastFailureTime(failureTime);
	armInfo.statUpdateTime = SmartTime(SmartTime::INIT_CURR_TIME);
	armInfo.lastSuccessTime = lastSuccessTime;
	armInfo.lastFailureTime = lastFailureTime;

	int64_t numSuccess, numFailure;
	PARSE_AS_MANDATORY("numSuccess", numSuccess, errObj);
	PARSE_AS_MANDATORY("numFailure", numFailure, errObj);
	armInfo.numUpdate = (size_t)numSuccess;
	armInfo.numFailure = (size_t)numFailure;

	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutArmInfo(
  JSONParser &parser)
{
	ArmStatus &status = m_impl->m_armStatus;
	ArmInfo armInfo;
	JSONRPCError errObj;
	CHECK_MANDATORY_PARAMS_EXISTENCE("params", errObj);
	parser.startObject("params");

	armInfo.running = status.getArmInfo().running;
	parseArmInfoParams(parser, armInfo, errObj);

	parser.endObject(); // params

	updateSelfMonitor(m_impl->monitorParseError, errObj.hasErrors());

	if (errObj.hasErrors()) {
		return HatoholArmPluginInterfaceHAPI2::buildErrorResponse(
		  JSON_RPC_INVALID_PARAMS, "Invalid method parameter(s).",
		  &errObj.getErrors(), &parser);
	}

	status.setArmInfo(armInfo);
	updateSelfMonitor(m_impl->monitorPluginInternal,
	                  armInfo.stat != ARM_WORK_STAT_OK);

	// TODO: add failure clause
	string result = "SUCCESS";
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholArmPluginGateHAPI2::onConnect(void)
{
	updateSelfMonitor(m_impl->monitorBrokerConn, false);
}

void HatoholArmPluginGateHAPI2::onConnectFailure(void)
{
	updateSelfMonitor(m_impl->monitorBrokerConn, true);
}
