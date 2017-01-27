/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#include <cmath>

#include "GateJSONEventMessage.h"

using namespace std;
using namespace mlpl;

const size_t GateJSONEventMessage::EVENT_ID_DIGIT_NUM = 20;

struct GateJSONEventMessage::Impl
{
	JsonNode *m_root;
	JsonObject *m_body;

	Impl(JsonNode *root)
	: m_root(root),
	  m_body(NULL)
	{
	}

	~Impl()
	{
	}

	bool validate(StringList &errors)
	{
		if (!validateEnvelope(errors))
			return false;

		JsonObject *rootObject = json_node_get_object(m_root);
		JsonObject *body =
			json_object_get_object_member(rootObject, "body");
		if (!validateEventBody(errors, body))
			return false;

		return true;
	}

	int64_t getID()
	{
		return json_object_get_int_member(getBody(), "id");
	}

	timespec getTimestamp()
	{
		timespec timestamp;
		JsonNode *timestampNode =
			json_object_get_member(getBody(), "timestamp");
		GType type = json_node_get_value_type(timestampNode);
		if (type == G_TYPE_DOUBLE) {
			gdouble rawTimestamp =
				json_node_get_double(timestampNode);
			timestamp.tv_sec = static_cast<time_t>(rawTimestamp);
			timestamp.tv_nsec =
				secondToNanoSecond(fmod(rawTimestamp, 1.0));
		} else {
			const gchar *rawTimestamp =
				json_node_get_string(timestampNode);
			GTimeVal timeValue;
			g_time_val_from_iso8601(rawTimestamp, &timeValue);
			timestamp.tv_sec = timeValue.tv_sec;
			timestamp.tv_nsec =
				microSecondToNanoSecond(timeValue.tv_usec);
		}
		return timestamp;
	}

	const gchar *getHostName()
	{
		return json_object_get_string_member(getBody(), "hostName");
	}

	const gchar *getContent()
	{
		return json_object_get_string_member(getBody(), "content");
	}

	TriggerSeverityType getSeverity()
	{
		JsonNode *node = json_object_get_member(getBody(), "severity");
		if (!node) {
			return TRIGGER_SEVERITY_UNKNOWN;
		}

		const string severity(json_node_get_string(node));
		return parseSeverity(severity);
	}

	EventType getType()
	{
		JsonNode *node = json_object_get_member(getBody(), "type");
		if (!node) {
			return EVENT_TYPE_BAD;
		}

		const string type(json_node_get_string(node));
		return parseType(type);
	}

private:
	JsonObject *getBody()
	{
		if (!m_body) {
			JsonObject *rootObject = json_node_get_object(m_root);
			JsonNode *bodyNode =
				json_object_get_member(rootObject, "body");
			m_body = json_node_get_object(bodyNode);
		}

		return m_body;
	}

	long secondToNanoSecond(gdouble second)
	{
		return second * 1000000000;
	}

	long microSecondToNanoSecond(long microSecond)
	{
		return microSecond * 1000;
	}

	TriggerSeverityType parseSeverity(const string &severity)
	{
		if (severity == "unknown") {
			return TRIGGER_SEVERITY_UNKNOWN;
		} else if (severity == "info") {
			return TRIGGER_SEVERITY_INFO;
		} else if (severity == "warning") {
			return TRIGGER_SEVERITY_WARNING;
		} else if (severity == "error") {
			return TRIGGER_SEVERITY_ERROR;
		} else if (severity == "critical") {
			return TRIGGER_SEVERITY_CRITICAL;
		} else if (severity == "emergency") {
			return TRIGGER_SEVERITY_EMERGENCY;
		} else {
			return NUM_TRIGGER_SEVERITY;
		}
	}

	static const EventType EVENT_TYPE_INVALID = static_cast<EventType>(-1);
	EventType parseType(const string &type)
	{
		if (type == "good") {
			return EVENT_TYPE_GOOD;
		} else if (type == "bad") {
			return EVENT_TYPE_BAD;
		} else if (type == "unknown") {
			return EVENT_TYPE_UNKNOWN;
		} else if (type == "notification") {
			return EVENT_TYPE_NOTIFICATION;
		} else {
			return EVENT_TYPE_INVALID;
		}
	}

	void addError(StringList &errors,
		      const char *format,
		      ...)
	{
		va_list ap;
		va_start(ap, format);
		using namespace mlpl::StringUtils;
		errors.push_back(vsprintf(format, ap));
		va_end(ap);
	}

	string inspectNode(JsonNode *node)
	{
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, node);
		gchar *rawJSON = json_generator_to_data(generator, NULL);
		string JSON(rawJSON);
		g_free(rawJSON);
		g_object_unref(generator);
		return JSON;
	}

	bool validateObjectMemberExistence(StringList &errors,
					   const gchar *context,
					   JsonObject *object,
					   const gchar *name)
	{
		JsonNode *memberNode = json_object_get_member(object, name);
		if (!memberNode) {
			addError(errors, "%s.%s must exist", context, name);
			return false;
		}

		return true;
	}

	bool validateObjectMember(StringList &errors,
				  const gchar *context,
				  JsonObject *object,
				  const gchar *name,
				  GType expectedValueType)
	{
		if (!validateObjectMemberExistence(errors, context, object, name)) {
			return false;
		}

		JsonNode *memberNode = json_object_get_member(object, name);
		GType valueType = json_node_get_value_type(memberNode);
		if (valueType != expectedValueType) {
			addError(errors,
				 "%s.%s must be %s: %s <%s>",
				 context,
				 name,
				 g_type_name(expectedValueType),
				 g_type_name(valueType),
				 inspectNode(memberNode).c_str());
			return false;
		}

		return true;
	}

	bool isValidTimeNode(JsonNode *node)
	{
		GType valueType = json_node_get_value_type(node);
		if (valueType == G_TYPE_DOUBLE) {
			return true;
		}

		if (valueType == G_TYPE_STRING) {
			const gchar *nodeValue = json_node_get_string(node);
			GTimeVal timeValue;
			return g_time_val_from_iso8601(nodeValue, &timeValue);
		}

		return false;
	}

	bool validateObjectMemberTime(StringList &errors,
				      const gchar *context,
				      JsonObject *object,
				      const gchar *name)
	{
		if (!validateObjectMemberExistence(errors, context, object, name)) {
			return false;
		}

		JsonNode *memberNode = json_object_get_member(object, name);
		if (!isValidTimeNode(memberNode)) {
			addError(errors,
				 "%s.%s must be "
				 "UNIX time in double or "
				 "ISO 8601 string: "
				 "<%s>",
				 context,
				 name,
				 inspectNode(memberNode).c_str());
			return false;
		}

		return true;
	}

	bool validateEnvelope(StringList &errors)
	{
		if (json_node_get_node_type(m_root) != JSON_NODE_OBJECT) {
			addError(errors, "JSON message must be an object");
			return false;
		}

		JsonObject *rootObject = json_node_get_object(m_root);
		if (!validateObjectMember(errors, "$", rootObject, "type",
					  G_TYPE_STRING))
			return false;
		if (!validateObjectMember(errors, "$", rootObject, "body",
					  JSON_TYPE_OBJECT))
			return false;

		const gchar *type =
			json_object_get_string_member(rootObject, "type");
		if (string(type) != "event") {
			addError(errors, "$.type must be \"event\" for now");
			return false;
		}

		return true;
	}

	bool validateBodyMember(StringList &errors,
				JsonObject *body,
				const gchar *name,
				GType expectedValueType)
	{
		return validateObjectMember(errors, "$.body", body, name,
					    expectedValueType);
	}

	bool validateBodyMemberTime(StringList &errors,
				    JsonObject *body,
				    const gchar *name)
	{
		return validateObjectMemberTime(errors, "$.body", body, name);
	}

	bool validateStringBodyMember(StringList &errors,
				      JsonObject *body,
				      const gchar *name,
				      JsonNode *memberNode)
	{
		GType valueType = json_node_get_value_type(memberNode);
		if (valueType != G_TYPE_STRING) {
			JsonGenerator *generator = json_generator_new();
			json_generator_set_root(generator, memberNode);
			gchar *memberJSON =
				json_generator_to_data(generator, NULL);
			addError(errors,
				 "$.body.%s must be string: <%s>",
				 name,
				 memberJSON);
			g_free(memberJSON);
			g_object_unref(generator);
			return false;
		}

		return true;
	}

	bool validateBodyMemberSeverity(StringList &errors,
					JsonObject *body,
					const gchar *name)
	{
		JsonNode *memberNode = json_object_get_member(body, name);
		if (!memberNode) {
			return true;
		}

		if (!validateStringBodyMember(errors, body, name, memberNode)) {
			return false;
		}

		const string severity(json_node_get_string(memberNode));
		if (parseSeverity(severity) == NUM_TRIGGER_SEVERITY) {
			addError(errors,
				 "$.body.%s must be valid severity: <%s> "
				 "available values: unknown, info, warning, "
				 "error, critical emergency",
				 name,
				 severity.c_str());
			return false;
		}

		return true;
	}

	bool validateBodyMemberType(StringList &errors,
				    JsonObject *body,
				    const gchar *name)
	{
		JsonNode *memberNode = json_object_get_member(body, name);
		if (!memberNode) {
			return true;
		}

		if (!validateStringBodyMember(errors, body, name, memberNode)) {
			return false;
		}

		const string type(json_node_get_string(memberNode));
		if (parseType(type) == EVENT_TYPE_INVALID) {
			addError(errors,
				 "$.body.%s must be valid type: <%s>: "
				 "available values: good, bad, unknown",
				 name,
				 type.c_str());
			return false;
		}

		return true;
	}

	bool validateEventBody(StringList &errors, JsonObject *body)
	{
		if (!validateBodyMember(errors, body, "id", G_TYPE_INT64))
			return false;
		if (!validateBodyMemberTime(errors, body, "timestamp"))
			return false;
		if (!validateBodyMember(errors, body, "hostName", G_TYPE_STRING))
			return false;
		if (!validateBodyMember(errors, body, "content", G_TYPE_STRING))
			return false;
		if (!validateBodyMemberSeverity(errors, body, "severity"))
			return false;
		if (!validateBodyMemberType(errors, body, "type"))
			return false;
		return true;
	}
};

GateJSONEventMessage::GateJSONEventMessage(JsonNode *root)
: m_impl(new Impl(root))
{
}

GateJSONEventMessage::~GateJSONEventMessage()
{
}

bool GateJSONEventMessage::validate(StringList &errors)
{
	return m_impl->validate(errors);
}

int64_t GateJSONEventMessage::getID()
{
	return m_impl->getID();
}

string GateJSONEventMessage::getIDString()
{
	const uint64_t id = static_cast<uint64_t>(m_impl->getID());
	string idString = to_string(id);
	string fixedIdString;
	const int digitNum = EVENT_ID_DIGIT_NUM;
	const int numPads = digitNum - idString.size();
	if (numPads > 0)
		fixedIdString = string(numPads, '0');
	fixedIdString += idString;
	return fixedIdString;
}

timespec GateJSONEventMessage::getTimestamp()
{
	return m_impl->getTimestamp();
}

const char *GateJSONEventMessage::getHostName()
{
	return m_impl->getHostName();
}

const char *GateJSONEventMessage::getContent()
{
	return m_impl->getContent();
}

TriggerSeverityType GateJSONEventMessage::getSeverity()
{
	return m_impl->getSeverity();
}

EventType GateJSONEventMessage::getType()
{
	return m_impl->getType();
}
