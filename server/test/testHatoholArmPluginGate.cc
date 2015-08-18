/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
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
using namespace mlpl;
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

void test_launchPluginProcessWithMLPLLoggerLevel(void)
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
			const ItemCategoryIdType itemCategoryId =
			  StringUtils::sprintf("%zd", i + 100);
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

	virtual void onReceivedReqFetchHistory(void) override
	{
		SmartBuffer *cmdBuf = getCurrBuffer();
		HapiParamReqFetchHistory *params =
		  getCommandBody<HapiParamReqFetchHistory>(*cmdBuf);
		ItemIdType itemId = getString(*cmdBuf, params,
		                      params->itemIdOffset,
		                      params->itemIdLength);
		time_t beginTime = static_cast<time_t>(LtoN(params->beginTime));
		time_t endTime = static_cast<time_t>(LtoN(params->endTime));

		VariableItemTablePtr itemTablePtr;
		HistoryInfoVect historyInfoVect;
		getTestHistory(historyInfoVect,
			       // Ignore serverId for this test.
			       // Use all history as this server's one.
			       ALL_SERVERS,
			       itemId, beginTime, endTime);
		HistoryInfoVectIterator it = historyInfoVect.begin();
		for (; it != historyInfoVect.end(); it++) {
			const HistoryInfo &historyInfo = *it;
			itemTablePtr->add(convert(historyInfo));
		}

		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_HISTORY);
		appendItemTable(resBuf,
		                static_cast<ItemTablePtr>(itemTablePtr));

		reply(resBuf);
	}

	virtual void onReceivedReqFetchTrigger(void) override
	{
		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_TRIGGERS);

		VariableItemTablePtr itemTablePtr;
		for (size_t i = 0; i < NumTestTriggerInfo; i++) {
			const TriggerInfo &triggerInfo = testTriggerInfo[i];
			if (triggerInfo.serverId != serverIdOfHapGate)
				continue;
			itemTablePtr->add(convert(triggerInfo));
		}
		HATOHOL_ASSERT(itemTablePtr->getNumberOfRows() >= 1,
		               "There's no test items. Inappropriate server "
		               "ID for HatoholArmPluginGate may be used.");
		appendItemTable(resBuf,
		                static_cast<ItemTablePtr>(itemTablePtr));

		reply(resBuf);
	}

	ServerIdType serverIdOfHapGate;
	SimpleSemaphore terminateSem;
};

typedef HatoholArmPluginTestPair<HatoholArmPluginBaseTest> TestPair;

struct TestReceiver {
	SimpleSemaphore sem;
	HistoryInfoVect historyInfoVect;

	TestReceiver(void)
	: sem(0)
	{
	}

	void callback(Closure0 *closure)
	{
		sem.post();
	}

	void callbackHistory(Closure1<HistoryInfoVect> *closure,
			     const HistoryInfoVect &_historyInfoVect)
	{
		historyInfoVect = _historyInfoVect;
		sem.post();
	}

	void callbackTrigger(Closure0 *closure)
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
	pair.gate->loadTestHostInfoCache();

	TestReceiver receiver;
	pair.gate->startOnDemandFetchItems(
	  {},
	  new ClosureTemplate0<TestReceiver>(
	    &receiver, &TestReceiver::callback));
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

void test_fetchEmptyHistory(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	pair.plugin->serverIdOfHapGate = arg.serverId;

	TestReceiver receiver;
	ItemInfo itemInfo;
	itemInfo.serverId = arg.serverId;
	itemInfo.globalHostId = ALL_HOSTS; // dummy
	itemInfo.hostIdInServer = ALL_LOCAL_HOSTS; // dummy
	itemInfo.id = 1;
	itemInfo.valueType = ITEM_INFO_VALUE_TYPE_FLOAT;
	pair.gate->startOnDemandFetchHistory(
	  itemInfo, 0, 0,
	  new ClosureTemplate1<TestReceiver, HistoryInfoVect>(
	    &receiver, &TestReceiver::callbackHistory));
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, receiver.sem.timedWait(TIMEOUT));
	cppcut_assert_equal(0, (int)receiver.historyInfoVect.size());
}

void test_fetchHistory(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	pair.plugin->serverIdOfHapGate = arg.serverId;

	ItemInfo itemInfo;
	itemInfo.serverId = testHistoryInfo[0].serverId;
	itemInfo.globalHostId = ALL_HOSTS;
	itemInfo.hostIdInServer = ALL_LOCAL_HOSTS;
	itemInfo.id = testHistoryInfo[0].itemId;
	itemInfo.valueType = ITEM_INFO_VALUE_TYPE_FLOAT;
	time_t beginTime = testHistoryInfo[0].clock.tv_sec + 1;
	time_t endTime = testHistoryInfo[NumTestHistoryInfo - 1].clock.tv_sec - 1;

	TestReceiver receiver;
	pair.gate->startOnDemandFetchHistory(
	  itemInfo, beginTime, endTime,
	  new ClosureTemplate1<TestReceiver, HistoryInfoVect>(
	    &receiver, &TestReceiver::callbackHistory));
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, receiver.sem.timedWait(TIMEOUT));

	HistoryInfoVect expectedHistoryVect;
	getTestHistory(expectedHistoryVect,
		       ALL_SERVERS, // Ignore serverId for this test
		       itemInfo.id, beginTime, endTime);
	string expected, actual;
	for (size_t i = 0; i < expectedHistoryVect.size(); i++) {
		expectedHistoryVect[i].serverId = itemInfo.serverId;
		expected += makeHistoryOutput(expectedHistoryVect[i]);
	}
	for (size_t i = 0; i < receiver.historyInfoVect.size(); i++)
		actual += makeHistoryOutput(receiver.historyInfoVect[i]);
	cppcut_assert_equal(expected, actual);
	cppcut_assert_not_equal((size_t)0, receiver.historyInfoVect.size());
}

void test_fetchTrigger(void)
{
	loadTestDBServer();
	loadTestDBServerHostDef();
	loadTestDBVMInfo();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();

	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	pair.plugin->serverIdOfHapGate = arg.serverId;

	TestReceiver receiver;
	pair.gate->startOnDemandFetchTriggers(
	  {},
	  new ClosureTemplate0<TestReceiver>(
	    &receiver, &TestReceiver::callbackTrigger));
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, receiver.sem.timedWait(TIMEOUT));

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();

	TriggerInfoList triggerInfoList;
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	triggersQueryOption.setTargetServerId(arg.serverId);
	triggersQueryOption.setExcludeFlags(EXCLUDE_INVALID_HOST|EXCLUDE_SELF_MONITORING);
	dbMonitoring.getTriggerInfoList(triggerInfoList,
					triggersQueryOption);
	vector<int> expectTriggerIdxVec;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &triggerInfo = testTriggerInfo[i];
		if (triggerInfo.serverId != arg.serverId)
			continue;
		expectTriggerIdxVec.push_back(i);
	}
	cppcut_assert_equal(expectTriggerIdxVec.size(), triggerInfoList.size());
	TriggerInfoListConstIterator actual = triggerInfoList.begin();
	for (int idx = 0; actual != triggerInfoList.end(); ++actual, idx++) {
		const TriggerInfo &expect = testTriggerInfo[expectTriggerIdxVec[idx]];
		assertEqual(expect, *actual);
	}
}

} // namespace testHatoholArmPluginGatePair
