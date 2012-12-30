#include <Logger.h>
using namespace mlpl;

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ArmZabbixAPI.h"
#include "JsonParserAgent.h"

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
	/*
	{
	  "auth":null,
	  "method":"user.login",
	  "id":1,
	  "params":{
	    "user":"admin",
	    "password":"zabbix"
	  },
	  "jsonrpc":"2.0"
	}
	*/
	JsonBuilder *builder = json_builder_new();
	json_builder_begin_object(builder);

	json_builder_set_member_name(builder, "auth");
	json_builder_add_null_value(builder);

	json_builder_set_member_name(builder, "method");
	json_builder_add_string_value(builder, "user.login");

	json_builder_set_member_name(builder, "id");
	json_builder_add_int_value(builder, 1);

        // "params"
	json_builder_set_member_name(builder, "params");
	json_builder_begin_object(builder);

	json_builder_set_member_name(builder, "user");
	json_builder_add_string_value(builder, "admin");

	json_builder_set_member_name(builder, "password");
	json_builder_add_string_value(builder, "zabbix");

	json_builder_end_object (builder);
	// End of "params"

	json_builder_set_member_name(builder, "jsonrpc");
	json_builder_add_string_value(builder, "2.0");

	json_builder_end_object (builder);

	JsonGenerator *generator = json_generator_new();
	JsonNode *root = json_builder_get_root(builder);
	json_generator_set_root(generator, root);
	gchar *str = json_generator_to_data(generator, NULL);
	string json_str = str;
	g_free(str);

	json_node_free(root);
	g_object_unref(generator);
	g_object_unref(builder);

	return json_str;
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
