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
#include "Helpers.h"
#include "HatoholArmPluginInterface.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginInterface {

struct TestContext {
	AtomicValue<bool> timedout;
	AtomicValue<bool> connected;
	AtomicValue<bool> quitOnConnected;
	AtomicValue<bool> quitOnReceived;

	TestContext(void)
	: timedout(false),
	  connected(false),
	  quitOnConnected(false),
	  quitOnReceived(false)
	{
	}

	string getReceivedMessage(void)
	{
		lock.lock();
		string msg = receivedMessage;
		lock.unlock();
		return msg;
	}

	void setReceivedMessage(const SmartBuffer &smbuf)
	{
		lock.lock();
		receivedMessage = string(smbuf, smbuf.size());
		lock.unlock();
	}

private:
	MutexLock   lock;
	string      receivedMessage;
};

class TestHatoholArmPluginInterface : public HatoholArmPluginInterface {
public:
	TestHatoholArmPluginInterface(
	  TestContext &ctx,
	  const string &addr = "test-hatohol-arm-plugin-interface; {create: always}")
	: HatoholArmPluginInterface(addr),
	  m_testCtx(ctx),
	  m_started(false)
	{
	}

	virtual void onConnected(Connection &conn) // override
	{
		m_testCtx.connected = true;
		if (m_testCtx.quitOnConnected)
			m_loop.quit();
	}

	virtual void onReceived(SmartBuffer &smbuf)
	{
		m_testCtx.setReceivedMessage(smbuf);
		m_loop.quit();
	}

	void run(void)
	{
		if (!m_started) {
			start();
			m_started = true;
		}
		m_loop.run(timeoutCb, this);
	}

	TestContext &getTestContext(void)
	{
		return m_testCtx;
	}

protected:
	static gboolean timeoutCb(gpointer data)
	{
		TestHatoholArmPluginInterface *obj =
		  static_cast<TestHatoholArmPluginInterface *>(data);
		obj->m_testCtx.timedout = true;
		obj->m_loop.quit();
		return G_SOURCE_REMOVE;
	}

private:
	GMainLoopAgent m_loop;
	TestContext   &m_testCtx;
	bool           m_started;
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	HatoholArmPluginInterface hapi;
}

void test_onConnected(void)
{
	TestContext testCtx;
	testCtx.quitOnConnected = true;
	TestHatoholArmPluginInterface hapi(testCtx);
	hapi.run();
	cppcut_assert_equal(false, (bool)testCtx.timedout);
	cppcut_assert_equal(true, (bool)testCtx.connected);
}

void test_sendAndonReceived(void)
{
	const string testMessage = "FOO";
	TestContext testCtx;

	// wait for connection
	testCtx.quitOnConnected = true;
	TestHatoholArmPluginInterface hapi(testCtx);
	hapi.run();
	cppcut_assert_equal(false, (bool)hapi.getTestContext().timedout);

	// send the message and receive it
	hapi.send(testMessage);
	testCtx.quitOnConnected = false;
	testCtx.quitOnReceived = true;
	hapi.run();
	cppcut_assert_equal(false, (bool)hapi.getTestContext().timedout);
	cppcut_assert_equal(testMessage,
	                    hapi.getTestContext().getReceivedMessage());
}

} // namespace testHatoholArmPluginInterface
