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
#include <map>
#include "RedmineAPIEmulator.h"

using namespace std;

struct RedmineAPIEmulator::PrivateContext {
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}

	static gboolean auth_callback(SoupAuthDomain *domain, SoupMessage *msg,
				      const char *username,
				      const char *password,
				      gpointer user_data);
	
	static void handlerIssuesJson(SoupServer *server, SoupMessage *msg,
				      const char *path, GHashTable *query,
				      SoupClientContext *client,
				      gpointer user_data);

	map<string, string> m_passwords;
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

void RedmineAPIEmulator::reset(void)
{
	m_ctx->m_passwords.clear();
}

void RedmineAPIEmulator::addUser(const std::string &userName,
				 const std::string &password)
{
	m_ctx->m_passwords[userName] = password;
}

gboolean RedmineAPIEmulator::PrivateContext::auth_callback
  (SoupAuthDomain *domain, SoupMessage *msg, const char *username,
   const char *password, gpointer user_data)
{
	RedmineAPIEmulator *emulator
	  = reinterpret_cast<RedmineAPIEmulator *>(user_data);
	map<string, string> &passwords = emulator->m_ctx->m_passwords;

	if (passwords.find(username) == passwords.end())
		return FALSE;
	return passwords[username] == string(password);
}

void RedmineAPIEmulator::setSoupHandlers(SoupServer *soupServer)
{
	SoupAuthDomain *domain = soup_auth_domain_basic_new(
	  SOUP_AUTH_DOMAIN_REALM, "RedminEmulatorRealm",
	  SOUP_AUTH_DOMAIN_BASIC_AUTH_CALLBACK, PrivateContext::auth_callback,
	  SOUP_AUTH_DOMAIN_BASIC_AUTH_DATA, this,
	  SOUP_AUTH_DOMAIN_ADD_PATH, "/",
	  NULL);
	soup_server_add_auth_domain(soupServer, domain);
	g_object_unref(domain);

	soup_server_add_handler(soupServer,
				"/projects/hatoholtestproject/issues.json",
				PrivateContext::handlerIssuesJson, this, NULL);
}

void RedmineAPIEmulator::PrivateContext::handlerIssuesJson
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
