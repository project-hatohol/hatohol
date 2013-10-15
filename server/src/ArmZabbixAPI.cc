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

#include <Logger.h>
#include <MutexLock.h>
using namespace mlpl;

#include <sstream>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ArmZabbixAPI.h"
#include "ArmZabbixAPI-template.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBClientZabbix.h"
#include "DBClientHatohol.h"
#include "ActionManager.h"

using namespace std;

static const char *MIME_JSON_RPC = "application/json-rpc";

struct ArmZabbixAPI::PrivateContext
{
	string         authToken;
	string         uri;
	string         username;
	string         password;
	int            zabbixServerId;
	SoupSession   *session;
	bool           gotTriggers;
	uint64_t       triggerid;
	VariableItemTablePtr functionsTablePtr;
	DBClientZabbix dbClientZabbix;
	DBClientHatohol  dbClientHatohol;
	ActionManager    actionManager;

	// constructors
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: zabbixServerId(serverInfo.id),
	  session(NULL),
	  gotTriggers(false),
	  triggerid(0),
	  dbClientZabbix(serverInfo.id)
	{
		// TODO: use serverInfo.ipAddress if it is given.
	}

	~PrivateContext()
	{
		if (session)
			g_object_unref(session);
	}
};

class connectionException : public HatoholException {};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(const MonitoringServerInfo &serverInfo)
: ArmBase(serverInfo),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	m_ctx->uri = "http://";
	m_ctx->uri += serverInfo.getHostAddress();
	m_ctx->uri += StringUtils::sprintf(":%d", serverInfo.port);
	m_ctx->uri += "/zabbix/api_jsonrpc.php";

	m_ctx->username = serverInfo.userName.c_str();
	m_ctx->password = serverInfo.password.c_str();
}

ArmZabbixAPI::~ArmZabbixAPI()
{
	// The body of serverInfo in ArmBase. So it can be used
	// anywhere in this function.
	const MonitoringServerInfo &svInfo = getServerInfo();
	
	MLPL_INFO("ArmZabbixAPI [%d:%s]: exit process started.\n",
	          svInfo.id, svInfo.hostName.c_str());

	// wait for the finish of the thread
	requestExit();
	stop();

	if (m_ctx)
		delete m_ctx;
	MLPL_INFO("ArmZabbixAPI [%d:%s]: exit process completed.\n",
	          svInfo.id, svInfo.hostName.c_str());
}

ItemTablePtr ArmZabbixAPI::getTrigger(int requestSince)
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

ItemTablePtr ArmZabbixAPI::getFunctions(void)
{
	if (!m_ctx->gotTriggers) {
		THROW_DATA_STORE_EXCEPTION(
		  "Cache for 'functions' is empty. 'triggers' may not have "
		  "been retrieved.");
	}
	return ItemTablePtr(m_ctx->functionsTablePtr);
}

ItemTablePtr ArmZabbixAPI::getItems(void)
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

ItemTablePtr ArmZabbixAPI::getHosts(const vector<uint64_t> &hostIdVector)
{
	SoupMessage *msg = queryHost(hostIdVector);
	if (!msg)
		THROW_DATA_STORE_EXCEPTION("Failed to query hosts.");

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of hosts: %d\n", numData);
	if (numData < 1)
		return ItemTablePtr(tablePtr);

	for (int i = 0; i < numData; i++)
		parseAndPushHostsData(parser, tablePtr, i);
	return ItemTablePtr(tablePtr);
}

ItemTablePtr ArmZabbixAPI::getApplications(const vector<uint64_t> &appIdVector)
{
	SoupMessage *msg = queryApplication(appIdVector);
	if (!msg)
		THROW_DATA_STORE_EXCEPTION("Failed to query application.");

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of aplications: %d\n", numData);
	if (numData < 1)
		return ItemTablePtr(tablePtr);

	for (int i = 0; i < numData; i++)
		parseAndPushApplicationsData(parser, tablePtr, i);
	return ItemTablePtr(tablePtr);
}

ItemTablePtr ArmZabbixAPI::getEvents(uint64_t eventIdOffset, uint64_t eventIdTill)
{
	SoupMessage *msg = queryEvent(eventIdOffset, eventIdTill);
	if (!msg)
		THROW_DATA_STORE_EXCEPTION("Failed to query events.");

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of events: %d\n", numData);
	if (numData < 1)
		return ItemTablePtr(tablePtr);

	for (int i = 0; i < numData; i++)
		parseAndPushEventsData(parser, tablePtr, i);
	return ItemTablePtr(tablePtr);
}

uint64_t ArmZabbixAPI::getLastEventId(void)
{
	string strLastEventId;
	uint64_t lastEventId = 0;

	SoupMessage *msg = queryGetLastEventId();
	if (!msg)
		MLPL_ERR("Failed to query eventID.");

	JsonParserAgent parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_DATA_STORE_EXCEPTION(
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");
	startElement(parser, 0);

	if (!parser.read("eventid", strLastEventId))
		THROW_DATA_STORE_EXCEPTION("Failed to read: eventid\n");

	lastEventId = convertStrToUint64(strLastEventId);
	MLPL_DBG("LastEventID: %"PRIu64"\n", lastEventId);

	return lastEventId;
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

bool ArmZabbixAPI::openSession(SoupMessage **msgPtr)
{
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, m_ctx->uri.c_str());

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

bool ArmZabbixAPI::updateAuthTokenIfNeeded(void)
{
	if (m_ctx->authToken.empty()) {
		MLPL_DBG("authToken is empty\n");
		if (!openSession())
			return false;
	}
	MLPL_DBG("authToken: %s\n", m_ctx->authToken.c_str());

	return true;
}

string ArmZabbixAPI::getAuthToken(void)
{
	// This function is used in the testing phase
	updateAuthTokenIfNeeded();
	return m_ctx->authToken;
}

SoupMessage *ArmZabbixAPI::queryCommon(JsonBuilderAgent &agent)
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

SoupMessage *ArmZabbixAPI::queryTrigger(int requestSince)
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

SoupMessage *ArmZabbixAPI::queryItem(void)
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

SoupMessage *ArmZabbixAPI::queryHost(const vector<uint64_t> &hostIdVector)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "host.get");

	agent.startObject("params");
	agent.add("output", "extend");
	if (!hostIdVector.empty()) {
		agent.startArray("hostids");
		vector<uint64_t>::const_iterator it = hostIdVector.begin();
		for (; it != hostIdVector.end(); ++it)
			agent.add(*it);
		agent.endArray();
	}
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

SoupMessage *ArmZabbixAPI::queryApplication(const vector<uint64_t> &appIdVector)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "application.get");

	agent.startObject("params");
	agent.add("output", "extend");
	if (!appIdVector.empty()) {
		agent.startArray("applicationids");
		vector<uint64_t>::const_iterator it = appIdVector.begin();
		for (; it != appIdVector.end(); ++it)
			agent.add(*it);
		agent.endArray();
	}
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

SoupMessage *ArmZabbixAPI::queryEvent(uint64_t eventIdOffset, uint64_t eventIdTill)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "event.get");

	agent.startObject("params");
	agent.add("output", "extend");
	string strEventIdFrom = StringUtils::sprintf("%"PRId64, eventIdOffset);
	agent.add("eventid_from", strEventIdFrom.c_str());
	if (eventIdTill != UNLIMITED) {
		string strEventIdTill = StringUtils::sprintf("%"PRId64, eventIdTill);
		agent.add("eventid_till", strEventIdTill.c_str());
	}
	agent.endObject(); // params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

SoupMessage *ArmZabbixAPI::queryGetLastEventId(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "event.get");

	agent.startObject("params");
	agent.add("output", "shorten");
	agent.add("sortfield", "eventid");
	agent.add("sortorder", "DESC");
	agent.add("limit", 1);
	agent.endObject(); //params

	agent.add("auth", m_ctx->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent);
}

string ArmZabbixAPI::getInitialJsonRequest(void)
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
		VariableItemGroupPtr itemGroup;
		pushFunctionsCacheOne(parser, itemGroup, i);
		m_ctx->functionsTablePtr->add(itemGroup);
	}
	parser.endObject();
}

void ArmZabbixAPI::parseAndPushTriggerData
  (JsonParserAgent &parser, VariableItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
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

void ArmZabbixAPI::pushApplicationid(JsonParserAgent &parser,
                                     ItemGroup *itemGroup)
{
	ItemId itemId = ITEM_ID_ZBX_ITEMS_APPLICATIONID;
	startObject(parser, "applications");
	int numElem = parser.countElements();
	if (numElem == 0) {
		itemGroup->ADD_NEW_ITEM(Uint64, itemId, 0, ITEM_DATA_NULL);
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

void ArmZabbixAPI::pushTriggersHostid(JsonParserAgent &parser,
                                      ItemGroup *itemGroup)
{
	ItemId itemId = ITEM_ID_ZBX_TRIGGERS_HOSTID;
	startObject(parser, "hosts");
	int numElem = parser.countElements();
	if (numElem == 0) {
		itemGroup->ADD_NEW_ITEM(Uint64, itemId, 0, ITEM_DATA_NULL);
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

uint64_t ArmZabbixAPI::convertStrToUint64(const string strData)
{
	uint64_t valU64;
	sscanf(strData.c_str(), "%"PRIu64, &valU64);
	return valU64;
}

void ArmZabbixAPI::parseAndPushItemsData
  (JsonParserAgent &parser, VariableItemTablePtr &tablePtr, int index)
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
	pushString(parser, grp, "prevorgvalue", ITEM_ID_ZBX_ITEMS_PREVORGVALUE);
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
	pushString(parser, grp, "filter",      ITEM_ID_ZBX_ITEMS_FILTER);
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

void ArmZabbixAPI::parseAndPushHostsData
  (JsonParserAgent &parser, VariableItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
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

void ArmZabbixAPI::parseAndPushApplicationsData(JsonParserAgent &parser,
                                                VariableItemTablePtr &tablePtr,
                                                int index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushUint64(parser, grp, "applicationid",
	           ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID);
	pushUint64(parser, grp, "hostid", ITEM_ID_ZBX_APPLICATIONS_HOSTID);
	pushString(parser, grp, "name",   ITEM_ID_ZBX_APPLICATIONS_NAME);
	pushUint64(parser, grp, "templateid",
	           ITEM_ID_ZBX_APPLICATIONS_TEMPLATEID);
	tablePtr->add(grp);
	parser.endElement();
}

void ArmZabbixAPI::parseAndPushEventsData
  (JsonParserAgent &parser, VariableItemTablePtr &tablePtr, int index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushUint64(parser, grp, "eventid",      ITEM_ID_ZBX_EVENTS_EVENTID);
	pushInt   (parser, grp, "source",       ITEM_ID_ZBX_EVENTS_SOURCE);
	pushInt   (parser, grp, "object",       ITEM_ID_ZBX_EVENTS_OBJECT);
	pushUint64(parser, grp, "objectid",     ITEM_ID_ZBX_EVENTS_OBJECTID);
	pushInt   (parser, grp, "clock",        ITEM_ID_ZBX_EVENTS_CLOCK);
	pushInt   (parser, grp, "value",        ITEM_ID_ZBX_EVENTS_VALUE);
	pushInt   (parser, grp, "acknowledged",
	           ITEM_ID_ZBX_EVENTS_ACKNOWLEDGED);
	pushInt   (parser, grp, "ns",           ITEM_ID_ZBX_EVENTS_NS);
	pushInt   (parser, grp, "value_changed",
	           ITEM_ID_ZBX_EVENTS_VALUE_CHANGED);
	tablePtr->add(grp);
	parser.endElement();
}

template<typename T>
void ArmZabbixAPI::updateOnlyNeededItem
  (const ItemTable *primaryTable,
   const ItemId pickupItemId, const ItemId checkItemId,
   ArmZabbixAPI::DataGetter dataGetter,
   DBClientZabbix::AbsentItemPicker absentItemPicker,
   DBClientZabbix::TableSaver tableSaver)
{
	if (primaryTable->getNumberOfRows() == 0)
		return;

	// make a vector that has items used in the table.
	vector<T> usedItemVector;
	makeItemVector<T>(usedItemVector, primaryTable, pickupItemId);
	if (usedItemVector.empty())
		return;

	// extract items that are not in the replication DB.
	vector<T> absentItemVector;
	(m_ctx->dbClientZabbix.*absentItemPicker)(absentItemVector,
	                                          usedItemVector);
	if (absentItemVector.empty())
		return;

	// get needed data via ZABBIX API
	ItemTablePtr tablePtr = (this->*dataGetter)(absentItemVector);
	(m_ctx->dbClientZabbix.*tableSaver)(tablePtr);

	// check the result
	checkObtainedItems<uint64_t>(tablePtr, absentItemVector, checkItemId);
}

ItemTablePtr ArmZabbixAPI::updateTriggers(void)
{
	int requestSince;
	int lastChange = m_ctx->dbClientZabbix.getTriggerLastChange();

	// TODO: to be considered that we may leak triggers.
	if (lastChange == DBClientZabbix::TRIGGER_CHANGE_TIME_NOT_FOUND)
		requestSince = 0;
	else
		requestSince = lastChange;
	ItemTablePtr tablePtr = getTrigger(requestSince);
	m_ctx->dbClientZabbix.addTriggersRaw2_0(tablePtr);
	return tablePtr;
}

void ArmZabbixAPI::updateFunctions(void)
{
	ItemTablePtr tablePtr = getFunctions();
	m_ctx->dbClientZabbix.addFunctionsRaw2_0(tablePtr);
}

ItemTablePtr ArmZabbixAPI::updateItems(void)
{
	ItemTablePtr tablePtr = getItems();
	m_ctx->dbClientZabbix.addItemsRaw2_0(tablePtr);
	return tablePtr;
}

void ArmZabbixAPI::updateHosts(void)
{
	// getHosts() tries to get all hosts when an empty vector is passed.
	static const vector<uint64_t> hostIdVector;
	ItemTablePtr tablePtr = getHosts(hostIdVector);
	m_ctx->dbClientZabbix.addHostsRaw2_0(tablePtr);
}

void ArmZabbixAPI::updateHosts(const ItemTable *triggers)
{
	updateOnlyNeededItem<uint64_t>(
	  triggers,
	  ITEM_ID_ZBX_TRIGGERS_HOSTID,
	  ITEM_ID_ZBX_HOSTS_HOSTID,
	  &ArmZabbixAPI::getHosts,
	  &DBClientZabbix::pickupAbsentHostIds,
	  &DBClientZabbix::addHostsRaw2_0);
}

ItemTablePtr ArmZabbixAPI::updateEvents(void)
{
	uint64_t eventIdOffset, eventIdTill;
	uint64_t dbLastEventId = m_ctx->dbClientZabbix.getLastEventId();
	uint64_t serverLastEventId = getLastEventId();
	ItemTablePtr tablePtr;
	while (dbLastEventId != serverLastEventId) {
		if (dbLastEventId == DBClientZabbix::EVENT_ID_NOT_FOUND) {
			eventIdOffset = 0;
			eventIdTill = 1000;
		} else {
			eventIdOffset = dbLastEventId + 1;
			eventIdTill = dbLastEventId + 1000;
		}
		tablePtr = getEvents(eventIdOffset, eventIdTill);
		m_ctx->dbClientZabbix.addEventsRaw2_0(tablePtr);
		makeHatoholEvents(tablePtr);
		dbLastEventId = m_ctx->dbClientZabbix.getLastEventId();
	}
	return tablePtr;
}

void ArmZabbixAPI::updateApplications(void)
{
	// getHosts() tries to get all hosts when an empty vector is passed.
	static const vector<uint64_t> appIdVector;
	ItemTablePtr tablePtr = getApplications(appIdVector);
	m_ctx->dbClientZabbix.addApplicationsRaw2_0(tablePtr);
}

void ArmZabbixAPI::updateApplications(const ItemTable *items)
{
	updateOnlyNeededItem<uint64_t>(
	  items,
	  ITEM_ID_ZBX_ITEMS_APPLICATIONID, 
	  ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID, 
	  &ArmZabbixAPI::getApplications,
	  &DBClientZabbix::pickupAbsentApplcationIds,
	  &DBClientZabbix::addApplicationsRaw2_0);
}

//
// virtual methods
//
gpointer ArmZabbixAPI::mainThread(HatoholThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmZabbixAPI (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

void ArmZabbixAPI::makeHatoholTriggers(void)
{
	TriggerInfoList triggerInfoList;
	m_ctx->dbClientZabbix.getTriggersAsHatoholFormat(triggerInfoList);
	m_ctx->dbClientHatohol.setTriggerInfoList(triggerInfoList,
	                                          m_ctx->zabbixServerId);
}

void ArmZabbixAPI::makeHatoholEvents(ItemTablePtr events)
{
	EventInfoList eventInfoList;
	DBClientZabbix::transformEventsToHatoholFormat(eventInfoList, events,
	                                               m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addEventInfoList(eventInfoList);
	m_ctx->actionManager.checkEvents(eventInfoList);
}

void ArmZabbixAPI::makeHatoholItems(ItemTablePtr items)
{
	ItemInfoList itemInfoList;
	DBClientZabbix::transformItemsToHatoholFormat(itemInfoList, items,
	                                              m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addItemInfoList(itemInfoList);
}

//
// This function just shows a warning if there is missing host ID.
//
template<typename T>
void ArmZabbixAPI::checkObtainedItems(const ItemTable *obtainedItemTable,
                                      const vector<T> &requestedItemVector,
                                      const ItemId itemId)
{
	size_t numRequested = requestedItemVector.size();
	size_t numObtained = obtainedItemTable->getNumberOfRows();
	if (numRequested != numObtained) {
		MLPL_WARN("requested: %zd, obtained: %zd\n",
		          numRequested, numObtained);
	}

	// make the set of obtained items
	set<T> obtainedItemSet;
	const ItemGroupList &grpList = obtainedItemTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		const ItemData *itemData = (*it)->getItem(itemId);
		T item = ItemDataUtils::get<T>(itemData);
		obtainedItemSet.insert(item);
	}

	// check the requested ID is in the obtained
	for (size_t i = 0; i < numRequested; i++) {
		const T &reqItem = requestedItemVector[i];
		typename set<T>::iterator it = obtainedItemSet.find(reqItem);
		if (it == obtainedItemSet.end()) {
			ostringstream ss;
			ss << reqItem;
			MLPL_WARN(
			  "Not found in the obtained items: %s "
			  "(%"PRIu64")\n",
		          ss.str().c_str(), itemId);
		} else {
			obtainedItemSet.erase(it);
		}
	}
}

bool ArmZabbixAPI::mainThreadOneProc(void)
{
	if (!updateAuthTokenIfNeeded())
		return false;

	try
	{
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			ItemTablePtr items = updateItems();
			makeHatoholItems(items);
			updateApplications(items);
			return true;
		}

		// get triggers
		ItemTablePtr triggers = updateTriggers();

		// update needed hosts
		updateHosts(triggers);

		// Currently functions are no longer updated, because ZABBIX
		// API can return host ID directly (If we use DBs as exactly
		// the same as those in Zabbix Server, we have to join
		// triggers, functions, and items to get the host ID).
		//
		// updateFunctions();

		makeHatoholTriggers();

		ItemTablePtr events = updateEvents();

		if (!getCopyOnDemandEnabled()) {
			ItemTablePtr items = updateItems();
			makeHatoholItems(items);
			updateApplications(items);
		}
	} catch (const DataStoreException &dse) {
		MLPL_ERR("Error on update\n");
		m_ctx->authToken = "";
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
