/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <set>
#include <cstdio>
#include <Logger.h>
#include <SeparatorInjector.h>
#include "ZabbixAPIEmulator.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "HatoholException.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

static const int COUNT_ELEMENT_NAMES = 10;

struct ZabbixAPIEmulator::APIHandlerArg
{
	SoupServer        *server;
	SoupMessage       *msg;
	const char        *path;
	GHashTable        *query;
	SoupClientContext *client;
	int64_t            id;
};

typedef std::map<string, ZabbixAPIEmulator::APIHandler> APIHandlerMap;
typedef APIHandlerMap::iterator APIHandlerMapIterator;

struct ZabbixAPIEmulator::ParameterEventGet {
	string output;
	string sortField;
	string sortOrder;
	int64_t limit;
	int64_t eventIdFrom;
	int64_t eventIdTill;

	ParameterEventGet(void)
	: output("extend"),
	  sortOrder("ASC"),
	  limit(0),
	  eventIdFrom(0),
	  eventIdTill(0)
	{
	}

	void reset(void) {
		output = "extend";
		sortOrder = "ASC";
		limit = 0;
		eventIdFrom = 0;
		eventIdTill = 0;
	}

	bool isInRange(const int64_t &id, const int64_t &num = 0)
	{
		if (limit != 0 && num > limit)
			return false;
		if (eventIdFrom != 0 && id < eventIdFrom)
			return false;
		if (eventIdTill != 0 && id > eventIdTill)
			return false;
		return true;
	}
};

struct ZabbixAPIEmulator::ZabbixAPIEvent {
	string eventid;
	string source;
	string object;
	string objectId;
	string clock;
	string value;
	string acknowledged;
	string ns;
	string value_changed;
};

struct ZabbixAPIEmulator::PrivateContext {
	OperationMode operationMode;
	APIVersion    apiVersion;
	set<string>   authTokens;
	APIHandlerMap apiHandlerMap;
	struct ParameterEventGet paramEvent;
	ZabbixAPIEventMap zbxEventMap;
	int64_t       firstEventId;
	int64_t       lastEventId;
	int64_t       expectedFirstEventId;
	int64_t       expectedLastEventId;
	
	// methods
	PrivateContext(void)
	: operationMode(OPE_MODE_NORMAL),
	  apiVersion(API_VERSION_2_0_4),
	  firstEventId(0),
	  lastEventId(0),
	  expectedFirstEventId(0),
	  expectedLastEventId(0)
	{
		initAPIHandlerMap();
	}

	virtual ~PrivateContext()
	{
	}

	void initAPIHandlerMap(void)
	{
		apiHandlerMap["apiinfo.version"] =
		  &ZabbixAPIEmulator::APIHandlerAPIVersion;
		apiHandlerMap["user.login"] = 
		  &ZabbixAPIEmulator::APIHandlerUserLogin;
		apiHandlerMap["trigger.get"] = 
		  &ZabbixAPIEmulator::APIHandlerTriggerGet;
		apiHandlerMap["item.get"] = 
		  &ZabbixAPIEmulator::APIHandlerItemGet;
		apiHandlerMap["host.get"] = 
		  &ZabbixAPIEmulator::APIHandlerHostGet;
		apiHandlerMap["event.get"] = 
		  &ZabbixAPIEmulator::APIHandlerEventGet;
		apiHandlerMap["application.get"] = 
		  &ZabbixAPIEmulator::APIHandlerApplicationGet;
		apiHandlerMap["hostgroup.get"] =
		  &ZabbixAPIEmulator::APIHandlerHostgroupGet;
	}

	void reset(void)
	{
		operationMode = OPE_MODE_NORMAL;
		apiVersion = API_VERSION_2_0_4;
		paramEvent.reset();
		firstEventId = 0;
		lastEventId = 0;
		expectedFirstEventId = 0;
		expectedLastEventId = 0;
	}

	void setupEventRange(void)
	{
		firstEventId = zbxEventMap.begin()->first;
		if (expectedFirstEventId > firstEventId)
			firstEventId = expectedFirstEventId;
		if (paramEvent.eventIdFrom > firstEventId)
			firstEventId = paramEvent.eventIdFrom;

		lastEventId = zbxEventMap.rbegin()->first;
		if (expectedLastEventId > 0 &&
		    expectedLastEventId < lastEventId) {
			lastEventId = expectedLastEventId;
		}
		if (paramEvent.eventIdTill > 0 &&
		    paramEvent.eventIdTill < lastEventId) {
			lastEventId = paramEvent.eventIdTill;
		}
	}

	bool isInRange(const int64_t &id, const int64_t &num = 0)
	{
		if (id < firstEventId)
			return false;
		if (id > lastEventId)
			return false;
		return paramEvent.isInRange(id, num);
	}
	void makeEventsJSONAscend(string &contents);
	void makeEventsJSONDescend(string &contents);
	string makeJSONString(const ZabbixAPIEvent &data);
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPIEmulator::ZabbixAPIEmulator(void)
: HttpServerStub("ZabbixAPIEmulator"), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ZabbixAPIEmulator::~ZabbixAPIEmulator()
{
	delete m_ctx;
}

void ZabbixAPIEmulator::reset(void)
{
	m_ctx->reset();
}

void ZabbixAPIEmulator::setOperationMode(OperationMode mode)
{
	m_ctx->operationMode = mode;
}

void ZabbixAPIEmulator::setAPIVersion(APIVersion version)
{
	m_ctx->apiVersion = version;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ZabbixAPIEmulator::setSoupHandlers(SoupServer *soupServer)
{
	soup_server_add_handler(soupServer, "/zabbix/api_jsonrpc.php",
	                        handlerAPI, this, NULL);
}

void ZabbixAPIEmulator::startObject(JSONParser &parser,
                                    const string &name)
{
	if (!parser.startObject(name)) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to read object: %s", name.c_str());
	}
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
	JSONParser parser(request);
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

	JSONParser parser(arg.msg->request_body->data);
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
	bool isNull = true;
	bool needAuth = true;
	if (method == "apiinfo.version")
		needAuth = false;
	if (!parser.isNull("auth", isNull) && needAuth)
		THROW_HATOHOL_EXCEPTION("Not found: auth");
	if (isNull) {
		if (method != "user.login" && needAuth)
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

string ZabbixAPIEmulator::getAPIVersionString(APIVersion version)
{
	switch (version) {
	case ZabbixAPIEmulator::API_VERSION_1_3_0:
		return "1.3";
	case ZabbixAPIEmulator::API_VERSION_1_4_0:
		return "1.4";
	case ZabbixAPIEmulator::API_VERSION_2_2_0:
		return "2.2.0";
	case ZabbixAPIEmulator::API_VERSION_2_3_0:
		return "2.3.0";
	case ZabbixAPIEmulator::API_VERSION_2_0_4:
	default:
		return "2.0.4";
	}
}

string ZabbixAPIEmulator::getAPIVersionString(void)
{
	return getAPIVersionString(m_ctx->apiVersion);
}

void ZabbixAPIEmulator::APIHandlerAPIVersion(APIHandlerArg &arg)
{
	string response = StringUtils::sprintf(
	  "{\"jsonrpc\":\"2.0\",\"result\":\"%s\",\"id\":%" PRId64 "}",
	  getAPIVersionString().c_str(), arg.id);
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerUserLogin(APIHandlerArg &arg)
{
	string authToken = generateAuthToken();
	m_ctx->authTokens.insert(authToken);
	const char *fmt = 
	  "{\"jsonrpc\":\"2.0\",\"result\":\"%s\",\"id\":%" PRId64 "}";
	string response = StringUtils::sprintf(fmt, authToken.c_str(), arg.id);
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerTriggerGet(APIHandlerArg &arg)
{
	const char *dataFileName;
	if (m_ctx->apiVersion >= API_VERSION_2_3_0) {
		dataFileName = "zabbix-api-2_3_0-res-triggers.json";
	} else if (m_ctx->apiVersion >= API_VERSION_2_2_0 &&
		   m_ctx->apiVersion < API_VERSION_2_2_2) {
		dataFileName = "zabbix-api-2_2_0-res-triggers.json";
	} else {
		if (hasParameter(arg, "selectHosts", "refer")) {
			dataFileName = "zabbix-api-res-triggers-003-hosts.json";
		} else {
			// current implementation doesn't have this case
			dataFileName = "zabbix-api-res-triggers-001.json";
		}
	}
	APIHandlerGetWithFile(arg, dataFileName);
}

void ZabbixAPIEmulator::APIHandlerItemGet(APIHandlerArg &arg)
{
	// check if selectApplications option is set
	string request(arg.msg->request_body->data,
	               arg.msg->request_body->length);
	JSONParser parser(request);
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
	if (m_ctx->apiVersion < API_VERSION_2_2_0) {
		if (selectApplications) {
			dataFileName = "zabbix-api-res-items-003.json";
		} else {
			// current implementation doesn't have this case
			dataFileName = "zabbix-api-res-items-001.json";
		}
	} else if (m_ctx->apiVersion < API_VERSION_2_3_0) {
		dataFileName = "zabbix-api-2_2_0-res-items.json";
	} else {
		dataFileName = "zabbix-api-2_3_0-res-items.json";
	}
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
	const char *dataFileName;
	if (hasParameter(arg, "selectGroups", "refer"))
		dataFileName = "zabbix-api-res-hosts-002.json";
	else
		dataFileName = "zabbix-api-res-hosts-001.json";
	APIHandlerGetWithFile(arg, dataFileName);
}

void ZabbixAPIEmulator::APIHandlerHostgroupGet(APIHandlerArg &arg)
{
	const char *dataFileName;
	if (hasParameter(arg, "selectHosts", "refer"))
		dataFileName = "zabbix-api-res-hostgroup-002-refer.json";
	else
		dataFileName = "zabbix-api-res-hostgroup-001.json";
	APIHandlerGetWithFile(arg, dataFileName);
}

void ZabbixAPIEmulator::PrivateContext::makeEventsJSONAscend(string &contents)
{
	int64_t numEvents = 0;
	SeparatorInjector injector(",");
	for (int64_t id = firstEventId; isInRange(id, numEvents + 1); ++id) {
		ZabbixAPIEventMapIterator it = zbxEventMap.find(id);
		if (it == zbxEventMap.end())
			continue;
		injector(contents);
		contents += makeJSONString(it->second);
		++numEvents;
	}
}

void ZabbixAPIEmulator::PrivateContext::makeEventsJSONDescend(string &contents)
{
	int64_t numEvents = 0;
	SeparatorInjector injector(",");
	for (int64_t id = lastEventId; isInRange(id, numEvents + 1); --id) {
		ZabbixAPIEventMapIterator it = zbxEventMap.find(id);
		if (it == zbxEventMap.end())
			continue;
		injector(contents);
		contents += makeJSONString(it->second);
		++numEvents;
	}
}

void ZabbixAPIEmulator::APIHandlerEventGet(APIHandlerArg &arg)
{
	loadTestEventsIfNeeded(arg);
	parseEventGetParameter(arg);
	m_ctx->setupEventRange();

	string contents;
	if (m_ctx->paramEvent.sortOrder == "ASC") {
		m_ctx->makeEventsJSONAscend(contents);
	} else if (m_ctx->paramEvent.sortOrder == "DESC") {
		m_ctx->makeEventsJSONDescend(contents);
	}
	string sendData = addJSONResponse(contents, arg);
	gsize length = sendData.size();
	soup_message_body_append(arg.msg->response_body, SOUP_MEMORY_COPY,
	                         sendData.c_str(), length);
	soup_message_set_status(arg.msg, SOUP_STATUS_OK);
}

void ZabbixAPIEmulator::APIHandlerApplicationGet(APIHandlerArg &arg)
{
	static const char *DATA_FILE;
	if (m_ctx->apiVersion < API_VERSION_2_2_0)
	   DATA_FILE = "zabbix-api-res-applications-003.json";
	else
	   DATA_FILE = "zabbix-api-2_2_0-res-applications.json";
	APIHandlerGetWithFile(arg, DATA_FILE);
}

void ZabbixAPIEmulator::makeEventJSONData(const string &path)
{
	static const char *EVENT_ELEMENT_NAMES[COUNT_ELEMENT_NAMES] = {
	  "eventid", "source", "object", "objectid", "clock", "value",
	  "acknowledged", "ns", "value_changed", NULL
	};

	gchar *contents;
	HATOHOL_ASSERT(
	  g_file_get_contents(path.c_str(), &contents, NULL, NULL),
	  "Failed to read file: %s", path.c_str());

	JSONParser parser(contents);
	g_free(contents);
	HATOHOL_ASSERT(!parser.hasError(), "%s", parser.getErrorMessage());

	HATOHOL_ASSERT(parser.startObject("result"),
	  "%s", parser.getErrorMessage());
	unsigned int numElements = parser.countElements();
	for (unsigned int i = 0; i < numElements; i++) {
		string parsedData[COUNT_ELEMENT_NAMES - 1] = {};

		parser.startElement(i);
		for (size_t j = 0; j < COUNT_ELEMENT_NAMES - 1; j++) {
			parser.read(EVENT_ELEMENT_NAMES[j],
				parsedData[j]);
		}
		parser.endElement();

		int64_t eventId = 0;
		sscanf(parsedData[0].c_str(), "%" PRId64, &eventId);
		ZabbixAPIEvent evt;
		evt.eventid = parsedData[0];
		evt.source = parsedData[1];
		evt.object = parsedData[2];
		evt.objectId = parsedData[3];
		evt.clock = parsedData[4];
		evt.value = parsedData[5];
		evt.acknowledged = parsedData[6];
		evt.ns = parsedData[7];
		evt.value_changed = parsedData[8];
		m_ctx->zbxEventMap[eventId] = evt;
	}
}

string ZabbixAPIEmulator::addJSONResponse(const string &slice,
                                            APIHandlerArg &arg)
{
	const char *fmt = 
	  "{\"jsonrpc\":\"2.0\",\"result\":[%s],\"id\":%" PRId64 "}";
	return StringUtils::sprintf(fmt, slice.c_str(), arg.id);
}

void ZabbixAPIEmulator::setExpectedFirstEventId(const EventIdType &id)
{
	m_ctx->expectedFirstEventId = id;
}

void ZabbixAPIEmulator::setExpectedLastEventId(const EventIdType &id)
{
	m_ctx->expectedLastEventId = id;
}

void ZabbixAPIEmulator::parseEventGetParameter(APIHandlerArg &arg)
{
	m_ctx->paramEvent.reset();

	JSONParser parser(arg.msg->request_body->data);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION("Error in parsing: %s",
				      parser.getErrorMessage());
	}
	parser.startObject("params");

	if (!parser.read("output", m_ctx->paramEvent.output)) {
		THROW_HATOHOL_EXCEPTION("Not found: output");
		if (m_ctx->paramEvent.output != "extend" &&
		    m_ctx->paramEvent.output != "shorten") {
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: output: %s",
			  m_ctx->paramEvent.output.c_str());
		}
	} else {
		m_ctx->paramEvent.output = "extend";
	}

	if (parser.read("sortfield", m_ctx->paramEvent.sortField)) {
		if (m_ctx->paramEvent.sortField != "eventid") {
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: sortfield: %s",
			  m_ctx->paramEvent.sortField.c_str());
		}
	} else {
		m_ctx->paramEvent.sortField = "";
	}

	if (parser.read("sortorder", m_ctx->paramEvent.sortOrder)) {
		if (m_ctx->paramEvent.sortOrder != "ASC" &&
		    m_ctx->paramEvent.sortOrder != "DESC") {
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: sortorder: %s",
			  m_ctx->paramEvent.sortOrder.c_str());
		}
	} else {
		m_ctx->paramEvent.sortOrder = "ASC";
	}

	string rawLimit;
	if (parser.read("limit", m_ctx->paramEvent.limit)) {
		sscanf(rawLimit.c_str(), "%" PRIu64, &m_ctx->paramEvent.limit);
		if (m_ctx->paramEvent.limit < 0)
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: limit: %" PRId64 "\n",
			  m_ctx->paramEvent.limit);
	} else {
		m_ctx->paramEvent.limit = 0;
	}

	string rawEventIdFrom;
	if(parser.read("eventid_from", rawEventIdFrom)) {
		sscanf(rawEventIdFrom.c_str(), "%" PRIu64,
		       &m_ctx->paramEvent.eventIdFrom);
		if (m_ctx->paramEvent.eventIdFrom < 0)
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: eventid_from: %" PRId64 "\n",
			  m_ctx->paramEvent.eventIdFrom);
	} else {
		m_ctx->paramEvent.eventIdFrom = 0;
	}

	string rawEventIdTill;
	if(parser.read("eventid_till", rawEventIdTill)) {
		sscanf(rawEventIdTill.c_str(), "%" PRIu64,
		       &m_ctx->paramEvent.eventIdTill);
		if (m_ctx->paramEvent.eventIdTill < 0)
			THROW_HATOHOL_EXCEPTION(
			  "Invalid parameter: eventid_till: %" PRId64 "\n",
			  m_ctx->paramEvent.eventIdTill);
	} else {
		m_ctx->paramEvent.eventIdFrom = 0;
	}
}

string ZabbixAPIEmulator::PrivateContext::makeJSONString(
  const ZabbixAPIEvent &data)
{
	const char *fmt =
	  "{"
	  "\"eventid\":\"%s\",\"source\":\"%s\",\"object\":\"%s\","
	  "\"objectid\":\"%s\",\"clock\":\"%s\",\"value\":\"%s\","
	  "\"acknowledged\":\"%s\",\"ns\":\"%s\"";
	string json = StringUtils::sprintf(
			fmt,
			data.eventid.c_str(),
			data.source.c_str(),
			data.object.c_str(),
			data.objectId.c_str(),
			data.clock.c_str(),
			data.value.c_str(),
			data.acknowledged.c_str(),
			data.ns.c_str());
	if (apiVersion < API_VERSION_2_2_0)
		json += StringUtils::sprintf(",\"value_changed\":\"%s\"",
					     data.value_changed.c_str());
	json += "}";
	return json;
}

void ZabbixAPIEmulator::loadTestEventsIfNeeded(APIHandlerArg &arg)
{
	if (!m_ctx->zbxEventMap.empty())
		return;
	static const char *DATA_FILE = "zabbix-api-res-events-002.json";
	string path = getFixturesDir() + DATA_FILE;
	makeEventJSONData(path);
}
