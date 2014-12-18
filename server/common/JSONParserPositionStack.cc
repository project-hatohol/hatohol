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

#include "JSONParser.h"

enum {
	OBJECT,
	ELEMENT,
};

JSONParser::PositionStack::PositionStack(JSONParser &parser)
: m_parser(parser)
{
}

JSONParser::PositionStack::~PositionStack()
{
	while (!m_stack.empty())
		pop();
}

bool JSONParser::PositionStack::pushObject(const std::string &member)
{
	if (!m_parser.startObject(member))
		return false;
	m_stack.push(OBJECT);
	return true;
}

bool JSONParser::PositionStack::pushElement(const unsigned int &index)
{
	if (!m_parser.startElement(index))
		return false;
	m_stack.push(ELEMENT);
	return true;
}

void JSONParser::PositionStack::pop(void)
{
	const int type = m_stack.top();
	m_stack.pop();
	if (type == OBJECT)
		m_parser.endObject();
	else if (type == ELEMENT)
		m_parser.endElement();
	else
		HATOHOL_ASSERT(false, "Unknown type: %d\n", type);
}
