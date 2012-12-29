#include <Logger.h>
using namespace mlpl;

#include <libsoup/soup.h>
#include "ArmZabbixAPI.h"

static const int DEFAULT_SERVER_PORT = 80;
static const int DEFAULT_RETRY_INTERVAL = 10;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(CommandLineArg &cmdArg)
: m_server_port(DEFAULT_SERVER_PORT),
  m_retry_interval(DEFAULT_RETRY_INTERVAL)
{
	m_server = "localhost";
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ArmZabbixAPI::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: %s\n", __PRETTY_FUNCTION__);
	
	SoupSession *session = soup_session_sync_new();
	string uri = "http://localhost/zabbix/api_jsonrpc.php";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      "application/json-rpc", NULL);
	string request_body = "{\"auth\":null,\"method\":\"user.login\",\"id\":1,\"params\":{\"user\":\"admin\",\"password\":\"zabbix\"},\"jsonrpc\":\"2.0\"}";
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	

	guint ret = soup_session_send_message(session, msg);
	if (ret != 200) {
		MLPL_ERR("Failed to get: code: %d: %s\n", ret, uri.c_str());
		sleep(m_retry_interval);
		return NULL;
	}
	MLPL_INFO("body: %d, %s\n", msg->response_body->length,
	                            msg->response_body->data);

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
