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

#ifndef RedmineAPIEmulator_h
#define RedmineAPIEmulator_h

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
	time_t startDate;
	time_t createdOn;
	time_t updatedOn;

	RedmineIssue(const size_t &_id = 0,
		     const std::string &_subject = "",
		     const std::string &_description = "",
		     const std::string &_authorName = "",
		     const int &_trackerId = 1);

	std::string getProjectName(void);
	std::string getTrackerName(void);
	std::string getStatusName(void);
	std::string getPriorityName(void);
	std::string getAuthorName(void);
	std::string getAssigneeName(void);
	std::string getStartDate(void);
	std::string getCreatedOn(void);
	std::string getUpdatedOn(void);
	std::string toJson(void);

	static std::string getDateString(time_t time);
	static std::string getTimeString(time_t time);
};

class RedmineAPIEmulator : public HttpServerStub {
public:
	RedmineAPIEmulator(void);
	virtual ~RedmineAPIEmulator();
	virtual void reset(void);
	void addUser(const std::string &userName, const std::string &password);
	const std::string &getLastRequest(void);
	const std::string &getLastResponse(void);
	const RedmineIssue &getLastIssue(void);

protected:
	virtual void setSoupHandlers(SoupServer *soupServer);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPIEmulator_h

