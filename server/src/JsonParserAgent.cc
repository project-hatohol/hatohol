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
using namespace mlpl;

#include <stdexcept>
#include "JsonParserAgent.h"

struct JsonParserAgent::PrivateContext
{
	string objectPosition;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
JsonParserAgent::JsonParserAgent(const string &data)
: m_reader(NULL),
  m_error(NULL),
  m_ctx(NULL)
{
	m_error = NULL;
	m_parser = json_parser_new();
	if (!json_parser_load_from_data(m_parser, data.c_str(), -1, &m_error))
		return;
	m_reader = json_reader_new(json_parser_get_root(m_parser));
	m_ctx = new PrivateContext;
}

JsonParserAgent::~JsonParserAgent()
{
	if (m_error)
		g_error_free(m_error);
	if (m_reader)
		g_object_unref(m_reader);
	if (m_parser)
		g_object_unref(m_parser);
	if (m_ctx)
		delete m_ctx;
}

const char *JsonParserAgent::getErrorMessage(void)
{
	if (!m_error)
		return "No error";
	return m_error->message;
}

bool JsonParserAgent::hasError(void)
{
	return m_error != NULL;
}

bool JsonParserAgent::read(const string &member, bool &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_reader, member.c_str())) {
		json_reader_end_member(m_reader);
		if (!m_ctx->objectPosition.empty())
			json_reader_read_member(m_reader, m_ctx->objectPosition.c_str());
		return false;
	}
	dest = json_reader_get_boolean_value(m_reader);
	json_reader_end_member(m_reader);
	return true;
}

bool JsonParserAgent::read(const string &member, int64_t &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_reader, member.c_str())) {
		json_reader_end_member(m_reader);
		if (!m_ctx->objectPosition.empty())
			json_reader_read_member(m_reader, m_ctx->objectPosition.c_str());
		return false;
	}
	dest = json_reader_get_int_value(m_reader);
	json_reader_end_member(m_reader);
	return true;
}

bool JsonParserAgent::read(const string &member, string &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_reader, member.c_str())) {
		json_reader_end_member(m_reader);
		if (!m_ctx->objectPosition.empty())
			json_reader_read_member(m_reader, m_ctx->objectPosition.c_str());
		return false;
	}
	dest = json_reader_get_string_value(m_reader);
	json_reader_end_member(m_reader);
	return true;
}

bool JsonParserAgent::read(int index, string &dest)
{
	internalCheck();
	if (!json_reader_read_element(m_reader,index)) {
		json_reader_end_element(m_reader);
		return false;
	}
	dest = json_reader_get_string_value(m_reader);
	json_reader_end_element(m_reader);
	return true;
}

bool JsonParserAgent::isNull(const string &member, bool &dest)
{
	internalCheck();
	if (!json_reader_read_member(m_reader, member.c_str())) {
		json_reader_end_member(m_reader);
		return false;
	}
	dest = json_reader_get_null_value(m_reader);
	json_reader_end_element(m_reader);
	return true;
}

bool JsonParserAgent::startObject(const string &member)
{
	if (!json_reader_read_member(m_reader, member.c_str())) {
		json_reader_end_member(m_reader);
		return false;
	}
	m_ctx->objectPosition = member.c_str();
	return true;
}

void JsonParserAgent::endObject(void)
{
	json_reader_end_member(m_reader);
}

bool JsonParserAgent::startElement(int index)
{
	return json_reader_read_element(m_reader, index);
}

void JsonParserAgent::endElement(void)
{
	json_reader_end_element(m_reader);
}

int JsonParserAgent::countElements(void)
{
	return json_reader_count_elements(m_reader);
}

void JsonParserAgent::getMemberName(void)
{
	const gchar *memberName = json_reader_get_member_name(m_reader);
	if (!memberName) {
		g_print("MemberName: FALSE\n");
		if (!m_ctx->objectPosition.empty()) {
			json_reader_end_member(m_reader);
			json_reader_read_member(m_reader, m_ctx->objectPosition.c_str());
		} else {
			json_reader_end_member(m_reader);
		}
	} else {
		g_print("MemberName: %s\n", memberName);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void JsonParserAgent::internalCheck(void)
{
	if (!m_reader)
		throw runtime_error("m_reader: NULL\n");

}
