/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstdio>
#include <string>
#include <Reaper.h>
#include "JsonParserAgent.h"
#include "ZabbixAPI.h"
#include "StringUtils.h"

using namespace std;
using namespace mlpl;

static const char *MIME_JSON_RPC = "application/json-rpc";
static const guint DEFAULT_TIMEOUT = 60;

struct ZabbixAPI::PrivateContext {

	string         uri;
	string         username;
	string         password;
	string         apiVersion;
	int            apiVersionMajor;
	int            apiVersionMinor;
	int            apiVersionMicro;
	SoupSession   *session;
	string         authToken;

	// constructors and destructor
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: apiVersionMajor(0),
	  apiVersionMinor(0),
	  apiVersionMicro(0),
	  session(NULL)
	{
		const bool forURI = true;
		uri = "http://";
		uri += serverInfo.getHostAddress(forURI);
		uri += StringUtils::sprintf(":%d", serverInfo.port);
		uri += "/zabbix/api_jsonrpc.php";

		username = serverInfo.userName.c_str();
		password = serverInfo.password.c_str();
	}

	virtual ~PrivateContext()
	{
		if (session)
			g_object_unref(session);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPI::ZabbixAPI(const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

ZabbixAPI::~ZabbixAPI()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ZabbixAPI::onUpdatedAuthToken(const string &authToken)
{
}

const string &ZabbixAPI::getAPIVersion(void)
{
	if (!m_ctx->apiVersion.empty())
		return m_ctx->apiVersion;

	SoupMessage *msg = queryAPIVersion();
	if (!msg)
		return m_ctx->apiVersion;
	Reaper<void> msgReaper(msg, g_object_unref);

	JsonParserAgent parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser: %s\n", parser.getErrorMessage());
		return m_ctx->apiVersion;
	}

	if (parser.read("result", m_ctx->apiVersion)) {
		MLPL_DBG("Zabbix API version: %s\n",
		         m_ctx->apiVersion.c_str());
	} else {
		MLPL_ERR("Failed to read API version\n");
	}

	if (!m_ctx->apiVersion.empty()) {
		StringList list;
		StringUtils::split(list, m_ctx->apiVersion, '.');
		StringListIterator it = list.begin();
		for (size_t i = 0; it != list.end(); ++i, ++it) {
			const string &str = *it;
			if (i == 0)
				m_ctx->apiVersionMajor = atoi(str.c_str());
			else if (i == 1)
				m_ctx->apiVersionMinor = atoi(str.c_str());
			else if (i == 2)
				m_ctx->apiVersionMicro = atoi(str.c_str());
			else
				break;
		}
	}
	return m_ctx->apiVersion;
}

bool ZabbixAPI::checkAPIVersion(int major, int minor, int micro)
{
	getAPIVersion();

	if (m_ctx->apiVersionMajor > major)
		return true;
	if (m_ctx->apiVersionMajor == major &&
	    m_ctx->apiVersionMinor >  minor)
		return true;
	if (m_ctx->apiVersionMajor == major &&
	    m_ctx->apiVersionMinor == minor &&
	    m_ctx->apiVersionMicro >= micro)
		return true;
	return false;
}

bool ZabbixAPI::openSession(SoupMessage **msgPtr)
{
	const string &version = getAPIVersion();
	if (version.empty())
		return false;

	SoupMessage *msg =
	  soup_message_new(SOUP_METHOD_POST, m_ctx->uri.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	string request_body = getInitialJsonRequest();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		MLPL_ERR("Failed to get: code: %d: %s\n",
	                 ret, m_ctx->uri.c_str());
		return false;
	}
	MLPL_DBG("body: %"G_GOFFSET_FORMAT", %s\n",
	         msg->response_body->length, msg->response_body->data);
	bool succeeded = parseInitialResponse(msg);
	if (!succeeded) {
		g_object_unref(msg);
		return false;
	}
	MLPL_DBG("authToken: %s\n", m_ctx->authToken.c_str());

	// copy the SoupMessage object if msgPtr is not NULL.
	if (msgPtr)
		*msgPtr = msg;
	else
		g_object_unref(msg);

	return true;
}


SoupSession *ZabbixAPI::getSession(void)
{
	// NOTE: current implementaion is not MT-safe.
	//       If we have to use this function from multiple threads,
	//       it is only necessary to prepare sessions by thread.
	if (!m_ctx->session)
		m_ctx->session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT,      DEFAULT_TIMEOUT,
			//FIXME: Sometimes it causes crash (issue #98)
			//SOUP_SESSION_IDLE_TIMEOUT, DEFAULT_IDLE_TIMEOUT,
			NULL);
	return m_ctx->session;
}

bool ZabbixAPI::updateAuthTokenIfNeeded(void)
{
	if (m_ctx->authToken.empty()) {
		MLPL_DBG("authToken is empty\n");
		if (!openSession())
			return false;
	}
	MLPL_DBG("authToken: %s\n", m_ctx->authToken.c_str());

	return true;
}

string ZabbixAPI::getAuthToken(void)
{
	// This function is used in the test class.
	updateAuthTokenIfNeeded();
	return m_ctx->authToken;
}

void ZabbixAPI::clearAuthToken(void)
{
	m_ctx->authToken.clear();
	onUpdatedAuthToken(m_ctx->authToken);
}

SoupMessage *ZabbixAPI::queryCommon(JsonBuilderAgent &agent)
{
	string request_body = agent.generate();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, m_ctx->uri.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		MLPL_ERR("Failed to get: code: %d: %s\n",
	                 ret, m_ctx->uri.c_str());
		return NULL;
	}
	return msg;
}

SoupMessage *ZabbixAPI::queryAPIVersion(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "apiinfo.version");
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

string ZabbixAPI::getInitialJsonRequest(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.addNull("auth");
	agent.add("method", "user.login");
	agent.add("id", 1);

	agent.startObject("params");
	agent.add("user", m_ctx->username);
	agent.add("password", m_ctx->password);
	agent.endObject();

	agent.add("jsonrpc", "2.0");
	agent.endObject();

	return agent.generate();
}

bool ZabbixAPI::parseInitialResponse(SoupMessage *msg)
{
	JsonParserAgent parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser: %s\n", parser.getErrorMessage());
		return false;
	}

	if (!parser.read("result", m_ctx->authToken)) {
		MLPL_ERR("Failed to read: result\n");
		return false;
	}
	onUpdatedAuthToken(m_ctx->authToken);
	return true;
}

