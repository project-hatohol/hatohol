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
#include <string.h>
using namespace mlpl;

#include <stdexcept>
#include "JsonParserAgent.h"

struct JsonParserAgent::PrivateContext
{
	JsonParser *parser;
	JsonNode *currentNode;
	JsonNode *previousNode;
	GError *error;

	PrivateContext(const string &data)
	: parser(NULL),
	  currentNode(NULL),
	  previousNode(NULL),
	  error(NULL)
	{
		parser = json_parser_new();
		if (!json_parser_load_from_data(parser, data.c_str(), -1, &error))
			return;
		currentNode = json_parser_get_root(parser);
	}

	virtual ~PrivateContext()
	{
		if (error)
			g_error_free(error);
		if (parser)
			g_object_unref(parser);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
JsonParserAgent::JsonParserAgent(const string &data)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(data);
}

JsonParserAgent::~JsonParserAgent()
{
	if (m_ctx)
		delete m_ctx;
}

const char *JsonParserAgent::getErrorMessage(void)
{
	if (!m_ctx->error)
		return "No error";
	return m_ctx->error->message;
}

bool JsonParserAgent::hasError(void)
{
	return m_ctx->error != NULL;
}

bool JsonParserAgent::read(const string &member, bool &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = json_node_get_boolean(m_ctx->currentNode);
	endObject();
	return true;
}

bool JsonParserAgent::read(const string &member, int64_t &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = json_node_get_int(m_ctx->currentNode);
	endObject();
	return true;
}

bool JsonParserAgent::read(const string &member, string &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = json_node_get_string(m_ctx->currentNode);
	endObject();
	return true;
}

bool JsonParserAgent::read(int index, string &dest)
{
	internalCheck();
	if (!startElement(index))
		return false;

	dest = json_node_get_string(m_ctx->currentNode);
	endElement();
	return true;
}

bool JsonParserAgent::isNull(const string &member, bool &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = JSON_NODE_HOLDS_NULL(m_ctx->currentNode);
	endObject();
	return true;
}

bool JsonParserAgent::isMember(const string &member)
{
	JsonObject *object;

	object = json_node_get_object(m_ctx->currentNode);
	if(!json_object_has_member(object, member.c_str())) {
		MLPL_DBG("The member '%s' is not defined in the current node.",
				member.c_str());
		return false;
	}

	return true;
}

bool JsonParserAgent::startObject(const string &member)
{
	JsonObject *object;

	if (!isMember(member))
		return false;

	object = json_node_get_object(m_ctx->currentNode);
	m_ctx->previousNode = m_ctx->currentNode;
	m_ctx->currentNode = json_object_get_member(object, member.c_str());
	return true;
}

void JsonParserAgent::endObject(void)
{
	JsonNode *tmp;

	if (m_ctx->previousNode != NULL)
		tmp = json_node_get_parent(m_ctx->previousNode);
	else
		tmp = NULL;

	m_ctx->currentNode = m_ctx->previousNode;
	m_ctx->previousNode = tmp;
}

bool JsonParserAgent::startElement(unsigned int index)
{
	switch (json_node_get_node_type(m_ctx->currentNode)) {
	case JSON_NODE_ARRAY: {
		JsonArray * array = json_node_get_array(m_ctx->currentNode);

		if (index >= json_array_get_length(array)) {
			MLPL_DBG("The index '%d' is greater than the size of "
				"the array at the current position.", index);
		return false;
		}

		m_ctx->previousNode = m_ctx->currentNode;
		m_ctx->currentNode = json_array_get_element(array, index);
	}
	break;
	case JSON_NODE_OBJECT: {
		JsonObject *object = json_node_get_object(m_ctx->currentNode);
		GList *members;
		const gchar *name;

		if(index >= json_object_get_size(object)) {
			MLPL_DBG("The index '%d' is greater than the size of "
				"the array at the current position.", index);
			return false;
		}

		m_ctx->previousNode = m_ctx->currentNode;
		members = json_object_get_members(object);
		name = static_cast<gchar *>(g_list_nth_data(members, index));
		m_ctx->currentNode = json_object_get_member(object, name);

		g_list_free(members);
	}
	break;
	default:
		HATOHOL_ASSERT(false, "The node isn't JSON_NODE_ARRAY and JSON_NODE_OBJECT.");
		return false;
	}

	return true;
}

void JsonParserAgent::endElement(void)
{
	endObject();
}

int JsonParserAgent::countElements(void)
{
	return json_array_get_length(json_node_get_array(m_ctx->currentNode));
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void JsonParserAgent::internalCheck(void)
{
	if (!m_ctx->currentNode)
		HATOHOL_ASSERT("currentNode: NULL\n");

}
