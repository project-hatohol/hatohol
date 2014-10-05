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
#include "NamedPipe.h"

using namespace std;
using namespace hfl;

static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

struct HapProcessStandard::Impl {
	// The getter and the user of serverInfo are running on different
	// threads. So we pass it safely with queue.
	SmartQueue<MonitoringServerInfo> serverInfoQueue;

	guint                firstStartAcquisitoinTaskId;
	guint                timerTag;
	NamedPipe            pipeRd, pipeWr;

	Impl(void)
	: firstStartAcquisitoinTaskId(INVALID_EVENT_ID),
	  timerTag(INVALID_EVENT_ID),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE)
	{
	}

	virtual ~Impl()
	{
		Utils::removeEventSourceIfNeeded(firstStartAcquisitoinTaskId);
		Utils::removeEventSourceIfNeeded(timerTag);
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
		HFL_ERR("%s\n", error->message);
		return EXIT_FAILURE;
	}
	const HapCommandLineArg &clarg = getCommandLineArg();
	if (clarg.brokerUrl)
		setBrokerUrl(clarg.brokerUrl);
	if (clarg.queueAddress)
		setQueueAddress(clarg.queueAddress);
	if (clarg.hapPipeName) {
		if (!initHapPipe(clarg.hapPipeName))
			return EXIT_FAILURE;
	}

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
gboolean HapProcessStandard::kickBasicAcquisition(void *data)
{
	HapProcessStandard *obj = static_cast<HapProcessStandard *>(data);
	obj->m_impl->timerTag = INVALID_EVENT_ID;
	obj->startAcquisition(&HapProcessStandard::acquireData);
	return G_SOURCE_REMOVE;
}

void HapProcessStandard::startAcquisition(
  AcquireFunc acquireFunc, const bool &setupTimer)
{
	if (m_impl->timerTag != INVALID_EVENT_ID) {
		// This condition may happen when unexpected initiation
		// happens and then onReady() is called. In the case,
		// we cancel the previous timer.
		HFL_INFO("Remove previously registered timer: %u\n",
		          m_impl->timerTag);
		bool succeeded =
		  Utils::removeEventSourceIfNeeded(m_impl->timerTag);
		if (!succeeded) {
			HFL_ERR("Failed to remove timer: %u\n",
		                 m_impl->timerTag);
		}
		m_impl->timerTag = INVALID_EVENT_ID;
	}

	// copy the paramter and set server info if needed
	HATOHOL_ASSERT(!m_impl->serverInfoQueue.empty(),
	               "No monitoring serverInfo.");
	while (m_impl->serverInfoQueue.size() > 1) // put away old data
		m_impl->serverInfoQueue.pop();

	// try to acquisition
	bool caughtException = false;
	string exceptionName;
	string exceptionMsg;
	HatoholError err(HTERR_UNINITIALIZED);
	HatoholArmPluginWatchType watchType = COLLECT_NG_PLGIN_INTERNAL_ERROR;
	try {
		err = (this->*acquireFunc)();
		if (err == HTERR_OK)
			getArmStatus().logSuccess();
		watchType = getHapWatchType(err);
	} catch (const HatoholException &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.getFancyMessage();
		caughtException = true;
		watchType = getHapWatchType(e.getErrCode());
	} catch (const exception &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.what();
		caughtException = true;
	} catch (...) {
		caughtException = true;
	}

	if (setupTimer) {
		setupNextTimer(err, caughtException,
		               exceptionName, exceptionMsg);
	}

	onCompletedAcquistion(err, watchType);
}

void HapProcessStandard::setupNextTimer(
  const HatoholError &err, const bool &caughtException,
  const string &exceptionName, const string &exceptionMsg)
{
	const MonitoringServerInfo &serverInfo = getMonitoringServerInfo();
	const int pollingIntervalSec = serverInfo.pollingIntervalSec;
	const int retryIntervalSec   = serverInfo.retryIntervalSec;
	guint intervalMSec = pollingIntervalSec * 1000;
	string errMsg;
	if (caughtException) {
		const char *name = exceptionName.c_str() ? : "Unknown";
		const char *msg  = exceptionMsg.c_str()  ? : "N/A";
		errMsg = StringUtils::sprintf(
		  "Caught an exception: (%s) %s", name, msg);
	} else if (err != HTERR_OK) {
		const char *name = err.getCodeName().c_str();
		const char *msg  = err.getMessage().c_str();
		const char *optMsg = err.getOptionMessage().empty() ?
		                       err.getOptionMessage().c_str() :
		                       "No optional message";
		errMsg = StringUtils::sprintf(
		  "Failed to get data: (%s) %s, %s", name, msg, optMsg);
	}
	if (!errMsg.empty()) {
		intervalMSec = retryIntervalSec * 1000;
		HFL_ERR("%s\n", errMsg.c_str());
		getArmStatus().logFailure(errMsg);
	}
	m_impl->timerTag = g_timeout_add(intervalMSec, kickBasicAcquisition,
	                                 this);
}

void HapProcessStandard::onCompletedAcquistion(
  const HatoholError &err, const HatoholArmPluginWatchType &watchType)
{
	try {
		sendArmInfo(getArmStatus().getArmInfo(), watchType);
	} catch (...) {
		HFL_ERR("Failed to send ArmInfo.\n");
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

HatoholError HapProcessStandard::fetchItem(void)
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
			unique_ptr<Impl> &impl = obj->m_impl;
			impl->firstStartAcquisitoinTaskId = INVALID_EVENT_ID;
			kickBasicAcquisition(obj);
		}
	};

	m_impl->serverInfoQueue.push(serverInfo);
	m_impl->firstStartAcquisitoinTaskId =
	  Utils::executeOnGLibEventLoop<HapProcessStandard>(
	    NoName::startAcquisition, this, ASYNC);
}

void HapProcessStandard::onReceivedReqFetchItem(void)
{
	struct NoName {
		static void startFetchItem(HapProcessStandard *obj)
		{
			obj->startAcquisition(&HapProcessStandard::fetchItem,
			                      false);
		}
	};

	// We call fetchItem() synchronously so that it can call reply()
	// that needs HatoholArmPluginInterface::Impl::currMessage,
	Utils::executeOnGLibEventLoop<HapProcessStandard>(
	  NoName::startFetchItem, this, SYNC);
}

int HapProcessStandard::onCaughtException(const exception &e)
{
	const MonitoringServerInfo &serverInfo = getMonitoringServerInfo();
	int retryIntervalSec;
	if (serverInfo.retryIntervalSec == 0)
		retryIntervalSec = DEFAULT_RETRY_INTERVAL;
	else
		retryIntervalSec = serverInfo.retryIntervalSec * 1000;
	HFL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
		  e.what(), retryIntervalSec);
	return retryIntervalSec;
}

//
// Method running on main thread incluing g_main_loop_run()
//
bool HapProcessStandard::initHapPipe(const string &hapPipeName)
{
	// NOTE: We not use PIPEs only to detect the death of the Hatohol server
	if (!m_impl->pipeRd.init(hapPipeName, _pipeRdErrCb, this))
		return false;
	if (!m_impl->pipeWr.init(hapPipeName, _pipeWrErrCb, this))
		return false;
	return true;
}

NamedPipe &HapProcessStandard::getHapPipeForRead(void)
{
	return m_impl->pipeRd;
}

NamedPipe &HapProcessStandard::getHapPipeForWrite(void)
{
	return m_impl->pipeWr;
}

void HapProcessStandard::exitProcess(void)
{
	g_main_loop_quit(getGMainLoop());
}

gboolean HapProcessStandard::pipeRdErrCb(
  GIOChannel *source, GIOCondition condition)
{
	HFL_INFO("Got callback (PIPE): %08x", condition);
	exitProcess();
	return TRUE;
}

gboolean HapProcessStandard::pipeWrErrCb(
  GIOChannel *source, GIOCondition condition)
{
	HFL_INFO("Got callback (PIPE): %08x", condition);
	exitProcess();
	return TRUE;
}

gboolean HapProcessStandard::_pipeRdErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	HapProcessStandard *obj = static_cast<HapProcessStandard *>(data);
	return obj->pipeRdErrCb(source, condition);
}

gboolean HapProcessStandard::_pipeWrErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	HapProcessStandard *obj = static_cast<HapProcessStandard *>(data);
	return obj->pipeWrErrCb(source, condition);
}
