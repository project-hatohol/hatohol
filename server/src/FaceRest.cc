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

#include <cstring>
#include <Logger.h>
using namespace mlpl;

#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "ConfigManager.h"
#include "UnifiedDataStore.h"

int FaceRest::API_VERSION_SERVERS  = 1;
int FaceRest::API_VERSION_TRIGGERS = 1;
int FaceRest::API_VERSION_EVENTS   = 1;
int FaceRest::API_VERSION_ITEMS    = 1;
int FaceRest::API_VERSION_ACTIONS  = 1;

typedef void (*RestHandler)
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data);

typedef uint64_t ServerID;
typedef uint64_t HostID;
typedef map<HostID, string> HostNameMap;
typedef map<ServerID, HostNameMap> HostNameMaps;

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForGetOverview = "/overview";
const char *FaceRest::pathForGetServers = "/servers";
const char *FaceRest::pathForGetTriggers = "/triggers";
const char *FaceRest::pathForGetEvents   = "/events";
const char *FaceRest::pathForGetItems    = "/items";
const char *FaceRest::pathForGetActions  = "/actions";

static const char *MIME_HTML = "text/html";
static const char *MIME_JSON = "application/json";
static const char *MIME_JAVASCRIPT = "text/javascript";

enum FormatType {
	FORMAT_HTML,
	FORMAT_JSON,
	FORMAT_JSONP,
};

struct FaceRest::HandlerArg
{
	FormatType formatType;
	const char *mimeType;
};

typedef map<string, FormatType> FormatTypeMap;
typedef FormatTypeMap::iterator FormatTypeMapIterator;;
static FormatTypeMap g_formatTypeMap;

typedef map<FormatType, const char *> MimeTypeMap;
typedef MimeTypeMap::iterator   MimeTypeMapIterator;;
static MimeTypeMap g_mimeTypeMap;

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
	soup_server_add_handler(m_soupServer, pathForGetOverview,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetOverview, NULL);
	soup_server_add_handler(m_soupServer, pathForGetServers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetServers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetTriggers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetTriggers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetEvents,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetEvents, NULL);
	soup_server_add_handler(m_soupServer, pathForGetItems,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetItems, NULL);
	soup_server_add_handler(m_soupServer, pathForGetActions,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerActions, NULL);
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

void FaceRest::replyError(SoupMessage *msg, const string &errorMessage)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.addFalse("result");
	agent.add("message", errorMessage.c_str());
	agent.endObject();
	string response = agent.generate();
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

// handlers
void FaceRest::handlerDefault(SoupServer *server, SoupMessage *msg,
                              const char *path, GHashTable *query,
                              SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

void FaceRest::launchHandlerInTryBlock
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	RestHandler handler = reinterpret_cast<RestHandler>(user_data);

	HandlerArg arg;

	// format
	string extension = Utils::getExtension(path);
	FormatTypeMapIterator fmtIt = g_formatTypeMap.find(extension);
	if (fmtIt == g_formatTypeMap.end()) {
		string errMsg = StringUtils::sprintf(
		  "Unsupported extension: %s, path: %s\n",
		  extension.c_str(), path);
		replyError(msg, errMsg);
		return;
	}
	arg.formatType = fmtIt->second;

	// MIME
	MimeTypeMapIterator mimeIt = g_mimeTypeMap.find(arg.formatType);
	HATOHOL_ASSERT(
	  mimeIt != g_mimeTypeMap.end(),
	  "Invalid formatType: %d, %s", arg.formatType, extension.c_str());
	arg.mimeType = mimeIt->second;

	try {
		(*handler)(server, msg, path, query, client, &arg);
	} catch (const HatoholException &e) {
		MLPL_INFO("Got Exception: %s\n", e.getFancyMessage().c_str());
		replyError(msg, e.getFancyMessage());
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

static void addServers(JsonBuilderAgent &agent)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);

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

static void addHostsIdNameHash(JsonBuilderAgent &agent,
			       MonitoringServerInfo &serverInfo,
			       HostNameMaps &hostMaps)
{
	HostNameMaps::iterator server_it = hostMaps.find(serverInfo.id);
	agent.startObject("hosts");
	HostNameMap &hosts = server_it->second;
	HostNameMap::iterator it = hosts.begin();
	for (; server_it != hostMaps.end() && it != hosts.end(); it++) {
		agent.startObject(StringUtils::toString(it->first));
		agent.add("name", it->second);
		agent.endObject();
	}
	agent.endObject();
}

static void addServersIdNameHash(JsonBuilderAgent &agent,
                                 HostNameMaps *hostMaps = NULL)
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
		if (hostMaps)
			addHostsIdNameHash(agent, serverInfo, *hostMaps);
		agent.endObject();
	}
	agent.endObject();
}

void FaceRest::handlerGetOverview
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_SERVERS);
	agent.addTrue("result");
	addOverview(agent);
	agent.endObject();

	replyJsonData(agent, msg, jsonpCallbackName, arg);
}

void FaceRest::handlerGetServers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_SERVERS);
	agent.addTrue("result");
	addServers(agent);
	agent.endObject();

	replyJsonData(agent, msg, jsonpCallbackName, arg);
}

void FaceRest::handlerGetTriggers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	TriggerInfoList triggerList;
	dataStore->getTriggerList(triggerList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_TRIGGERS);
	agent.addTrue("result");
	agent.add("numberOfTriggers", triggerList.size());
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	HostNameMaps hostMaps;
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		agent.startObject();
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

	replyJsonData(agent, msg, jsonpCallbackName, arg);
}

void FaceRest::handlerGetEvents
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	EventInfoList eventList;
	dataStore->getEventList(eventList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_EVENTS);
	agent.addTrue("result");
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

	replyJsonData(agent, msg, jsonpCallbackName, arg);
}

void FaceRest::handlerGetItems
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	ItemInfoList itemList;
	dataStore->getItemList(itemList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_ITEMS);
	agent.addTrue("result");
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

	replyJsonData(agent, msg, jsonpCallbackName, arg);
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

void FaceRest::handlerActions
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	if (strcasecmp(msg->method, "GET") == 0) {
		handlerGetActions(server, msg, path, query, client, arg);
	} else if (strcasecmp(msg->method, "POST") == 0) {
		handlerPostAction(server, msg, path, query, client, arg);
	} else {
		MLPL_ERR("Unknown method: %s\n", msg->method);
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void FaceRest::handlerGetActions
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	ActionDefList actionList;
	dataStore->getActionList(actionList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_ACTIONS);
	agent.addTrue("result");
	agent.add("numberOfActions", actionList.size());
	agent.startArray("actions");
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
	}
	agent.endArray();
	addServersIdNameHash(agent);
	agent.endObject();

	replyJsonData(agent, msg, jsonpCallbackName, arg);
}

void FaceRest::handlerPostAction
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	ActionDef actionDef;
	// mandatory parameters
	string actionType;
	gpointer value = g_hash_table_lookup(query, "type");
	if (!value) {
		MLPL_ERR("actionType is not specified.\n");
		soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
		return;
	}
	string actionTypeStr((const char *)value);
	if (actionTypeStr == "command") {
		actionDef.type = ACTION_COMMAND;
	} else if (actionTypeStr == "resident") {
		actionDef.type = ACTION_RESIDENT;
	} else {
		MLPL_ERR("Unknown actionType: %s\n", actionTypeStr.c_str());
		soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
	}

	// optional parameters
	soup_message_set_status(msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);

	// make a response

	// make a response
	MLPL_BUG("Not implemented: %s, %p\n", __PRETTY_FUNCTION__, dataStore);
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("apiVersion", API_VERSION_ACTIONS);
	agent.addTrue("result");
	// TODO: add action ID.
	agent.endObject();
	replyJsonData(agent, msg, jsonpCallbackName, arg);
}
