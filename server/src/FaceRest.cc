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

#include <cstring>
#include <Logger.h>
#include <Reaper.h>
#include <MutexLock.h>
#include <SmartTime.h>
using namespace mlpl;

#include <uuid/uuid.h>

#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "ConfigManager.h"
#include "UnifiedDataStore.h"
#include "DBClientUser.h"

int FaceRest::API_VERSION = 3;
const char *FaceRest::SESSION_ID_HEADER_NAME = "X-Hatohol-Session";

typedef void (*RestHandler)
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data);

typedef uint64_t ServerID;
typedef uint64_t HostID;
typedef uint64_t TriggerID;
typedef map<HostID, string> HostNameMap;
typedef map<ServerID, HostNameMap> HostNameMaps;

typedef map<TriggerID, string> TriggerBriefMap;
typedef map<ServerID, TriggerBriefMap> TriggerBriefMaps;

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForTest        = "/test";
const char *FaceRest::pathForLogin       = "/login";
const char *FaceRest::pathForLogout      = "/logout";
const char *FaceRest::pathForGetOverview = "/overview";
const char *FaceRest::pathForGetServer   = "/server";
const char *FaceRest::pathForGetHost     = "/host";
const char *FaceRest::pathForGetTrigger  = "/trigger";
const char *FaceRest::pathForGetEvent    = "/event";
const char *FaceRest::pathForGetItem     = "/item";
const char *FaceRest::pathForAction      = "/action";
const char *FaceRest::pathForUser        = "/user";

static const char *MIME_HTML = "text/html";
static const char *MIME_JSON = "application/json";
static const char *MIME_JAVASCRIPT = "text/javascript";

#define REPLY_ERROR(MSG, ARG, ERR_CODE, ERR_MSG_FMT, ...) \
do { \
	string optMsg = StringUtils::sprintf(ERR_MSG_FMT, ##__VA_ARGS__); \
	replyError(MSG, ARG, ERR_CODE, optMsg); \
} while (0)

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

struct FaceRest::HandlerArg
{
	string     formatString;
	FormatType formatType;
	const char *mimeType;
	string      id; // we assume URL form is http://example.com/request/id
	string      jsonpCallbackName;
	string      sessionId;
	UserIdType  userId;
};

typedef map<string, FormatType> FormatTypeMap;
typedef FormatTypeMap::iterator FormatTypeMapIterator;
static FormatTypeMap g_formatTypeMap;

typedef map<FormatType, const char *> MimeTypeMap;
typedef MimeTypeMap::iterator   MimeTypeMapIterator;
static MimeTypeMap g_mimeTypeMap;

// Key: session ID, value: user ID
typedef map<string, SessionInfo *>           SessionIdMap;
typedef map<string, SessionInfo *>::iterator SessionIdMapIterator;

// constructor
SessionInfo::SessionInfo(void)
: userId(INVALID_USER_ID),
  loginTime(SmartTime::INIT_CURR_TIME),
  lastAccessTime(SmartTime::INIT_CURR_TIME)
{
}

struct FaceRest::PrivateContext {
	static bool         testMode;
	static MutexLock    lock;
	static SessionIdMap sessionIdMap;

	static void insertSessionId(const string &sessionId, UserIdType userId)
	{
		SessionInfo *sessionInfo = new SessionInfo();
		sessionInfo->userId = userId;
		lock.lock();
		sessionIdMap[sessionId] = sessionInfo;
		lock.unlock();
	}

	static bool removeSessionId(const string &sessionId) {
		lock.lock();
		SessionIdMapIterator it = sessionIdMap.find(sessionId);
		bool found = it != sessionIdMap.end();
		if (found)
			sessionIdMap.erase(it);
		lock.unlock();
		if (!found) {
			MLPL_WARN("Failed to erase session ID: %s\n",
			          sessionId.c_str());
		}
		return found;
	}

	static const SessionInfo *getSessionInfo(const string &sessionId) {
		SessionIdMapIterator it = sessionIdMap.find(sessionId);
		if (it == sessionIdMap.end())
			return NULL;
		return it->second;
	}
};

bool         FaceRest::PrivateContext::testMode = false;
MutexLock    FaceRest::PrivateContext::lock;
SessionIdMap FaceRest::PrivateContext::sessionIdMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void FaceRest::init(void)
{
	g_formatTypeMap["html"] = FORMAT_HTML;
	g_formatTypeMap["json"] = FORMAT_JSON;
	g_formatTypeMap["jsonp"] = FORMAT_JSONP;

	g_mimeTypeMap[FORMAT_HTML] = MIME_HTML;
	g_mimeTypeMap[FORMAT_JSON] = MIME_JSON;
	g_mimeTypeMap[FORMAT_JSONP] = MIME_JAVASCRIPT;
}

void FaceRest::reset(const CommandLineArg &arg)
{
	bool foundTestMode = false;
	for (size_t i = 0; i < arg.size(); i++) {
		if (arg[i] == "--test-mode")
			foundTestMode = true;
	}

	if (foundTestMode)
		MLPL_INFO("Run as a test mode.\n");
	PrivateContext::testMode = foundTestMode;
}

bool FaceRest::isTestMode(void)
{
	return PrivateContext::testMode;
}

FaceRest::FaceRest(CommandLineArg &cmdArg)
: m_port(DEFAULT_PORT),
  m_soupServer(NULL)
{
	DBClientConfig dbConfig;
	int port = dbConfig.getFaceRestPort();
	if (port != 0 && Utils::isValidPort(port))
		m_port = port;

	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--face-rest-port")
			i = parseCmdArgPort(cmdArg, i);
	}
	MLPL_INFO("started face-rest, port: %d\n", m_port);
}

FaceRest::~FaceRest()
{
	// wait for the finish of the thread
	stop();

	MLPL_INFO("FaceRest: stop process: started.\n");
	if (m_soupServer) {
		SoupSocket *sock = soup_server_get_listener(m_soupServer);
		soup_socket_disconnect(sock);
		g_object_unref(m_soupServer);
	}
	MLPL_INFO("FaceRest: stop process: completed.\n");
}

void FaceRest::stop(void)
{
	HATOHOL_ASSERT(m_soupServer, "m_soupServer: NULL");
	soup_server_quit(m_soupServer);

	HatoholThreadBase::stop();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceRest::mainThread(HatoholThreadArg *arg)
{
	GMainContext *gMainCtx = g_main_context_new();
	m_soupServer = soup_server_new(SOUP_SERVER_PORT, m_port,
	                               SOUP_SERVER_ASYNC_CONTEXT, gMainCtx,
	                               NULL);
	HATOHOL_ASSERT(m_soupServer, "failed: soup_server_new: %u\n", m_port);
	soup_server_add_handler(m_soupServer, NULL, handlerDefault, this, NULL);
	soup_server_add_handler(m_soupServer, "/hello.html",
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerHelloPage, NULL);
	soup_server_add_handler(m_soupServer, "/test",
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerTest, NULL);
	soup_server_add_handler(m_soupServer, pathForLogin,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerLogin, NULL);
	soup_server_add_handler(m_soupServer, pathForLogout,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerLogout, NULL);
	soup_server_add_handler(m_soupServer, pathForGetOverview,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetOverview, NULL);
	soup_server_add_handler(m_soupServer, pathForGetServer,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetServer, NULL);
	soup_server_add_handler(m_soupServer, pathForGetHost,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetHost, NULL);
	soup_server_add_handler(m_soupServer, pathForGetTrigger,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetTrigger, NULL);
	soup_server_add_handler(m_soupServer, pathForGetEvent,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetEvent, NULL);
	soup_server_add_handler(m_soupServer, pathForGetItem,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetItem, NULL);
	soup_server_add_handler(m_soupServer, pathForAction,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerAction, NULL);
	soup_server_add_handler(m_soupServer, pathForUser,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerUser, NULL);
	soup_server_run(m_soupServer);
	g_main_context_unref(gMainCtx);
	MLPL_INFO("exited face-rest\n");
	return NULL;
}

size_t FaceRest::parseCmdArgPort(CommandLineArg &cmdArg, size_t idx)
{
	if (idx == cmdArg.size() - 1) {
		MLPL_ERR("needs port number.");
		return idx;
	}

	idx++;
	string &port_str = cmdArg[idx];
	int port = atoi(port_str.c_str());
	if (!Utils::isValidPort(port, false)) {
		MLPL_ERR("invalid port: %s, %d\n", port_str.c_str(), port);
		return idx;
	}

	m_port = port;
	return idx;
}

void FaceRest::addHatoholError(JsonBuilderAgent &agent,
                               const HatoholError &err)
{
	agent.add("apiVersion", API_VERSION);
	agent.add("errorCode", err.getCode());
	if (!err.getOptionMessage().empty())
		agent.add("optionMessages", err.getOptionMessage().c_str());
}

void FaceRest::replyError(SoupMessage *msg, const HandlerArg *arg,
                          const HatoholError &hatoholError)
{
	replyError(msg, arg, hatoholError.getCode(),
	           hatoholError.getOptionMessage());
}

void FaceRest::replyError(SoupMessage *msg, const HandlerArg *arg,
                          const HatoholErrorCode &errorCode,
                          const string &optionMessage)
{
	if (optionMessage.empty())
		MLPL_ERR("error: %d\n", errorCode);
	else
		MLPL_ERR("error: %d, %s", errorCode, optionMessage.c_str());

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, errorCode);
	agent.endObject();
	string response = agent.generate();
	if (!arg->jsonpCallbackName.empty())
		response = wrapForJsonp(response, arg->jsonpCallbackName);
	soup_message_headers_set_content_type(msg->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

string FaceRest::getJsonpCallbackName(GHashTable *query, HandlerArg *arg)
{
	if (arg->formatType != FORMAT_JSONP)
		return "";
	gpointer value = g_hash_table_lookup(query, "callback");
	if (!value)
		THROW_HATOHOL_EXCEPTION("Not found parameter: callback");

	const char *callbackName = (const char *)value;
	string errMsg;
	if (!Utils::validateJSMethodName(callbackName, errMsg)) {
		THROW_HATOHOL_EXCEPTION(
		  "Invalid callback name: %s", errMsg.c_str());
	}
	return callbackName;
}

string FaceRest::wrapForJsonp(const string &jsonBody,
                              const string &callbackName)
{
	string jsonp = callbackName;
	jsonp += "(";
	jsonp += jsonBody;
	jsonp += ")";
	return jsonp;
}

void FaceRest::replyJsonData(JsonBuilderAgent &agent, SoupMessage *msg,
                             const string &jsonpCallbackName, HandlerArg *arg)
{
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(msg->response_headers,
	                                      arg->mimeType, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

const SessionInfo *FaceRest::getSessionInfo(const string &sessionId)
{
	return PrivateContext::getSessionInfo(sessionId);
}

void FaceRest::parseQueryServerId(GHashTable *query, uint32_t &serverId)
{
	serverId = ALL_SERVERS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "serverId");
	if (!value)
		return;

	uint32_t svId;
	if (sscanf(value, "%"PRIu32, &svId) == 1)
		serverId = svId;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

void FaceRest::parseQueryHostId(GHashTable *query, uint64_t &hostId)
{
	hostId = ALL_HOSTS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "hostId");
	if (!value)
		return;

	uint64_t id;
	if (sscanf(value, "%"PRIu64, &id) == 1)
		hostId = id;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

void FaceRest::parseQueryTriggerId(GHashTable *query, uint64_t &triggerId)
{
	triggerId = ALL_TRIGGERS;
	if (!query)
		return;
	gchar *value = (gchar *)g_hash_table_lookup(query, "triggerId");
	if (!value)
		return;

	uint64_t id;
	if (sscanf(value, "%"PRIu64, &id) == 1)
		triggerId = id;
	else
		MLPL_INFO("Invalid requested ID: %s\n", value);
}

// handlers
void FaceRest::handlerDefault(SoupServer *server, SoupMessage *msg,
                              const char *path, GHashTable *query,
                              SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

bool FaceRest::parseFormatType(GHashTable *query, HandlerArg &arg)
{
	arg.formatString.clear();
	if (!query) {
		arg.formatType = FORMAT_JSON;
		return true;
	}

	gchar *format = (gchar *)g_hash_table_lookup(query, "fmt");
	if (!format) {
		arg.formatType = FORMAT_JSON; // default value
		return true;
	}
	arg.formatString = format;

	FormatTypeMapIterator fmtIt = g_formatTypeMap.find(format);
	if (fmtIt == g_formatTypeMap.end())
		return false;
	arg.formatType = fmtIt->second;
	return true;
}

void FaceRest::launchHandlerInTryBlock
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *_query, SoupClientContext *client, gpointer user_data)
{
	RestHandler handler = reinterpret_cast<RestHandler>(user_data);
	HandlerArg arg;

	const char *sessionId =
	   soup_message_headers_get_one(msg->request_headers,
	                                SESSION_ID_HEADER_NAME);
	if (!sessionId) {
		// We should return an error. But now, we just set
		// USER_ID_ADMIN to keep compatiblity until the user privilege
		// feature is completely implemnted.
		if (path != pathForLogin)
			arg.userId = USER_ID_ADMIN;
		else
			arg.userId = INVALID_USER_ID;
	} else {
		arg.sessionId = sessionId;
		PrivateContext::lock.lock();
		const SessionInfo *sessionInfo = getSessionInfo(sessionId);
		if (!sessionInfo) {
			PrivateContext::lock.unlock();
			replyError(msg, &arg, HTERR_NOT_FOUND_SESSION_ID);
			return;
		}
		arg.userId = sessionInfo->userId;
		PrivateContext::lock.unlock();
	}

	// We expect URIs  whose style are the following.
	//
	// Examples:
	// http://localhost:33194/action
	// http://localhost:33194/action?fmt=json
	// http://localhost:33194/action/2345?fmt=html

	GHashTable *query = _query;
	Reaper<GHashTable> postQueryReaper;
	if (strcasecmp(msg->method, "POST") == 0) {
		// The POST request contains query parameters in the body
		// according to application/x-www-form-urlencoded.
		query = soup_form_decode(msg->request_body->data);
		postQueryReaper.set(query, g_hash_table_unref);
	}

	// a format type
	if (!parseFormatType(query, arg)) {
		REPLY_ERROR(msg, &arg, HTERR_UNSUPORTED_FORMAT,
		            "%s", arg.formatString.c_str());
		return;
	}

	// ID
	StringVector pathElemVect;
	StringUtils::split(pathElemVect, path, '/');
	if (pathElemVect.size() >= 2)
		arg.id = pathElemVect[1];

	// MIME
	MimeTypeMapIterator mimeIt = g_mimeTypeMap.find(arg.formatType);
	HATOHOL_ASSERT(
	  mimeIt != g_mimeTypeMap.end(),
	  "Invalid formatType: %d, %s", arg.formatType, path);
	arg.mimeType = mimeIt->second;

	// jsonp callback name
	arg.jsonpCallbackName = getJsonpCallbackName(query, &arg);

	try {
		(*handler)(server, msg, path, query, client, &arg);
	} catch (const HatoholException &e) {
		REPLY_ERROR(msg, &arg, HTERR_GOT_EXCEPTION,
		            "%s", e.getFancyMessage().c_str());
	}
}

void FaceRest::handlerHelloPage
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	string response;
	const char *pageTemplate =
	  "<html>"
	  "HATOHOL Server ver. %s"
	  "</html>";
	response = StringUtils::sprintf(pageTemplate, PACKAGE_VERSION);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

static void addOverviewEachServer(JsonBuilderAgent &agent,
                                  MonitoringServerInfo &svInfo)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	agent.add("serverId", svInfo.id);
	agent.add("serverHostName", svInfo.hostName);
	agent.add("serverIpAddr", svInfo.ipAddress);
	agent.add("serverNickname", svInfo.nickname);

	// TODO: This implementeation is not effective.
	//       We should add a function only to get the number of list.
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, svInfo.id);
	agent.add("numberOfHosts", hostInfoList.size());

	ItemInfoList itemInfoList;
	dataStore->getItemList(itemInfoList, svInfo.id);
	agent.add("numberOfItems", itemInfoList.size());

	TriggerInfoList triggerInfoList;
	dataStore->getTriggerList(triggerInfoList, svInfo.id);
	agent.add("numberOfTriggers", triggerInfoList.size());

	// TODO: These elements should be fixed
	// after the funtion concerned is added
	agent.add("numberOfUsers", 0);
	agent.add("numberOfOnlineUsers", 0);
	agent.add("numberOfMonitoredItemsPerSecond", 0);

	// HostGroups
	// TODO: We temtatively returns 'No group'. We should fix it
	//       after host group is supported in Hatohol server.
	agent.startObject("hostGroups");
	size_t numHostGroup = 1;
	uint64_t hostGroupIds[1] = {0};
	for (size_t i = 0; i < numHostGroup; i++) {
		agent.startObject(StringUtils::toString(hostGroupIds[i]));
		agent.add("name", "No group");
		agent.endObject();
	}
	agent.endObject();

	// SystemStatus
	agent.startArray("systemStatus");
	for (size_t i = 0; i < numHostGroup; i++) {
		uint64_t hostGroupId = hostGroupIds[i];
		for (int severity = 0;
		     severity < NUM_TRIGGER_SEVERITY; severity++) {
			agent.startObject();
			agent.add("hostGroupId", hostGroupId);
			agent.add("severity", severity);
			agent.add(
			  "numberOfHosts",
			  dataStore->getNumberOfTriggers
			    (svInfo.id, hostGroupId,
			     (TriggerSeverityType)severity));
			agent.endObject();
		}
	}
	agent.endArray();

	// HostStatus
	agent.startArray("hostStatus");
	for (size_t i = 0; i < numHostGroup; i++) {
		uint64_t hostGroupId = hostGroupIds[i];
		uint32_t svid = svInfo.id;
		agent.startObject();
		agent.add("hostGroupId", hostGroupId);
		agent.add("numberOfGoodHosts",
		          dataStore->getNumberOfGoodHosts(svid, hostGroupId));
		agent.add("numberOfBadHosts",
		          dataStore->getNumberOfBadHosts(svid, hostGroupId));
		agent.endObject();
	}
	agent.endArray();
}

static void addOverview(JsonBuilderAgent &agent)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("serverStatus");
	for (; it != monitoringServers.end(); ++it) {
		agent.startObject();
		addOverviewEachServer(agent, *it);
		agent.endObject();
	}
	agent.endArray();

	agent.startArray("badServers");
	// TODO: implemented
	agent.endArray();
}

static void addServers(JsonBuilderAgent &agent, uint32_t targetServerId)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers, targetServerId);

	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject();
		agent.add("id", serverInfo.id);
		agent.add("type", serverInfo.type);
		agent.add("hostName", serverInfo.hostName);
		agent.add("ipAddress", serverInfo.ipAddress);
		agent.add("nickname", serverInfo.nickname);
		agent.endObject();
	}
	agent.endArray();
}

static void addHosts(JsonBuilderAgent &agent,
                     uint32_t targetServerId, uint64_t targetHostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, targetServerId, targetHostId);

	agent.add("numberOfHosts", hostInfoList.size());
	agent.startArray("hosts");
	HostInfoListIterator it = hostInfoList.begin();
	for (; it != hostInfoList.end(); ++it) {
		HostInfo &hostInfo = *it;
		agent.startObject();
		agent.add("id", hostInfo.id);
		agent.add("serverId", hostInfo.serverId);
		agent.add("hostName", hostInfo.hostName);
		agent.endObject();
	}
	agent.endArray();
}

static string getHostName(const ServerID serverId, const HostID hostId)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string hostName;
	HostInfoList hostInfoList;
	dataStore->getHostList(hostInfoList, serverId, hostId);
	if (hostInfoList.empty()) {
		MLPL_WARN("Failed to get HostInfo: "
		          "%"PRIu32", %"PRIu64"\n",
		          serverId, hostId);
	} else {
		HostInfo &hostInfo = *hostInfoList.begin();
		hostName = hostInfo.hostName;
	}
	return hostName;
}

static void addHostsIdNameHash(
  JsonBuilderAgent &agent, MonitoringServerInfo &serverInfo,
  HostNameMaps &hostMaps, bool lookupHostName = false)
{
	HostNameMaps::iterator server_it = hostMaps.find(serverInfo.id);
	agent.startObject("hosts");
	ServerID serverId = server_it->first;
	HostNameMap &hosts = server_it->second;
	HostNameMap::iterator it = hosts.begin();
	for (; server_it != hostMaps.end() && it != hosts.end(); it++) {
		HostID hostId = it->first;
		string &hostName = it->second;
		if (lookupHostName)
			hostName = getHostName(serverId, hostId);
		agent.startObject(StringUtils::toString(hostId));
		agent.add("name", hostName);
		agent.endObject();
	}
	agent.endObject();
}

static string getTriggerBrief(
  const ServerID serverId, const TriggerID triggerId)
{
	// TODO: use UnifiedDataStore
	DBClientHatohol dbHatohol;
	string triggerBrief;
	TriggerInfo triggerInfo;
	bool succeeded = dbHatohol.getTriggerInfo(triggerInfo,
	                                          serverId, triggerId);
	if (!succeeded) {
		MLPL_WARN("Failed to get TriggerInfo: "
		          "%"PRIu32", %"PRIu64"\n",
		          serverId, triggerId);
	} else {
		triggerBrief = triggerInfo.brief;
	}
	return triggerBrief;
}

static void addTriggersIdBriefHash(
  JsonBuilderAgent &agent, MonitoringServerInfo &serverInfo,
  TriggerBriefMaps &triggerMaps, bool lookupTriggerBrief = false)
{
	TriggerBriefMaps::iterator server_it = triggerMaps.find(serverInfo.id);
	agent.startObject("triggers");
	ServerID serverId = server_it->first;
	TriggerBriefMap &triggers = server_it->second;
	TriggerBriefMap::iterator it = triggers.begin();
	for (; server_it != triggerMaps.end() && it != triggers.end(); it++) {
		TriggerID triggerId = it->first;
		string &triggerBrief = it->second;
		if (lookupTriggerBrief)
			triggerBrief = getTriggerBrief(serverId, triggerId);
		agent.startObject(StringUtils::toString(triggerId));
		agent.add("brief", triggerBrief);
		agent.endObject();
	}
	agent.endObject();
}

static void addServersIdNameHash(
  JsonBuilderAgent &agent,
  HostNameMaps *hostMaps = NULL, bool lookupHostName = false,
  TriggerBriefMaps *triggerMaps = NULL, bool lookupTriggerBrief = false)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);

	agent.startObject("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject(StringUtils::toString(serverInfo.id));
		agent.add("name", serverInfo.hostName);
		if (hostMaps) {
			addHostsIdNameHash(agent, serverInfo,
			                   *hostMaps, lookupHostName);
		}
		if (triggerMaps) {
			addTriggersIdBriefHash(agent, serverInfo, *triggerMaps,
			                       lookupTriggerBrief);
		}
		agent.endObject();
	}
	agent.endObject();
}

void FaceRest::handlerTest
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	if (PrivateContext::testMode)
		agent.addTrue("testMode");
	else
		agent.addFalse("testMode");
	agent.endObject();
	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerLogin
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	gchar *user = (gchar *)g_hash_table_lookup(query, "user");
	if (!user) {
		MLPL_INFO("Not found: user\n");
		replyError(msg, arg, HTERR_AUTH_FAILED);
		return;
	}
	gchar *password = (gchar *)g_hash_table_lookup(query, "password");
	if (!password) {
		MLPL_INFO("Not found: password\n");
		replyError(msg, arg, HTERR_AUTH_FAILED);
		return;
	}

	DBClientUser dbUser;
	UserIdType userId = dbUser.getUserId(user, password);
	if (userId == INVALID_USER_ID) {
		MLPL_INFO("Failed to authenticate: %s.\n", user);
		replyError(msg, arg, HTERR_AUTH_FAILED);
		return;
	}

	uuid_t sessionUuid;
	uuid_generate(sessionUuid);
	static const size_t uuidBufSize = 37;
	char uuidBuf[uuidBufSize];
	uuid_unparse(sessionUuid, uuidBuf);
	string sessionId = uuidBuf;
	PrivateContext::insertSessionId(sessionId, userId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("sessionId", sessionId);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerLogout
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	if (!PrivateContext::removeSessionId(arg->sessionId)) {
		replyError(msg, arg, HTERR_NOT_FOUND_SESSION_ID);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetOverview
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addOverview(agent);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetServer
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	uint32_t targetServerId;
	parseQueryServerId(query, targetServerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addServers(agent, targetServerId);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetHost
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	uint32_t targetServerId;
	parseQueryServerId(query, targetServerId);
	uint64_t targetHostId;
	parseQueryHostId(query, targetHostId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	addHosts(agent, targetServerId, targetHostId);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetTrigger
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{

	uint32_t serverId;
	parseQueryServerId(query, serverId);
	uint64_t hostId;
	parseQueryHostId(query, hostId);
	uint64_t triggerId;
	parseQueryTriggerId(query, triggerId);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList triggerList;
	dataStore->getTriggerList(triggerList, serverId, hostId, triggerId);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfTriggers", triggerList.size());
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	HostNameMaps hostMaps;
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		agent.startObject();
		agent.add("id",       triggerInfo.id);
		agent.add("status",   triggerInfo.status);
		agent.add("severity", triggerInfo.severity);
		agent.add("lastChangeTime", triggerInfo.lastChangeTime.tv_sec);
		agent.add("serverId", triggerInfo.serverId);
		agent.add("hostId",   triggerInfo.hostId);
		agent.add("brief",    triggerInfo.brief);
		agent.endObject();

		hostMaps[triggerInfo.serverId][triggerInfo.hostId]
		  = triggerInfo.hostName;
	}
	agent.endArray();
	addServersIdNameHash(agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetEvent
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	EventInfoList eventList;
	EventQueryOption option;
	option.setUserId(arg->userId);
	dataStore->getEventList(eventList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfEvents", eventList.size());
	agent.startArray("events");
	EventInfoListIterator it = eventList.begin();
	HostNameMaps hostMaps;
	for (; it != eventList.end(); ++it) {
		EventInfo &eventInfo = *it;
		agent.startObject();
		agent.add("serverId",  eventInfo.serverId);
		agent.add("time",      eventInfo.time.tv_sec);
		agent.add("type",      eventInfo.type);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("status",    eventInfo.status);
		agent.add("severity",  eventInfo.severity);
		agent.add("hostId",    eventInfo.hostId);
		agent.add("brief",     eventInfo.brief);
		agent.endObject();

		hostMaps[eventInfo.serverId][eventInfo.hostId]
		  = eventInfo.hostName;
	}
	agent.endArray();
	addServersIdNameHash(agent, &hostMaps);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerGetItem
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ItemInfoList itemList;
	dataStore->getItemList(itemList);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfItems", itemList.size());
	agent.startArray("items");
	ItemInfoListIterator it = itemList.begin();
	for (; it != itemList.end(); ++it) {
		ItemInfo &itemInfo = *it;
		agent.startObject();
		agent.add("serverId",  itemInfo.serverId);
		agent.add("hostId",    itemInfo.hostId);
		agent.add("brief",     itemInfo.brief.c_str());
		agent.add("lastValueTime", itemInfo.lastValueTime.tv_sec);
		agent.add("lastValue", itemInfo.lastValue);
		agent.add("prevValue", itemInfo.prevValue);
		agent.add("itemGroupName", itemInfo.itemGroupName);
		agent.endObject();
	}
	agent.endArray();
	addServersIdNameHash(agent);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

template <typename T>
static void setActionCondition(
  JsonBuilderAgent &agent, const ActionCondition &cond,
  const string &member, ActionConditionEnableFlag bit,
  T value)
{
		if (cond.isEnable(bit))
			agent.add(member, value);
		else
			agent.addNull(member);
}

void FaceRest::handlerAction
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	if (strcasecmp(msg->method, "GET") == 0) {
		handlerGetAction(server, msg, path, query, client, arg);
	} else if (strcasecmp(msg->method, "POST") == 0) {
		handlerPostAction(server, msg, path, query, client, arg);
	} else if (strcasecmp(msg->method, "DELETE") == 0) {
		handlerDeleteAction(server, msg, path, query, client, arg);
	} else {
		MLPL_ERR("Unknown method: %s\n", msg->method);
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void FaceRest::handlerGetAction
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	ActionDefList actionList;
	dataStore->getActionList(actionList);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfActions", actionList.size());
	agent.startArray("actions");
	HostNameMaps hostMaps;
	TriggerBriefMaps triggerMaps;
	ActionDefListIterator it = actionList.begin();
	for (; it != actionList.end(); ++it) {
		const ActionDef &actionDef = *it;
		const ActionCondition &cond = actionDef.condition;
		agent.startObject();
		agent.add("actionId",  actionDef.id);
		agent.add("enableBits", cond.enableBits);
		setActionCondition<uint32_t>(
		  agent, cond, "serverId", ACTCOND_SERVER_ID, cond.serverId);
		setActionCondition<uint64_t>(
		  agent, cond, "hostId", ACTCOND_HOST_ID, cond.hostId);
		setActionCondition<uint64_t>(
		  agent, cond, "hostGroupId", ACTCOND_HOST_GROUP_ID,
		   cond.hostGroupId);
		setActionCondition<uint64_t>(
		  agent, cond, "triggerId", ACTCOND_TRIGGER_ID, cond.triggerId);
		setActionCondition<uint32_t>(
		  agent, cond, "triggerStatus", ACTCOND_TRIGGER_STATUS,
		  cond.triggerStatus);
		setActionCondition<uint32_t>(
		  agent, cond, "triggerSeverity", ACTCOND_TRIGGER_SEVERITY,
		  cond.triggerSeverity);
		agent.add("triggerSeverityComparatorType",
		          cond.triggerSeverityCompType);
		agent.add("type",      actionDef.type);
		agent.add("workingDirectory",
		          actionDef.workingDir.c_str());
		agent.add("command", actionDef.command);
		agent.add("timeout", actionDef.timeout);
		agent.endObject();
		// We don't know the host name at this point.
		// We'll get it later.
		if (cond.isEnable(ACTCOND_HOST_ID))
			hostMaps[cond.serverId][cond.hostId] = "";
		if (cond.isEnable(ACTCOND_TRIGGER_ID))
			triggerMaps[cond.serverId][cond.triggerId] = "";
	}
	agent.endArray();
	const bool lookupHostName = true;
	const bool lookupTriggerBrief = true;
	addServersIdNameHash(agent, &hostMaps, lookupHostName,
	                     &triggerMaps, lookupTriggerBrief);
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerPostAction
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	//
	// mandatory parameters
	//
	char *value;
	bool exist;
	bool succeeded;
	ActionDef actionDef;

	// action type
	succeeded = getParamWithErrorReply<int>(
	              query, msg, arg,
	              "type", "%d", (int &)actionDef.type, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(msg, arg, HTERR_NOT_FOUND_PARAMETER, "type");
		return;
	}
	if (!(actionDef.type == ACTION_COMMAND ||
	      actionDef.type == ACTION_RESIDENT)) {
		REPLY_ERROR(msg, arg, HTERR_INVALID_PARAMETER,
		            "type: %d\n", actionDef.type);
		return;
	}

	// command
	value = (char *)g_hash_table_lookup(query, "command");
	if (!value) {
		replyError(msg, arg, HTERR_NOT_FOUND_PARAMETER, "command");
		return;
	}
	actionDef.command = value;

	//
	// optional parameters
	//
	ActionCondition &cond = actionDef.condition;

	// workingDirectory
	value = (char *)g_hash_table_lookup(query, "workingDirectory");
	if (value) {
		actionDef.workingDir = value;
	}

	// timeout
	succeeded = getParamWithErrorReply<int>(
	              query, msg, arg,
	              "timeout", "%d", actionDef.timeout, &exist);
	if (!succeeded)
		return;
	if (!exist)
		actionDef.timeout = 0;

	// serverId
	succeeded = getParamWithErrorReply<int>(
	              query, msg, arg,
	              "serverId", "%d", cond.serverId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_SERVER_ID);

	// hostId
	succeeded = getParamWithErrorReply<uint64_t>(
	              query, msg, arg,
	              "hostId", "%"PRIu64, cond.hostId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_HOST_ID);

	// hostGroupId
	succeeded = getParamWithErrorReply<uint64_t>(
	              query, msg, arg,
	              "hostGroupId", "%"PRIu64, cond.hostGroupId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_HOST_GROUP_ID);

	// triggerId
	succeeded = getParamWithErrorReply<uint64_t>(
	              query, msg, arg,
	              "triggerId", "%"PRIu64, cond.triggerId, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_TRIGGER_ID);

	// triggerStatus
	succeeded = getParamWithErrorReply<int>(
	              query, msg, arg,
	              "triggerStatus", "%d", cond.triggerStatus, &exist);
	if (!succeeded)
		return;
	if (exist)
		cond.enable(ACTCOND_TRIGGER_STATUS);

	// triggerSeverity
	succeeded = getParamWithErrorReply<int>(
	              query, msg, arg,
	              "triggerSeverity", "%d", cond.triggerSeverity, &exist);
	if (!succeeded)
		return;
	if (exist) {
		cond.enable(ACTCOND_TRIGGER_SEVERITY);

		// triggerSeverityComparatorType
		succeeded = getParamWithErrorReply<int>(
		              query, msg, arg,
		              "triggerSeverityCompType", "%d",
		              (int &)cond.triggerSeverityCompType, &exist);
		if (!succeeded)
			return;
		if (!exist) {
			replyError(msg, arg, HTERR_NOT_FOUND_PARAMETER, 
			           "triggerSeverityCompType");
			return;
		}
		if (!(cond.triggerSeverityCompType == CMP_EQ ||
		      cond.triggerSeverityCompType == CMP_EQ_GT)) {
			REPLY_ERROR(msg, arg, HTERR_INVALID_PARAMETER,
			            "type: %d", cond.triggerSeverityCompType);
			return;
		}
	}

	// save the obtained action
	dataStore->addAction(actionDef);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("id", actionDef.id);
	agent.endObject();
	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerDeleteAction
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	if (arg->id.empty()) {
		replyError(msg, arg, HTERR_NOT_FOUND_ID_IN_URL);
		return;
	}
	int actionId;
	if (sscanf(arg->id.c_str(), "%d", &actionId) != 1) {
		REPLY_ERROR(msg, arg, HTERR_INVALID_PARAMETER,
		            "id: %s", arg->id.c_str());
		return;
	}
	ActionIdList actionIdList;
	actionIdList.push_back(actionId);
	dataStore->deleteActionList(actionIdList);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("id", arg->id);
	agent.endObject();
	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerUser
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	if (strcasecmp(msg->method, "GET") == 0) {
		handlerGetUser(server, msg, path, query, client, arg);
	} else if (strcasecmp(msg->method, "POST") == 0) {
		handlerPostUser(server, msg, path, query, client, arg);
	} else if (strcasecmp(msg->method, "DELETE") == 0) {
		handlerDeleteUser(server, msg, path, query, client, arg);
	} else {
		MLPL_ERR("Unknown method: %s\n", msg->method);
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void FaceRest::handlerGetUser
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();

	UserInfoList userList;
	DataQueryOption option;
	option.setUserId(arg->userId);
	dataStore->getUserList(userList, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.add("numberOfUsers", userList.size());
	agent.startArray("users");
	UserInfoListIterator it = userList.begin();
	for (; it != userList.end(); ++it) {
		const UserInfo &userInfo = *it;
		agent.startObject();
		agent.add("userId",  userInfo.id);
		agent.add("name", userInfo.name);
		agent.add("flags", userInfo.flags);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerPostUser
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	// Get query parameters
	char *value;
	bool exist;
	bool succeeded;
	UserInfo userInfo;

	// name
	value = (char *)g_hash_table_lookup(query, "user");
	if (!value) {
		REPLY_ERROR(msg, arg, HTERR_NOT_FOUND_PARAMETER, "user");
		return;
	}
	userInfo.name = value;

	// password
	value = (char *)g_hash_table_lookup(query, "password");
	if (!value) {
		REPLY_ERROR(msg, arg, HTERR_NOT_FOUND_PARAMETER, "password");
		return;
	}
	userInfo.password = value;

	// flags
	succeeded = getParamWithErrorReply<OperationPrivilegeFlag>(
	              query, msg, arg,
	              "flags", "%"FMT_OPPRVLG, userInfo.flags, &exist);
	if (!succeeded)
		return;
	if (!exist) {
		REPLY_ERROR(msg, arg, HTERR_NOT_FOUND_PARAMETER, "flags");
		return;
	}

	// try to add
	DataQueryOption option;
	option.setUserId(arg->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->addUser(userInfo, option);

	// make a response
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userInfo.id);
	agent.endObject();
	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

void FaceRest::handlerDeleteUser
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	if (arg->id.empty()) {
		replyError(msg, arg, HTERR_NOT_FOUND_ID_IN_URL);
		return;
	}
	int userId;
	if (sscanf(arg->id.c_str(), "%"FMT_USER_ID, &userId) != 1) {
		REPLY_ERROR(msg, arg, HTERR_INVALID_PARAMETER,
		            "id: %s", arg->id.c_str());
		return;
	}

	DataQueryOption option;
	option.setUserId(arg->userId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->deleteUser(userId, option);

	// replay
	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	if (err == HTERR_OK)
		agent.add("id", userId);
	agent.endObject();
	replyJsonData(agent, msg, arg->jsonpCallbackName, arg);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
template<typename T>
bool FaceRest::getParamWithErrorReply(
  GHashTable *query, SoupMessage *msg, const HandlerArg *arg,
  const char *paramName, const char *scanFmt, T &dest, bool *exist)
{
	char *value = (char *)g_hash_table_lookup(query, paramName);
	if (exist)
		*exist = value;
	if (!value)
		return true;

	if (sscanf(value, scanFmt, &dest) != 1) {
		REPLY_ERROR(msg, arg, HTERR_INVALID_PARAMETER,
		            "%s: %s\n", paramName, value);
		return false;
	}
	return true;
}

