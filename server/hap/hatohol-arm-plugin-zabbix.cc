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

#include <HapZabbixAPI.h>
#include <SimpleSemaphore.h>
#include "HapProcess.h"

using namespace mlpl;

class HapProcessZabbixAPI : public HapProcess, public HapZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	virtual ~HapProcessZabbixAPI();
	int mainLoopRun(void);

protected:
	gpointer hapMainThread(HatoholThreadArg *arg) override;

	// called from HapZabbixAPI
	void onPreWaitInitiatedAck(void) override;
	void onPostWaitInitiatedAck(void) override;
	void onReady(void) override;

	/**
	 * Wait for the callback of onReady.
	 *
	 * @return true if exit is requsted. Otherwise false is returned.
	 */
	bool waitOnReady(void);

	/**
	 * Get the MonitoringServerInfo from Hatohol and
	 * do a basic setup with it. The obtained MonitoringServerInfo is
	 * stored in PrivateContext.
	 *
	 * @return true if exit is requsted. Otherwise false is returned.
	 */
	bool initMonitoringServerInfo(void);

	/**
	 * Sleep for a specified time.
	 *
	 * @param sleepTimeInSec A sleep time.
	 *
	 * @return true if exit is requsted. Otherwise false is returned.
	 */
	bool sleepForMainThread(const int &sleepTimeInSec);

	gpointer _hapMainThread(HatoholThreadArg *arg);
	void acquireData(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

// ---------------------------------------------------------------------------
// PrivateContext
// ---------------------------------------------------------------------------
struct HapProcessZabbixAPI::PrivateContext {
	MonitoringServerInfo serverInfo;
	SimpleSemaphore      startSem;
	SimpleSemaphore      mainThreadSem;
	// TODO: set readyFlag false when the connection is lost.
	AtomicValue<bool>    readyFlag;

	PrivateContext(void)
	: startSem(0),
	  mainThreadSem(0),
	  readyFlag(false)
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
	m_ctx = new PrivateContext();
}

HapProcessZabbixAPI::~HapProcessZabbixAPI()
{
	delete m_ctx;
}

int HapProcessZabbixAPI::mainLoopRun(void)
{
	const GError *error = getErrorOfCommandLineArg();
	if (getErrorOfCommandLineArg()) {
		MLPL_ERR("%s\n", error->message);
		return EXIT_FAILURE;
	}
	const HapCommandLineArg &clarg = getCommandLineArg();
	if (clarg.brokerUrl)
		setBrokerUrl(clarg.brokerUrl);
	if (clarg.queueAddress)
		setQueueAddress(clarg.queueAddress);

	enableWaitInitiatedAck();
	ackInitiated(); // To pass the first initiation
	HapZabbixAPI::start();
	HapProcess::start();
	g_main_loop_run(getGMainLoop());
	return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
//
// Method running on HapProcess's thread
//
gpointer HapProcessZabbixAPI::hapMainThread(HatoholThreadArg *arg)
{
top:
	m_ctx->startSem.wait(); // Wait for completion of initiation
	gpointer ret = NULL;
	try {
		ret = _hapMainThread(arg);
	} catch (const HapInitiatedException &e) {
		m_ctx->readyFlag = false;
		ackInitiated();
		goto top;
	} catch (...) {
		m_ctx->startSem.post();
		throw;
	}
	return ret;
}

gpointer HapProcessZabbixAPI::_hapMainThread(HatoholThreadArg *arg)
{
	if (!m_ctx->readyFlag && waitOnReady())
		return NULL;
	clearAuthToken();
	MLPL_INFO("Status: ready.\n");

	// TODO: HapZabbixAPI get MonitoringServerInfo in onInitiated()
	//       We should fix to use it to reduce the communication.
	if (initMonitoringServerInfo())
		return NULL;

	bool shouldExit = false;
	// TODO: this structure should be in the base class.
	while (!shouldExit) {
		try {
			acquireData();
		} catch (const HatoholException &e) {
			getArmStatus().logFailure(e.getFancyMessage());
			sendArmInfo(getArmStatus().getArmInfo());
			throw;
		}
		getArmStatus().logSuccess();
		sendArmInfo(getArmStatus().getArmInfo());
		shouldExit =
		  sleepForMainThread(m_ctx->serverInfo.pollingIntervalSec);
	}
	return NULL;
}

bool HapProcessZabbixAPI::waitOnReady(void)
{
	const size_t waitTimeSec = 10 * 60;
	bool shouldExit = false;
	while (!shouldExit) {
		shouldExit = sleepForMainThread(waitTimeSec);
		if (m_ctx->readyFlag)
			break;
		else
			MLPL_INFO("Waitting for the ready state.\n");
	}
	return shouldExit;
}

bool HapProcessZabbixAPI::initMonitoringServerInfo(void)
{
	const size_t sleepTimeSec = 30;
	while (true) {
		bool succeeded = getMonitoringServerInfo(m_ctx->serverInfo);
		if (succeeded)
			break;
		MLPL_INFO("Failed to get MonitoringServerInfo. "
		          "Retry after %zd sec.\n",  sleepTimeSec);
		if (sleepForMainThread(sleepTimeSec))
			return true;
	}
	setMonitoringServerInfo(m_ctx->serverInfo);
	setExceptionSleepTime(m_ctx->serverInfo.retryIntervalSec*1000);
	return false;
}

bool HapProcessZabbixAPI::sleepForMainThread(const int &sleepTimeInSec)
{
	SimpleSemaphore::Status status =
	  m_ctx->mainThreadSem.timedWait(sleepTimeInSec * 1000);
	// TODO: Add a mechanism to exit
	HATOHOL_ASSERT(
	  status != SimpleSemaphore::STAT_ERROR_UNKNOWN,
	  "Unexpected result: %d\n", status);
	throwInitiatedExceptionIfNeeded();

	return false;
}

void HapProcessZabbixAPI::acquireData(void)
{
	updateAuthTokenIfNeeded();
	workOnHostsAndHostgroupElements();
	workOnHostgroups();
	workOnTriggers();
	workOnEvents();
}

//
// Methods running on HapZabbixAPI's thread
//
void HapProcessZabbixAPI::onPreWaitInitiatedAck(void)
{
	m_ctx->mainThreadSem.post();
}

void HapProcessZabbixAPI::onPostWaitInitiatedAck(void)
{
	// If hapMainThread isn't in mainThreadSem.timedWait() when
	// onPreWaitInitiatedAck() is called, the count of mainThreadSem
	// overabounds. So we forecely reset here.
	m_ctx->mainThreadSem.init(0);
	m_ctx->startSem.post();
}

void HapProcessZabbixAPI::onReady(void)
{
	m_ctx->readyFlag = true;
	m_ctx->mainThreadSem.post();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	MLPL_INFO("hatohol-arm-plugin-zabbix. ver: %s\n", PACKAGE_VERSION);
	HapProcessZabbixAPI hapProc(argc, argv);
	return hapProc.mainLoopRun();
}
