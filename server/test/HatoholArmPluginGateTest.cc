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
  const MonitoringServerInfo &serverInfo, HapgTestArg &_arg)
: HatoholArmPluginGate(serverInfo),
  m_arg(_arg)
{
}

string HatoholArmPluginGateTest::callGenerateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return generateBrokerAddress(serverInfo);
}

void HatoholArmPluginGateTest::onSessionChanged(Session *session)
{
	if (m_arg.numRetry && session) {
		if (m_arg.retryCount < m_arg.numRetry)
			session->close();
		m_arg.retryCount++;
	}
}

void HatoholArmPluginGateTest::onReceived(SmartBuffer &smbuf)
{
	if (m_arg.numRetry && m_arg.retryCount <= m_arg.numRetry)
		return;
	m_arg.rcvMessage = std::string(smbuf, smbuf.size());
	m_arg.mainSem.post();
}

void HatoholArmPluginGateTest::onTerminated(const siginfo_t *siginfo)
{
	if (siginfo->si_signo == SIGCHLD &&
	    siginfo->si_code  == CLD_EXITED) {
		return;
	}
	m_arg.mainSem.post();
	m_arg.abnormalChildTerm = true;
	MLPL_ERR("si_signo: %d, si_code: %d\n",
	         siginfo->si_signo, siginfo->si_code);
}

int HatoholArmPluginGateTest::onCaughtException(const exception &e)
{
	printf("onCaughtException: %s\n", e.what());
	if (m_arg.numRetry) {
		canncelRetrySleepIfNeeded();
		return m_arg.retrySleepTime;
	}

	if (m_arg.rcvMessage.empty())
		m_arg.gotUnexceptedException = true;
	return HatoholArmPluginGate::NO_RETRY;
}

void HatoholArmPluginGateTest::onLaunchedProcess(
  const bool &succeeded, const ArmPluginInfo &armPluginInfo)
{
	m_arg.launchSucceeded = succeeded;
	m_arg.launchedSem.post();
}

void HatoholArmPluginGateTest::canncelRetrySleepIfNeeded(void)
{
	if (!m_arg.cancelRetrySleep)
		return;
	m_arg.mainSem.post();
}
