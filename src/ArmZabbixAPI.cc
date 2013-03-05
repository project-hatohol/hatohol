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

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ArmZabbixAPI.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"

static const int DEFAULT_SERVER_PORT = 80;
static const int DEFAULT_RETRY_INTERVAL = 10;
static const int DEFAULT_REPEAT_INTERVAL = 30;

static const char *MIME_JSON_RPC = "application/json-rpc";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(const char *server)
: m_server(server),
  m_server_port(DEFAULT_SERVER_PORT),
  m_retry_interval(DEFAULT_RETRY_INTERVAL),
  m_repeat_interval(DEFAULT_REPEAT_INTERVAL)
{
	m_server = "localhost";
	m_uri = "http://";
	m_uri += m_server;
	m_uri += "/zabbix/api_jsonrpc.php";
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string ArmZabbixAPI::getInitialJsonRequest(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.addNull("auth");
	agent.add("method", "user.login");
	agent.add("id", 1);

	agent.startObject("params");
	agent.add("user" , "admin");
	agent.add("password", "zabbix");
	agent.endObject();

	agent.add("jsonrpc", "2.0");
	agent.endObject();

	return agent.generate();
}

bool ArmZabbixAPI::parseInitialResponse(SoupMessage *msg)
{
	JsonParserAgent parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser: %s\n", parser.getErrorMessage());
		return false;
	}

	if (!parser.read("result", m_auth_token)) {
		MLPL_ERR("Failed to read: result\n");
		return false;
	}
	return true;
}

void ArmZabbixAPI::getTrigger(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "trigger.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("selectFunctions", "extend");
	agent.endObject();

	agent.add("auth", m_auth_token);
	agent.add("id", 1);
	agent.endObject();

	string request_body = agent.generate();
	SoupSession *session = soup_session_sync_new();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to get: code: %d: %s\n", ret, m_uri.c_str());
		return;
	}
	MLPL_DBG("body: %d, %s\n", msg->response_body->length,
	                           msg->response_body->data);
}

bool ArmZabbixAPI::mainThreadOneProc(void)
{
	SoupSession *session = soup_session_sync_new();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	string request_body = getInitialJsonRequest();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	

	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to get: code: %d: %s\n", ret, m_uri.c_str());
		return false;
	}
	MLPL_DBG("body: %d, %s\n", msg->response_body->length,
	                           msg->response_body->data);
	if (!parseInitialResponse(msg))
		return false;
	MLPL_DBG("auth token: %s\n", m_auth_token.c_str());

	g_object_unref(msg);
	return true;
}

gpointer ArmZabbixAPI::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: ArmZabbixAPI (server: %s)\n",
	          __PRETTY_FUNCTION__, m_server.c_str());
	while (true) {
		int sleepTime = m_repeat_interval;
		if (!mainThreadOneProc())
			sleepTime = m_retry_interval;
		sleep(sleepTime);
	}
	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
