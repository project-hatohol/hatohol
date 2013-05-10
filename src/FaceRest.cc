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

#include <Logger.h>
using namespace mlpl;

#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "AsuraException.h"
#include "ConfigManager.h"
#include "VirtualDataStoreZabbix.h"

typedef void (*RestHandler)
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data);

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForGetServers = "/servers";
const char *FaceRest::pathForGetTriggers = "/triggers";
const char *FaceRest::pathForGetEvents   = "/events";

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
	ASURA_ASSERT(m_soupServer, "m_soupServer: NULL");
	soup_server_quit(m_soupServer);

	AsuraThreadBase::stop();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceRest::mainThread(AsuraThreadArg *arg)
{
	m_soupServer = soup_server_new(SOUP_SERVER_PORT, m_port, NULL);
	ASURA_ASSERT(m_soupServer, "failed: soup_server_new: %u\n", m_port);
	soup_server_add_handler(m_soupServer, NULL, handlerDefault, this, NULL);
	soup_server_add_handler(m_soupServer, "/hello.html",
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerHelloPage, NULL);
	soup_server_add_handler(m_soupServer, pathForGetServers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetServers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetTriggers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetTriggers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetEvents,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetEvents, NULL);
	soup_server_run(m_soupServer);
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
		throw AsuraException("Not found parameter: callback");

	const char *callbackName = (const char *)value;
	string errMsg;
	if (!Utils::validateJSMethodName(callbackName, errMsg)) {
		THROW_ASURA_EXCEPTION(
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
	ASURA_ASSERT(
	  mimeIt != g_mimeTypeMap.end(),
	  "Invalid formatType: %d, %s", arg.formatType, extension.c_str());
	arg.mimeType = mimeIt->second;

	try {
		(*handler)(server, msg, path, query, client, &arg);
	} catch (const AsuraException &e) {
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
	  "ASURA ver. %s"
	  "</html>";
	response = StringUtils::sprintf(pageTemplate, PACKAGE_VERSION);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetServers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	ConfigManager *configManager = ConfigManager::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.addTrue("result");
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
	agent.endObject();
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(msg->response_headers,
	                                      arg->mimeType, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetTriggers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	VirtualDataStoreZabbix *vdsz = VirtualDataStoreZabbix::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	TriggerInfoList triggerList;
	vdsz->getTriggerList(triggerList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.addTrue("result");
	agent.add("numberOfTriggers", triggerList.size());
	agent.startArray("triggers");
	TriggerInfoListIterator it = triggerList.begin();
	for (; it != triggerList.end(); ++it) {
		TriggerInfo &triggerInfo = *it;
		agent.startObject();
		agent.add("status",   triggerInfo.status);
		agent.add("severity", triggerInfo.severity);
		agent.add("lastChangeTime", triggerInfo.lastChangeTime.tv_sec);
		agent.add("serverId", triggerInfo.serverId);
		agent.add("hostId",   triggerInfo.hostId);
		agent.add("hostName", triggerInfo.hostName);
		agent.add("brief",    triggerInfo.brief);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(msg->response_headers,
	                                      arg->mimeType, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetEvents
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, HandlerArg *arg)
{
	VirtualDataStoreZabbix *vdsz = VirtualDataStoreZabbix::getInstance();
	string jsonpCallbackName = getJsonpCallbackName(query, arg);

	EventInfoList eventList;
	vdsz->getEventList(eventList);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.addTrue("result");
	agent.add("numberOfEvents", eventList.size());
	agent.startArray("events");
	EventInfoListIterator it = eventList.begin();
	for (; it != eventList.end(); ++it) {
		EventInfo &eventInfo = *it;
		TriggerInfo &triggerInfo = eventInfo.triggerInfo;
		agent.startObject();
		agent.add("serverId", eventInfo.serverId);
		agent.add("time",   eventInfo.time.tv_sec);
		agent.add("eventValue", eventInfo.eventValue);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("status",         triggerInfo.status);
		agent.add("severity",       triggerInfo.severity);
		agent.add("lastChangeTime", triggerInfo.lastChangeTime.tv_sec);
		agent.add("hostId",         triggerInfo.hostId);
		agent.add("hostName",       triggerInfo.hostName);
		agent.add("brief",          triggerInfo.brief);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();
	string response = agent.generate();
	if (!jsonpCallbackName.empty())
		response = wrapForJsonp(response, jsonpCallbackName);
	soup_message_headers_set_content_type(msg->response_headers,
	                                      arg->mimeType, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}
