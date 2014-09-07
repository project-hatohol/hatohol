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

#include "JSONParserRewinder.h"

enum {
	OBJECT,
	ELEMENT,
};

JSONParserRewinder::JSONParserRewinder(JSONParserAgent &parser)
: m_parser(parser)
{
}

JSONParserRewinder::~JSONParserRewinder()
{
	while (!m_stack.empty()) {
		const int type = m_stack.top();
		m_stack.pop();
		if (type == OBJECT)
			m_parser.endObject();
		else if (type == ELEMENT)
			m_parser.endElement();
		else
			HATOHOL_ASSERT(false, "Unknown type: %d\n", type);
	}
}

void JSONParserRewinder::pushObject(void)
{
	m_stack.push(OBJECT);
}

void JSONParserRewinder::pushElement(void)
{
	m_stack.push(ELEMENT);
}
