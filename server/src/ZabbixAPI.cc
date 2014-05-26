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

#include <cstdio>
#include <string>
#include "ZabbixAPI.h"
#include "StringUtils.h"

using namespace std;
using namespace mlpl;

struct ZabbixAPI::PrivateContext {

	string         uri;
	string         username;
	string         password;

	// constructors and destructor
	PrivateContext(const MonitoringServerInfo &serverInfo)
	{
		const bool forURI = true;
		uri = "http://";
		uri += serverInfo.getHostAddress(forURI);
		uri += StringUtils::sprintf(":%d", serverInfo.port);
		uri += "/zabbix/api_jsonrpc.php";

		username = serverInfo.userName.c_str();
		password = serverInfo.password.c_str();
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPI::ZabbixAPI(const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

ZabbixAPI::~ZabbixAPI()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
