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

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(CommandLineArg &cmdArg)
: m_server_port(DEFAULT_SERVER_PORT),
  m_retry_interval(DEFAULT_RETRY_INTERVAL),
  m_repeat_interval(DEFAULT_REPEAT_INTERVAL)
{
	m_server = "localhost";
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

bool ArmZabbixAPI::mainThreadOneProc(void)
{
	SoupSession *session = soup_session_sync_new();
	string uri = "http://localhost/zabbix/api_jsonrpc.php";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      "application/json-rpc", NULL);
	string request_body = getInitialJsonRequest();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	

	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to get: code: %d: %s\n", ret, uri.c_str());
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
	MLPL_INFO("started: %s\n", __PRETTY_FUNCTION__);
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
