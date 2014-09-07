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

#include <Mutex.h>
#include "HapProcessStandard.h"

using namespace std;
using namespace mlpl;

struct HapProcessStandard::Impl {
	MonitoringServerInfo serverInfo;
	Mutex                lock;
	guint                timerTag;
	AtomicValue<bool>    hasNewServerInfo;

	Impl(void)
	: timerTag(INVALID_EVENT_ID),
	  hasNewServerInfo(false)
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
	m_impl->lock.lock();
	int pollingIntervalSec = m_impl->serverInfo.pollingIntervalSec;
	int retryIntervalSec   = m_impl->serverInfo.retryIntervalSec;
	if (m_impl->hasNewServerInfo) {
		m_impl->hasNewServerInfo = false;
	}
	m_impl->lock.unlock();

	// try to acquisition
	bool caughtException = false;
	string exceptionName;
	string exceptionMsg;
	try {
		acquireData();
		getArmStatus().logSuccess();
	} catch (const HatoholException &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.getFancyMessage();
		caughtException = true;
	} catch (const exception &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.what();
		caughtException = true;
	} catch (...) {
		caughtException = true;
	}

	// Set up a timer for next aquisition
	guint intervalMSec = pollingIntervalSec * 1000;
	if (caughtException) {
		intervalMSec = retryIntervalSec * 1000;
		const char *name = exceptionName.c_str() ? : "Unknown";
		const char *msg  = exceptionMsg.c_str()  ? : "N/A";
		string errMsg = StringUtils::sprintf(
		  "Caught an exception: (%s) %s", name, msg);
		MLPL_ERR("%s\n", errMsg.c_str());
		getArmStatus().logFailure(errMsg);
	}
	m_impl->timerTag = g_timeout_add(intervalMSec, acquisitionTimerCb, this);

	// update ArmInfo
	try {
		sendArmInfo(getArmStatus().getArmInfo());
	} catch (...) {
		MLPL_ERR("Failed to send ArmInfo.\n");
	}
}

void HapProcessStandard::acquireData(void)
{
}

//
// Method running on HatoholArmPluginStandard thread
//
void HapProcessStandard::onReady(const MonitoringServerInfo &serverInfo)
{
	m_impl->hasNewServerInfo = true;
	struct NoName {
		static void startAcquisition(HapProcessStandard *obj)
		{
			obj->startAcquisition();
		}
	};
	Utils::executeOnGLibEventLoop<HapProcessStandard>(
	  NoName::startAcquisition, this, ASYNC);
}

