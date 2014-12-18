/*
 * Copyright (C) 2013 Project Hatohol
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

#include <Logger.h>
#include <string.h>
#include <stdexcept>
#include "JSONParser.h"
using namespace std;
using namespace mlpl;

struct JSONParser::Impl
{
	JsonParser *parser;
	JsonNode *currentNode;
	JsonNode *previousNode;
	GError *error;

	Impl(const string &data)
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

	virtual ~Impl()
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
JSONParser::JSONParser(const string &data)
: m_impl(new Impl(data))
{
}

JSONParser::~JSONParser()
{
}

const char *JSONParser::getErrorMessage(void)
{
	if (!m_impl->error)
		return "No error";
	return m_impl->error->message;
}

bool JSONParser::hasError(void)
{
	return m_impl->error != NULL;
}

bool JSONParser::read(const string &member, bool &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = json_node_get_boolean(m_impl->currentNode);
	endObject();
	return true;
}

bool JSONParser::read(const string &member, int64_t &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = json_node_get_int(m_impl->currentNode);
	endObject();
	return true;
}

bool JSONParser::read(const string &member, string &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	const gchar *str = json_node_get_string(m_impl->currentNode);
	dest = str ? str : "";
	endObject();
	return true;
}

bool JSONParser::read(int index, string &dest)
{
	internalCheck();
	if (!startElement(index))
		return false;

	const gchar *str = json_node_get_string(m_impl->currentNode);
	dest = str ? str : "";
	endElement();
	return true;
}

bool JSONParser::isNull(const string &member, bool &dest)
{
	internalCheck();
	if (!startObject(member))
		return false;

	dest = JSON_NODE_HOLDS_NULL(m_impl->currentNode);
	endObject();
	return true;
}

bool JSONParser::isMember(const string &member)
{
	JsonObject *object;

	object = json_node_get_object(m_impl->currentNode);
	if(!json_object_has_member(object, member.c_str()))
		return false;

	return true;
}

bool JSONParser::startObject(const string &member)
{
	JsonObject *object;

	if (!isMember(member)) {
		MLPL_DBG("The member '%s' is not defined in the current node.\n",
			 member.c_str());
		return false;
	}

	object = json_node_get_object(m_impl->currentNode);
	m_impl->previousNode = m_impl->currentNode;
	m_impl->currentNode = json_object_get_member(object, member.c_str());
	return true;
}

void JSONParser::endObject(void)
{
	JsonNode *tmp;

	if (m_impl->previousNode != NULL)
		tmp = json_node_get_parent(m_impl->previousNode);
	else
		tmp = NULL;

	m_impl->currentNode = m_impl->previousNode;
	m_impl->previousNode = tmp;
}

bool JSONParser::startElement(unsigned int index)
{
	switch (json_node_get_node_type(m_impl->currentNode)) {
	case JSON_NODE_ARRAY: {
		JsonArray * array = json_node_get_array(m_impl->currentNode);

		if (index >= json_array_get_length(array)) {
			MLPL_DBG("The index '%d' is greater than the size of "
				"the array at the current position.\n", index);
			return false;
		}

		m_impl->previousNode = m_impl->currentNode;
		m_impl->currentNode = json_array_get_element(array, index);
	}
	break;
	case JSON_NODE_OBJECT: {
		JsonObject *object = json_node_get_object(m_impl->currentNode);
		GList *members;
		const gchar *name;

		if(index >= json_object_get_size(object)) {
			MLPL_DBG("The index '%d' is greater than the size of "
				"the array at the current position.\n", index);
			return false;
		}

		m_impl->previousNode = m_impl->currentNode;
		members = json_object_get_members(object);
		name = static_cast<gchar *>(g_list_nth_data(members, index));
		m_impl->currentNode = json_object_get_member(object, name);

		g_list_free(members);
	}
	break;
	default:
		HATOHOL_ASSERT(false, "The node isn't neither JSON_NODE_ARRAY nor JSON_NODE_OBJECT.");
		return false;
	}

	return true;
}

void JSONParser::endElement(void)
{
	endObject();
}

unsigned int JSONParser::countElements(void)
{
	return json_array_get_length(json_node_get_array(m_impl->currentNode));
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void JSONParser::internalCheck(void)
{
	HATOHOL_ASSERT(m_impl->currentNode, "CurrentNode: NULL");
}
