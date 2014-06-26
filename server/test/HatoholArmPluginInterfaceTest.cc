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

#include <cppcutter.h>
#include <qpid/messaging/Connection.h>
#include "HatoholArmPluginInterfaceTest.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

// ---------------------------------------------------------------------------
//  HapiTestHelper
// ---------------------------------------------------------------------------
HapiTestHelper::HapiTestHelper(void)
: m_connectedSem(0),
  m_initiatedSem(0),
  m_handledCommandSem(0),
  m_lastHandledCode(NUM_HAPI_CMD)
{
	for (size_t i = 0; i < NUM_HAPI_CMD; i++)
		m_timesHandled[i] = 0;
}

void HapiTestHelper::onConnected(Connection &conn)
{
	m_connectedSem.post();
}

void HapiTestHelper::onInitiated(void)
{
	m_initiatedSem.post();
}

void HapiTestHelper::onHandledCommand(const HapiCommandCode &code)
{
	m_handledCodeLock.writeLock();
	m_lastHandledCode = code;
	m_timesHandled[code]++;
	m_handledCodeLock.unlock();
	m_initiatedSem.post();
}

SimpleSemaphore &HapiTestHelper::getConnectedSem(void)
{
	return m_connectedSem;
}

SimpleSemaphore &HapiTestHelper::getInitiatedSem(void)
{
	return m_initiatedSem;
}

SimpleSemaphore &HapiTestHelper::getHandledCommandSem(void)
{
	return m_initiatedSem;
}

void HapiTestHelper::assertWaitSemaphore(SimpleSemaphore &sem)
{
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, sem.timedWait(TIMEOUT));
}

void HapiTestHelper::assertWaitConnected(void)
{
	cut_trace(assertWaitSemaphore(getConnectedSem()));
}

void HapiTestHelper::assertWaitInitiated(void)
{
	cut_trace(assertWaitSemaphore(getInitiatedSem()));
}

void HapiTestHelper::assertWaitHandledCommand(
  const HapiCommandCode &code, const size_t &minCalledTimes)
{
	while (true) {
		m_handledCodeLock.readLock();
		bool shouldExit = (m_timesHandled[code] >= minCalledTimes);
		m_handledCodeLock.unlock();
		if (shouldExit)
			break;
		cut_trace(assertWaitSemaphore(getHandledCommandSem()));
	}
}

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTestBasic
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTestBasic::HatoholArmPluginInterfaceTestBasic(
  const bool workInServer)
: HatoholArmPluginInterface(workInServer)
{
	setQueueAddress("test-hatohol-arm-plugin-interface");
}

void HatoholArmPluginInterfaceTestBasic::onConnected(Connection &conn)
{
	HapiTestHelper::onConnected(conn);
}

void HatoholArmPluginInterfaceTestBasic::onInitiated(void)
{
	HapiTestHelper::onInitiated();
}

uint32_t HatoholArmPluginInterfaceTestBasic::callGetIncrementedSequenceId(void)
{
	return getIncrementedSequenceId();
}

void HatoholArmPluginInterfaceTestBasic::callSetSequenceId(
  const uint32_t &sequenceId)
{
	setSequenceId(sequenceId);
}

void HatoholArmPluginInterfaceTestBasic::assertStartAndWaitConnected(void)
{
	start();
	assertWaitConnected();
}

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTest
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTest::HatoholArmPluginInterfaceTest(void)
: m_rcvSem(0),
  m_msgIntercept(false)
{
}

HatoholArmPluginInterfaceTest::HatoholArmPluginInterfaceTest(
  HatoholArmPluginInterfaceTest &hapiSv)
: HatoholArmPluginInterfaceTestBasic(false),
  m_rcvSem(0),
  m_msgIntercept(false)
{
	setQueueAddress(hapiSv.getQueueAddress());
}

void HatoholArmPluginInterfaceTest::onReceived(mlpl::SmartBuffer &smbuf)
{
	if (!m_msgIntercept) {
		HatoholArmPluginInterfaceTestBasic::onReceived(smbuf);
		return;
	}
	setMessage(smbuf);
	m_rcvSem.post();
}

SimpleSemaphore &HatoholArmPluginInterfaceTest::getRcvSem(void)
{
	return m_rcvSem;
}

string HatoholArmPluginInterfaceTest::getMessage(void)
{
	m_lock.lock();
	string msg = m_message;
	m_lock.unlock();
	return msg;
}

void HatoholArmPluginInterfaceTest::setMessageIntercept(void)
{
	m_msgIntercept = true;
}

void HatoholArmPluginInterfaceTest::setMessage(const SmartBuffer &smbuf)
{
	m_lock.lock();
	m_message = string(smbuf, smbuf.size());
	m_lock.unlock();
}

