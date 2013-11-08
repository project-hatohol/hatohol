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
	JsonReader *reader;
	JsonNode *previousNode;
	GError *error;

	PrivateContext(const string &data)
	: parser(NULL),
	  currentNode(NULL),
	  reader(NULL),
	  previousNode(NULL),
	  error(NULL)
	{
		parser = json_parser_new();
		if (!json_parser_load_from_data(parser, data.c_str(), -1, &error))
			return;
		currentNode = json_parser_get_root(parser);
		reader = json_reader_new(currentNode);
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
	if (m_ctx->error)
		g_error_free(m_ctx->error);
	if (m_ctx->reader)
		g_object_unref(m_ctx->reader);
	if (m_ctx->parser)
		g_object_unref(m_ctx->parser);
	if (m_ctx->currentNode)
		g_object_unref(m_ctx->currentNode);
	if (m_ctx->previousNode)
		g_object_unref(m_ctx->previousNode);
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
	if (isMember(member)) {
		if (!json_reader_read_member(m_ctx->reader, member.c_str())) {
			json_reader_end_member(m_ctx->reader);
			return false;
		}
		dest = json_reader_get_boolean_value(m_ctx->reader);
		json_reader_end_member(m_ctx->reader);
		return true;
	}
	return false;
}

bool JsonParserAgent::read(const string &member, int64_t &dest)
{
	internalCheck();
	if (isMember(member)) {
		if (!json_reader_read_member(m_ctx->reader, member.c_str())) {
			json_reader_end_member(m_ctx->reader);
			return false;
		}
		dest = json_reader_get_int_value(m_ctx->reader);
		json_reader_end_member(m_ctx->reader);
		return true;
	}
	return false;
}

bool JsonParserAgent::read(const string &member, string &dest)
{
	internalCheck();
	if (isMember(member)) {
		if (!json_reader_read_member(m_ctx->reader, member.c_str())) {
			json_reader_end_member(m_ctx->reader);
			return false;
		}
		dest = json_reader_get_string_value(m_ctx->reader);
		json_reader_end_member(m_ctx->reader);
		return true;
	}
	return false;
}

bool JsonParserAgent::read(int index, string &dest)
{
	internalCheck();
	if (!json_reader_read_element(m_ctx->reader,index)) {
		json_reader_end_element(m_ctx->reader);
		return false;
	}
	dest = json_reader_get_string_value(m_ctx->reader);
	json_reader_end_element(m_ctx->reader);
	return true;
}

bool JsonParserAgent::isNull(const string &member, bool &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_ctx->reader, member.c_str())) {
		json_reader_end_member(m_ctx->reader);
		return false;
	}
	dest = json_reader_get_null_value(m_ctx->reader);
	json_reader_end_element(m_ctx->reader);
	return true;
}

bool JsonParserAgent::isMember(const string &member)
{
	JsonObject *object;

	object = json_node_get_object(m_ctx->currentNode);
	if(!json_object_has_member(object, member.c_str())) {
		MLPL_ERR("The member '%s' is not defined in the current node.",
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
	json_reader_end_member(m_ctx->reader);
}

bool JsonParserAgent::startElement(int index)
{
	return json_reader_read_element(m_ctx->reader, index);
}

void JsonParserAgent::endElement(void)
{
	json_reader_end_element(m_ctx->reader);
}

int JsonParserAgent::countElements(void)
{
	return json_reader_count_elements(m_ctx->reader);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void JsonParserAgent::internalCheck(void)
{
	if (!m_ctx->reader)
		throw runtime_error("reader: NULL\n");

}
