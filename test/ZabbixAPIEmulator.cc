#include <map>
#include <set>
#include <cstdio>
#include <Logger.h>

#include "ZabbixAPIEmulator.h"
#include "JsonParserAgent.h"
#include "AsuraException.h"
#include "Helpers.h"

using namespace mlpl;

struct ZabbixAPIEmulator::APIHandlerArg
{
	SoupServer        *server;
	SoupMessage       *msg;
	const char        *path;
	GHashTable        *query;
	SoupClientContext *client;
};

typedef map<string, ZabbixAPIEmulator::APIHandler> APIHandlerMap;
typedef APIHandlerMap::iterator APIHandlerMapIterator;

struct ZabbixAPIEmulator::PrivateContext {
	GThread    *thread;
	guint       port;
	SoupServer *soupServer;
	OperationMode operationMode;
	set<string>   authTokens;
	APIHandlerMap apiHandlerMap;
	
	// methods
	PrivateContext(void)
	: thread(NULL),
	  port(0),
	  soupServer(NULL),
	  operationMode(OPE_MODE_NORMAL)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPIEmulator::ZabbixAPIEmulator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->apiHandlerMap["user.login"] = 
	  &ZabbixAPIEmulator::APIHandlerUserLogin;
}

ZabbixAPIEmulator::~ZabbixAPIEmulator()
{
	if (isRunning()) {
		soup_server_disconnect(m_ctx->soupServer);
		g_object_unref(m_ctx->soupServer);
		m_ctx->soupServer = NULL;
	}
	if (m_ctx)
		delete m_ctx;
}

bool ZabbixAPIEmulator::isRunning(void)
{
	return m_ctx->thread;
}

void ZabbixAPIEmulator::start(guint port)
{
	if (isRunning()) {
		MLPL_WARN("Thread is already running.");
		return;
	}
	
	m_ctx->soupServer = soup_server_new(SOUP_SERVER_PORT, port, NULL);
	soup_server_add_handler(m_ctx->soupServer, NULL, handlerDefault,
	                        this, NULL);
	soup_server_add_handler(m_ctx->soupServer, "/zabbix/api_jsonrpc.php",
	                        handlerAPI, this, NULL);
	m_ctx->thread = g_thread_new("ZabbixAPIEmulator", _mainThread, this);
}

void ZabbixAPIEmulator::setOperationMode(OperationMode mode)
{
	m_ctx->operationMode = mode;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ZabbixAPIEmulator::_mainThread(gpointer data)
{
	ZabbixAPIEmulator *obj = static_cast<ZabbixAPIEmulator *>(data);
	return obj->mainThread();
}

gpointer ZabbixAPIEmulator::mainThread(void)
{
	soup_server_run(m_ctx->soupServer);
	return NULL;
}

void ZabbixAPIEmulator::startObject(JsonParserAgent &parser,
                                    const string &name)
{
	if (!parser.startObject(name)) {
		THROW_ASURA_EXCEPTION(
		  "Failed to read object: %s", name.c_str());
	}
}

void ZabbixAPIEmulator::handlerDefault
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

void ZabbixAPIEmulator::handlerAPI
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	APIHandlerArg arg;
	arg.server = server;
	arg.msg    = msg;
	arg.path   = path,
	arg.query  = query;
	arg.client = client;
	ZabbixAPIEmulator *obj = static_cast<ZabbixAPIEmulator *>(user_data);
	try {
		obj->handlerAPIDispatch(arg);
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s\n", e.what());
		soup_message_set_status(msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
	}
}

void ZabbixAPIEmulator::handlerAPIDispatch(APIHandlerArg &arg)
{
	JsonParserAgent parser(arg.msg->request_body->data);
	if (parser.hasError()) {
		THROW_ASURA_EXCEPTION("Error in parsing: %s",
		                      parser.getErrorMessage());
	}

	// jsonrpc
	string rpcVersion;
	if (!parser.read("jsonrpc", rpcVersion))
		THROW_ASURA_EXCEPTION("Not found: jsonrpc");
	if (rpcVersion != "2.0") {
		THROW_ASURA_EXCEPTION("Invalid parameter: jsonrpc: %s",
	                              rpcVersion.c_str());
	}

	// id
	int64_t id;
	if (!parser.read("id", id))
		THROW_ASURA_EXCEPTION("Not found: id");

	// method
	string method;
	if (!parser.read("method", method))
		THROW_ASURA_EXCEPTION("Not found: method");

	// auth
	bool isNull;
	if (!parser.isNull("auth", isNull))
		THROW_ASURA_EXCEPTION("Not found: auth");
	if (isNull) {
		if (method != "user.login")
			THROW_ASURA_EXCEPTION("auth: empty");
	} else {
		string auth;
		if (!parser.read("auth", auth))
			THROW_ASURA_EXCEPTION("Not found: auth");
		set<string>::iterator it = m_ctx->authTokens.find(auth);
		if (it == m_ctx->authTokens.end()) {
			THROW_ASURA_EXCEPTION("Not found: auth token: %s",
			                      auth.c_str());
		}
	}

	// dispatch
	APIHandlerMapIterator it = m_ctx->apiHandlerMap.find(method);
	if (it == m_ctx->apiHandlerMap.end())
		THROW_ASURA_EXCEPTION("Unknown method: %s", method.c_str());
	APIHandler handler = it->second;
	(this->*handler)(arg);
}

void ZabbixAPIEmulator::APIHandlerUserLogin(APIHandlerArg &arg)
{
	static const char *LOGIN_RES_FILE = "zabbix-api-res-login-001.json";
	string path = getFixturesDir() + LOGIN_RES_FILE;
	gchar *contents;
	gsize length;
	gboolean succeeded =
	  g_file_get_contents(path.c_str(), &contents, &length, NULL);
	if (!succeeded)
		THROW_ASURA_EXCEPTION("Failed to read file: %s", path.c_str());
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_TAKE,
	                         contents, length);
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}
