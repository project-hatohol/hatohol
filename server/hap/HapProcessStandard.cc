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

#include <SmartQueue.h>
#include "HapProcessStandard.h"

using namespace std;
using namespace mlpl;

static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

struct HapProcessStandard::Impl {
	// The getter and the user of serverInfo are running on different
	// threads. So we pass it safely with queue.
	SmartQueue<MonitoringServerInfo> serverInfoQueue;

	guint                timerTag;

	Impl(void)
	: timerTag(INVALID_EVENT_ID)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessStandard::HapProcessStandard(int argc, char *argv[])
: HapProcess(argc, argv),
  m_impl(new Impl())
{
	initGLib();
}

HapProcessStandard::~HapProcessStandard()
{
}

int HapProcessStandard::mainLoopRun(void)
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

	HatoholArmPluginStandard::start();
	g_main_loop_run(getGMainLoop());
	return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
//
// Method running on HapProcess's thread
//
gboolean HapProcessStandard::acquisitionTimerCb(void *data)
{
	HapProcessStandard *obj = static_cast<HapProcessStandard *>(data);
	obj->m_impl->timerTag = INVALID_EVENT_ID;
	obj->startAcquisition();
	return G_SOURCE_REMOVE;
}

void HapProcessStandard::startAcquisition(void)
{
	if (m_impl->timerTag != INVALID_EVENT_ID) {
		// This condition may happen when unexpected initiation
		// happens and then onReady() is called. In the case,
		// we cancel the previous timer.
		MLPL_INFO("Remove previously registered timer: %u\n",
		          m_impl->timerTag);
		bool succeeded =
		  Utils::removeEventSourceIfNeeded(m_impl->timerTag);
		if (!succeeded) {
			MLPL_ERR("Failed to remove timer: %u\n",
		                 m_impl->timerTag);
		}
		m_impl->timerTag = INVALID_EVENT_ID;
	}

	// copy the paramter and set server info if needed
	HATOHOL_ASSERT(!m_impl->serverInfoQueue.empty(),
	               "No monitoring serverInfo.");
	while (m_impl->serverInfoQueue.size() > 1) // put away old data
		m_impl->serverInfoQueue.pop();

	const MonitoringServerInfo &serverInfo = getMonitoringServerInfo();
	const int pollingIntervalSec = serverInfo.pollingIntervalSec;
	const int retryIntervalSec   = serverInfo.retryIntervalSec;

	// try to acquisition
	bool caughtException = false;
	string exceptionName;
	string exceptionMsg;
	HatoholError err(HTERR_UNINITIALIZED);
	HatoholArmPluginWatchType watchResult = COLLECT_NG_PLGIN_INTERNAL_ERROR;
	try {
		err = acquireData();
		if (err == HTERR_OK)
			getArmStatus().logSuccess();
		watchResult = getHapWatchType(err);
	} catch (const HatoholException &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.getFancyMessage();
		caughtException = true;
		watchResult = getHapWatchType(e.getErrCode());
	} catch (const exception &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.what();
		caughtException = true;
	} catch (...) {
		caughtException = true;
	}

	// Set up a timer for next aquisition
	guint intervalMSec = pollingIntervalSec * 1000;
	string errMsg;
	if (caughtException) {
		const char *name = exceptionName.c_str() ? : "Unknown";
		const char *msg  = exceptionMsg.c_str()  ? : "N/A";
		string errMsg = StringUtils::sprintf(
		  "Caught an exception: (%s) %s", name, msg);
	} else if (err != HTERR_OK) {
		const char *name = err.getCodeName().c_str();
		const char *msg  = err.getMessage().c_str();
		const char *optMsg = err.getOptionMessage().empty() ?
		                       err.getOptionMessage().c_str() :
		                       "No optional message";
		string errMsg = StringUtils::sprintf(
		  "Failed to get data: (%s) %s, %s", name, msg, optMsg);
	}
	if (!errMsg.empty()) {
		intervalMSec = retryIntervalSec * 1000;
		MLPL_ERR("%s\n", errMsg.c_str());
		getArmStatus().logFailure(errMsg);
	}
	m_impl->timerTag = g_timeout_add(intervalMSec, acquisitionTimerCb, this);

	// update ArmInfo
	try {
		sendArmInfo(getArmStatus().getArmInfo(), watchResult);
	} catch (...) {
		MLPL_ERR("Failed to send ArmInfo.\n");
	}
}

const MonitoringServerInfo &
HapProcessStandard::getMonitoringServerInfo(void) const
{
	return m_impl->serverInfoQueue.front();
}

HatoholError HapProcessStandard::acquireData(void)
{
	return HTERR_NOT_IMPLEMENTED;
}

HatoholArmPluginWatchType HapProcessStandard::getHapWatchType(
  const HatoholError &err)
{
	if (err == HTERR_OK)
		return COLLECT_OK;
	return COLLECT_NG_HATOHOL_INTERNAL_ERROR;
}

//
// Method running on HatoholArmPluginStandard thread
//
void HapProcessStandard::onReady(const MonitoringServerInfo &serverInfo)
{
	struct NoName {
		static void startAcquisition(HapProcessStandard *obj)
		{
			obj->startAcquisition();
		}
	};

	m_impl->serverInfoQueue.push(serverInfo);
	Utils::executeOnGLibEventLoop<HapProcessStandard>(
	  NoName::startAcquisition, this, ASYNC);
}

int HapProcessStandard::onCaughtException(const exception &e)
{
	const MonitoringServerInfo &serverInfo = getMonitoringServerInfo();
	int retryIntervalSec;
	if (serverInfo.retryIntervalSec == 0)
		retryIntervalSec = DEFAULT_RETRY_INTERVAL;
	else
		retryIntervalSec = serverInfo.retryIntervalSec * 1000;
	MLPL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
		  e.what(), retryIntervalSec);
	return retryIntervalSec;
}
