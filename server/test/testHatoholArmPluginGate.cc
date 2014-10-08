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

#include <fstream>
#include <errno.h>
#include <cppcutter.h>
#include <string>
#include <qpid/messaging/Message.h>
#include <Mutex.h>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginGate.h"
#include "HatoholArmPluginBase.h"
#include "HatoholArmPluginTestPair.h"
#include "DBTablesTest.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "hapi-test-plugin.h"
#include "HatoholArmPluginGateTest.h"
#include "ThreadLocalDBCache.h"
#include "HatoholDBUtils.h"

using namespace std;
using namespace hfl;
using namespace qpid::messaging;

static const size_t TIMEOUT = 5000;

namespace testHatoholArmPluginGate {

static void _assertStartAndExit(HapgTestCtx &ctx)
{
	if (ctx.checkMessage)
		ctx.expectRcvMessage = testMessage;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = ctx.monitoringSystemType;
	serverInfo.id = getTestArmPluginInfo(ctx.monitoringSystemType).serverId;
	HatoholArmPluginGateTest *hapg =
	  new HatoholArmPluginGateTest(serverInfo, ctx);
	HatoholArmPluginGatePtr pluginGate(hapg, false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	pluginGate->start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, ctx.launchedSem.timedWait(TIMEOUT));
	cppcut_assert_equal(ctx.expectedResultOfStart, ctx.launchSucceeded);

	SimpleSemaphore::Status status = SimpleSemaphore::STAT_OK;
	if (ctx.waitMainSem)
		status = ctx.mainSem.timedWait(TIMEOUT);
	ctx.onReleasedMainSem();

	pluginGate->exitSync();
	// These assertions must be after pluginGate->exitSync()
	// to be sure to exit the thread.
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, status);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	if (!ctx.expectAbnormalChildTerm)
		cppcut_assert_equal(false, ctx.abnormalChildTerm);
	if (ctx.checkMessage)
		cppcut_assert_equal(string(testMessage), ctx.rcvMessage);
	cppcut_assert_equal(false, ctx.gotUnexceptedException);
	if (ctx.checkNumRetry)
		cppcut_assert_equal(ctx.numRetry, ctx.retryCount-1);
}
#define assertStartAndExit(A) cut_trace(_assertStartAndExit(A))

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBServer();
	loadTestDBArmPlugin();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	HatoholArmPluginGatePtr pluginGate(
	  new HatoholArmPluginGate(serverInfo), false);
}

void test_startAndWaitExit(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
}

void test_startWithInvalidPath(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
	ctx.expectedResultOfStart = false;
	assertStartAndExit(ctx);
}

void test_startWithPassivePlugin(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	ctx.expectedResultOfStart = true;
	assertStartAndExit(ctx);
}

void test_getPid(void)
{
	// TOOD: Share the code with test_startAndWaitExit().
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
	cppcut_assert_not_equal(0, ctx.pluginPid);
}

void test_launchPluginProcessWithHFLLoggerLevel(void)
{
	// TOOD: Share the code with test_startAndWaitExit().
	struct Ctx : public HapgTestCtx {
		bool   hasError;
		string targetEnvLine;
		string paramFilePath;
		TentativeEnvVariable envVar;

		Ctx(void)
		: hasError(false),
		  envVar(Logger::LEVEL_ENV_VAR_NAME)
		{
			const char *TEST_ENV_WORD = "TEST_LEVEL";
			envVar.set(TEST_ENV_WORD);
			targetEnvLine = StringUtils::sprintf("%s=%s",
			  Logger::LEVEL_ENV_VAR_NAME, TEST_ENV_WORD);
			createParamFile();
		}

		virtual ~Ctx()
		{
			errno = 0;
			remove(paramFilePath.c_str());
			cut_assert_errno();
		}

		void createParamFile(void)
		{
			paramFilePath = StringUtils::sprintf(
			                  HAPI_TEST_PLUGIN_PARAM_FILE_FMT,
			                  getpid());
			ofstream ofs(paramFilePath.c_str(), ios::trunc);
			if (!ofs) {
				cut_notify("Failed to open: %s\n",
				           paramFilePath.c_str());
				hasError = true;
				return;
			}
			ofs << "dontExit\t1" << endl;
		}

		void setEnv(const string &name, const string &value)
		{
			if (setenv(name.c_str(), value.c_str(), 1) != 0) {
				cut_notify("Failed to call setenv(%s): %d\n",
				           name.c_str(), errno);
				hasError = true;
			}
		}

		virtual void onReleasedMainSem(void) override
		{
			readoutTargetEnvVar();
			if (pluginPid)
				kill(pluginPid, SIGKILL);
		}

		void readoutTargetEnvVar(void)
		{
			string path = StringUtils::sprintf("/proc/%d/environ",
			                                   pluginPid);
			ifstream ifs(path.c_str());
			if (!ifs) {
				cut_notify("Failed to open: %s\n",
				           path.c_str());
				hasError = true;
				return;
			}

			string line;
			while (getline(ifs, line, '\0')) {
				if (line == targetEnvLine)
					return;
			}
			cut_notify("Not found the target env line: %s in %s\n",
			           targetEnvLine.c_str(), path.c_str());
			hasError = true;
		}
	} ctx;

	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.expectAbnormalChildTerm = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
	cppcut_assert_equal(false, ctx.hasError);
}

void test_generateBrokerAddress(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.id = 530;
	cppcut_assert_equal(
	  string("hatohol-arm-plugin.530"),
	  HatoholArmPluginGateTest::callGenerateBrokerAddress(serverInfo));
}

void test_retryToConnect(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	ctx.numRetry = 3;
	ctx.checkNumRetry = true;
	assertStartAndExit(ctx);
}

void test_abortRetryWait(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.checkMessage = false;
	ctx.numRetry = 1;
	ctx.retrySleepTime = 3600 * 1000; // any value OK if it's long enough
	ctx.cancelRetrySleep = true;
	assertStartAndExit(ctx);
}

} // namespace testHatoholArmPluginGate

// ---------------------------------------------------------------------------
// new namespace
// ---------------------------------------------------------------------------
namespace testHatoholArmPluginGatePair {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

struct HatoholArmPluginBaseTest :
  public HatoholArmPluginBase, public HapiTestHelper
{
	HatoholArmPluginBaseTest(void *params)
	: serverIdOfHapGate(INVALID_SERVER_ID),
	  terminateSem(0)
	{
	}

	virtual void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}

	virtual void onInitiated(void) override
	{
		HatoholArmPluginBase::onInitiated();
		HapiTestHelper::onInitiated();
	}

	virtual void onReceivedTerminate(void) override
	{
		terminateSem.post();
	}

	virtual void onReceivedReqFetchItem(void) override
	{
		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_ITEMS);

		// Fill test items
		VariableItemTablePtr itemTablePtr;
		ItemCategoryNameMap itemCategoryNameMap;
		for (size_t i = 0; i < NumTestItemInfo; i++) {
			const ItemInfo &itemInfo = testItemInfo[i];
			if (itemInfo.serverId != serverIdOfHapGate)
				continue;
			const ItemCategoryIdType itemCategoryId = i + 100;
			itemTablePtr->add(convert(itemInfo, itemCategoryId));
			itemCategoryNameMap[itemCategoryId] =
			  itemInfo.itemGroupName;
		}
		HATOHOL_ASSERT(itemTablePtr->getNumberOfRows() >= 1,
		               "There's no test items. Inappropriate server "
		               "ID for HatoholArmPluginGate may be used.");
		appendItemTable(resBuf,
		                static_cast<ItemTablePtr>(itemTablePtr));

		// Add item category
		ItemTablePtr itemCategoryTablePtr =
		   convert(itemCategoryNameMap);
		appendItemTable(resBuf, itemCategoryTablePtr);

		reply(resBuf);
	}

	ServerIdType serverIdOfHapGate;
	SimpleSemaphore terminateSem;
};

typedef HatoholArmPluginTestPair<HatoholArmPluginBaseTest> TestPair;

struct TestReceiver {
	SimpleSemaphore sem;

	TestReceiver(void)
	: sem(0)
	{
	}

	void callback(ClosureBase *closure)
	{
		sem.post();
	}
};

void test_sendTerminateCommand(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);

	pair.gate->callSendTerminateCommand();
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    pair.plugin->terminateSem.timedWait(TIMEOUT));
}

void test_fetchItem(void)
{
	loadTestDBServer(); // for getItemInfoList().

	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	pair.plugin->serverIdOfHapGate = arg.serverId;

	TestReceiver receiver;
	pair.gate->startOnDemandFetchItem(
	  new Closure<TestReceiver>(&receiver, &TestReceiver::callback));
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, receiver.sem.timedWait(TIMEOUT));

	// Check the obtained items
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	ItemInfoList itemList;
	dbMonitoring.getItemInfoList(itemList,
	                             ItemsQueryOption(USER_ID_SYSTEM));
	vector<int> expectItemIdxVec;
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		const ItemInfo &itemInfo = testItemInfo[i];
		if (itemInfo.serverId != arg.serverId)
			continue;
		expectItemIdxVec.push_back(i);
	}
	cppcut_assert_equal(expectItemIdxVec.size(), itemList.size());
	ItemInfoListConstIterator actual = itemList.begin();
	for (int idx = 0; actual != itemList.end(); ++actual, idx++) {
		const ItemInfo &expect = testItemInfo[expectItemIdxVec[idx]];
		assertEqual(expect, *actual);
	}
}

} // namespace testHatoholArmPluginGatePair
