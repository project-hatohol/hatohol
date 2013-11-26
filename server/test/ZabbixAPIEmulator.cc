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

#include <map>
#include <set>
#include <cstdio>
#include <Logger.h>

#include "ZabbixAPIEmulator.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "HatoholException.h"
#include "Helpers.h"

static const int COUNT_ELEMENT_NAMES = 10;

using namespace mlpl;

struct ZabbixAPIEmulator::APIHandlerArg
{
	SoupServer        *server;
	SoupMessage       *msg;
	const char        *path;
	GHashTable        *query;
	SoupClientContext *client;
	int64_t            id;
};

typedef map<string, ZabbixAPIEmulator::APIHandler> APIHandlerMap;
typedef APIHandlerMap::iterator APIHandlerMapIterator;

struct ZabbixAPIEmulator::ParameterEventGet {
	string output;
	string sortField;
	string sortOrder;
	int64_t limit;
	uint64_t eventIdFrom;
	uint64_t eventIdTill;

	ParameterEventGet(void)
	: limit(0),
	  eventIdFrom(0),
	  eventIdTill(0)
	{
		sortOrder = "ASC";
	}
};

struct ZabbixAPIEmulator::JsonKeys {
	int64_t eventid;
	int64_t source;
	int64_t object;
	int64_t objectId;
	int64_t clock;
	int64_t value;
	int64_t acknowledged;
	int64_t ns;
	int64_t value_changed;

	JsonKeys(void)
	: source(0),
	  object(0),
	  objectId(0),
	  clock(0),
	  value(0),
	  acknowledged(0),
	  ns(0),
	  value_changed(0)
	{
	}
};

typedef map<int64_t, ZabbixAPIEmulator::JsonKeys> JsonData;
typedef JsonData::iterator JsonDataIterator;

struct ZabbixAPIEmulator::PrivateContext {
	GThread    *thread;
	guint       port;
	SoupServer *soupServer;
	OperationMode operationMode;
	set<string>   authTokens;
	APIHandlerMap apiHandlerMap;
	size_t        numEventSlices;
	size_t        currEventSliceIndex;
	vector<string> slicedEventVector;
	GMainContext   *gMainCtx;
	struct ParameterEventGet paramEvent;
	JsonData	jsonData;
	
	// methods
	PrivateContext(void)
	: thread(NULL),
	  port(0),
	  soupServer(NULL),
	  operationMode(OPE_MODE_NORMAL),
	  numEventSlices(0),
	  currEventSliceIndex(0),
	  gMainCtx(0)
	{
		gMainCtx = g_main_context_new();
	}

	virtual ~PrivateContext()
	{
		if (thread) {
#ifdef GLIB_VERSION_2_32
			g_thread_unref(thread);
#else
			// nothing to do
#endif // GLIB_VERSION_2_32
		}
		if (gMainCtx)
			g_main_context_unref(gMainCtx);
	}

	void reset(void)
	{
		numEventSlices = 0;
		currEventSliceIndex = 0;
		slicedEventVector.clear();
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
	m_ctx->apiHandlerMap["trigger.get"] = 
	  &ZabbixAPIEmulator::APIHandlerTriggerGet;
	m_ctx->apiHandlerMap["item.get"] = 
	  &ZabbixAPIEmulator::APIHandlerItemGet;
	m_ctx->apiHandlerMap["host.get"] = 
	  &ZabbixAPIEmulator::APIHandlerHostGet;
	m_ctx->apiHandlerMap["event.get"] = 
	  &ZabbixAPIEmulator::APIHandlerEventGet;
	m_ctx->apiHandlerMap["application.get"] = 
	  &ZabbixAPIEmulator::APIHandlerApplicationGet;
}

ZabbixAPIEmulator::~ZabbixAPIEmulator()
{
	if (isRunning()) {
		soup_server_quit(m_ctx->soupServer);
		g_object_unref(m_ctx->soupServer);
		m_ctx->soupServer = NULL;
	}
	if (m_ctx)
		delete m_ctx;
}

void ZabbixAPIEmulator::reset(void)
{
	m_ctx->reset();
}

void ZabbixAPIEmulator::setNumberOfEventSlices(size_t numSlices)
{
	m_ctx->numEventSlices = numSlices;
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
	
	m_ctx->soupServer =
	  soup_server_new(SOUP_SERVER_PORT, port,
	                  SOUP_SERVER_ASYNC_CONTEXT, m_ctx->gMainCtx, NULL);
	soup_server_add_handler(m_ctx->soupServer, NULL, handlerDefault,
	                        this, NULL);
	soup_server_add_handler(m_ctx->soupServer, "/zabbix/api_jsonrpc.php",
	                        handlerAPI, this, NULL);
#ifdef GLIB_VERSION_2_32
	m_ctx->thread = g_thread_new("ZabbixAPIEmulator", _mainThread, this);
#else
	m_ctx->thread = g_thread_create(_mainThread, this, TRUE, NULL);
#endif // GLIB_VERSION_2_32
}

void ZabbixAPIEmulator::stop(void)
{
	soup_server_quit(m_ctx->soupServer);
	g_thread_join(m_ctx->thread);
	m_ctx->thread = NULL;
	g_object_unref(m_ctx->soupServer);
	m_ctx->soupServer = NULL;
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
		THROW_HATOHOL_EXCEPTION(
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

bool ZabbixAPIEmulator::hasParameter
  (APIHandlerArg &arg, const string &paramName, const string &expectedValue)
{
	string request(arg.msg->request_body->data,
	               arg.msg->request_body->length);
	JsonParserAgent parser(request);
	if (parser.hasError())
		THROW_HATOHOL_EXCEPTION("Failed to parse: %s", request.c_str());
	
	if (!parser.startObject("params"))
		return false;
	string value;
	HATOHOL_ASSERT(parser.read(paramName, value), "Failed to read: %s: %s",
	             paramName.c_str(), parser.getErrorMessage());
	HATOHOL_ASSERT(value == expectedValue,
	             "value: %s: not supported (expected: %s)",
	             value.c_str(), expectedValue.c_str());
	return true;
}

string ZabbixAPIEmulator::generateAuthToken(void)
{
	string token;
	for (int i = 0; i < 16; i++) {
		unsigned int randVal = random() % 0x100;
		token += StringUtils::sprintf("%02x", randVal);
	}
	return token;
}

void ZabbixAPIEmulator::handlerAPIDispatch(APIHandlerArg &arg)
{
	if (m_ctx->operationMode == OPE_MODE_HTTP_NOT_FOUND) {
		soup_message_set_status(arg.msg, SOUP_STATUS_NOT_FOUND);
		return;
	}

	JsonParserAgent parser(arg.msg->request_body->data);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION("Error in parsing: %s",
		                      parser.getErrorMessage());
	}

	// jsonrpc
	string rpcVersion;
	if (!parser.read("jsonrpc", rpcVersion))
		THROW_HATOHOL_EXCEPTION("Not found: jsonrpc");
	if (rpcVersion != "2.0") {
		THROW_HATOHOL_EXCEPTION("Invalid parameter: jsonrpc: %s",
	                              rpcVersion.c_str());
	}

	// id
	int64_t id;
	if (!parser.read("id", id))
		THROW_HATOHOL_EXCEPTION("Not found: id");
	arg.id = id;

	// method
	string method;
	if (!parser.read("method", method))
		THROW_HATOHOL_EXCEPTION("Not found: method");

	// auth
	bool isNull;
	if (!parser.isNull("auth", isNull))
		THROW_HATOHOL_EXCEPTION("Not found: auth");
	if (isNull) {
		if (method != "user.login")
			THROW_HATOHOL_EXCEPTION("auth: empty");
	} else {
		string auth;
		if (!parser.read("auth", auth))
			THROW_HATOHOL_EXCEPTION("Not found: auth");
		set<string>::iterator it = m_ctx->authTokens.find(auth);
		if (it == m_ctx->authTokens.end()) {
			THROW_HATOHOL_EXCEPTION("Not found: auth token: %s",
			                      auth.c_str());
		}
	}

	// dispatch
	APIHandlerMapIterator it = m_ctx->apiHandlerMap.find(method);
	if (it == m_ctx->apiHandlerMap.end())
		THROW_HATOHOL_EXCEPTION("Unknown method: %s", method.c_str());
	APIHandler handler = it->second;
	(this->*handler)(arg);
}

void ZabbixAPIEmulator::APIHandlerGetWithFile
  (APIHandlerArg &arg, const string &dataFile)
{
	string path = getFixturesDir() + dataFile;
	gchar *contents;
	gsize length;
	gboolean succeeded =
	  g_file_get_contents(path.c_str(), &contents, &length, NULL);
	if (!succeeded)
		THROW_HATOHOL_EXCEPTION("Failed to read file: %s", path.c_str());
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_TAKE,
	                         contents, length);
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerUserLogin(APIHandlerArg &arg)
{
	string authToken = generateAuthToken();
	m_ctx->authTokens.insert(authToken);
	const char *fmt = 
	  "{\"jsonrpc\":\"2.0\",\"result\":\"%s\",\"id\":%"PRId64"}";
	string response = StringUtils::sprintf(fmt, authToken.c_str(), arg.id);
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerTriggerGet(APIHandlerArg &arg)
{
	const char *dataFileName;
	if (hasParameter(arg, "selectHosts", "refer"))
		dataFileName = "zabbix-api-res-triggers-003-hosts.json";
	else
		dataFileName = "zabbix-api-res-triggers-001.json";
	APIHandlerGetWithFile(arg, dataFileName);
}

void ZabbixAPIEmulator::APIHandlerItemGet(APIHandlerArg &arg)
{
	// check if selectApplications option is set
	string request(arg.msg->request_body->data,
	               arg.msg->request_body->length);
	JsonParserAgent parser(request);
	if (parser.hasError())
		THROW_HATOHOL_EXCEPTION("Failed to parse: %s", request.c_str());
	
	bool selectApplications = false;
	if (parser.startObject("params")) {
		string selectAppStr;
		if (parser.read("selectApplications", selectAppStr)) {
			HATOHOL_ASSERT(selectAppStr == "refer",
			             "selectApplications: %s: not supported",
			             selectAppStr.c_str());
			selectApplications = true;
		}
	}

	// make response
	const char *dataFileName;
	if (selectApplications)
		dataFileName = "zabbix-api-res-items-003.json";
	else
		dataFileName = "zabbix-api-res-items-001.json";
	string path = getFixturesDir() + dataFileName;
	gchar *contents;
	gsize length;
	gboolean succeeded =
	  g_file_get_contents(path.c_str(), &contents, &length, NULL);
	if (!succeeded)
		THROW_HATOHOL_EXCEPTION("Failed to read file: %s", path.c_str());
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_TAKE,
	                         contents, length);
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerHostGet(APIHandlerArg &arg)
{
	static const char *DATA_FILE = "zabbix-api-res-hosts-001.json";
	APIHandlerGetWithFile(arg, DATA_FILE);
}

void ZabbixAPIEmulator::APIHandlerEventGet(APIHandlerArg &arg)
{
	string contents;
	gsize length;
	static const char *DATA_FILE = "zabbix-api-res-events-002.json";
	string path = getFixturesDir() + DATA_FILE;
	parseEventGetParameter(arg);
	makeEventJsonData(path);


	string sendData = addJsonResponse(contents, arg);
	length = sendData.size();
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_TAKE,
	                         sendData.c_str(), length);
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerApplicationGet(APIHandlerArg &arg)
{
	static const char *DATA_FILE =
	   "zabbix-api-res-applications-003.json";
	APIHandlerGetWithFile(arg, DATA_FILE);
}

void ZabbixAPIEmulator::makeEventJsonData(const string &path)
{
	static const char *EVENT_ELEMENT_NAMES[COUNT_ELEMENT_NAMES] = {
	  "eventid", "source", "object", "objectid", "clock", "value",
	  "acknowledged", "ns", "value_changed", NULL
	};

	gchar *contents;
	HATOHOL_ASSERT(
	  g_file_get_contents(path.c_str(), &contents, NULL, NULL),
	  "Failed to read file: %s", path.c_str());

	JsonParserAgent parser(contents);
	g_free(contents);
	HATOHOL_ASSERT(!parser.hasError(), "%s", parser.getErrorMessage());

	HATOHOL_ASSERT(parser.startObject("result"),
	  "%s", parser.getErrorMessage());
	unsigned int numElements = parser.countElements();
	for (unsigned int i = 0; i < numElements; i++) {
		int64_t parsedData[COUNT_ELEMENT_NAMES - 1] = {};

		parser.startElement(i);
		for (size_t j = 0; j < COUNT_ELEMENT_NAMES - 1; j++) {
			parser.read(EVENT_ELEMENT_NAMES[j],
				parsedData[j]);
		}
		parser.endElement();

		JsonKeys key;
		key.eventid = parsedData[0];
		key.source = parsedData[1];
		key.object = parsedData[2];
		key.objectId = parsedData[3];
		key.clock = parsedData[4];
		key.value = parsedData[5];
		key.acknowledged = parsedData[6];
		key.ns = parsedData[7];
		key.value_changed = parsedData[8];
		m_ctx->jsonData[parsedData[0]] = key;
	}
}

string ZabbixAPIEmulator::addJsonResponse(const string &slice,
                                            APIHandlerArg &arg)
{
	const char *fmt = 
	  "{\"jsonrpc\":\"2.0\",\"result\":[%s],\"id\":%"PRId64"}";
	return StringUtils::sprintf(fmt, slice.c_str(), arg.id);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
void ZabbixAPIEmulator::parseEventGetParameter(APIHandlerArg &arg)
{
	JsonParserAgent parser(arg.msg->request_body->data);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION("Error in parsing: %s",
				      parser.getErrorMessage());
	}
	parser.startObject("params");

	if (!parser.read("output", m_ctx->paramEvent.output))
		THROW_HATOHOL_EXCEPTION("Not found: output");
	if (m_ctx->paramEvent.output != "extend" && m_ctx->paramEvent.output != "shorten") {
		THROW_HATOHOL_EXCEPTION("Invalid parameter: output: %s",
				m_ctx->paramEvent.output.c_str());
	}

	if (parser.read("sortfield", m_ctx->paramEvent.sortField)) {
		if (m_ctx->paramEvent.sortField != "eventid") {
			THROW_HATOHOL_EXCEPTION("Invalid parameter: sortfield: %s",
					m_ctx->paramEvent.sortField.c_str());
		}
	}

	if (parser.read("sortorder", m_ctx->paramEvent.sortOrder)) {
		if (m_ctx->paramEvent.sortOrder != "ASC" && m_ctx->paramEvent.sortOrder != "DESC") {
			THROW_HATOHOL_EXCEPTION("Invalid parameter: sortorder: %s",
					m_ctx->paramEvent.sortOrder.c_str());
		}
	}

	if (parser.read("limit", m_ctx->paramEvent.limit)) {
		if (m_ctx->paramEvent.limit < 0)
			THROW_HATOHOL_EXCEPTION("Invalid parameter: limit: %"PRId64"\n", m_ctx->paramEvent.limit);
	} else {
		THROW_HATOHOL_EXCEPTION("Not found: limit");
	}

	string rawEventIdFrom;
	if(parser.read("eventid_from", rawEventIdFrom)) {
		sscanf(rawEventIdFrom.c_str(), "%"PRIu64, &m_ctx->paramEvent.eventIdFrom);
		if (m_ctx->paramEvent.eventIdFrom < 0)
			THROW_HATOHOL_EXCEPTION("Invalid parameter: eventid_from: %"PRId64"\n", m_ctx->paramEvent.eventIdFrom);
	} else {
		THROW_HATOHOL_EXCEPTION("Not Found: eventid_from");
	}

	string rawEventIdTill;
	if(parser.read("eventid_till", rawEventIdTill)) {
		sscanf(rawEventIdTill.c_str(), "%"PRIu64, &m_ctx->paramEvent.eventIdTill);
		if (m_ctx->paramEvent.eventIdTill < 0)
			THROW_HATOHOL_EXCEPTION("Invalid parameter: eventid_till: %"PRId64"\n", m_ctx->paramEvent.eventIdTill);
	} else {
		THROW_HATOHOL_EXCEPTION("Not Found: eventid_till");
	}
}
