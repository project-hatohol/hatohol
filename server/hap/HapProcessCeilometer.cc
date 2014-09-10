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
};


struct HapProcessCeilometer::Impl {
	string osUsername;
	string osPassword;
	string osTenantName;
	string osAuthURL;

	MonitoringServerInfo serverInfo;
	string token;
	OpenStackEndPoint ceilometerEP;

	Impl(void)
	{
		// Temporary paramters
		osUsername   = "admin";
		osPassword   = "admin";
		osTenantName = "admin";
		osAuthURL    = "http://botctl:35357/v2.0";
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
void HapProcessCeilometer::updateAuthTokenIfNeeded(void)
{
	if (!m_impl->token.empty())
		return;

	string url = m_impl->osAuthURL;
	url += "/tokens";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, url.c_str());
	Reaper<void> msgReaper(msg, g_object_unref);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
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

	string request_body = builder.generate();
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         request_body.c_str(), request_body.size());
	SoupSession *session = soup_session_sync_new();
	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to connect: %d, URL: %s\n", ret, url.c_str());
		return;
	}
	if (!parseReplyToknes(msg)) {
		MLPL_DBG("body: %" G_GOFFSET_FORMAT ", %s\n",
		         msg->response_body->length, msg->response_body->data);
		return;
	}
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

	if (!read(parser, "id", m_impl->token))
		return false;

	parser.endObject(); // access
	if (!startObject(parser, "serviceCatalog"))
		return false;

	const unsigned int count = parser.countElements();
	for (unsigned int idx = 0; idx < count; idx++) {
		if (!parserEndpoints(parser, idx))
			return false;
		if (!m_impl->ceilometerEP.publicURL.empty())
			break;
	}

	// check if there's information about the endpoint of ceilometer
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
	if (name != "ceilometer")
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
		                      m_impl->ceilometerEP.publicURL);
		parser.endElement();
		if (!succeeded)
			return false;

		// NOTE: Currently we only use the first information even if
		// there're multiple endpoints
		break;
	}

	return true;
}

HatoholError HapProcessCeilometer::getAlarmList(void)
{
	string url = m_impl->ceilometerEP.publicURL;
	url += "/v2/alarms";
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, url.c_str());
	Reaper<void> msgReaper(msg, g_object_unref);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	soup_message_headers_append(msg->request_headers,
	                            "X-Auth-Token", m_impl->token.c_str());
	SoupSession *session = soup_session_sync_new();
	guint ret = soup_session_send_message(session, msg);
	if (ret != SOUP_STATUS_OK) {
		MLPL_ERR("Failed to connect: %d, URL: %s\n", ret, url.c_str());
		return HTERR_BAD_REST_RESPONSE_CEILOMETER;
	}
	VariableItemTablePtr trigTablePtr;
	HatoholError err = parseReplyGetAlarmList(msg, trigTablePtr);
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
	if (num != NUM_EXPECT_ELEM)
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

	// TODO: parse HOST ID from data like the following.
	//  /1/threshold_rule/query/0/field "resource"
	//  /1/threshold_rule/query/0/value "eb79c816-8994-4e21-a26c-51d7d19d0e45"
	//  /1/threshold_rule/query/0/op    "eq"
	HostIdType hostId = INAPPLICABLE_HOST_ID;
	string hostName = "No name";

	// brief. We use the alarm name as a brief.
	if (!parserRewinder.pushObject("threshold_rule"))
		return HTERR_FAILED_TO_PARSE_JSON_DATA;

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

	const unsigned int count = parser.countElements();
	for (unsigned int i = 0; i < count; i++) {
		HatoholError err = parseAlarmElement(parser, tablePtr, i);
		if (err != HTERR_OK)
			return err;
	}
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

void HapProcessCeilometer::acquireData(void)
{
	MLPL_DBG("acquireData\n");
	updateAuthTokenIfNeeded();
	getAlarmList(); // Trigger
	// TODO: Add get trigger, event, and so on.
}

