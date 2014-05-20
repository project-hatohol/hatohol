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

#include <string>
#include "RedmineAPIEmulator.h"

using namespace std;

struct RedmineAPIEmulator::PrivateContext {
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

RedmineAPIEmulator::RedmineAPIEmulator(void)
: HttpServerStub("RedmineAPIEmulator")
{
	m_ctx = new PrivateContext();
}

RedmineAPIEmulator::~RedmineAPIEmulator()
{
	delete m_ctx;
}

void RedmineAPIEmulator::setSoupHandlers(SoupServer *soupServer)
{
	soup_server_add_handler(soupServer,
				"/projects/hatoholtestproject/issues.json",
				handlerIssuesJson, this, NULL);
}

void RedmineAPIEmulator::handlerIssuesJson
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	string method = msg->method;
	if (method == "GET") {

	} else if (method == "PUT") {

	} else if (method == "POST") {

	} else if (method == "DELETE") {

	} else {
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}
