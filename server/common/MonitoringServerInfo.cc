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

#include <StringUtils.h>
#include "Utils.h"
#include "MonitoringServerInfo.h"

using namespace std;
using namespace mlpl;

string MonitoringServerInfo::getHostAddress(bool forURI) const
{
	if (ipAddress.empty())
		return hostName;

	if (!forURI)
		return ipAddress;

	if (!Utils::isValidIPv4Address(ipAddress) &&
	    Utils::isValidIPv6Address(ipAddress)) {
		return StringUtils::sprintf("[%s]", ipAddress.c_str());
	} else {
		return ipAddress;
	}
}

std::string MonitoringServerInfo::getDisplayName(void) const
{
	if (!nickname.empty())
		return nickname;
	if (!hostName.empty())
		return hostName;
	return ipAddress;
}

void MonitoringServerInfo::initialize(MonitoringServerInfo &monSvInfo)
{
	monSvInfo.id = AUTO_INCREMENT_VALUE;
	monSvInfo.type = MONITORING_SYSTEM_UNKNOWN;
	monSvInfo.port               = 0;
	monSvInfo.pollingIntervalSec = 0;
	monSvInfo.retryIntervalSec   = 0;
	monSvInfo.hostName  = "localhost";
	monSvInfo.ipAddress = "127.0.0.1";
}

