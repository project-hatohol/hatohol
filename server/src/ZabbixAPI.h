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
	 * Get an API version of the ZABBIX server.
	 * Note that this method is NOT MT-safe.
	 *
	 * @retrun An API version.
	 */
	const std::string &getAPIVersion(void);
	bool checkAPIVersion(int major, int minor, int micro);

	SoupSession *getSession(void);
	SoupMessage *queryCommon(JsonBuilderAgent &agent);
	SoupMessage *queryAPIVersion(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ZabbixAPI_h
