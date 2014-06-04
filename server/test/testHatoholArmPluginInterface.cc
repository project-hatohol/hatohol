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
#include <SimpleSemaphore.h>
#include "Helpers.h"
#include "HatoholArmPluginInterface.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginInterface {

static const size_t TIMEOUT = 5000;

struct TestContext {
	SimpleSemaphore   sem;
	AtomicValue<bool> connected;
	AtomicValue<bool> quitOnConnected;
	AtomicValue<bool> quitOnReceived;

	TestContext(void)
	: sem(0),
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

class TestBasicHatoholArmPluginInterface : public HatoholArmPluginInterface {
	struct OtherSide {
		TestContext ctx;
		TestBasicHatoholArmPluginInterface *obj;
		OtherSide(const string &queueAddr)
		: obj(NULL)
		{
			ctx.quitOnConnected = true;
			obj = new TestBasicHatoholArmPluginInterface(
			            ctx, queueAddr, false);
			obj->start();
			cppcut_assert_equal(
			  SimpleSemaphore::STAT_OK,
			  obj->getTestContext().sem.timedWait(TIMEOUT));
		}

		virtual ~OtherSide()
		{
			if (obj)
				delete obj;
		}
	};

public:
	TestBasicHatoholArmPluginInterface(
	  TestContext &ctx,
	  const string &addr = "test-hatohol-arm-plugin-interface",
	  const bool workInServer = true)
	: HatoholArmPluginInterface(addr, workInServer),
	  m_testCtx(ctx)
	{
	}

	virtual void onConnected(Connection &conn) override
	{
		m_testCtx.connected = true;
		if (m_testCtx.quitOnConnected)
			m_testCtx.sem.post();
	}

	TestContext &getTestContext(void)
	{
		return m_testCtx;
	}

	void sendAsOther(const string &msg)
	{
		OtherSide other(getQueueAddress());
		other.obj->send(msg);
	}

	void sendAsOther(const SmartBuffer &smbuf)
	{
		OtherSide other(getQueueAddress());
		other.obj->send(smbuf);
	}

protected:

	TestContext &m_testCtx;
};

class TestHatoholArmPluginInterface : public TestBasicHatoholArmPluginInterface {
public:
	SimpleSemaphore rcvSem;

	TestHatoholArmPluginInterface(TestContext &ctx)
	: TestBasicHatoholArmPluginInterface(ctx),
	  rcvSem(0)
	{
	}

	virtual void onReceived(SmartBuffer &smbuf) override
	{
		m_testCtx.setReceivedMessage(smbuf);
		rcvSem.post();
	}
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
	hapi.start();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    testCtx.sem.timedWait(TIMEOUT));
	cppcut_assert_equal(true, (bool)testCtx.connected);
}

void test_sendAndonReceived(void)
{
	const string testMessage = "FOO";
	TestContext testCtx;

	// wait for connection
	testCtx.quitOnConnected = true;
	TestHatoholArmPluginInterface hapi(testCtx);
	hapi.start();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    testCtx.sem.timedWait(TIMEOUT));

	// send the message and receive it
	hapi.sendAsOther(testMessage);
	testCtx.quitOnConnected = false;
	testCtx.quitOnReceived = true;
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapi.rcvSem.timedWait(TIMEOUT));
	cppcut_assert_equal(testMessage,
	                    hapi.getTestContext().getReceivedMessage());
}

void test_registCommandHandler(void)
{
	struct Hapi : public TestBasicHatoholArmPluginInterface {
		TestContext           testCtx;
		const HapiCommandCode testCmdCode;
		HapiCommandCode       gotCmdCode;

		Hapi(void)
		: TestBasicHatoholArmPluginInterface(testCtx),
		  testCmdCode((HapiCommandCode)5),
		  gotCmdCode((HapiCommandCode)0)
		{
			testCtx.quitOnConnected = true;
			registCommandHandler(
			  testCmdCode, (CommandHandler)&Hapi::handler);
		}

		void sendCmdCode(void)
		{
			SmartBuffer cmdBuf(sizeof(HapiCommandHeader));
			HapiCommandHeader *header =
			  cmdBuf.getPointer<HapiCommandHeader>();
			header->type = HAPI_MSG_COMMAND;
			header->code = testCmdCode;
			sendAsOther(cmdBuf);
		}

		void handler(const HapiCommandHeader *header)
		{
			gotCmdCode = (HapiCommandCode)header->code;
			testCtx.sem.post();
		}
	} hapi;

	// wait for the connection
	hapi.start();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapi.testCtx.sem.timedWait(TIMEOUT));

	// send command code and wait for the callback.
	hapi.sendCmdCode();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapi.testCtx.sem.timedWait(TIMEOUT));
	cppcut_assert_equal(hapi.testCmdCode, hapi.gotCmdCode);
}

void test_onGotResponse(void)
{
	struct Hapi : public TestBasicHatoholArmPluginInterface {
		TestContext           testCtx;
		HapiResponseCode      gotResCode;

		Hapi(void)
		: TestBasicHatoholArmPluginInterface(testCtx),
		  gotResCode(NUM_HAPI_CMD_RES)
		{
			testCtx.quitOnConnected = true;
		}
		
		void sendReply(void)
		{
			SmartBuffer resBuf(sizeof(HapiResponseHeader));
			HapiResponseHeader *header =
			  resBuf.getPointer<HapiResponseHeader>();
			header->type = HAPI_MSG_RESPONSE;
			header->code = HAPI_RES_OK;
			sendAsOther(resBuf);
		}

		void onGotResponse(const HapiResponseHeader *header,
		                   SmartBuffer &resBuf) override
		{
			gotResCode =
			  static_cast<HapiResponseCode>(header->code);
			testCtx.sem.post();
		}
	} hapi;

	// wait for the connection
	hapi.start();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapi.testCtx.sem.timedWait(TIMEOUT));

	// send command code and wait for the callback.
	hapi.sendReply();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapi.testCtx.sem.timedWait(TIMEOUT));
	cppcut_assert_equal(HAPI_RES_OK, hapi.gotResCode);
}

} // namespace testHatoholArmPluginInterface
