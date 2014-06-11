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
// HapiTestCtx
// ---------------------------------------------------------------------------
HapiTestCtx::HapiTestCtx(void)
: connected(false),
  quitOnConnected(false),
  useCustomOnReceived(false)
{
}

string HapiTestCtx::getReceivedMessage(void)
{
	lock.lock();
	string msg = receivedMessage;
	lock.unlock();
	return msg;
}

void HapiTestCtx::setReceivedMessage(const SmartBuffer &smbuf)
{
	lock.lock();
	receivedMessage = string(smbuf, smbuf.size());
	lock.unlock();
}

// ---------------------------------------------------------------------------
//  HapiTestHelper
// ---------------------------------------------------------------------------
HapiTestHelper::HapiTestHelper(void)
: m_connectedSem(0),
  m_initiatedSem(0)
{
}

void HapiTestHelper::onConnected(Connection &conn)
{
	m_connectedSem.post();
}

void HapiTestHelper::onInitiated(void)
{
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

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTestBasic
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTestBasic::HatoholArmPluginInterfaceTestBasic(
  HapiTestCtx &ctx, const string &addr, const bool workInServer)
: HatoholArmPluginInterface(addr, workInServer),
  m_testCtx(ctx)
{
}

void HatoholArmPluginInterfaceTestBasic::onConnected(Connection &conn)
{
	HapiTestHelper::onConnected(conn);
}

void HatoholArmPluginInterfaceTestBasic::onInitiated(void)
{
	HapiTestHelper::onInitiated();
}

HapiTestCtx &HatoholArmPluginInterfaceTestBasic::getHapiTestCtx(void)
{
	return m_testCtx;
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

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTest
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTest::HatoholArmPluginInterfaceTest(HapiTestCtx &ctx)
: HatoholArmPluginInterfaceTestBasic(ctx),
  m_rcvSem(0)
{
}

HatoholArmPluginInterfaceTest::HatoholArmPluginInterfaceTest(
  HapiTestCtx &ctx, HatoholArmPluginInterfaceTest &hapiSv)
: HatoholArmPluginInterfaceTestBasic(ctx, hapiSv.getQueueAddress(), false),
  m_rcvSem(0)
{
}

void HatoholArmPluginInterfaceTest::onReceived(mlpl::SmartBuffer &smbuf)
{
	if (!getHapiTestCtx().useCustomOnReceived) {
		HatoholArmPluginInterfaceTestBasic::onReceived(smbuf);
		return;
	}
	getHapiTestCtx().setReceivedMessage(smbuf);
	m_rcvSem.post();
}

SimpleSemaphore &HatoholArmPluginInterfaceTest::getRcvSem(void)
{
	return m_rcvSem;
}
