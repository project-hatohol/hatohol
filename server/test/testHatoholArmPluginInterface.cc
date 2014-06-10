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
	                    hapi.getRcvSem().timedWait(TIMEOUT));
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

void test_getString(void)
{
	SmartBuffer buf(10);
	char *head = buf.getPointer<char>(1);
	const size_t offset = 3;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = '\0';
	cut_assert_equal_string(
	  "AB", HatoholArmPluginInterface::getString(buf, head, offset, 2));
}

void test_getStringWithTooLongParam(void)
{
	SmartBuffer buf(3);
	char *head = buf.getPointer<char>(0);
	const size_t offset = 0;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = '\0';
	cut_assert_equal_string(
	  NULL,
	  HatoholArmPluginInterface::getString(buf, head, offset, 3));
}

void test_getStringWithoutNullTerm(void)
{
	SmartBuffer buf(3);
	char *head = buf.getPointer<char>(0);
	const size_t offset = 0;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = 'C';
	cut_assert_equal_string(
	  NULL,
	  HatoholArmPluginInterface::getString(buf, head, offset, 2));
}

void test_putString(void)
{
	char _buf[50];
	char *refAddr = &_buf[15];
	char *buf = &_buf[20];
	string str = "Cat";
	uint16_t offset;
	uint16_t length;
	char *next = HatoholArmPluginInterface::putString(buf, refAddr, str,
	                                                  &offset, &length);
	cppcut_assert_equal((uint16_t)5, offset);
	cppcut_assert_equal((uint16_t)3, length);
	cppcut_assert_equal(&_buf[24], next);
	cut_assert_equal_string("Cat", buf);
}

void test_getIncrementedSequenceId(void)
{
	HapiTestCtx ctx;
	HatoholArmPluginInterfaceTest plugin(ctx);
	cppcut_assert_equal(1u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(2u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(3u, plugin.callGetIncrementedSequenceId());
}

void test_getIncrementedSequenceIdAroundMax(void)
{
	HapiTestCtx ctx;
	HatoholArmPluginInterfaceTest plugin(ctx);
	plugin.callSetSequenceId(HatoholArmPluginInterface::SEQ_ID_MAX-1);
	cppcut_assert_equal(
	  HatoholArmPluginInterface::SEQ_ID_MAX,
	  plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(0u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(1u, plugin.callGetIncrementedSequenceId());
}

} // namespace testHatoholArmPluginInterface
