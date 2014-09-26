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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <SimpleSemaphore.h>
#include <Mutex.h>
#include "HapProcessZabbixAPI.h"
#include "Utils.h"

using namespace std;
using namespace mlpl;

static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

// ---------------------------------------------------------------------------
// PrivateContext
// ---------------------------------------------------------------------------
struct HapProcessZabbixAPI::PrivateContext {

	PrivateContext(void)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessZabbixAPI::HapProcessZabbixAPI(int argc, char *argv[])
: HapProcessStandard(argc, argv),
  m_ctx(NULL)
{
	initGLib();
	m_ctx = new PrivateContext();
}

HapProcessZabbixAPI::~HapProcessZabbixAPI()
{
	delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholError HapProcessZabbixAPI::acquireData(void)
{
	updateAuthTokenIfNeeded();
	workOnHostsAndHostgroupElements();
	workOnHostgroups();
	workOnTriggers();
	workOnEvents();
	getArmStatus().logSuccess();

	return HTERR_OK;
}

HatoholArmPluginWatchType HapProcessZabbixAPI::getHapWatchType(
  const HatoholError &err)
{
	if (err == HTERR_OK)
		return COLLECT_OK;
	else if (err == HTERR_FAILED_CONNECT_ZABBIX)
		return COLLECT_NG_DISCONNECT_ZABBIX;
	else if (err == HTERR_FAILED_TO_PARSE_JSON_DATA)
		return COLLECT_NG_PARSER_ERROR;
	return COLLECT_NG_HATOHOL_INTERNAL_ERROR;
}

//
// Methods running on HapZabbixAPI's thread
//
int HapProcessZabbixAPI::onCaughtException(const exception &e)
{
	const MonitoringServerInfo &serverInfo =
	  HapProcessStandard::getMonitoringServerInfo();
	int retryIntervalSec;
	if (serverInfo.retryIntervalSec == 0)
		retryIntervalSec = DEFAULT_RETRY_INTERVAL;
	else
		retryIntervalSec = serverInfo.retryIntervalSec * 1000;
	MLPL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
		  e.what(), retryIntervalSec);
	return retryIntervalSec;
}

