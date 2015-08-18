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

#include <string>
#include <stdlib.h>
#include <map>
#include <set>
#include <queue>
#include "RedmineAPIEmulator.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "Helpers.h"

using namespace std;

const guint EMULATOR_PORT = 44444;
RedmineAPIEmulator g_redmineEmulator;

RedmineIssue::RedmineIssue(const size_t &_id,
			   const std::string &_subject,
			   const std::string &_description,
			   const std::string &_authorName,
			   const int &_trackerId)
: id(_id), subject(_subject), description(_description),
  projectId(1), trackerId(_trackerId), statusId(1), priorityId(1),
  authorId(1), authorName(_authorName), assigneeId(0), doneRatio(0),
  startDate(time(NULL)), createdOn(startDate), updatedOn(startDate)
{
}

string RedmineIssue::getProjectName(void) const
{
	// TODO
	return "HatoholTestProject";
}

string RedmineIssue::getTrackerName(void) const
{
	// TODO
	return "Bug";
}

string RedmineIssue::getStatusName(void) const
{
	// TODO
	return "New";
}

string RedmineIssue::getPriorityName(void) const
{
	// TODO
	return "Normal";
}

string RedmineIssue::getStartDate(void) const
{
	return getDateString(startDate);
};

string RedmineIssue::getCreatedOn(void) const
{
	return getTimeString(createdOn);
};
string RedmineIssue::getUpdatedOn(void) const
{
	return getTimeString(updatedOn);
};

string RedmineIssue::getDateString(time_t time)
{
	struct tm tm;
	gmtime_r(&time, &tm);
	char dateString[128];
	strftime(dateString, sizeof(dateString), "%Y-%m-%d", &tm);
	return string(dateString);
}

string RedmineIssue::getTimeString(time_t time)
{
	struct tm tm;
	gmtime_r(&time, &tm);
	char timeString[128];
	strftime(timeString, sizeof(timeString), "%Y-%m-%dT%H:%M:%SZ", &tm);
	return string(timeString);
}

string RedmineIssue::toJSON(void) const
{
	JSONBuilder agent;
	agent.startObject();
	agent.startObject("issue");
	agent.add("id", id);

	agent.startObject("project");
	agent.add("id", projectId);
	agent.add("name", getProjectName());
	agent.endObject();

	agent.startObject("tracker");
	agent.add("id", trackerId);
	agent.add("name", getTrackerName());
	agent.endObject();

	agent.startObject("status");
	agent.add("id", statusId);
	agent.add("name", getStatusName());
	agent.endObject();

	agent.startObject("priority");
	agent.add("id", priorityId);
	agent.add("name", getPriorityName());
	agent.endObject();

	agent.startObject("author");
	agent.add("id", authorId);
	agent.add("name", authorName);
	agent.endObject();

	if (assigneeId) {
		agent.startObject("assigned_to");
		agent.add("id", assigneeId);
		agent.add("name", assigneeName);
		agent.endObject();
	}

	agent.add("subject", subject);
	agent.add("description", description);
	agent.add("start_date", getStartDate());
	agent.add("done_ratio", "0");
	agent.add("spent_hours", ":0.0,");
	agent.add("created_on", getCreatedOn());
	agent.add("updated_on", getUpdatedOn());

	agent.endObject();
	agent.endObject();

	return agent.generate();
}

struct Response {
	guint  soupStatus;
	string body;
};

struct RedmineAPIEmulator::PrivateContext {
	PrivateContext(void)
	: m_issueId(0)
	{
	}
	virtual ~PrivateContext()
	{
	}

	static gboolean authCallback(SoupAuthDomain *domain, SoupMessage *msg,
				      const char *username,
				      const char *password,
				      gpointer user_data);
	static void handlerIssuesJSON(SoupServer *server, SoupMessage *msg,
				      const char *path, GHashTable *query,
				      SoupClientContext *client,
				      gpointer user_data);
	static void handlerIssueJSON(SoupServer *server, SoupMessage *msg,
				     const char *path, GHashTable *query,
				     SoupClientContext *client,
				     gpointer user_data);
	bool handlerDummyResponse(SoupMessage *msg,
				  const char *path, GHashTable *query,
				  SoupClientContext *client);
	string buildResponse(RedmineIssue &issue);
	void replyGetIssue(SoupMessage *msg);
	void replyPostIssue(SoupMessage *msg);
	void replyPutIssue(SoupMessage *msg);
	int getTrackerId(const string &trackerId);

	map<string, string> m_passwords;
	string m_currentUser;
	size_t m_issueId;
	string m_lastRequestPath;
	string m_lastRequestMethod;
	string m_lastRequestQuery;
	string m_lastRequestBody;
	string m_lastResponseBody;
	RedmineIssue m_lastIssue;
	queue<Response> m_dummyResponseQueue;
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
	m_ctx->m_lastRequestPath.clear();
	m_ctx->m_lastRequestMethod.clear();
	m_ctx->m_lastRequestQuery.clear();
	m_ctx->m_lastRequestBody.clear();
	m_ctx->m_lastResponseBody.clear();
	m_ctx->m_lastIssue = RedmineIssue();
	queue<Response> empty;
	std::swap(m_ctx->m_dummyResponseQueue, empty);
}

void RedmineAPIEmulator::addUser(const std::string &userName,
				 const std::string &password)
{
	m_ctx->m_passwords[userName] = password;
}

const string &RedmineAPIEmulator::getLastRequestPath(void) const
{
	return m_ctx->m_lastRequestPath;
}

const string &RedmineAPIEmulator::getLastRequestMethod(void) const
{
	return m_ctx->m_lastRequestMethod;
}

const string &RedmineAPIEmulator::getLastRequestQuery(void) const
{
	return m_ctx->m_lastRequestQuery;
}

const string &RedmineAPIEmulator::getLastRequestBody(void) const
{
	return m_ctx->m_lastRequestBody;
}

const string &RedmineAPIEmulator::getLastResponseBody(void) const
{
	return m_ctx->m_lastResponseBody;
}

const RedmineIssue &RedmineAPIEmulator::getLastIssue(void) const
{
	return m_ctx->m_lastIssue;
}

void RedmineAPIEmulator::queueDummyResponse(const guint &soupStatus,
					    const std::string &body)
{
	Response res;
	res.soupStatus = soupStatus;
	res.body = body;
	m_ctx->m_dummyResponseQueue.push(res);
}

gboolean RedmineAPIEmulator::PrivateContext::authCallback
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
	  SOUP_AUTH_DOMAIN_BASIC_AUTH_CALLBACK, PrivateContext::authCallback,
	  SOUP_AUTH_DOMAIN_BASIC_AUTH_DATA, this,
	  SOUP_AUTH_DOMAIN_ADD_PATH, "/",
	  NULL);
	soup_server_add_auth_domain(soupServer, domain);
	g_object_unref(domain);

	soup_server_add_handler(soupServer,
				"/issues.json",
				PrivateContext::handlerIssuesJSON, this, NULL);
	soup_server_add_handler(soupServer,
				"/issues/",
				PrivateContext::handlerIssueJSON, this, NULL);
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

int RedmineAPIEmulator::PrivateContext::getTrackerId(const string &trackerId)
{
	int id = atoi(trackerId.c_str());
	if (id > 0 && id < 4) {
		// There are 3 trackers by default
		return id;
	}
	return -1;
}

void RedmineAPIEmulator::PrivateContext::replyGetIssue(SoupMessage *msg)
{
	string path = getFixturesDir() + "redmine-issues.json";
	gchar *contents = NULL;
	gsize length;
	gboolean succeeded =
		g_file_get_contents(path.c_str(), &contents, &length, NULL);
	if (!succeeded) {
		THROW_HATOHOL_EXCEPTION("Failed to read file: %s",
					path.c_str());
	}
	soup_message_set_status(msg, SOUP_STATUS_OK);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
				 contents, length);
	g_free(contents);
}

void RedmineAPIEmulator::PrivateContext::replyPostIssue(SoupMessage *msg)
{
	m_lastRequestBody.assign(msg->request_body->data,
				 msg->request_body->length);
	JSONParser agent(m_lastRequestBody);

	if (agent.hasError()) {
		soup_message_set_status(
		  msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}

	string errors, subject, description, trackerIdString;
	int trackerId = 1;
	if (agent.startObject("issue")) {
		agent.read("subject", subject);
		if (subject.empty())
			addError(errors, ERR_NO_SUBJECT);

		agent.read("description", description);

		bool hasProjectId = agent.read("project_id", trackerIdString);
		if (!hasProjectId) {
			soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
			return;
		}
		bool hasTrackerId = agent.read("tracker_id", trackerIdString);
		if (hasTrackerId) {
			trackerId = getTrackerId(trackerIdString);
			if (trackerId <= 0) {
				soup_message_set_status(
				  msg, SOUP_STATUS_NOT_FOUND);
				return;
			}
		}
	} else {
		addError(errors, ERR_NO_SUBJECT);
	}

	agent.endObject();
	agent.endObject();

	if (errors.empty()) {
		RedmineIssue issue(++m_issueId, subject, description,
				   m_currentUser, trackerId);
		m_lastResponseBody = issue.toJSON();
		m_lastIssue = issue;
		soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
					 m_lastResponseBody.c_str(),
					 m_lastResponseBody.size());
		soup_message_set_status(msg, SOUP_STATUS_OK);
	} else {
		errors += "]}";
		soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
					 errors.c_str(), errors.size());
		soup_message_set_status(msg, SOUP_STATUS_UNPROCESSABLE_ENTITY);
		m_lastResponseBody = errors;
	}
}

void RedmineAPIEmulator::PrivateContext::replyPutIssue(SoupMessage *msg)
{
	m_lastRequestBody.assign(msg->request_body->data,
				 msg->request_body->length);
	JSONParser agent(m_lastRequestBody);

	if (agent.hasError()) {
		soup_message_set_status(
		  msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}

	// TODO: Implement typical errors of Redmine
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

bool RedmineAPIEmulator::PrivateContext::handlerDummyResponse
  (SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client)
{
	if (m_dummyResponseQueue.empty())
		return false;

	Response &res = m_dummyResponseQueue.front();
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
				 res.body.c_str(), res.body.size());
	soup_message_set_status(msg, res.soupStatus);
	m_dummyResponseQueue.pop();

	return true;
}

void RedmineAPIEmulator::PrivateContext::handlerIssuesJSON
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	RedmineAPIEmulator *emulator
	  = reinterpret_cast<RedmineAPIEmulator *>(user_data);
	RedmineAPIEmulator::PrivateContext *priv = emulator->m_ctx;

	priv->m_lastRequestPath = path;

	string method = msg->method;
	priv->m_lastRequestMethod = method;

	SoupURI *uri = soup_message_get_uri(msg);
	const char *queryString = uri->query;
	priv->m_lastRequestQuery = queryString ? queryString : "";

	if (priv->handlerDummyResponse(msg, path ,query, client))
		return;

	if (method == "GET") {
		priv->replyGetIssue(msg);
	} else if (method == "PUT") {
		soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
	} else if (method == "POST") {
		priv->replyPostIssue(msg);
	} else if (method == "DELETE") {
		// TODO
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else {
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void RedmineAPIEmulator::PrivateContext::handlerIssueJSON
  (SoupServer *server, SoupMessage *msg, const char *path, GHashTable *query,
   SoupClientContext *client, gpointer user_data)
{
	RedmineAPIEmulator *emulator
	  = reinterpret_cast<RedmineAPIEmulator *>(user_data);
	RedmineAPIEmulator::PrivateContext *priv = emulator->m_ctx;

	priv->m_lastRequestPath = path;

	string method = msg->method;
	priv->m_lastRequestMethod = method;

	SoupURI *uri = soup_message_get_uri(msg);
	const char *queryString = uri->query;
	priv->m_lastRequestQuery = queryString ? queryString : "";

	if (priv->handlerDummyResponse(msg, path ,query, client))
		return;

	if (method == "GET") {
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else if (method == "PUT") {
		priv->replyPutIssue(msg);
	} else if (method == "POST") {
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else if (method == "DELETE") {
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else {
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}
