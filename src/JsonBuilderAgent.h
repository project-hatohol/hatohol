/* Hatohol
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

#ifndef JsonBuilderAgent_h
#define JsonBuilderAgent_h

#include <string>
using namespace std;

#include <json-glib/json-glib.h>

class JsonBuilderAgent
{
public:
	JsonBuilderAgent(void);
	~JsonBuilderAgent();
	string generate(void);
	void startObject(const char *member = NULL);
	void startObject(const string &member);
	void endObject(void);
	void startArray(const string &member);
	void endArray(void);
	void add(const string &member, const string &value);
	void add(const string &member, gint64 value);
	void add(const gint64 value);
	void addTrue(const string &member);
	void addFalse(const string &member);
	void addNull(const string &member);

private:
	JsonBuilder *m_builder;
};

#endif // JsonBuilderAgent_h
