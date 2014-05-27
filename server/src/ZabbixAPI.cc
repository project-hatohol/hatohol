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
#include "DataStoreException.h"

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

	bool                 gotTriggers;
	VariableItemTablePtr functionsTablePtr;

	// constructors and destructor
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: apiVersionMajor(0),
	  apiVersionMinor(0),
	  apiVersionMicro(0),
	  session(NULL),
	  gotTriggers(false)
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

ItemTablePtr ZabbixAPI::getTrigger(int requestSince)
{
	SoupMessage *msg = queryTrigger(requestSince);
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

	VariableItemTablePtr tablePtr;
	int numTriggers = parser.countElements();
	MLPL_DBG("The number of triggers: %d\n", numTriggers);
	if (numTriggers < 1)
		return ItemTablePtr(tablePtr);

	m_ctx->gotTriggers = false;
	m_ctx->functionsTablePtr = VariableItemTablePtr();
	for (int i = 0; i < numTriggers; i++)
		parseAndPushTriggerData(parser, tablePtr, i);
	m_ctx->gotTriggers = true;
	return ItemTablePtr(tablePtr);
}

ItemTablePtr ZabbixAPI::getItems(void)
{
	SoupMessage *msg = queryItem();
	if (!msg)
		THROW_DATA_STORE_EXCEPTION("Failed to query items.");

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of items: %d\n", numData);
	if (numData < 1)
		return ItemTablePtr(tablePtr);

	for (int i = 0; i < numData; i++)
		parseAndPushItemsData(parser, tablePtr, i);
	return ItemTablePtr(tablePtr);
}


SoupMessage *ZabbixAPI::queryTrigger(int requestSince)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "trigger.get");

	agent.startObject("params");
	agent.add("output", "extend");

	// We are no longer request 'functions'.
	// See also comments in mainThreadOneProc().
	//
	// agent.add("selectFunctions", "extend");
	if (requestSince > 0)
		agent.add("lastChangeSince", requestSince);
	agent.add("selectHosts", "refer");
	agent.addTrue("active");
	agent.endObject();

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

SoupMessage *ZabbixAPI::queryItem(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "item.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("selectApplications", "refer");
	agent.addTrue("monitored");
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

ItemTablePtr ZabbixAPI::getFunctions(void)
{
	if (!m_ctx->gotTriggers) {
		THROW_DATA_STORE_EXCEPTION(
		  "Cache for 'functions' is empty. 'triggers' may not have "
		  "been retrieved.");
	}
	return ItemTablePtr(m_ctx->functionsTablePtr);
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

void ZabbixAPI::startObject(JsonParserAgent &parser, const string &name)
{
	if (!parser.startObject(name)) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to read object: %s", name.c_str());
	}
}

void ZabbixAPI::startElement(JsonParserAgent &parser, const int &index)
{
	if (!parser.startElement(index)) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to start element: %d",index);
	}
}

#if 0 // See the comment in parseAndPushTriggerData()
void ArmZabbixAPI::pushFunctionsCache(JsonParserAgent &parser)
{
	startObject(parser, "functions");
	int numFunctions = parser.countElements();
	for (int i = 0; i < numFunctions; i++) {
		VariableItemGroupPtr itemGroup;
		pushFunctionsCacheOne(parser, itemGroup, i);
		m_ctx->functionsTablePtr->add(itemGroup);
	}
	parser.endObject();
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
#endif

void ZabbixAPI::getString(JsonParserAgent &parser, const string &name,
                          string &value)
{
	if (!parser.read(name.c_str(), value)) {
		THROW_DATA_STORE_EXCEPTION("Failed to read: %s", name.c_str());
	}
}

int ZabbixAPI::pushInt(JsonParserAgent &parser, ItemGroup *itemGroup,
                       const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	int valInt = atoi(value.c_str());
	itemGroup->add(new ItemInt(itemId, valInt), false);
	return valInt;
}

uint64_t ZabbixAPI::pushUint64(JsonParserAgent &parser, ItemGroup *itemGroup,
                               const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	uint64_t valU64;
	sscanf(value.c_str(), "%"PRIu64, &valU64);
	itemGroup->add(new ItemUint64(itemId, valU64), false);
	return valU64;
}

string ZabbixAPI::pushString(JsonParserAgent &parser, ItemGroup *itemGroup,
                             const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	itemGroup->add(new ItemString(itemId, value), false);
	return value;
}

void ZabbixAPI::parseAndPushTriggerData(
  JsonParserAgent &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushUint64(parser, grp, "triggerid",   ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
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
	if (checkAPIVersion(2, 3, 0)) {
		// Zabbix 2.4 doesn't have "value_falgs" property
		grp->add(new ItemInt(ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS, 0),
			 false);
	} else {
		pushInt(parser, grp, "value_flags",
			ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS);
	}
	pushInt   (parser, grp, "flags",       ITEM_ID_ZBX_TRIGGERS_FLAGS);

	// get hostid
	pushTriggersHostid(parser, grp);

	tablePtr->add(grp);

	// We are no longer request 'functions'.
	// See also comments in mainThreadOneProc().
	//
	// get functions
	// pushFunctionsCache(parser);

	parser.endElement();
}

void ZabbixAPI::parseAndPushItemsData(
  JsonParserAgent &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
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
	if (checkAPIVersion(2, 2, 0)) {
		// Zabbix 2.2 doesn't have "prevorgvalue" property
		grp->add(new ItemString(ITEM_ID_ZBX_ITEMS_PREVORGVALUE, ""),
			 false);
	} else {
		pushString(parser, grp, "prevorgvalue",
			   ITEM_ID_ZBX_ITEMS_PREVORGVALUE);
	}
	pushString(parser, grp, "snmpv3_securityname",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYNAME);
	pushInt   (parser, grp, "snmpv3_securitylevel",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYLEVEL);
	pushString(parser, grp, "snmpv3_authpassphrase",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_AUTHPASSPHRASE);
	pushString(parser, grp, "snmpv3_privpassphrase",
	           ITEM_ID_ZBX_ITEMS_SNMPV3_PRIVPASSPHRASE);
	pushString(parser, grp, "formula",     ITEM_ID_ZBX_ITEMS_FORMULA);
	pushString(parser, grp, "error",       ITEM_ID_ZBX_ITEMS_ERROR);
	pushUint64(parser, grp, "lastlogsize", ITEM_ID_ZBX_ITEMS_LASTLOGSIZE);
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
	if (checkAPIVersion(2, 3, 0)) {
		// Zabbix 2.4 doesn't have "filter" property
		grp->add(new ItemString(ITEM_ID_ZBX_ITEMS_FILTER, ""),
			 false);
	} else {
		pushString(parser, grp, "filter", ITEM_ID_ZBX_ITEMS_FILTER);
	}
	pushUint64(parser, grp, "interfaceid", ITEM_ID_ZBX_ITEMS_INTERFACEID);
	pushString(parser, grp, "port",        ITEM_ID_ZBX_ITEMS_PORT);
	pushString(parser, grp, "description", ITEM_ID_ZBX_ITEMS_DESCRIPTION);
	pushInt   (parser, grp, "inventory_link",
	           ITEM_ID_ZBX_ITEMS_INVENTORY_LINK);
	pushString(parser, grp, "lifetime",    ITEM_ID_ZBX_ITEMS_LIFETIME);

	// application
	pushApplicationid(parser, grp);

	tablePtr->add(grp);

	parser.endElement();
}

void ZabbixAPI::pushTriggersHostid(JsonParserAgent &parser,
                                   ItemGroup *itemGroup)
{
	ItemId itemId = ITEM_ID_ZBX_TRIGGERS_HOSTID;
	startObject(parser, "hosts");
	int numElem = parser.countElements();
	if (numElem == 0) {
		const uint64_t dummyData = 0;
		itemGroup->addNewItem(itemId, dummyData, ITEM_DATA_NULL);
	} else  {
		for (int i = 0; i < numElem; i++) {
			startElement(parser, i);
			pushUint64(parser, itemGroup, "hostid", itemId);
			break; // we use the first applicationid
		}
		parser.endElement();
	}
	parser.endObject();
}

void ZabbixAPI::pushApplicationid(JsonParserAgent &parser, ItemGroup *itemGroup)
{
	ItemId itemId = ITEM_ID_ZBX_ITEMS_APPLICATIONID;
	startObject(parser, "applications");
	int numElem = parser.countElements();
	if (numElem == 0) {
		const uint64_t dummyData = 0;
		itemGroup->addNewItem(itemId, dummyData, ITEM_DATA_NULL);
	} else  {
		for (int i = 0; i < numElem; i++) {
			startElement(parser, i);
			pushUint64(parser, itemGroup, "applicationid", itemId);
			break; // we use the first applicationid
		}
		parser.endElement();
	}
	parser.endObject();
}
