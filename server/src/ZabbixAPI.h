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

#ifndef ZabbixAPI_h
#define ZabbixAPI_h

#include <string>
#include <libsoup/soup.h>
#include "DBClientConfig.h"
#include "JsonBuilderAgent.h"

class ZabbixAPI
{
public:
	ZabbixAPI(const MonitoringServerInfo &serverInfo);
	virtual ~ZabbixAPI();

protected:
	/**
	 * Called when the authtoken is updated.
	 */
	virtual void onUpdatedAuthToken(const std::string &authToken);

	/**
	 * Get the API version of the target ZABBIX server.
	 * Note that this method is NOT MT-safe.
	 *
	 * @retrun An API version.
	 */
	const std::string &getAPIVersion(void);

	/**
	 * Compare the API version of the target Zabbix server
	 * with the specified version.
	 *
	 * @param major A major version. Ex. 1 if version is 1.2.3.
	 * @param minor A minor version. Ex. 2 if version is 1.2.3.
	 * @param micro A micro version. Ex. 3 if version is 1.2.3.
	 *
	 * @return
	 * true if the API version of the server is equal to or greater than
	 * that of specified version. Otherwise false is returned.
	 */
	bool checkAPIVersion(int major, int minor, int micro);

	/**
	 * Open a session with with Zabbix API server
	 *
	 * @param msgPtr
	 * An address of a SoupMessage object pointer. If this parameter is
	 * not NULL, a SoupMessage object pointer is copied to this parameter.
	 * Otherwise, the object is freeed internally. And the parameter is
	 * not changed.
	 *
	 * @return
	 * true if a session is oppned successfully. Otherwise, false is
	 * returned.
	 */
	bool openSession(SoupMessage **msgPtr = NULL);

	SoupSession *getSession(void);
	bool updateAuthTokenIfNeeded(void);
	std::string getAuthToken(void);
	void clearAuthToken(void);

	SoupMessage *queryCommon(JsonBuilderAgent &agent);
	SoupMessage *queryAPIVersion(void);
	std::string getInitialJsonRequest(void);
	bool parseInitialResponse(SoupMessage *msg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPI_h
