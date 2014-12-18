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

#include <SimpleSemaphore.h>
#include <Mutex.h>
#include <Reaper.h>
#include <stack>
#include <cstring>
#include <libsoup/soup.h>
#include "Utils.h"
#include "JSONBuilder.h"
#include "JSONParser.h"
#include "HapProcessCeilometer.h"

using namespace std;
using namespace mlpl;

static const char *MIME_JSON = "application/json";

struct OpenStackEndPoint {
	string publicURL;

	void clear(void)
	{
		publicURL.clear();
	}
};

struct AcquireContext
{
	StringVector  alarmIds;

	static void clear(AcquireContext *ctx)
	{
		ctx->alarmIds.clear();
	}
};

struct HapProcessCeilometer::HttpRequestArg
{
	const char *method;
	string      url;
	string      body;
	bool        useAuthToken;
	mlpl::Reaper<SoupMessage> msgPtr;

	HttpRequestArg(const char *_method, const string &_url)
	: method(_method),
	  url(_url),
	  useAuthToken(true)
	{
	}
};

struct HapProcessCeilometer::Impl {
	string osUsername;
	string osPassword;
	string osTenantName;
	string osAuthURL;

	MonitoringServerInfo serverInfo;
	string token;
	OpenStackEndPoint ceilometerEP;
	OpenStackEndPoint novaEP;
	SmartTime         tokenExpires;
	AcquireContext    acquireCtx;

	Impl(void)
	{
	}

	void clear(void)
	{
		token.clear();
		ceilometerEP.clear();
		novaEP.clear();
		tokenExpires = SmartTime();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessCeilometer::HapProcessCeilometer(int argc, char *argv[])
: HapProcessStandard(argc, argv),
  m_impl(new Impl())
{
}

HapProcessCeilometer::~HapProcessCeilometer()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool HapProcessCeilometer::canSkipAuthentification(void)
{
	if (m_impl->token.empty())
		return false;

	if (SmartTime(SmartTime::INIT_CURR_TIME) >= m_impl->tokenExpires)
		return false;

	return true;
}

HatoholError HapProcessCeilometer::updateAuthTokenIfNeeded(void)
{
	if (canSkipAuthentification())
		return HTERR_OK;

	string url = m_impl->osAuthURL;
	url += "/tokens";

	JSONBuilder builder;
	builder.startObject();
	builder.startObject("auth");
	builder.add("tenantName", m_impl->osTenantName);
	builder.startObject("passwordCredentials");
	builder.add("username", m_impl->osUsername);
	builder.add("password", m_impl->osPassword);
	builder.endObject(); // passwordCredentials
	builder.endObject(); // auth
	builder.endObject();

	HttpRequestArg arg(SOUP_METHOD_POST, url);
	arg.useAuthToken = false;
	arg.body = builder.generate();
	HatoholError err = sendHttpRequest(arg);
	if (err != HTERR_OK)
		return err;
	SoupMessage *msg = arg.msgPtr.get();
	if (!parseReplyToknes(msg)) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	return HTERR_OK;
}

bool HapProcessCeilometer::parseReplyToknes(SoupMessage *msg)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser %s\n", parser.getErrorMessage());
		return false;
	}

	if (!startObject(parser, "access"))
		return false;
	if (!startObject(parser, "token"))
		return false;

	// Toekn ID
	if (!read(parser, "id", m_impl->token))
		return false;

	// Expiration
	SmartTime tokenExpires(SmartTime::INIT_CURR_TIME);
	string issuedAt, expires;
	if (!read(parser, "issued_at", issuedAt))
		return false;
	if (!read(parser, "expires", expires))
		return false;
	SmartTime validDuration = parseStateTimestamp(expires);
	validDuration -= parseStateTimestamp(issuedAt);
	tokenExpires += validDuration.getAsTimespec();
	m_impl->tokenExpires = tokenExpires;
	const timespec margin = {5 * 60, 0};
	m_impl->tokenExpires -= SmartTime(margin);
	MLPL_DBG("Token expires: %s\n", ((string)m_impl->tokenExpires).c_str());

	parser.endObject(); // access
	if (!startObject(parser, "serviceCatalog"))
		return false;

	const unsigned int count = parser.countElements();
	for (unsigned int idx = 0; idx < count; idx++) {
		if (!parserEndpoints(parser, idx))
			return false;
		if (!m_impl->ceilometerEP.publicURL.empty() &&
		    !m_impl->novaEP.publicURL.empty())
			break;
	}

	// check if there's information about the endpoint of ceilometer
	if (m_impl->novaEP.publicURL.empty()) {
		MLPL_ERR("Failed to get an endpoint of nova\n");
		return false;
	}
	if (m_impl->ceilometerEP.publicURL.empty()) {
		MLPL_ERR("Failed to get an endpoint of ceilometer\n");
		return false;
	}
	return true;
}

bool HapProcessCeilometer::parserEndpoints(JSONParser &parser,
                                    const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (! parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return false;
	}

	string name;
	if (!read(parser, "name", name))
		return false;
	OpenStackEndPoint *osEndPoint = NULL;
	if (name == "ceilometer")
		osEndPoint = &m_impl->ceilometerEP;
	else if (name == "nova")
		osEndPoint = &m_impl->novaEP;
	else
		return true;

	if (!parserRewinder.pushObject("endpoints"))
		return false;

	const unsigned int count = parser.countElements();
	for (unsigned int i = 0; i < count; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse an element, index: %u\n", i);
			return false;
		}
		bool succeeded = read(parser, "publicURL",
		                      osEndPoint->publicURL);
		parser.endElement();
		if (!succeeded)
			return false;

		// NOTE: Currently we only use the first information even if
		// there're multiple endpoints
		break;
	}

	return true;
}

HatoholError HapProcessCeilometer::sendHttpRequest(HttpRequestArg &arg)
{
	const string &url = arg.url;
	HATOHOL_ASSERT(arg.method, "Method is not set.");
	HATOHOL_ASSERT(!url.empty(), "URL is not set.");
	SoupMessage *msg = soup_message_new(arg.method, url.c_str());
	if (!msg) {
		MLPL_ERR("Failed create SoupMessage: URL: %s\n", url.c_str());
		return HTERR_INVALID_URL;
	}
	HATOHOL_ASSERT(
	  arg.msgPtr.set(msg, (void (*)(SoupMessage*))g_object_unref),
	  "msgPtr seem to already have the pointer.");
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	if (arg.useAuthToken) {
		soup_message_headers_append(
		  msg->request_headers, "X-Auth-Token", m_impl->token.c_str());
	}
	if (!arg.body.empty()) {
		soup_message_body_append(msg->request_body,
		                         SOUP_MEMORY_TEMPORARY,
		                         arg.body.c_str(), arg.body.size());
	}
	SoupSession *session = soup_session_sync_new();
	guint ret = soup_session_send_message(session, msg);
	g_object_unref(session);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to connect: (%d) %s, URL: %s\n",
		         ret, soup_status_get_phrase(ret), url.c_str());
		return HTERR_BAD_REST_RESPONSE;
	}
	return HTERR_OK;
}

HatoholError HapProcessCeilometer::getInstanceList(void)
{
	string url = m_impl->novaEP.publicURL;
	url += "/servers/detail?all_tenants=1";
	HttpRequestArg arg(SOUP_METHOD_GET, url);
	HatoholError err = sendHttpRequest(arg);
	if (err != HTERR_OK)
		return err;
	SoupMessage *msg = arg.msgPtr.get();

	VariableItemTablePtr hostTablePtr;
	err = parseReplyInstanceList(msg, hostTablePtr);
	if (err != HTERR_OK) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
	} else if (hostTablePtr->getNumberOfRows() == 0) {
		return HTERR_OK;
	}
	sendTable(HAPI_CMD_SEND_HOSTS, static_cast<ItemTablePtr>(hostTablePtr));
	return err;
}

HatoholError HapProcessCeilometer::parseReplyInstanceList(
  SoupMessage *msg, VariableItemTablePtr &tablePtr)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser %s\n", parser.getErrorMessage());
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	if (!startObject(parser, "servers"))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	HatoholError err(HTERR_OK);
	const unsigned int count = parser.countElements();
	for (unsigned int i = 0; i < count; i++) {
		err = parseInstanceElement(parser, tablePtr, i);
		if (err != HTERR_OK)
			break;
	}
	return err;
}

HatoholError HapProcessCeilometer::parseInstanceElement(
  JSONParser &parser, VariableItemTablePtr &tablePtr,
  const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (!parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	// ID
	string id;
	if (!read(parser, "id", id))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// name
	string name;
	if (!read(parser, "name", name))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// tenant id
	string tenantId;
	if (!read(parser, "tenant_id", tenantId))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// user id
	string userId;
	if (!read(parser, "user_id", userId))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// NOTE: tenantId and userID is not used currently.
	// We will use them as elements for the host group.

	// fill
	// TODO: Define ItemID without ZBX.
	VariableItemGroupPtr grp;
	grp->addNewItem(ITEM_ID_ZBX_HOSTS_HOSTID, generateHashU64(id));
	grp->addNewItem(ITEM_ID_ZBX_HOSTS_NAME, name);
	tablePtr->add(grp);

	return HTERR_OK;
}

HatoholError HapProcessCeilometer::getAlarmList(void)
{
	string url = m_impl->ceilometerEP.publicURL;
	url += "/v2/alarms";
	HttpRequestArg arg(SOUP_METHOD_GET, url);
	HatoholError err = sendHttpRequest(arg);
	if (err != HTERR_OK)
		return err;
	SoupMessage *msg = arg.msgPtr.get();

	VariableItemTablePtr trigTablePtr;
	err = parseReplyGetAlarmList(msg, trigTablePtr);
	if (err != HTERR_OK) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
	}
	sendTable(HAPI_CMD_SEND_UPDATED_TRIGGERS,
	          static_cast<ItemTablePtr>(trigTablePtr));
	return err;
}

TriggerStatusType HapProcessCeilometer::parseAlarmState(const string &state)
{
	if (state == "ok")
		return TRIGGER_STATUS_OK;
	if (state == "insufficient data")
		return TRIGGER_STATUS_UNKNOWN;
	if (state == "alarm")
		return TRIGGER_STATUS_PROBLEM;
	MLPL_ERR("Unknown alarm: %s\n", state.c_str());
	return TRIGGER_STATUS_UNKNOWN;
}

SmartTime HapProcessCeilometer::parseStateTimestamp(
  const string &stateTimestamp)
{
	int year  = 0;
	int month = 0;
	int day   = 0;
	int hour  = 0;
	int min   = 0;
	int sec   = 0;
	int us    = 0;
	const size_t num = sscanf(stateTimestamp.c_str(),
	                          "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
	                          &year, &month, &day, &hour, &min, &sec, &us);
	const size_t NUM_EXPECT_ELEM = 7;
	// We sometimes get the timestamp without the usec part.
	// So we also accept the result with NUM_EXPECT_ELEM-1.
	if (num != NUM_EXPECT_ELEM-1 && num != NUM_EXPECT_ELEM)
		MLPL_ERR("Failed to parser time: %s\n", stateTimestamp.c_str());

	tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_sec  = sec;
	tm.tm_min  = min;
	tm.tm_hour = hour;
	tm.tm_mday = day;
	tm.tm_mon  = month - 1;
	tm.tm_year = year - 1900;
	const timespec ts = {mktime(&tm), us*1000};
	return SmartTime(ts);
}

uint64_t HapProcessCeilometer::generateHashU64(const string &str)
{
	GChecksum *checkSum =  g_checksum_new(G_CHECKSUM_MD5);
	g_checksum_update(checkSum, (const guchar *)str.c_str(), str.size());

	gsize len = 16;
	guint8 buf[len];
	g_checksum_get_digest(checkSum, buf, &len);
	g_checksum_free(checkSum);

	uint64_t csum64;
	memcpy(&csum64, buf, sizeof(uint64_t));
	return csum64;
}

HatoholError HapProcessCeilometer::parseAlarmElement(
  JSONParser &parser, VariableItemTablePtr &tablePtr,
  const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (!parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	// trigger ID (alarm_id)
	string alarmId;
	if (!read(parser, "alarm_id", alarmId))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	// TODO: Fix a structure to save ID.
	// We temporarily generate the 64bit triggerID and host ID from UUID.
	// Strictly speaking, this way is not safe.
	const uint64_t triggerId = generateHashU64(alarmId);

	// status
	string state;
	if (!read(parser, "state", state))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	const TriggerStatusType status = parseAlarmState(state);

	// severity
	// NOTE: Ceilometer doesn't have a concept of severity.
	// We use one fixed level.
	const TriggerSeverityType severity = TRIGGER_SEVERITY_ERROR;

	// last change time
	string stateTimestamp;
	if (!read(parser, "state_timestamp", stateTimestamp))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	SmartTime lastChangeTime = parseStateTimestamp(stateTimestamp);

	if (!parserRewinder.pushObject("threshold_rule"))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// Try to get a host ID
	HostIdType hostId = INVALID_HOST_ID;
	HatoholError err = parseAlarmHost(parser, hostId);
	if (err != HTERR_OK)
		return err;
	if (hostId == INVALID_HOST_ID)
		hostId = INAPPLICABLE_HOST_ID;

	// brief. We use the alarm name as a brief.
	string meterName;
	if (!read(parser, "meter_name", meterName))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	string brief = meterName;

	// fill
	// TODO: Define ItemID without ZBX.
	VariableItemGroupPtr grp;
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_TRIGGERID,   triggerId);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_VALUE,       status);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_PRIORITY,    severity);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_LASTCHANGE,
	                (int)lastChangeTime.getAsTimespec().tv_sec);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_DESCRIPTION, brief);
	grp->addNewItem(ITEM_ID_ZBX_TRIGGERS_HOSTID,      hostId);
	tablePtr->add(grp);

	// Register the Alarm ID
	m_impl->acquireCtx.alarmIds.push_back(alarmId);

	return HTERR_OK;
}

HatoholError HapProcessCeilometer::parseReplyGetAlarmList(
  SoupMessage *msg, VariableItemTablePtr &tablePtr)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser %s\n", parser.getErrorMessage());
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	HatoholError err(HTERR_OK);
	const unsigned int count = parser.countElements();
	m_impl->acquireCtx.alarmIds.reserve(count);
	for (unsigned int i = 0; i < count; i++) {
		err = parseAlarmElement(parser, tablePtr, i);
		if (err != HTERR_OK)
			break;
	}
	return err;
}

HatoholError HapProcessCeilometer::getAlarmHistories(void)
{
	HatoholError err(HTERR_OK);
	for (size_t i = 0; i < m_impl->acquireCtx.alarmIds.size(); i++) {
		err = getAlarmHistory(i);
		if (err != HTERR_OK) {
			MLPL_ERR("Failed to get alarm history: %s\n",
			         m_impl->acquireCtx.alarmIds[i].c_str());
		}
	}
	return err;
}

string  HapProcessCeilometer::getHistoryQueryOption(
  const SmartTime &lastTime)
{
	if (!lastTime.hasValidTime())
		return "";

	tm tim;
	const timespec &ts = lastTime.getAsTimespec();
	HATOHOL_ASSERT(
	  localtime_r(&ts.tv_sec, &tim),
	  "Failed to call gmtime_r(): %s\n", ((string)lastTime).c_str());
	string query = StringUtils::sprintf(
	  "?q.field=timestamp&q.op=gt&q.value="
	  "%04d-%02d-%02dT%02d%%3A%02d%%3A%02d.%06ld",
	  1900+tim.tm_year, tim.tm_mon + 1, tim.tm_mday,
	  tim.tm_hour, tim.tm_min, tim.tm_sec, ts.tv_nsec/1000);
	return query;
}

HatoholError HapProcessCeilometer::getAlarmHistory(const unsigned int &index)
{
	const char *alarmId = m_impl->acquireCtx.alarmIds[index].c_str();
	const SmartTime lastTime = getTimeOfLastEvent(generateHashU64(alarmId));
	string url = StringUtils::sprintf(
	               "%s/v2/alarms/%s/history%s",
	               m_impl->ceilometerEP.publicURL.c_str(),
	               alarmId, getHistoryQueryOption(lastTime).c_str());
	HttpRequestArg arg(SOUP_METHOD_GET, url);
	HatoholError err = sendHttpRequest(arg);
	if (err != HTERR_OK)
		return err;
	SoupMessage *msg = arg.msgPtr.get();

	AlarmTimeMap alarmTimeMap;
	err = parseReplyGetAlarmHistory(msg, alarmTimeMap);
	if (err != HTERR_OK) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
		return err;
	}

	MLPL_DBG("The numebr of updated alarms: %zd url: %s\n",
	         alarmTimeMap.size(), url.c_str());
	if (alarmTimeMap.empty())
		return HTERR_OK;

	// Build the table
	VariableItemTablePtr eventTablePtr;
	AlarmTimeMapConstIterator it = alarmTimeMap.begin();
	for (; it != alarmTimeMap.end(); ++it) {
		const ItemGroupPtr &historyElement = it->second;
		eventTablePtr->add(historyElement);
	}
	sendTable(HAPI_CMD_SEND_UPDATED_EVENTS,
	          static_cast<ItemTablePtr>(eventTablePtr));
	return HTERR_OK;
}

HatoholError HapProcessCeilometer::parseReplyGetAlarmHistory(
  SoupMessage *msg, AlarmTimeMap &alarmTimeMap)
{
	JSONParser parser(msg->response_body->data);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parser %s\n", parser.getErrorMessage());
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	HatoholError err(HTERR_OK);
	const unsigned int count = parser.countElements();
	for (unsigned int i = 0; i < count; i++) {
		err = parseReplyGetAlarmHistoryElement(parser, alarmTimeMap, i);
		if (err != HTERR_OK)
			break;
	}
	return err;
}

HatoholError HapProcessCeilometer::parseReplyGetAlarmHistoryElement(
  JSONParser &parser, AlarmTimeMap &alarmTimeMap, const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (! parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	// Event ID
	string eventIdStr;
	if (!read(parser, "event_id", eventIdStr))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	// TODO: Fix a structure to save ID.
	// We temporarily generate the 64bit triggerID and host ID from UUID.
	// Strictly speaking, this way is not safe.
	const uint64_t eventId = generateHashU64(eventIdStr);

	// Timestamp
	string timestampStr;
	if (!read(parser, "timestamp", timestampStr))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	const SmartTime timestamp = parseStateTimestamp(timestampStr);

	// type
	string typeStr;
	if (!read(parser, "type", typeStr))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

	// Status
	EventType type = EVENT_TYPE_UNKNOWN;
	if (typeStr == "creation" || typeStr == "state transition") {
		string detail;
		if (!read(parser, "detail", detail))
			return HTERR_FAILED_TO_PARSE_JSON_DATA;
		type = parseAlarmHistoryDetail(detail);
	} else {
		MLPL_BUG("Unknown type: %s\n", typeStr.c_str());
	}

	// Trigger ID (alarm ID)
	string alarmIdStr;
	if (!read(parser, "alarm_id", alarmIdStr))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	const uint64_t alarmId = generateHashU64(alarmIdStr);

	// Fill table.
	// TODO: Define ItemID without ZBX.
	// TODO: This is originally defined in HatoholDBUtils.cc.
	//       But this Zabbix dependent constant shouldn't be used.
	static const int EVENT_OBJECT_TRIGGER = 0;
	const timespec &ts = timestamp.getAsTimespec();
	VariableItemGroupPtr grp;
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_EVENTID,   eventId);
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_OBJECT,    EVENT_OBJECT_TRIGGER);
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_OBJECTID,  alarmId);
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_CLOCK,     (int)ts.tv_sec);
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_VALUE,     type);
	grp->addNewItem(ITEM_ID_ZBX_EVENTS_NS,        (int)ts.tv_nsec);
	grp->freeze();
	alarmTimeMap.insert(pair<SmartTime, ItemGroupPtr>(timestamp,
	                                                  (ItemGroupPtr)grp));
	return HTERR_OK;
}

EventType HapProcessCeilometer::parseAlarmHistoryDetail(
  const std::string &detail)
{
	JSONParser parser(detail);
	if (parser.hasError()) {
		MLPL_ERR("Failed to parse: %s\n", detail.c_str());
		return EVENT_TYPE_UNKNOWN;
	}

	string state;
	if (!read(parser, "state", state)) {
		MLPL_ERR("Not found 'state': %s\n", detail.c_str());
		return EVENT_TYPE_UNKNOWN;
	}

	if (state == "ok")
		return EVENT_TYPE_GOOD;
	else if (state == "alarm")
		return EVENT_TYPE_BAD;
	else if (state == "insufficient data")
		return EVENT_TYPE_UNKNOWN;
	MLPL_ERR("Unknown state: %s\n", state.c_str());
	return EVENT_TYPE_UNKNOWN;
}

HatoholError HapProcessCeilometer::parseAlarmHost(
  JSONParser &parser, HostIdType &hostId)
{
	// This method and parseAlarmHostEach() return HTERR_OK when an element
	// doesn't exists. It is a possible cause.
	JSONParser::PositionStack parserRewinder(parser);
	if (!parserRewinder.pushObject("query"))
		return HTERR_OK;

	const unsigned int count = parser.countElements();
	for (unsigned int idx = 0; idx < count; idx++) {
		HatoholError err = parseAlarmHostEach(parser, hostId, idx);
		if (err != HTERR_OK)
			return err;
	}
	return HTERR_OK;
}

HatoholError HapProcessCeilometer::parseAlarmHostEach(
  JSONParser &parser, HostIdType &hostId, const unsigned int &index)
{
	JSONParser::PositionStack parserRewinder(parser);
	if (!parserRewinder.pushElement(index)) {
		MLPL_ERR("Failed to parse an element, index: %u\n", index);
		return HTERR_FAILED_TO_PARSE_JSON_DATA;
	}

	// field
	string field;
	if (!read(parser, "field", field))
		return HTERR_OK;
	if (field != "resource") {
		MLPL_INFO("Unknown field: %s\n", field.c_str());
		return HTERR_OK;
	}

	// field
	string value;
	if (!read(parser, "value", value))
		return HTERR_OK;

	// op
	string op;
	if (!read(parser, "op", op))
		return HTERR_OK;
	if (op != "eq") {
		MLPL_INFO("Unknown operator: %s\n", op.c_str());
		return HTERR_OK;
	}

	hostId = generateHashU64(value);
	return HTERR_OK;
}

bool HapProcessCeilometer::startObject(
  JSONParser &parser, const string &name)
{
	if (parser.startObject(name))
		return true;
	MLPL_ERR("Not found object: %s\n", name.c_str());
	return false;
}

bool HapProcessCeilometer::read(
  JSONParser &parser, const string &member, string &dest)
{
	if (parser.read(member, dest))
		return true;
	MLPL_ERR("Failed to read: %s\n", member.c_str());
	return false;
}

HatoholError HapProcessCeilometer::acquireData(void)
{
	Reaper<AcquireContext>
	  acqCtxCleaner(&m_impl->acquireCtx, AcquireContext::clear);

	MLPL_DBG("acquireData\n");
	m_impl->serverInfo = getMonitoringServerInfo();
	m_impl->osUsername   = m_impl->serverInfo.userName;
	m_impl->osPassword   = m_impl->serverInfo.password;
	m_impl->osTenantName = m_impl->serverInfo.dbName;
	m_impl->osAuthURL    = m_impl->serverInfo.hostName;

	HatoholError (HapProcessCeilometer::*funcs[])(void) = {
		&HapProcessCeilometer::updateAuthTokenIfNeeded,
		&HapProcessCeilometer::getInstanceList,     // Host
		&HapProcessCeilometer::getAlarmList,        // Trigger
		&HapProcessCeilometer::getAlarmHistories,   // Event
	};

	for (size_t i = 0; i < ARRAY_SIZE(funcs); i++) {
		try {
			HatoholError err = (this->*funcs[i])();
			if (err != HTERR_OK) {
				m_impl->clear();
				return err;
			}
		} catch (...) {
			m_impl->clear();
			throw;
		}
	}
	return HTERR_OK;
}

