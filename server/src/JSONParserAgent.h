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

#ifndef JSONParserAgent_h
#define JSONParserAgent_h

#include <string>
#include <stdint.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "HatoholException.h"

class JSONParserAgent
{
public:
	JSONParserAgent(const std::string &data);
	virtual ~JSONParserAgent();
	const char *getErrorMessage(void);
	bool hasError(void);
	bool read(const std::string &member, bool &dest);
	bool read(const std::string &member, int64_t &dest);
	bool read(const std::string &member, std::string &dest);
	bool read(int index, std::string &dest);
	bool isMember(const std::string &member);

	/**
	 * checks whether the element is Null.
	 *
	 * @param member
	 * A key name to be checked.
	 *
	 * @param dest
	 * The result is returned to this value.
	 *
	 * @return
	 * true if the parse is successed, which doesn't depends on whether
	 * the element is null. Otherwise false is returned.
	 */ 
	bool isNull(const std::string &member, bool &dest);
	bool startObject(const std::string &member);
	void endObject(void);
	bool startElement(unsigned int index);
	void endElement(void);
	unsigned int countElements(void);

protected:
	void internalCheck(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // JSONParserAgent_h
