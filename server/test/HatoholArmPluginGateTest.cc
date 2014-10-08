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
using namespace hfl;
using namespace qpid::messaging;

HatoholArmPluginGateTest::HatoholArmPluginGateTest(
  const MonitoringServerInfo &serverInfo, HapgTestCtx &_ctx)
: HatoholArmPluginGate(serverInfo),
  m_ctx(_ctx)
{
}

string HatoholArmPluginGateTest::callGenerateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return generateBrokerAddress(serverInfo);
}

void HatoholArmPluginGateTest::callSendTerminateCommand(void)
{
	sendTerminateCommand();
}

void HatoholArmPluginGateTest::onSessionChanged(Session *session)
{
	if (m_ctx.numRetry && session) {
		if (m_ctx.retryCount < m_ctx.numRetry)
			session->close();
		m_ctx.retryCount++;
	}
}

void HatoholArmPluginGateTest::onReceived(SmartBuffer &smbuf)
{
	if (m_ctx.useDefaultReceivedHandler) {
		HatoholArmPluginGate::onReceived(smbuf);
		return;
	}
	if (m_ctx.numRetry && m_ctx.retryCount <= m_ctx.numRetry)
		return;

	string rcvMessage = std::string(smbuf, smbuf.size());
	if (!m_ctx.expectRcvMessage.empty()) {
		if (!m_ctx.rcvMessage.empty())
			return; // don't receive more.
		if (m_ctx.expectRcvMessage != rcvMessage)
			return; // don't post the semaphore.
	}
	m_ctx.rcvMessage = rcvMessage;
	m_ctx.mainSem.post();
}

void HatoholArmPluginGateTest::onTerminated(const siginfo_t *siginfo)
{
	HatoholArmPluginGate::onTerminated(siginfo);
	if (siginfo->si_signo == SIGCHLD &&
	    siginfo->si_code  == CLD_EXITED) {
		return;
	}
	m_ctx.mainSem.post();
	m_ctx.abnormalChildTerm = true;
	HFL_ERR("si_signo: %d, si_code: %d\n",
	         siginfo->si_signo, siginfo->si_code);
}

int HatoholArmPluginGateTest::onCaughtException(const exception &e)
{
	HFL_INFO("onCaughtException: %s\n", e.what());
	if (m_ctx.numRetry) {
		canncelRetrySleepIfNeeded();
		return m_ctx.retrySleepTime;
	}

	if (m_ctx.rcvMessage.empty())
		m_ctx.gotUnexceptedException = true;
	return HatoholArmPluginGate::NO_RETRY;
}

void HatoholArmPluginGateTest::onLaunchedProcess(
  const bool &succeeded, const ArmPluginInfo &armPluginInfo)
{
	m_ctx.launchSucceeded = succeeded;
	m_ctx.pluginPid = getPid();
	m_ctx.launchedSem.post();
}

void HatoholArmPluginGateTest::onConnected(Connection &conn)
{
	HatoholArmPluginGate::onConnected(conn);
	HapiTestHelper::onConnected(conn);
}

void HatoholArmPluginGateTest::onInitiated(void)
{
	HatoholArmPluginGate::onInitiated();
	HapiTestHelper::onInitiated();
}

void HatoholArmPluginGateTest::onHandledCommand(const HapiCommandCode &code)
{
	HatoholArmPluginGate::onHandledCommand(code);
	HapiTestHelper::onHandledCommand(code);
}

void HatoholArmPluginGateTest::canncelRetrySleepIfNeeded(void)
{
	if (!m_ctx.cancelRetrySleep)
		return;
	m_ctx.mainSem.post();
}

NamedPipe &HatoholArmPluginGateTest::callGetHapPipeForRead(void)
{
	return getHapPipeForRead();
}

NamedPipe &HatoholArmPluginGateTest::callGetHapPipeForWrite(void)
{
	return getHapPipeForWrite();
}
