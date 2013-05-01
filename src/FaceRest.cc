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

static const char *MIME_JSON = "application/json";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceRest::FaceRest(CommandLineArg &cmdArg)
: m_port(DEFAULT_PORT),
  m_soupServer(NULL)
{
	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--face-rest-port")
			i = parseCmdArgPort(cmdArg, i);
	}
	MLPL_INFO("started face-rest, port: %d\n", m_port);
	m_stopMutex = new MutexLock();
}

FaceRest::~FaceRest()
{
	if (m_stopMutex)
		delete m_stopMutex;
	if (m_soupServer) {
		SoupSocket *sock = soup_server_get_listener(m_soupServer);
		soup_socket_disconnect(sock);
		g_object_unref(m_soupServer);
	}
}

void FaceRest::stop(void)
{
	ASURA_ASSERT(m_soupServer, "m_soupServer: NULL");

	// We make and use a copy of m_stopMutex. This object will be
	// destroyed soon after calling soup_server_quit() when auto
	// delete flag is set. So the this->m_stopMutex will be invalid.
	MutexLock *stopMutex = m_stopMutex;

	soup_server_quit(m_soupServer);

	// wait for the return from soup_server_run() in the
	// main thread. This synchronization ensures that the delete of
	// this instance after stop() is safe.
	stopMutex->lock();
	stopMutex->unlock();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceRest::mainThread(AsuraThreadArg *arg)
{
	m_soupServer = soup_server_new(SOUP_SERVER_PORT, m_port, NULL);
	ASURA_ASSERT(m_soupServer, "failed: soup_server_new: %u\n", m_port);
	soup_server_add_handler(m_soupServer, NULL, handlerDefault, this, NULL);
	soup_server_add_handler(m_soupServer, pathForGetServers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetServers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetTriggers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetTriggers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetEvents,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetEvents, NULL);
	m_stopMutex->lock();
	soup_server_run(m_soupServer);
	m_stopMutex->unlock();
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
	if (port < 0 || port > 65536) {
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
	try {
		(*handler)(server, msg, path, query, client, user_data);
	} catch (const AsuraException &e) {
		MLPL_INFO("Got Exception: %s\n", e.getFancyMessage().c_str());
		replyError(msg, e.getFancyMessage());
	}
}

void FaceRest::handlerGetServers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	ConfigManager *configManager = ConfigManager::getInstance();

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
	soup_message_headers_set_content_type(msg->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetTriggers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	VirtualDataStoreZabbix *vdsz = VirtualDataStoreZabbix::getInstance();

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
	soup_message_headers_set_content_type(msg->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetEvents
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	VirtualDataStoreZabbix *vdsz = VirtualDataStoreZabbix::getInstance();

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
		agent.startObject();
		agent.add("serverId", eventInfo.serverId);
		agent.add("time",   eventInfo.time.tv_sec);
		agent.add("eventValue", eventInfo.eventValue);
		agent.add("triggerId", eventInfo.triggerId);
		agent.add("status",   eventInfo.triggerInfo.status);
		agent.add("severity", eventInfo.triggerInfo.severity);
		agent.add("hostId",   eventInfo.triggerInfo.hostId);
		agent.add("hostName", eventInfo.triggerInfo.hostName);
		agent.add("brief",    eventInfo.triggerInfo.brief);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();
	string response = agent.generate();
	soup_message_headers_set_content_type(msg->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}
