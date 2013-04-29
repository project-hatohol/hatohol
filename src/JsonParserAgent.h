/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef JsonParserAgent_h
#define JsonParserAgent_h

#include <string>
#include <stdint.h>
using namespace std;

#include <glib.h>
#include <json-glib/json-glib.h>

class JsonParserAgent
{
public:
	JsonParserAgent(const string &data);
	virtual ~JsonParserAgent();
	const char *getErrorMessage(void);
	bool hasError(void);
	bool read(const string &member, bool &dest);
	bool read(const string &member, int64_t &dest);
	bool read(const string &member, string &dest);
	bool read(int index, string &dest);

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
	bool isNull(const string &member, bool &dest);
	bool startObject(const string &member);
	void endObject(void);
	bool startElement(int index);
	void endElement(void);
	int countElements(void);

protected:
	void internalCheck(void);

private:
	JsonParser *m_parser;
	JsonReader *m_reader;
	GError *m_error;
};

#endif // JsonParserAgent_h
