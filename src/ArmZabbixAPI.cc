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
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBClientZabbix.h"

static const int DEFAULT_RETRY_INTERVAL = 10;
static const int DEFAULT_REPEAT_INTERVAL = 30;

static const char *MIME_JSON_RPC = "application/json-rpc";

struct ArmZabbixAPI::PrivateContext
{
	string         server;
	string         authToken;
	string         uri;
	int            serverPort;
	int            retryInterval;	// in sec
	int            repeatInterval;	// in sec;
	int            zabbixServerId;
	SoupSession   *session;
	bool           gotTriggers;
	uint64_t       triggerid;
	ItemTablePtr   functionsTablePtr;
	DBClientZabbix dbClientZabbix;
	volatile int   exitRequest;

	// constructors
	PrivateContext(const char *serverName, int port, int _zabbixServerId)
	: server(serverName),
	  serverPort(port),
	  retryInterval(DEFAULT_RETRY_INTERVAL),
	  repeatInterval(DEFAULT_REPEAT_INTERVAL),
	  zabbixServerId(_zabbixServerId),
	  session(NULL),
	  gotTriggers(false),
	  triggerid(0),
	  dbClientZabbix(_zabbixServerId),
	  exitRequest(0)
	{
	}

	~PrivateContext()
	{
		if (session)
			g_object_unref(session);
	}

	bool hasExitRequest(void) const
	{
		return g_atomic_int_get(&exitRequest);
	}

	void setExitRequest(void)
	{
		g_atomic_int_set(&exitRequest, 1);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(int zabbixServerId, const char *server,
                           int serverPort)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(server, serverPort, zabbixServerId);
	m_ctx->server = "localhost";
	m_ctx->uri = "http://";
	m_ctx->uri += m_ctx->server;
	m_ctx->uri += StringUtils::sprintf(":%d", serverPort);
	m_ctx->uri += "/zabbix/api_jsonrpc.php";
}

ArmZabbixAPI::~ArmZabbixAPI()
{
	if (m_ctx)
		delete m_ctx;
}

void ArmZabbixAPI::setPollingInterval(int sec)
{
	m_ctx->repeatInterval = sec;
}

int ArmZabbixAPI::getPollingInterval(void) const
{
	return m_ctx->repeatInterval;
}

void ArmZabbixAPI::requestExit(void)
{
	m_ctx->setExitRequest();
}

ItemTablePtr ArmZabbixAPI::getTrigger(void)
{
	SoupMessage *msg = queryTrigger();
	if (!msg)
		THROW_DATA_STORE_EXCEPTION("Failed to query triggers.");

	JsonParserAgent parser(msg->response_body->data);
	if (parser.hasError()) {
		g_object_unref(msg);
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	g_object_unref(msg);
	startObject(parser, "result");

	ItemTablePtr tablePtr;
	int numTriggers = parser.countElements();
	if (numTriggers < 1) {
		MLPL_DBG("The number of triggers: %d\n", numTriggers);
		return tablePtr;
	}

	m_ctx->gotTriggers = false;
	m_ctx->functionsTablePtr = ItemTablePtr();
	for (int i = 0; i < numTriggers; i++)
		parseAndPushTriggerData(parser, tablePtr, i);
	m_ctx->gotTriggers = true;
	return tablePtr;
}

ItemTablePtr ArmZabbixAPI::getFunctions(void)
{
	if (!m_ctx->gotTriggers) {
		THROW_DATA_STORE_EXCEPTION(
		  "Cache for 'functions' is empty. 'triggers' may not have "
		  "been retrieved.");
	}
	return m_ctx->functionsTablePtr;
}

ItemTablePtr ArmZabbixAPI::getItems(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "item.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	string request_body = agent.generate();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_ctx->uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to get: code: %d: %s", ret, m_ctx->uri.c_str());
	}

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	ItemTablePtr tablePtr;
	int numData = parser.countElements();
	if (numData < 1) {
		MLPL_DBG("The number of hosts: %d\n", numData);
		return tablePtr;
	}

	for (int i = 0; i < numData; i++)
		parseAndPushItemsData(parser, tablePtr, i);
	return tablePtr;
}

ItemTablePtr ArmZabbixAPI::getHosts(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "host.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	string request_body = agent.generate();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_ctx->uri.c_str());

	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to get: code: %d: %s", ret, m_ctx->uri.c_str());
	}

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	ItemTablePtr tablePtr;
	int numData = parser.countElements();
	if (numData < 1) {
		MLPL_DBG("The number of hosts: %d\n", numData);
		return tablePtr;
	}

	for (int i = 0; i < numData; i++)
		parseAndPushHostsData(parser, tablePtr, i);
	return tablePtr;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SoupSession *ArmZabbixAPI::getSession(void)
{
	// NOTE: current implementaion is not MT-safe.
	//       If we have to use this function from multiple threads,
	//       it is only necessary to prepare sessions by thread.
	if (!m_ctx->session)
		m_ctx->session = soup_session_sync_new();
	return m_ctx->session;
}

bool ArmZabbixAPI::openSession(void)
{
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_ctx->uri.c_str());

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
	MLPL_DBG("body: %d, %s\n", msg->response_body->length,
	                           msg->response_body->data);
	bool succeeded = parseInitialResponse(msg);
	g_object_unref(msg);
	if (!succeeded)
		return false;
	MLPL_DBG("auth token: %s\n", m_ctx->authToken.c_str());
	return true;
}

SoupMessage *ArmZabbixAPI::queryTrigger(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "trigger.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("selectFunctions", "extend");
	agent.endObject();

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	string request_body = agent.generate();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, m_ctx->uri.c_str());
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

	if (!parser.read("result", m_ctx->authToken)) {
		MLPL_ERR("Failed to read: result\n");
		return false;
	}
	return true;
}

void ArmZabbixAPI::startObject(JsonParserAgent &parser, const string &name)
{
	if (!parser.startObject(name)) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to read object: %s", name.c_str());
	}
}

void ArmZabbixAPI::startElement(JsonParserAgent &parser, int index)
{
	if (!parser.startElement(index)) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to start element: %d",index);
	}
}

void ArmZabbixAPI::getString(JsonParserAgent &parser, const string &name,
                             string &value)
{
	if (!parser.read(name.c_str(), value)) {
		THROW_DATA_STORE_EXCEPTION("Failed to read: %s", name.c_str());
	}
}

int ArmZabbixAPI::pushInt(JsonParserAgent &parser, ItemGroup *itemGroup,
                          const string &name, ItemId itemId)
{
	string value;
	getString(parser, name, value);
	int valInt = atoi(value.c_str());
	itemGroup->add(new ItemInt(itemId, valInt), false);
	return valInt;
}

uint64_t ArmZabbixAPI::pushUint64(JsonParserAgent &parser, ItemGroup *itemGroup,
                                  const string &name, ItemId itemId)
{
	string value;
	getString(parser, name, value);
	uint64_t valU64;
	sscanf(value.c_str(), "%"PRIu64, &valU64);
	itemGroup->add(new ItemUint64(itemId, valU64), false);
	return valU64;
}

string ArmZabbixAPI::pushString(JsonParserAgent &parser, ItemGroup *itemGroup,
                                const string &name, ItemId itemId)
{
	string value;
	getString(parser, name, value);
	itemGroup->add(new ItemString(itemId, value), false);
	return value;
}

void ArmZabbixAPI::pushFunctionsCacheOne(JsonParserAgent &parser,
                                         ItemGroup *grp, int index)
{
	startElement(parser, index);
	pushUint64(parser, grp, "functionid", ITEM_ID_ZBX_FUNCTIONS_FUNCTIONID);
	pushUint64(parser, grp, "itemid",     ITEM_ID_ZBX_FUNCTIONS_ITEMID);
	grp->add(new ItemUint64(ITEM_ID_ZBX_FUNCTIONS_TRIGGERID,
	                        m_ctx->triggerid), false);
	pushString(parser, grp, "function",   ITEM_ID_ZBX_FUNCTIONS_FUNCTION);
	pushString(parser, grp, "parameter",  ITEM_ID_ZBX_FUNCTIONS_PARAMETER);
	parser.endElement();
}

void ArmZabbixAPI::pushFunctionsCache(JsonParserAgent &parser)
{
	startObject(parser, "functions");
	int numFunctions = parser.countElements();
	for (int i = 0; i < numFunctions; i++) {
		ItemGroupPtr itemGroup;
		pushFunctionsCacheOne(parser, itemGroup, i);
		m_ctx->functionsTablePtr->add(itemGroup);
	}
	parser.endObject();
}

void ArmZabbixAPI::parseAndPushTriggerData(JsonParserAgent &parser,
                                           ItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	ItemGroupPtr grp;
	m_ctx->triggerid =
	  pushUint64(parser, grp, "triggerid", ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
	pushString(parser, grp, "expression",  ITEM_ID_ZBX_TRIGGERS_EXPRESSION);
	pushString(parser, grp, "description", ITEM_ID_ZBX_TRIGGERS_DESCRIPTION);
	pushString(parser, grp, "url",         ITEM_ID_ZBX_TRIGGERS_URL);
	pushInt   (parser, grp, "status",      ITEM_ID_ZBX_TRIGGERS_STATUS);
	pushInt   (parser, grp, "value",       ITEM_ID_ZBX_TRIGGERS_VALUE);
	pushInt   (parser, grp, "priority",    ITEM_ID_ZBX_TRIGGERS_PRIORITY);
	pushInt   (parser, grp, "lastchange",  ITEM_ID_ZBX_TRIGGERS_LASTCHANGE);
	pushString(parser, grp, "comments",    ITEM_ID_ZBX_TRIGGERS_COMMENTS);
	pushString(parser, grp, "error",       ITEM_ID_ZBX_TRIGGERS_ERROR);
	pushUint64(parser, grp, "templateid",  ITEM_ID_ZBX_TRIGGERS_TEMPLATEID);
	pushInt   (parser, grp, "type",        ITEM_ID_ZBX_TRIGGERS_TYPE);
	pushInt   (parser, grp, "value_flags", ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS);
	pushInt   (parser, grp, "flags",       ITEM_ID_ZBX_TRIGGERS_FLAGS);
	tablePtr->add(grp);

	// get functions
	pushFunctionsCache(parser);

	parser.endElement();
}

void ArmZabbixAPI::parseAndPushItemsData(JsonParserAgent &parser,
                                         ItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	ItemGroupPtr grp;
	pushUint64(parser, grp, "itemid",       ITEM_ID_ZBX_ITEMS_ITEMID);
	pushInt   (parser, grp, "type",         ITEM_ID_ZBX_ITEMS_TYPE);
	pushString(parser, grp, "snmp_community",
	           ITEM_ID_ZBX_ITEMS_SNMP_COMMUNITY);
	pushString(parser, grp, "snmp_oid",     ITEM_ID_ZBX_ITEMS_SNMP_OID);
	pushUint64(parser, grp, "hostid",       ITEM_ID_ZBX_ITEMS_HOSTID);
	pushString(parser, grp, "name",         ITEM_ID_ZBX_ITEMS_NAME);
	pushString(parser, grp, "key_",         ITEM_ID_ZBX_ITEMS_KEY_);
	pushInt   (parser, grp, "delay",        ITEM_ID_ZBX_ITEMS_DELAY);
	pushInt   (parser, grp, "history",      ITEM_ID_ZBX_ITEMS_HISTORY);
	pushInt   (parser, grp, "trends",       ITEM_ID_ZBX_ITEMS_TRENDS);
	pushString(parser, grp, "lastvalue",    ITEM_ID_ZBX_ITEMS_LASTVALUE);

	pushInt   (parser, grp, "lastclock",    ITEM_ID_ZBX_ITEMS_LASTCLOCK);
	pushString(parser, grp, "prevvalue",    ITEM_ID_ZBX_ITEMS_PREVVALUE);
	pushInt   (parser, grp, "status",       ITEM_ID_ZBX_ITEMS_STATUS);
	pushInt   (parser, grp, "value_type",   ITEM_ID_ZBX_ITEMS_VALUE_TYPE);
	pushString(parser, grp, "trapper_hosts",
	           ITEM_ID_ZBX_ITEMS_TRAPPER_HOSTS);

	pushString(parser, grp, "units",        ITEM_ID_ZBX_ITEMS_UNITS);
	pushInt   (parser, grp, "multiplier",   ITEM_ID_ZBX_ITEMS_MULTIPLIER);
	pushInt   (parser, grp, "delta",        ITEM_ID_ZBX_ITEMS_DELTA);
	pushString(parser, grp, "prevorgvalue", ITEM_ID_ZBX_ITEMS_PREVORGVALUE);
	pushString(parser, grp, "snmpv3_securityname",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYNAME);
	pushInt   (parser, grp, "snmpv3_securitylevel",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYLEVEL);
	pushString(parser, grp, "snmpv3_authpassphrase",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_AUTHPASSPHRASE);
	pushString(parser, grp, "snmpv3_privpassphrase",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_PRIVPASSPRASE);
	pushString(parser, grp, "formula",     ITEM_ID_ZBX_ITEMS_FORMULA);
	pushString(parser, grp, "error",       ITEM_ID_ZBX_ITEMS_ERROR);
	pushString(parser, grp, "lastlogsize", ITEM_ID_ZBX_ITEMS_LASTLOGSIZE);
	pushString(parser, grp, "logtimefmt",  ITEM_ID_ZBX_ITEMS_LOGTIMEFMT);
	pushUint64(parser, grp, "templateid",  ITEM_ID_ZBX_ITEMS_TEMPLATEID);
	pushUint64(parser, grp, "valuemapid",  ITEM_ID_ZBX_ITEMS_VALUEMAPID);
	pushString(parser, grp, "delay_flex",  ITEM_ID_ZBX_ITEMS_DELAY_FLEX);
	pushString(parser, grp, "params",      ITEM_ID_ZBX_ITEMS_PARAMS);
	pushString(parser, grp, "ipmi_sensor", ITEM_ID_ZBX_ITEMS_IPMI_SENSOR);
	pushInt   (parser, grp, "data_type",   ITEM_ID_ZBX_ITEMS_DATA_TYPE);
	pushInt   (parser, grp, "authtype",    ITEM_ID_ZBX_ITEMS_AUTHTYPE);
	pushString(parser, grp, "username",    ITEM_ID_ZBX_ITEMS_USERNAME);
	pushString(parser, grp, "password",    ITEM_ID_ZBX_ITEMS_PASSWORD);
	pushString(parser, grp, "publickey",   ITEM_ID_ZBX_ITEMS_PUBLICKEY);
	pushString(parser, grp, "privatekey",  ITEM_ID_ZBX_ITEMS_PRIVATEKEY);
	pushInt   (parser, grp, "mtime",       ITEM_ID_ZBX_ITEMS_MTIME);
	pushInt   (parser, grp, "lastns",      ITEM_ID_ZBX_ITEMS_LASTNS);
	pushInt   (parser, grp, "flags",       ITEM_ID_ZBX_ITEMS_FLAGS);
	pushString(parser, grp, "filter",      ITEM_ID_ZBX_ITEMS_FILTER);
	pushUint64(parser, grp, "interfaceid", ITEM_ID_ZBX_ITEMS_INTERFACEID);
	pushString(parser, grp, "port",        ITEM_ID_ZBX_ITEMS_PORT);
	pushString(parser, grp, "description", ITEM_ID_ZBX_ITEMS_DESCRIPTION);
	pushInt   (parser, grp, "inventory_link",
	           ITEM_ID_ZBX_ITEMS_INVENTORY_LINK);
	pushString(parser, grp, "lifetime",    ITEM_ID_ZBX_ITEMS_LIFETIME);
	tablePtr->add(grp);

	parser.endElement();
}

void ArmZabbixAPI::parseAndPushHostsData(JsonParserAgent &parser,
                                         ItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	ItemGroupPtr grp;
	pushUint64(parser, grp, "hostid",       ITEM_ID_ZBX_HOSTS_HOSTID);
	pushUint64(parser, grp, "proxy_hostid", ITEM_ID_ZBX_HOSTS_PROXY_HOSTID);
	pushString(parser, grp, "host",         ITEM_ID_ZBX_HOSTS_HOST);
	pushInt   (parser, grp, "status",       ITEM_ID_ZBX_HOSTS_STATUS);
	pushInt   (parser, grp, "disable_until",
	           ITEM_ID_ZBX_HOSTS_DISABLE_UNTIL);
	pushString(parser, grp, "error",        ITEM_ID_ZBX_HOSTS_ERROR);
	pushInt   (parser, grp, "available",    ITEM_ID_ZBX_HOSTS_AVAILABLE);
	pushInt   (parser, grp, "errors_from",  ITEM_ID_ZBX_HOSTS_ERRORS_FROM);
	pushInt   (parser, grp, "lastaccess",   ITEM_ID_ZBX_HOSTS_LASTACCESS);
	pushInt   (parser, grp, "ipmi_authtype",
	           ITEM_ID_ZBX_HOSTS_IPMI_AUTHTYPE);
	pushInt   (parser, grp, "ipmi_privilege",
	           ITEM_ID_ZBX_HOSTS_IPMI_PRIVILEGE);
	pushString(parser, grp, "ipmi_username",
	           ITEM_ID_ZBX_HOSTS_IPMI_USERNAME);
	pushString(parser, grp, "ipmi_password",
	           ITEM_ID_ZBX_HOSTS_IPMI_PASSWORD);
	pushInt   (parser, grp, "ipmi_disable_until",
	           ITEM_ID_ZBX_HOSTS_IPMI_DISABLE_UNTIL);
	pushInt   (parser, grp, "ipmi_available",
	           ITEM_ID_ZBX_HOSTS_IPMI_AVAILABLE);
	pushInt   (parser, grp, "snmp_disable_until",
	           ITEM_ID_ZBX_HOSTS_SNMP_DISABLE_UNTIL);
	pushInt   (parser, grp, "snmp_available",
	           ITEM_ID_ZBX_HOSTS_SNMP_AVAILABLE);
	pushUint64(parser, grp, "maintenanceid",
	           ITEM_ID_ZBX_HOSTS_MAINTENANCEID);
	pushInt   (parser, grp, "maintenance_status",
	           ITEM_ID_ZBX_HOSTS_MAINTENANCE_STATUS);
	pushInt   (parser, grp, "maintenance_type",
	           ITEM_ID_ZBX_HOSTS_MAINTENANCE_TYPE);
	pushInt   (parser, grp, "maintenance_from",
	           ITEM_ID_ZBX_HOSTS_MAINTENANCE_FROM);
	pushInt   (parser, grp, "ipmi_errors_from",
	           ITEM_ID_ZBX_HOSTS_IPMI_ERRORS_FROM);
	pushInt   (parser, grp, "snmp_errors_from",
	           ITEM_ID_ZBX_HOSTS_SNMP_ERRORS_FROM);
	pushString(parser, grp, "ipmi_error", ITEM_ID_ZBX_HOSTS_IPMI_ERROR);
	pushString(parser, grp, "snmp_error", ITEM_ID_ZBX_HOSTS_SNMP_ERROR);
	pushInt   (parser, grp, "jmx_disable_until",
	           ITEM_ID_ZBX_HOSTS_JMX_DISABLE_UNTIL);
	pushInt   (parser, grp, "jmx_available",
	           ITEM_ID_ZBX_HOSTS_JMX_AVAILABLE);
	pushInt   (parser, grp, "jmx_errors_from",
	           ITEM_ID_ZBX_HOSTS_JMX_ERRORS_FROM);
	pushString(parser, grp, "jmx_error", ITEM_ID_ZBX_HOSTS_JMX_ERROR);
	pushString(parser, grp, "name",      ITEM_ID_ZBX_HOSTS_NAME);
	tablePtr->add(grp);
	parser.endElement();
}

void ArmZabbixAPI::updateTriggers(void)
{
	ItemTablePtr tablePtr = getTrigger();
	m_ctx->dbClientZabbix.addTriggersRaw2_0(tablePtr);
}

//
// virtual methods
//
gpointer ArmZabbixAPI::mainThread(AsuraThreadArg *arg)
{
	MLPL_INFO("started: ArmZabbixAPI (server: %s)\n",
	          __PRETTY_FUNCTION__, m_ctx->server.c_str());
	while (!m_ctx->hasExitRequest()) {
		int sleepTime = m_ctx->repeatInterval;
		if (!mainThreadOneProc()) {
			if (m_ctx->hasExitRequest())
				break;
			sleepTime = m_ctx->retryInterval;
		}
		sleep(sleepTime);
	}
	return NULL;
}

//
// virtual methods defined in this class
//
bool ArmZabbixAPI::mainThreadOneProc(void)
{
	if (!openSession())
		return false;
	updateTriggers();

	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
