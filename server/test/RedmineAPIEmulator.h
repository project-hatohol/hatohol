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

#pragma once
#include "HttpServerStub.h"

struct RedmineIssue {
	size_t id;
	std::string subject;
	std::string description;
	int projectId;
	int trackerId;
	int statusId;
	int priorityId;
	int authorId;
	std::string authorName;
	int assigneeId;
	std::string assigneeName;
	int doneRatio;
	time_t startDate;
	time_t createdOn;
	time_t updatedOn;

	RedmineIssue(const size_t &_id = 0,
		     const std::string &_subject = "",
		     const std::string &_description = "",
		     const std::string &_authorName = "",
		     const int &_trackerId = 1);

	std::string getProjectName(void) const;
	std::string getTrackerName(void) const;
	std::string getStatusName(void) const;
	std::string getPriorityName(void) const;
	std::string getAuthorName(void) const;
	std::string getAssigneeName(void) const;
	std::string getStartDate(void) const;
	std::string getCreatedOn(void) const;
	std::string getUpdatedOn(void) const;
	std::string toJSON(void) const;

	static std::string getDateString(time_t time);
	static std::string getTimeString(time_t time);
};

class RedmineAPIEmulator : public HttpServerStub {
public:
	RedmineAPIEmulator(void);
	virtual ~RedmineAPIEmulator();
	virtual void reset(void);
	void addUser(const std::string &userName, const std::string &password);
	const std::string &getLastRequestPath(void) const;
	const std::string &getLastRequestMethod(void) const;
	const std::string &getLastRequestQuery(void) const;
	const std::string &getLastRequestBody(void) const;
	const std::string &getLastResponseBody(void) const;
	const RedmineIssue &getLastIssue(void) const;
	void queueDummyResponse(const guint &soupStatus,
				const std::string &body = "");

protected:
	virtual void setSoupHandlers(SoupServer *soupServer);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

extern const guint EMULATOR_PORT;
extern RedmineAPIEmulator g_redmineEmulator;

