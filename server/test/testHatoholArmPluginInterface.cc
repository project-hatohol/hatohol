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
#include "HatoholArmPluginInterfaceTest.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginInterface {

static const size_t TIMEOUT = 5000;

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	HatoholArmPluginInterface hapi;
}

void test_onConnected(void)
{
	HapiTestCtx testCtx;
	testCtx.quitOnConnected = true;
	HatoholArmPluginInterfaceTest hapi(testCtx);
	hapi.start();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    testCtx.sem.timedWait(TIMEOUT));
	cppcut_assert_equal(true, (bool)testCtx.connected);
}

void test_sendAndonReceived(void)
{
	const string testMessage = "FOO";
	HapiTestCtx testCtx;

	// wait for connection
	testCtx.quitOnConnected = true;
	HatoholArmPluginInterfaceTest hapi(testCtx);
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
	                    hapi.getHapiTestCtx().getReceivedMessage());
}

void test_registCommandHandler(void)
{
	struct Hapi : public HatoholArmPluginInterfaceTestBasic {
		HapiTestCtx           testCtx;
		const HapiCommandCode testCmdCode;
		HapiCommandCode       gotCmdCode;

		Hapi(void)
		: HatoholArmPluginInterfaceTestBasic(testCtx),
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
	struct Hapi : public HatoholArmPluginInterfaceTestBasic {
		HapiTestCtx           testCtx;
		HapiResponseCode      gotResCode;

		Hapi(void)
		: HatoholArmPluginInterfaceTestBasic(testCtx),
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
