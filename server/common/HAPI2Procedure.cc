/*
 * Copyright (C) 2015 Project Hatohol
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

#include "HAPI2Procedure.h"

using namespace std;
using namespace mlpl;

static const map<string, HAPI2ProcedureType> procedureTypeMap = {
	{ "exchangeProfile",  HAPI2_EXCHANGE_PROFILE },
	{ "getMonitoringServerInfo", HAPI2_MONITORING_SERVER_INFO },
	{ "getLastInfo", HAPI2_LAST_INFO},
	{ "putItems", HAPI2_PUT_ITEMS },
	{ "putHistory", HAPI2_PUT_HISTORY },
	{ "updateHosts", HAPI2_UPDATE_HOSTS },
	{ "updateHostGroups", HAPI2_UPDATE_HOST_GROUPS },
	{ "updateHostGroupMembership", HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP },
	{ "updateTriggers", HAPI2_UPDATE_TRIGGERS },
	{ "updateEvents", HAPI2_UPDATE_EVENTS },
	{ "updateHostParent", HAPI2_UPDATE_HOST_PARENTS },
	{ "updateArmInfo", HAPI2_UPDATE_ARM_INFO },
	{ "fetchItems", HAPI2_PROCEDURE_TYPE_HAP },
	{ "fetchHistory", HAPI2_PROCEDURE_TYPE_HAP },
	{ "fetchTriggers", HAPI2_PROCEDURE_TYPE_HAP },
	{ "fetchEvents", HAPI2_PROCEDURE_TYPE_HAP},
};

struct HAPI2Procedure::Impl
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
			json_object_get_object_member(rootObject, "method");
		if (!validateProcedure(errors, body))
			return false;

		return true;
	}

	HAPI2ProcedureType getHAPI2ProcedureType()
	{
		JsonObject *rootObject = json_node_get_object(m_root);
		JsonNode *node = json_object_get_member(rootObject, "method");
		if (!node) {
			return HAPI2_PROCEDURE_TYPE_BAD;
		}

		const string type(json_node_get_string(node));
		return parseProcedureType(type);
	}

	string getHAPI2Params()
	{
		JsonObject *rootObject = json_node_get_object(m_root);
		JsonNode *node = json_object_get_member(rootObject, "params");
		if (!node) {
			return "__INVALID_PARAMS";
		}

		const string params(json_node_get_string(node));
		return params;
	}
private:
	HAPI2ProcedureType parseProcedureType(const string &method)
	{
		auto it = procedureTypeMap.find(method);
		if (it == procedureTypeMap.end())
			return HAPI2_PROCEDURE_TYPE_BAD;
		return it->second;
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

	bool validateEnvelope(StringList &errors)
	{
		if (json_node_get_node_type(m_root) != JSON_NODE_OBJECT) {
			addError(errors, "JSON message must be an object");
			return false;
		}

		JsonObject *rootObject = json_node_get_object(m_root);
		if (!validateObjectMember(errors, "$", rootObject, "jsonrpc",
					  G_TYPE_STRING))
			return false;

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

	bool validateBodyMemberProcedure(StringList &errors,
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

		const string method(json_node_get_string(memberNode));
		if (parseProcedureType(method) ==
		    HAPI2_PROCEDURE_TYPE_BAD || HAPI2_PROCEDURE_TYPE_HAP) {
			addError(errors,
				 "$.method.%s must be valid procedure type: <%s> "
				 "available server procedure types: "
				 "exchangeProfile, "
				 "getMonitoringServerInfo, "
				 "getLastInfo, putItems"
				 "putHistory, updateHosts"
				 "updateHostGroups, "
				 "updateHostGroupMembership, "
				 "updateTriggers, updateEvents"
				 "updateHostParent"
				 "updateArmInfo",
				 name,
				 method.c_str());
			return false;
		}

		return true;
	}

	bool validateProcedure(StringList &errors, JsonObject *body)
	{
		if (!validateBodyMemberProcedure(errors, body, "method"))
			return false;
		return true;
	}
};

HAPI2Procedure::HAPI2Procedure(JsonNode *root)
: m_impl(new Impl(root))
{
}

HAPI2Procedure::~HAPI2Procedure()
{
}

bool HAPI2Procedure::validate(StringList &errors)
{
	return m_impl->validate(errors);
}

HAPI2ProcedureType HAPI2Procedure::getType()
{
	return m_impl->getHAPI2ProcedureType();
}

string HAPI2Procedure::getParams()
{
	return m_impl->getHAPI2Params();
}
