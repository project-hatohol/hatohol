#include <Logger.h>
using namespace mlpl;

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
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
	gboolean ret;
	GError *error = NULL;
	JsonParser *parser = json_parser_new();
	ret = json_parser_load_from_data(parser, msg->response_body->data,
	                                 -1, &error);
	if (!ret) {
		MLPL_ERR("Failed to parse:  %s\n", error->message);
		g_error_free(error);
		g_object_unref(parser);
		return false;
	}

	JsonNode *root = json_parser_get_root(parser);
	JsonReader *reader = json_reader_new(json_parser_get_root(parser));

	if (!json_reader_read_member(reader, "result")) {
		MLPL_ERR("Failed to access\n");
	} else {
		m_auth_token = json_reader_get_string_value(reader);
		MLPL_DBG("Auth token: %s\n", m_auth_token.c_str());
	}
	json_reader_end_member(reader);

	g_object_unref (reader);
	g_object_unref(parser);
	return true;
}

gpointer ArmZabbixAPI::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: %s\n", __PRETTY_FUNCTION__);
	
	SoupSession *session = soup_session_sync_new();
	string uri = "http://localhost/zabbix/api_jsonrpc.php";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      "application/json-rpc", NULL);
	string request_body = getInitialJsonRequest();
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
	parseInitialResponse(msg);
	g_object_unref(msg);
	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
