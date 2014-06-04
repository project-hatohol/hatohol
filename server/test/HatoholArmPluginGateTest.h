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

#ifndef HatoholArmPluginGateTest_h
#define HatoholArmPluginGateTest_h
#include <string>
#include <exception>
#include <qpid/messaging/Session.h>
#include <SmartBuffer.h>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginGate.h"
#include "DBClientTest.h"

struct HapgTestArg {
	MonitoringSystemType monitoringSystemType;
	bool                 expectedResultOfStart;
	bool                 waitMainSem;
	bool                 checkMessage;
	size_t               numRetry;
	size_t               retrySleepTime; // msec.
	bool                 cancelRetrySleep;
	bool                 checkNumRetry;

	HapgTestArg(void)
	: monitoringSystemType(MONITORING_SYSTEM_HAPI_TEST),
	  expectedResultOfStart(false),
	  waitMainSem(false),
	  checkMessage(false),
	  numRetry(0),
	  retrySleepTime(1),
	  cancelRetrySleep(false),
	  checkNumRetry(false)
	{
	}
};

class HatoholArmPluginGateTest : public HatoholArmPluginGate {
public:
	const HapgTestArg &arg;
	bool        abnormalChildTerm;
	std::string rcvMessage;
	bool        gotUnexceptedException;
	size_t      retryCount;
	mlpl::SimpleSemaphore launchedSem;
	bool                  launchSucceeded;
	mlpl::SimpleSemaphore mainSem;

	HatoholArmPluginGateTest(const MonitoringServerInfo &serverInfo,
	                           const HapgTestArg &_arg);
	static std::string callGenerateBrokerAddress(
	  const MonitoringServerInfo &serverInfo);

	// We assume these virtual funcitons are called from
	// the plugin's thread.
	// I.e. we must not call cutter's assertions in them.
	virtual void onSessionChanged(
	  qpid::messaging::Session *session) override;
	virtual void onReceived(mlpl::SmartBuffer &smbuf) override;
	virtual void onTerminated(const siginfo_t *siginfo) override;
	virtual int onCaughtException(const std::exception &e) override;
	virtual void onLaunchedProcess(
	  const bool &succeeded, const ArmPluginInfo &armPluginInfo) override;
	void canncelRetrySleepIfNeeded(void);
};

#endif // HatoholArmPluginGateTest_h
