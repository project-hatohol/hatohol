/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstdio>
#include <string>
#include <Reaper.h>
#include "JSONParser.h"
#include "ZabbixAPI.h"
#include "StringUtils.h"
#include "DataStoreException.h"
#include "HatoholError.h"

using namespace std;
using namespace mlpl;

static const char *MIME_JSON_RPC = "application/json-rpc";
static const guint DEFAULT_TIMEOUT = 60;
static const size_t HISTORY_LIMIT_PER_ONCE = 1000;

const uint64_t ZabbixAPI::EVENT_ID_NOT_FOUND = -1;
const size_t ZabbixAPI::EVENT_ID_DIGIT_NUM = 20;

struct ZabbixAPI::Impl {

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
	Impl(void)
	: apiVersionMajor(0),
	  apiVersionMinor(0),
	  apiVersionMicro(0),
	  session(NULL),
	  gotTriggers(false)
	{
	}

	virtual ~Impl()
	{
		if (session)
			g_object_unref(session);
	}

	void setMonitoringServerInfo(const MonitoringServerInfo &serverInfo)
	{
		const bool forURI = true;
		uri = "http://";
		uri += serverInfo.getHostAddress(forURI);
		uri += StringUtils::sprintf(":%d", serverInfo.port);
		uri += "/zabbix/api_jsonrpc.php";

		username = serverInfo.userName.c_str();
		password = serverInfo.password.c_str();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPI::ZabbixAPI(void)
: m_impl(new Impl())
{
}

ZabbixAPI::~ZabbixAPI()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ZabbixAPI::setMonitoringServerInfo(const MonitoringServerInfo &serverInfo)
{
	m_impl->setMonitoringServerInfo(serverInfo);
}

void ZabbixAPI::onUpdatedAuthToken(const string &authToken)
{
}

const string &ZabbixAPI::getAPIVersion(void)
{
	if (!m_impl->apiVersion.empty())
		return m_impl->apiVersion;

	HatoholError queryRet;
	SoupMessage *msg = queryAPIVersion(queryRet);
	if (!msg)
		return m_impl->apiVersion;
	Reaper<void> msgReaper(msg, g_object_unref);

	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser: %s\n", parser.getErrorMessage());
		return m_impl->apiVersion;
	}

	if (parser.read("result", m_impl->apiVersion)) {
		MLPL_DBG("Zabbix API version: %s\n",
		         m_impl->apiVersion.c_str());
	} else {
		MLPL_ERR("Failed to read API version\n");
	}

	if (!m_impl->apiVersion.empty()) {
		StringList list;
		StringUtils::split(list, m_impl->apiVersion, '.');
		StringListIterator it = list.begin();
		for (size_t i = 0; it != list.end(); ++i, ++it) {
			const string &str = *it;
			if (i == 0)
				m_impl->apiVersionMajor = atoi(str.c_str());
			else if (i == 1)
				m_impl->apiVersionMinor = atoi(str.c_str());
			else if (i == 2)
				m_impl->apiVersionMicro = atoi(str.c_str());
			else
				break;
		}
	}
	return m_impl->apiVersion;
}

bool ZabbixAPI::checkAPIVersion(int major, int minor, int micro)
{
	getAPIVersion();

	if (m_impl->apiVersionMajor > major)
		return true;
	if (m_impl->apiVersionMajor == major &&
	    m_impl->apiVersionMinor >  minor)
		return true;
	if (m_impl->apiVersionMajor == major &&
	    m_impl->apiVersionMinor == minor &&
	    m_impl->apiVersionMicro >= micro)
		return true;
	return false;
}

bool ZabbixAPI::openSession(SoupMessage **msgPtr)
{
	const string &version = getAPIVersion();
	if (version.empty())
		return false;

	SoupMessage *msg =
	  soup_message_new(SOUP_METHOD_POST, m_impl->uri.c_str());
	if (!msg) {
		MLPL_ERR("Failed to call: soup_message_new: uri: %s\n",
		         m_impl->uri.c_str());
		return false;
	}
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	string request_body = getInitialJSONRequest();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		MLPL_ERR("Failed to get from %s, Status: %d (%s)\n",
	                 m_impl->uri.c_str(),
			 ret, soup_status_get_phrase(ret));
		return false;
	}
	MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
	         msg->response_body->length, msg->response_body->data);
	bool succeeded = parseInitialResponse(msg);
	if (!succeeded) {
		g_object_unref(msg);
		return false;
	}
	MLPL_DBG("authToken: %s\n", m_impl->authToken.c_str());

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
	if (!m_impl->session)
		m_impl->session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT,      DEFAULT_TIMEOUT,
			//FIXME: Sometimes it causes crash (issue #98)
			//SOUP_SESSION_IDLE_TIMEOUT, DEFAULT_IDLE_TIMEOUT,
			NULL);
	return m_impl->session;
}

bool ZabbixAPI::updateAuthTokenIfNeeded(void)
{
	if (m_impl->authToken.empty()) {
		MLPL_DBG("authToken is empty\n");
		if (!openSession())
			return false;
	}
	MLPL_DBG("authToken: %s\n", m_impl->authToken.c_str());

	return true;
}

string ZabbixAPI::getAuthToken(void)
{
	// This function is used in the test class.
	updateAuthTokenIfNeeded();
	return m_impl->authToken;
}

void ZabbixAPI::clearAuthToken(void)
{
	m_impl->authToken.clear();
	onUpdatedAuthToken(m_impl->authToken);
}

ItemTablePtr ZabbixAPI::getTrigger(int requestSince)
{
	HatoholError queryRet;
	SoupMessage *msg = queryTrigger(queryRet, requestSince);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query triggers.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		g_object_unref(msg);
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	g_object_unref(msg);
	startObject(parser, "result");

	VariableItemTablePtr tablePtr;
	int numTriggers = parser.countElements();
	MLPL_DBG("The number of triggers: %d\n", numTriggers);
	if (numTriggers < 1)
		return ItemTablePtr(tablePtr);

	m_impl->gotTriggers = false;
	m_impl->functionsTablePtr = VariableItemTablePtr();
	for (int i = 0; i < numTriggers; i++)
		parseAndPushTriggerData(parser, tablePtr, i);
	m_impl->gotTriggers = true;
	return ItemTablePtr(tablePtr);
}

ItemTablePtr ZabbixAPI::getTriggerExpandedDescription(int requestSince)
{
	HatoholError queryRet;
	SoupMessage *msg =
	  queryTriggerExpandedDescription(queryRet, requestSince);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query triggers.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		g_object_unref(msg);
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	g_object_unref(msg);
	startObject(parser, "result");
	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of trigger expanded descriptions: %d\n", numData);
	if (numData < 1)
		return ItemTablePtr(tablePtr);
	for (int i = 0; i < numData; i++) {
		parseAndPushTriggerExpandedDescriptionData(parser, tablePtr, i);
	}

	return ItemTablePtr(tablePtr);
}

static void pushItemData(
  const ItemId itemId, const ItemGroupPtr &itemGrpPtr,
  VariableItemGroupPtr &grp)
{
	const ItemData *itemData = itemGrpPtr->getItem(itemId);
	grp->add(itemData);
}

ItemTablePtr ZabbixAPI::mergePlainTriggersAndExpandedDescriptions(
  const ItemTablePtr triggers, const ItemTablePtr expandedDescriptions)
{
	const ItemGroupList &trigGrpList = triggers->getItemGroupList();
	const ItemGroupList &expandedDescriptionGrpList =
	  expandedDescriptions->getItemGroupList();
	ItemGroupListConstIterator trigGrpItr = trigGrpList.begin();

	TriggerIdItemGrpMap expandedTrigIdGrpMap;
	ItemGroupListConstIterator expandedDescGrpItr =
	  expandedDescriptionGrpList.begin();
	for (; expandedDescGrpItr != expandedDescriptionGrpList.end(); ++expandedDescGrpItr) {
		const ItemGroup *itemGroup = *expandedDescGrpItr;
		ItemGroupPtr expandedDescGrpPtr = *expandedDescGrpItr;
		const TriggerIdType &expandedItemGrpId =
		  *expandedDescGrpPtr->getItem(ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
		expandedTrigIdGrpMap.insert(
		  pair<TriggerIdType, ItemGroupPtr>(expandedItemGrpId, itemGroup));
	}

	VariableItemTablePtr mergedTablePtr;
	for (; trigGrpItr != trigGrpList.end(); ++trigGrpItr) {
		ItemGroupPtr trigItemGrpPtr = *trigGrpItr;
		const TriggerIdType &trigItemGrpId =
		  *trigItemGrpPtr->getItem(ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
		TriggerIdItemGrpMapConstIterator it =
		  expandedTrigIdGrpMap.find(trigItemGrpId);
		VariableItemGroupPtr grp;
		pushItemData(ITEM_ID_ZBX_TRIGGERS_TRIGGERID,
		             trigItemGrpPtr, grp);

		pushItemData(ITEM_ID_ZBX_TRIGGERS_VALUE,
		             trigItemGrpPtr, grp);

		pushItemData(ITEM_ID_ZBX_TRIGGERS_PRIORITY,
		             trigItemGrpPtr, grp);

		pushItemData(ITEM_ID_ZBX_TRIGGERS_LASTCHANGE,
		             trigItemGrpPtr, grp);

		pushItemData(ITEM_ID_ZBX_TRIGGERS_DESCRIPTION,
		             trigItemGrpPtr, grp);

		pushItemData(ITEM_ID_ZBX_TRIGGERS_HOSTID,
		             trigItemGrpPtr, grp);

		if (it != expandedTrigIdGrpMap.end()) {
			pushItemData(ITEM_ID_ZBX_TRIGGERS_EXPANDED_DESCRIPTION,
			             it->second, grp);
		}
		mergedTablePtr->add(grp);
	}
	return static_cast<ItemTablePtr>(mergedTablePtr);
}

ItemTablePtr ZabbixAPI::getItems(void)
{
	HatoholError queryRet;
	SoupMessage *msg = queryItem(queryRet);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query items.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
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

ItemTablePtr ZabbixAPI::getHistory(const ItemIdType &itemId,
				   const ZabbixAPI::ValueType &valueType,
				   const time_t &beginTime,
				   const time_t &endTime)
{
	HatoholError queryRet;
	SoupMessage *msg = queryHistory(queryRet, itemId, valueType,
					beginTime, endTime);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query history.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}

	startObject(parser, "result");
	VariableItemTablePtr tablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of history: %d\n", numData);
	for (int i = 0; i < numData; i++) {
		startElement(parser, i);
		VariableItemGroupPtr grp;
		pushString(parser, grp, "itemid", ITEM_ID_ZBX_HISTORY_ITEMID);
		pushUint64(parser, grp, "clock",  ITEM_ID_ZBX_HISTORY_CLOCK);
		pushUint64(parser, grp, "ns",     ITEM_ID_ZBX_HISTORY_NS);
		pushString(parser, grp, "value",  ITEM_ID_ZBX_HISTORY_VALUE);
		tablePtr->add(grp);
		parser.endElement();
	}

	return ItemTablePtr(tablePtr);
}

void ZabbixAPI::getHosts(
  ItemTablePtr &hostsTablePtr, ItemTablePtr &hostsGroupsTablePtr)
{
	HatoholError queryRet;
	SoupMessage *msg = queryHost(queryRet);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query hosts.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr variableHostsTablePtr, variableHostsGroupsTablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of hosts: %d\n", numData);
	if (numData < 1)
		return;

	for (int i = 0; i < numData; i++) {
		parseAndPushHostsData(parser, variableHostsTablePtr, i);
		parseAndPushHostsGroupsData(parser,
		                            variableHostsGroupsTablePtr, i);
	}
	hostsTablePtr = ItemTablePtr(variableHostsTablePtr);
	hostsGroupsTablePtr = ItemTablePtr(variableHostsGroupsTablePtr);
}

void ZabbixAPI::getGroups(ItemTablePtr &groupsTablePtr)
{
	HatoholError queryRet;
	SoupMessage *msg = queryGroup(queryRet);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query groups.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");

	VariableItemTablePtr variableGroupsTablePtr;
	int numData = parser.countElements();
	MLPL_DBG("The number of groups: %d\n", numData);

	for (int i = 0; i < numData; i++)
		parseAndPushGroupsData(parser, variableGroupsTablePtr, i);

	groupsTablePtr = ItemTablePtr(variableGroupsTablePtr);
}

ItemTablePtr ZabbixAPI::getApplications(const ItemCategoryIdVector &appIdVector)
{
	HatoholError queryRet;
	SoupMessage *msg = queryApplication(appIdVector, queryRet);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query application.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
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

ItemTablePtr ZabbixAPI::getApplications(ItemTablePtr items)
{
	ItemCategoryIdVector appIdVector;
	const ItemGroupList &itemGroupList = items->getItemGroupList();
	ItemGroupListConstIterator itemGrpIt = itemGroupList.begin();
	appIdVector.reserve(itemGroupList.size());
	for (; itemGrpIt != itemGroupList.end(); ++itemGrpIt) {
		const ItemGroup *itemGrp = *itemGrpIt;
		appIdVector.push_back(
		  *itemGrp->getItem(ITEM_ID_ZBX_ITEMS_APPLICATIONID));
	}
	return getApplications(appIdVector);
}

ItemTablePtr ZabbixAPI::getEvents(uint64_t eventIdFrom, uint64_t eventIdTill)
{
	HatoholError queryRet;
	SoupMessage *msg = queryEvent(eventIdFrom, eventIdTill, queryRet);
	if (!msg) {
		if (queryRet == HTERR_INTERNAL_ERROR) {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_INTERNAL_ERROR,
			  "Failed to query events.");
		} else {
			THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
			  HTERR_FAILED_CONNECT_ZABBIX,
			  "%s", queryRet.getMessage().c_str());
		}
	}
	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
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

uint64_t ZabbixAPI::getEndEventId(const bool &isFirst)
{
	string strValue;
	uint64_t returnValue = 0;

	HatoholError queryRet;
	SoupMessage *msg = queryEndEventId(isFirst, queryRet);
	if (!msg) {
		if (isFirst)
			MLPL_ERR("Failed to query first eventID.\n");
		else
			MLPL_ERR("Failed to query last eventID.\n");
		return EVENT_ID_NOT_FOUND;
	}

	JSONParser parser(msg->response_body->data);
	g_object_unref(msg);
	if (parser.hasError()) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to parser: %s", parser.getErrorMessage());
	}
	startObject(parser, "result");
	if (parser.countElements() == 0)
		return EVENT_ID_NOT_FOUND;

	startElement(parser, 0);

	if (!parser.read("eventid", strValue))
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to read: eventid\n");

	returnValue = StringUtils::toUint64(strValue);
	if (isFirst)
		MLPL_DBG("The first event ID in monitoring server: %" PRIu64 "\n", returnValue);
	else
		MLPL_DBG("The last event ID in monitoring server: %" PRIu64 "\n", returnValue);

	return returnValue;
}

SoupMessage *ZabbixAPI::queryEvent(uint64_t eventIdFrom, uint64_t eventIdTill,
				   HatoholError &queryRet)
{
	using StringUtils::sprintf;

	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "event.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("eventid_from", sprintf("%" PRIu64, eventIdFrom));
	if (eventIdTill != UNLIMITED)
		agent.add("eventid_till", sprintf("%" PRIu64, eventIdTill));
	agent.endObject(); // params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryEndEventId(const bool &isFirst, HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "event.get");

	agent.startObject("params");
	agent.add("output", "shorten");
	agent.add("sortfield", "eventid");
	if (isFirst)
		agent.add("sortorder", "ASC");
	else
		agent.add("sortorder", "DESC");
	agent.add("limit", 1);
	agent.endObject(); //params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryTrigger(HatoholError &queryRet, int requestSince)
{
	JSONBuilder agent;
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
	agent.add("active", JSONTrue);
	agent.endObject();

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryTriggerExpandedDescription(HatoholError &queryRet,
                                                      int requestSince)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "trigger.get");

	agent.startObject("params");
	agent.startArray("output");
	agent.add("extend");
	agent.add("description");
	agent.endArray();
	if (requestSince > 0)
		agent.add("lastChangeSince", requestSince);
	agent.add("expandDescription", 1);
	agent.add("selectHosts", "refer");
	agent.add("active", JSONTrue);
	agent.endObject(); //params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryItem(HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "item.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("selectApplications", "refer");
	agent.add("monitored", JSONTrue);
	agent.endObject(); // params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryHistory(HatoholError &queryRet,
				     const ItemIdType &itemId,
				     const ZabbixAPI::ValueType &valueType,
				     const time_t &beginTime,
				     const time_t &endTime)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "history.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("history", valueType);
	agent.add("itemids", itemId);
	agent.add("time_from", beginTime);
	agent.add("time_till", endTime);
	agent.add("sortfield", "clock");
	agent.add("sortorder", "ASC");
	agent.add("limit", HISTORY_LIMIT_PER_ONCE);
	agent.endObject(); // params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryHost(HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "host.get");

	agent.startObject("params");
	agent.add("output", "extend");
	agent.add("selectGroups", "refer");
	agent.add("monitored_hosts", JSONTrue);
	agent.endObject(); // params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryGroup(HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "hostgroup.get");

	agent.startObject("params");
	agent.add("real_hosts", JSONTrue);
	agent.add("output", "extend");
	agent.add("selectHosts", "refer");
	agent.endObject(); //params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

SoupMessage *ZabbixAPI::queryApplication(
  const ItemCategoryIdVector &appIdVector, HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "application.get");

	agent.startObject("params");
	agent.add("output", "extend");
	if (!appIdVector.empty()) {
		agent.startArray("applicationids");
		ItemCategoryIdVecotrConstIterator it = appIdVector.begin();
		for (; it != appIdVector.end(); ++it)
			agent.add(*it);
		agent.endArray();
	}
	agent.endObject(); // params

	agent.add("auth", m_impl->authToken);
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}


ItemTablePtr ZabbixAPI::getFunctions(void)
{
	if (!m_impl->gotTriggers) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_INTERNAL_ERROR,
		  "Cache for 'functions' is empty. 'triggers' may not have "
		  "been retrieved.");
	}
	return ItemTablePtr(m_impl->functionsTablePtr);
}

SoupMessage *ZabbixAPI::queryCommon(JSONBuilder &agent, HatoholError &queryRet)
{
	string request_body = agent.generate();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, m_impl->uri.c_str());
	if (!msg) {
		MLPL_ERR("Failed to call: soup_message_new: uri: %s\n",
		         m_impl->uri.c_str());
		queryRet = HTERR_INTERNAL_ERROR;
		return NULL;
	}
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON_RPC, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	guint ret = soup_session_send_message(getSession(), msg);
	if (ret != SOUP_STATUS_OK) {
		g_object_unref(msg);
		MLPL_ERR("Failed to get from %s, Status: %d (%s)\n",
	                 m_impl->uri.c_str(),
			 ret, soup_status_get_phrase(ret));
		queryRet = HTERR_FAILED_CONNECT_ZABBIX;
		return NULL;
	}
	queryRet = HTERR_OK;
	return msg;
}

SoupMessage *ZabbixAPI::queryAPIVersion(HatoholError &queryRet)
{
	JSONBuilder agent;
	agent.startObject();
	agent.add("jsonrpc", "2.0");
	agent.add("method", "apiinfo.version");
	agent.add("id", 1);
	agent.endObject();

	return queryCommon(agent, queryRet);
}

string ZabbixAPI::getInitialJSONRequest(void)
{
	JSONBuilder agent;
	agent.startObject();
	agent.addNull("auth");
	agent.add("method", "user.login");
	agent.add("id", 1);

	agent.startObject("params");
	agent.add("user", m_impl->username);
	agent.add("password", m_impl->password);
	agent.endObject();

	agent.add("jsonrpc", "2.0");
	agent.endObject();

	return agent.generate();
}

bool ZabbixAPI::parseInitialResponse(SoupMessage *msg)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser: %s\n", parser.getErrorMessage());
		return false;
	}

	if (!parser.read("result", m_impl->authToken)) {
		MLPL_ERR("Failed to read: result\n");
		return false;
	}
	onUpdatedAuthToken(m_impl->authToken);
	return true;
}

void ZabbixAPI::startObject(JSONParser &parser, const string &name)
{
	if (!parser.startObject(name)) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		 HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to read object: %s", name.c_str());
	}
}

void ZabbixAPI::startElement(JSONParser &parser, const int &index)
{
	if (!parser.startElement(index)) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to start element: %d",index);
	}
}

#if 0 // See the comment in parseAndPushTriggerData()
void ArmZabbixAPI::pushFunctionsCache(JSONParser &parser)
{
	startObject(parser, "functions");
	int numFunctions = parser.countElements();
	for (int i = 0; i < numFunctions; i++) {
		VariableItemGroupPtr itemGroup;
		pushFunctionsCacheOne(parser, itemGroup, i);
		m_impl->functionsTablePtr->add(itemGroup);
	}
	parser.endObject();
}

void ArmZabbixAPI::pushFunctionsCacheOne(JSONParser &parser,
                                         ItemGroup *grp, int index)
{
	startElement(parser, index);
	pushUint64(parser, grp, "functionid", ITEM_ID_ZBX_FUNCTIONS_FUNCTIONID);
	pushUint64(parser, grp, "itemid",     ITEM_ID_ZBX_FUNCTIONS_ITEMID);
	grp->add(new ItemUint64(ITEM_ID_ZBX_FUNCTIONS_TRIGGERID,
	                        m_impl->triggerid), false);
	pushString(parser, grp, "function",   ITEM_ID_ZBX_FUNCTIONS_FUNCTION);
	pushString(parser, grp, "parameter",  ITEM_ID_ZBX_FUNCTIONS_PARAMETER);
	parser.endElement();
}
#endif

void ZabbixAPI::getString(JSONParser &parser, const string &name,
                          string &value)
{
	if (!parser.read(name.c_str(), value)) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_TO_PARSE_JSON_DATA,
		  "Failed to read: %s", name.c_str());
	}
}

int ZabbixAPI::pushInt(JSONParser &parser, ItemGroup *itemGroup,
                       const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	int valInt = atoi(value.c_str());
	itemGroup->add(new ItemInt(itemId, valInt), false);
	return valInt;
}

uint64_t ZabbixAPI::pushUint64(JSONParser &parser, ItemGroup *itemGroup,
                               const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	uint64_t valU64;
	sscanf(value.c_str(), "%" PRIu64, &valU64);
	itemGroup->add(new ItemUint64(itemId, valU64), false);
	return valU64;
}

string ZabbixAPI::pushString(JSONParser &parser, ItemGroup *itemGroup,
                             const string &name, const ItemId &itemId)
{
	string value;
	getString(parser, name, value);
	itemGroup->add(new ItemString(itemId, value), false);
	return value;
}

string ZabbixAPI::pushString(
  JSONParser &parser, ItemGroup *itemGroup,
  const string &name, const ItemId &itemId,
  const size_t &digitNum, const char &padChar)
{
	string value;
	getString(parser, name, value);
	int numPads = digitNum - value.size();
	string fixedValue;
	if (numPads > 0)
		fixedValue = string(numPads, padChar);
	fixedValue += value;
	itemGroup->add(new ItemString(itemId, fixedValue), false);
	return fixedValue;
}

void ZabbixAPI::parseAndPushTriggerData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "triggerid",   ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
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
	if (checkAPIVersion(2, 2, 0)) {
		// Zabbix 2.4 doesn't have "value_flags" property.
		// In addition it's already deprecated on Zabbix 2.2.
		grp->add(new ItemInt(ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS, 0),
			 false);
	} else {
		pushInt(parser, grp, "value_flags",
			ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS);
	}
	pushInt   (parser, grp, "flags",       ITEM_ID_ZBX_TRIGGERS_FLAGS);

	// get hostid
	pushTriggersHostId(parser, grp);

	tablePtr->add(grp);

	// We are no longer request 'functions'.
	// See also comments in mainThreadOneProc().
	//
	// get functions
	// pushFunctionsCache(parser);

	parser.endElement();
}

void ZabbixAPI::parseAndPushTriggerExpandedDescriptionData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "triggerid",   ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
	pushString(parser, grp, "description", ITEM_ID_ZBX_TRIGGERS_EXPANDED_DESCRIPTION);
	tablePtr->add(grp);
	parser.endElement();
}

void ZabbixAPI::parseAndPushItemsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "itemid",       ITEM_ID_ZBX_ITEMS_ITEMID);
	pushInt   (parser, grp, "type",         ITEM_ID_ZBX_ITEMS_TYPE);
	pushString(parser, grp, "snmp_community",
	           ITEM_ID_ZBX_ITEMS_SNMP_COMMUNITY);
	pushString(parser, grp, "snmp_oid",     ITEM_ID_ZBX_ITEMS_SNMP_OID);
	pushString(parser, grp, "hostid",       ITEM_ID_ZBX_ITEMS_HOSTID);
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
	pushApplicationId(parser, grp);

	tablePtr->add(grp);

	parser.endElement();
}

void ZabbixAPI::parseAndPushHostsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "hostid",       ITEM_ID_ZBX_HOSTS_HOSTID);
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

void ZabbixAPI::parseAndPushHostsGroupsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	startObject(parser, "groups");
	int numElement = parser.countElements();
	parser.endObject(); // Get number of element first.

	for (int i = 0; i < numElement; i++) {
		VariableItemGroupPtr grp;
		const uint64_t hostgroupId = 0;
		grp->addNewItem(hostgroupId);

		pushUint64(parser, grp, "hostid",
		           ITEM_ID_ZBX_HOSTS_GROUPS_HOSTID);
		startObject(parser, "groups");
		startElement(parser, i);
		pushUint64(parser, grp, "groupid",
		           ITEM_ID_ZBX_HOSTS_GROUPS_GROUPID);
		parser.endElement();
		parser.endObject();
		tablePtr->add(grp);
	}

	parser.endElement();
}

void ZabbixAPI::parseAndPushGroupsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushUint64(parser, grp, "groupid",      ITEM_ID_ZBX_GROUPS_GROUPID);
	pushString(parser, grp, "name",         ITEM_ID_ZBX_GROUPS_NAME);
	pushInt   (parser, grp, "internal",     ITEM_ID_ZBX_GROUPS_INTERNAL);
	tablePtr->add(grp);
	parser.endElement();
}

void ZabbixAPI::parseAndPushApplicationsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "applicationid",
	           ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID);
	pushString(parser, grp, "hostid", ITEM_ID_ZBX_APPLICATIONS_HOSTID);
	pushString(parser, grp, "name",   ITEM_ID_ZBX_APPLICATIONS_NAME);
	if (checkAPIVersion(2, 2, 0)) {
		// TODO: Zabbix 2.2 returns array of templateid, but Hatohol
		// stores only one templateid.
		parser.startObject("templateids");
		string value;
		uint64_t valU64 = 0;
		parser.read(0, value);
		sscanf(value.c_str(), "%" PRIu64, &valU64);
		grp->add(
		  new ItemUint64(ITEM_ID_ZBX_APPLICATIONS_TEMPLATEID, valU64),
		  false);
		parser.endObject();
	} else {
		pushUint64(parser, grp, "templateid",
			   ITEM_ID_ZBX_APPLICATIONS_TEMPLATEID);
	}
	tablePtr->add(grp);
	parser.endElement();
}

void ZabbixAPI::parseAndPushEventsData(
  JSONParser &parser, VariableItemTablePtr &tablePtr, const int &index)
{
	startElement(parser, index);
	VariableItemGroupPtr grp;
	pushString(parser, grp, "eventid",      ITEM_ID_ZBX_EVENTS_EVENTID,
	           EVENT_ID_DIGIT_NUM, '0');
	pushInt   (parser, grp, "source",       ITEM_ID_ZBX_EVENTS_SOURCE);
	pushInt   (parser, grp, "object",       ITEM_ID_ZBX_EVENTS_OBJECT);
	pushString(parser, grp, "objectid",     ITEM_ID_ZBX_EVENTS_OBJECTID);
	pushInt   (parser, grp, "clock",        ITEM_ID_ZBX_EVENTS_CLOCK);
	pushInt   (parser, grp, "value",        ITEM_ID_ZBX_EVENTS_VALUE);
	pushInt   (parser, grp, "acknowledged",
	           ITEM_ID_ZBX_EVENTS_ACKNOWLEDGED);
	pushInt   (parser, grp, "ns",           ITEM_ID_ZBX_EVENTS_NS);
	if (checkAPIVersion(2, 2, 0)) {
		// Zabbix 2.2 doesn't have "value_changed" property
		grp->add(new ItemInt(ITEM_ID_ZBX_EVENTS_VALUE_CHANGED, 0),
			 false);
	} else {
		pushInt(parser, grp, "value_changed",
			ITEM_ID_ZBX_EVENTS_VALUE_CHANGED);
	}
	tablePtr->add(grp);
	parser.endElement();
}

template <typename T>
void ZabbixAPI::pushSomethingId(
  JSONParser &parser, ItemGroup *itemGroup, const ItemId &itemId,
  const string &objectName, const string &elementName, const T &dummyValue)
{
	startObject(parser, objectName);
	int numElem = parser.countElements();
	if (numElem == 0) {
		const T dummyData = dummyValue;
		itemGroup->addNewItem(itemId, dummyData, ITEM_DATA_NULL);
	} else  {
		for (int i = 0; i < numElem; i++) {
			startElement(parser, i);
			pushString(parser, itemGroup, elementName, itemId);
			break; // we use the first applicationid
		}
		parser.endElement();
	}
	parser.endObject();
}

void ZabbixAPI::pushTriggersHostId(JSONParser &parser, ItemGroup *itemGroup)
{
	static const LocalHostIdType dummyHostName = "";
	pushSomethingId<LocalHostIdType>(
	  parser, itemGroup, ITEM_ID_ZBX_TRIGGERS_HOSTID,
	  "hosts", "hostid", dummyHostName);
}

void ZabbixAPI::pushApplicationId(JSONParser &parser, ItemGroup *itemGroup)
{
	pushSomethingId<ItemCategoryIdType>(
	  parser, itemGroup, ITEM_ID_ZBX_ITEMS_APPLICATIONID,
	  "applications", "applicationid", NO_ITEM_CATEGORY_ID);
}

ItemInfoValueType ZabbixAPI::toItemValueType(
  const ZabbixAPI::ValueType &valueType)
{
	switch (valueType) {
	case ZabbixAPI::VALUE_TYPE_FLOAT:
		return ITEM_INFO_VALUE_TYPE_FLOAT;
	case ZabbixAPI::VALUE_TYPE_INTEGER:
		return ITEM_INFO_VALUE_TYPE_INTEGER;
	case ZabbixAPI::VALUE_TYPE_STRING:
		return ITEM_INFO_VALUE_TYPE_STRING;
	case ZabbixAPI::VALUE_TYPE_LOG:
	case ZabbixAPI::VALUE_TYPE_TEXT:
	default:
		return ITEM_INFO_VALUE_TYPE_UNKNOWN;
	}
}

ZabbixAPI::ValueType ZabbixAPI::fromItemValueType(
  const ItemInfoValueType &valueType)
{
	switch (valueType) {
	case ITEM_INFO_VALUE_TYPE_FLOAT:
		return ZabbixAPI::VALUE_TYPE_FLOAT;
	case ITEM_INFO_VALUE_TYPE_INTEGER:
		return ZabbixAPI::VALUE_TYPE_INTEGER;
	case ITEM_INFO_VALUE_TYPE_STRING:
		return ZabbixAPI::VALUE_TYPE_STRING;
	default:
		// should detect at caller side by fetching the item
		return ZabbixAPI::VALUE_TYPE_UNKNOWN;
	}
}
