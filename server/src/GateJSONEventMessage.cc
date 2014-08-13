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

#include <cmath>

#include "GateJSONEventMessage.h"

using namespace std;
using namespace mlpl;

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
		gdouble rawTimestamp =
			json_object_get_double_member(getBody(), "timestamp");
		timespec timestamp;
		timestamp.tv_sec = static_cast<time_t>(rawTimestamp);
		timestamp.tv_nsec =
			secondToNanoSecond(fmod(rawTimestamp, 1.0));
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
			JsonGenerator *generator = json_generator_new();
			json_generator_set_root(generator, memberNode);
			gchar *memberJSON =
				json_generator_to_data(generator, NULL);
			addError(errors,
				 "%s.%s must be %s: %s <%s>",
				 context,
				 name,
				 g_type_name(expectedValueType),
				 g_type_name(valueType),
				 memberJSON);
			g_free(memberJSON);
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
			JsonGenerator *generator = json_generator_new();
			json_generator_set_root(generator, memberNode);
			gchar *memberJSON =
				json_generator_to_data(generator, NULL);
			addError(errors,
				 "%s.%s must be "
				 "UNIX time in double or "
				 "ISO 8601 string: "
				 "<%s>",
				 context,
				 name,
				 memberJSON);
			g_free(memberJSON);
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
