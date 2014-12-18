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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <HapZabbixAPI.h>
#include <SimpleSemaphore.h>
#include <Mutex.h>
#include "Utils.h"
#include "HapProcess.h"

using namespace std;
using namespace mlpl;

static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

class HapProcessZabbixAPI : public HapProcess, public HapZabbixAPI {
public:
	HapProcessZabbixAPI(int argc, char *argv[]);
	virtual ~HapProcessZabbixAPI();
	int mainLoopRun(void);
	virtual int onCaughtException(const exception &e) override;

protected:
	// called from HapZabbixAPI
	void onReady(const MonitoringServerInfo &serverInfo) override;

	static gboolean acquisitionTimerCb(void *data);
	void startAcquisition(void);
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
	Mutex                lock;
	guint                timerTag;
	bool                 hasNewServerInfo;

	PrivateContext(void)
	: timerTag(INVALID_EVENT_ID),
	  hasNewServerInfo(false)
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

	HapZabbixAPI::start();
	g_main_loop_run(getGMainLoop());
	return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
//
// Method running on HapProcess's thread
//
gboolean  HapProcessZabbixAPI::acquisitionTimerCb(void *data)
{
	HapProcessZabbixAPI *obj = static_cast<HapProcessZabbixAPI *>(data);
	obj->m_ctx->timerTag = INVALID_EVENT_ID;
	obj->startAcquisition();
	return G_SOURCE_REMOVE;
}

void HapProcessZabbixAPI::startAcquisition(void)
{
	if (m_ctx->timerTag != INVALID_EVENT_ID) {
		// This condition may happen when unexpected initiation
		// happens and then onReady() is called. In the case,
		// we cancel the previous timer.
		MLPL_INFO("Remove previously registered timer: %u\n",
		          m_ctx->timerTag);
		bool succeeded =
		  Utils::removeEventSourceIfNeeded(m_ctx->timerTag);
		if (!succeeded) {
			MLPL_ERR("Failed to remove timer: %u\n",
		                 m_ctx->timerTag);
		}
		m_ctx->timerTag = INVALID_EVENT_ID;
	}

	// copy the paramter and set server info if needed
	m_ctx->lock.lock();
	int pollingIntervalSec = m_ctx->serverInfo.pollingIntervalSec;
	int retryIntervalSec   = m_ctx->serverInfo.retryIntervalSec;
	if (m_ctx->hasNewServerInfo) {
		setMonitoringServerInfo(m_ctx->serverInfo);
		m_ctx->hasNewServerInfo = false;
	}
	m_ctx->lock.unlock();

	// try to acquisition
	bool caughtException = false;
	string exceptionName;
	string exceptionMsg;
	HatoholErrorCode exceptionErrorCode;
	HatoholArmPluginWatchType type = COLLECT_NG_PLGIN_INTERNAL_ERROR;
	try {
		acquireData();
		type = COLLECT_OK;
	} catch (const HatoholException &e) {
		exceptionName = DEMANGLED_TYPE_NAME(e);
		exceptionMsg  = e.getFancyMessage();
		caughtException = true;
		exceptionErrorCode = e.getErrCode();
		if (exceptionErrorCode == HTERR_FAILED_CONNECT_ZABBIX) {
			type = COLLECT_NG_DISCONNECT_ZABBIX;
		} else if (exceptionErrorCode == HTERR_FAILED_TO_PARSE_JSON_DATA) {
			type = COLLECT_NG_PARSER_ERROR;
		}
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
	} else {
		type = COLLECT_OK;
	}

	m_ctx->timerTag = g_timeout_add(intervalMSec, acquisitionTimerCb, this);

	// update ArmInfo
	try {
		sendArmInfo(getArmStatus().getArmInfo(), type);
	} catch (...) {
		MLPL_ERR("Failed to send ArmInfo.\n");
	}
}

void HapProcessZabbixAPI::acquireData(void)
{
	updateAuthTokenIfNeeded();
	workOnHostsAndHostgroupElements();
	workOnHostgroups();
	workOnTriggers();
	workOnEvents();
	getArmStatus().logSuccess();
}

//
// Methods running on HapZabbixAPI's thread
//
void HapProcessZabbixAPI::onReady(const MonitoringServerInfo &serverInfo)
{
	m_ctx->lock.lock();
	m_ctx->serverInfo = serverInfo;
	m_ctx->hasNewServerInfo = true;
	m_ctx->lock.unlock();
	struct NoName {
		static void startAcquisition(HapProcessZabbixAPI *obj)
		{
			obj->startAcquisition();
		}
	};
	Utils::executeOnGLibEventLoop<HapProcessZabbixAPI>(
	  NoName::startAcquisition, this, ASYNC);
}

int HapProcessZabbixAPI::onCaughtException(const exception &e)
{
	int retryIntervalSec;
	if (m_ctx->serverInfo.retryIntervalSec == 0)
		retryIntervalSec = DEFAULT_RETRY_INTERVAL;
	else
		retryIntervalSec = m_ctx->serverInfo.retryIntervalSec * 1000;
	MLPL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
		  e.what(), retryIntervalSec);
	return retryIntervalSec;
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
