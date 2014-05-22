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
#include <stdlib.h>
#include <map>
#include <set>
#include "RedmineAPIEmulator.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"

using namespace std;

struct RedmineAPIEmulator::PrivateContext {
	PrivateContext(void)
	: m_issueId(0)
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
	string buildReply(const string &subject,
			  const string &description,
			  const int trackerId);
	void replyPostIssue(SoupMessage *msg);
	int getTrackerId(const string &trackerId);

	map<string, string> m_passwords;
	string m_currentUser;
	size_t m_issueId;
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
	m_ctx->m_currentUser.clear();
	m_ctx->m_issueId = 0;
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
	if (passwords[username] != string(password))
		return FALSE;
	emulator->m_ctx->m_currentUser = username;
	return TRUE;
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

typedef enum {
	ERR_NO_SUBJECT,
	ERR_OTHER,
	N_ERRORS
} RedmineErrorType;

void addError(string &errors, RedmineErrorType type,
	      const string &message = "")
{
	if (errors.empty()) {
		errors = "{\"errors\":[";
	} else {
		errors += ",";
	}
	switch (type) {
	case ERR_NO_SUBJECT:
		errors += "\"Subject can't be blank\"";
		break;
	case ERR_OTHER:
		errors += "\"" + message + "\"";
	default:
		break;
	}
}

string RedmineAPIEmulator::PrivateContext::buildReply(
  const string &subject, const string &description, const int trackerId)
{
	time_t current = time(NULL);
	struct tm tm;
	gmtime_r(&current, &tm);
	char dateString[128], timeString[128];
	strftime(dateString, sizeof(dateString), "%Y-%m-%d", &tm);
	strftime(timeString, sizeof(timeString), "%Y-%m-%dT%H:%M:%SZ", &tm);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.startObject("issue");
	agent.add("id", ++m_issueId);

	agent.startObject("project");
	agent.add("id", "1");
	agent.add("name", "HatoholTestProject");
	agent.endObject();

	agent.startObject("tracker");
	agent.add("id", trackerId);
	agent.add("name", "Bug");
	agent.endObject();

	agent.startObject("status");
	agent.add("id", "1");
	agent.add("name", "New");
	agent.endObject();

	agent.startObject("priority");
	agent.add("id", "2");
	agent.add("name", "Normal");
	agent.endObject();

	agent.startObject("author");
	agent.add("id", "1");
	agent.add("name", m_currentUser);
	agent.endObject();

	agent.add("subject", subject);
	agent.add("description", description);
	agent.add("start_date", dateString);
	agent.add("done_ratio", "0");
	agent.add("spent_hours", ":0.0,");
	agent.add("created_on", timeString);
	agent.add("updated_on", timeString);

	agent.endObject();
	agent.endObject();

	return agent.generate();
}

int RedmineAPIEmulator::PrivateContext::getTrackerId(const string &trackerId)
{
	int id = atoi(trackerId.c_str());
	if (id > 0 && id < 4) {
		// There are 3 trackers by default
		return id;
	}
	return -1;
}

void RedmineAPIEmulator::PrivateContext::replyPostIssue(SoupMessage *msg)
{
	string body(msg->request_body->data,
		    msg->request_body->length);
	string errors;
	JsonParserAgent agent(body);

	if (agent.hasError()) {
		soup_message_set_status(
		  msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}

	string subject, description, trackerIdString;
	int trackerId = 1;
	if (agent.startObject("issue")) {
		agent.read("subject", subject);
		if (subject.empty())
			addError(errors, ERR_NO_SUBJECT);

		agent.read("description", description);

		bool hasTrackerId = agent.read("tracker_id", trackerIdString);
		trackerId = getTrackerId(trackerIdString);
		if (hasTrackerId && trackerId <= 0) {
			soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
			return;
		}
	} else {
		addError(errors, ERR_NO_SUBJECT);
	}

	agent.endObject();
	agent.endObject();

	if (errors.empty()) {
		string reply = buildReply(subject, description, trackerId);
		soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
					 reply.c_str(), reply.size());
		soup_message_set_status(msg, SOUP_STATUS_OK);
	} else {
		errors += "]}";
		soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
					 errors.c_str(), errors.size());
		soup_message_set_status(msg, SOUP_STATUS_UNPROCESSABLE_ENTITY);
	}
}

void RedmineAPIEmulator::PrivateContext::handlerIssuesJson
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	RedmineAPIEmulator *emulator
	  = reinterpret_cast<RedmineAPIEmulator *>(user_data);

	string method = msg->method;
	if (method == "GET") {

	} else if (method == "PUT") {

	} else if (method == "POST") {
		emulator->m_ctx->replyPostIssue(msg);
	} else if (method == "DELETE") {

	} else {
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}
