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
: sem(0),
  connected(false),
  quitOnConnected(false),
  quitOnReceived(false)
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
//  HatoholArmPluginInterfaceTestHelper
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTestHelper::HatoholArmPluginInterfaceTestHelper(void)
: m_connectedSem(0)
{
}

void HatoholArmPluginInterfaceTestHelper::onConnected(Connection &conn)
{
	m_connectedSem.post();
}

SimpleSemaphore &HatoholArmPluginInterfaceTestHelper::getConnectedSem(void)
{
	return m_connectedSem;
}

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTestBasic
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTestBasic::OtherSide::OtherSide(
  const string &queueAddr)
: obj(NULL)
{
	ctx.quitOnConnected = true;
	obj = new HatoholArmPluginInterfaceTestBasic(ctx, queueAddr, false);
	obj->start();
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK,
	  obj->getHapiTestCtx().sem.timedWait(TIMEOUT));
}

HatoholArmPluginInterfaceTestBasic::OtherSide::~OtherSide()
{
	if (obj)
		delete obj;
}

HatoholArmPluginInterfaceTestBasic::HatoholArmPluginInterfaceTestBasic(
  HapiTestCtx &ctx, const string &addr, const bool workInServer)
: HatoholArmPluginInterface(addr, workInServer),
  m_testCtx(ctx)
{
}

void HatoholArmPluginInterfaceTestBasic::onConnected(Connection &conn)
{
	m_testCtx.connected = true;
	if (m_testCtx.quitOnConnected)
		m_testCtx.sem.post();
}

HapiTestCtx &HatoholArmPluginInterfaceTestBasic::getHapiTestCtx(void)
{
	return m_testCtx;
}

void HatoholArmPluginInterfaceTestBasic::sendAsOther(const string &msg)
{
	OtherSide other(getQueueAddress());
	other.obj->send(msg);
}

void HatoholArmPluginInterfaceTestBasic::sendAsOther(const SmartBuffer &smbuf)
{
	OtherSide other(getQueueAddress());
	other.obj->send(smbuf);
}

// ---------------------------------------------------------------------------
//  HatoholArmPluginInterfaceTest
// ---------------------------------------------------------------------------
HatoholArmPluginInterfaceTest::HatoholArmPluginInterfaceTest(HapiTestCtx &ctx)
: HatoholArmPluginInterfaceTestBasic(ctx),
  m_rcvSem(0)
{
}

void HatoholArmPluginInterfaceTest::onReceived(mlpl::SmartBuffer &smbuf)
{
	getHapiTestCtx().setReceivedMessage(smbuf);
	m_rcvSem.post();
}

SimpleSemaphore &HatoholArmPluginInterfaceTest::getRcvSem(void)
{
	return m_rcvSem;
}
