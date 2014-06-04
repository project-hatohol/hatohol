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

#include "HatoholArmPluginGateTest.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

HatoholArmPluginGateTest::HatoholArmPluginGateTest(
  const MonitoringServerInfo &serverInfo, const HapgTestArg &_arg)
: HatoholArmPluginGate(serverInfo),
  arg(_arg),
  abnormalChildTerm(false),
  gotUnexceptedException(false),
  retryCount(0),
  launchedSem(0),
  launchSucceeded(false),
  mainSem(0)
{
}

string HatoholArmPluginGateTest::callGenerateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return generateBrokerAddress(serverInfo);
}

void HatoholArmPluginGateTest::onSessionChanged(Session *session)
{
	if (arg.numRetry && session) {
		if (retryCount < arg.numRetry)
			session->close();
		retryCount++;
	}
}

void HatoholArmPluginGateTest::onReceived(SmartBuffer &smbuf)
{
	if (arg.numRetry && retryCount <= arg.numRetry)
		return;
	rcvMessage = std::string(smbuf, smbuf.size());
	mainSem.post();
}

void HatoholArmPluginGateTest::onTerminated(const siginfo_t *siginfo)
{
	if (siginfo->si_signo == SIGCHLD &&
	    siginfo->si_code  == CLD_EXITED) {
		return;
	}
	mainSem.post();
	abnormalChildTerm = true;
	MLPL_ERR("si_signo: %d, si_code: %d\n",
	         siginfo->si_signo, siginfo->si_code);
}

int HatoholArmPluginGateTest::onCaughtException(const exception &e)
{
	printf("onCaughtException: %s\n", e.what());
	if (arg.numRetry) {
		canncelRetrySleepIfNeeded();
		return arg.retrySleepTime;
	}

	if (rcvMessage.empty())
		gotUnexceptedException = true;
	return HatoholArmPluginGate::NO_RETRY;
}

void HatoholArmPluginGateTest::onLaunchedProcess(
  const bool &succeeded, const ArmPluginInfo &armPluginInfo)
{
	launchSucceeded = succeeded;
	launchedSem.post();
}

void HatoholArmPluginGateTest::canncelRetrySleepIfNeeded(void)
{
	if (!arg.cancelRetrySleep)
		return;
	mainSem.post();
}
