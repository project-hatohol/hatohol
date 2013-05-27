/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <semaphore.h>
#include <errno.h>
#include "ArmNagiosNDOUtils.h"

using namespace std;

struct ArmNagiosNDOUtils::PrivateContext
{
	string         server;
	string         authToken;
	string         uri;
	int            serverPort;
	int            retryInterval;   // in sec
	int            repeatInterval;  // in sec;
	int            serverId;
	volatile int   exitRequest;
	sem_t          sleepSemaphore;

	// constructors
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: server(serverInfo.hostName),
	  serverPort(serverInfo.port),
	  retryInterval(serverInfo.pollingIntervalSec),
	  repeatInterval(serverInfo.retryIntervalSec),
	  serverId(serverInfo.id),
	  exitRequest(0)
	{
		// TODO: use serverInfo.ipAddress if it is given.

		const int pshared = 1;
		ASURA_ASSERT(sem_init(&sleepSemaphore, pshared, 0) == 0,
		             "Failed to sem_init(): %d\n", errno);
	}

	~PrivateContext()
	{
		if (sem_destroy(&sleepSemaphore) != 0)
			MLPL_ERR("Failed to call sem_destroy(): %d\n", errno);
	}

	bool hasExitRequest(void) const
	{
		return g_atomic_int_get(&exitRequest);
	}

	void setExitRequest(void)
	{
		g_atomic_int_set(&exitRequest, 1);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmNagiosNDOUtils::ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

ArmNagiosNDOUtils::~ArmNagiosNDOUtils()
{
	int serverId = m_ctx->serverId;
	string serverName = m_ctx->server;
	if (m_ctx)
		delete m_ctx;
	MLPL_INFO("ArmNagiosNDOUtils [%d:%s]: exit process completed.\n",
	          serverId, serverName.c_str());
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
