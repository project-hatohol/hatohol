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

#include <HapZabbixAPI.h>
#include <SimpleSemaphore.h>
#include "HapProcess.h"

using namespace mlpl;

class HapProcessZabbixAPI : public HapProcess, public HapZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	int mainLoopRun(void);

protected:
	gpointer hapMainThread(HatoholThreadArg *arg) override;

	bool initMonitoringServerInfo(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

// ---------------------------------------------------------------------------
// PrivateContext
// ---------------------------------------------------------------------------
struct HapProcessZabbixAPI::PrivateContext {
	MonitoringServerInfo serverInfo;
	SimpleSemaphore      mainThreadSem;

	PrivateContext(void)
	: mainThreadSem(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessZabbixAPI::HapProcessZabbixAPI(int argc, char *argv[])
: HapProcess(argc, argv),
  m_ctx(NULL)
{
	initGLib();
}

int HapProcessZabbixAPI::mainLoopRun(void)
{
	g_main_loop_run(getGMainLoop());
	return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HapProcessZabbixAPI::hapMainThread(HatoholThreadArg *arg)
{
	if (!initMonitoringServerInfo())
		return NULL;
	return NULL;
}

bool HapProcessZabbixAPI::initMonitoringServerInfo(void)
{
	const size_t sleepTimeMSec = 30 * 1000;
	while (true) {
		bool succeeded = getMonitoringServerInfo(m_ctx->serverInfo);
		if (succeeded)
			break;
		MLPL_INFO("Failed to get MonitoringServerInfo. "
		          "Retry after %zd sec.\n",  sleepTimeMSec/1000);
		SimpleSemaphore::Status status =
		  m_ctx->mainThreadSem.timedWait(sleepTimeMSec);
		// TODO: Add a mechanism to exit
		if (status == SimpleSemaphore::STAT_OK ||
		    status == SimpleSemaphore::STAT_ERROR_UNKNOWN) {
			HATOHOL_ASSERT(true, "Unexpected result: %d\n", status);
		}
	}
	setExceptionSleepTime(m_ctx->serverInfo.retryIntervalSec*1000);
	return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	HapProcessZabbixAPI hapProc(argc, argv);
	return hapProc.mainLoopRun();
}
