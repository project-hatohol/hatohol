/*
 * Copyright (C) 2013 Project Hatohol
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
#include "ArmBase.h"
#include "DataStoreManager.h"

namespace testDataStoreManager {

DataStore *g_dataStore = NULL;

static MonitoringServerInfo testMonitoringServerInfo;

class TestArmBase : public ArmBase {
public:
	TestArmBase(void)
	: ArmBase("TestArmBase", testMonitoringServerInfo)
	{
	}

	virtual OneProcEndType mainThreadOneProc(void) override
	{
		return COLLECT_OK;
	}
};

class TestDataStore : public DataStore {
	TestArmBase armBase;
	ArmBase &getArmBase(void)
	{
		return armBase;
	}
};

static void deleteDataStore(DataStore *dataStore)
{
	while (true) {
		int usedCnt = g_dataStore->getUsedCount();
		g_dataStore->unref();
		if (usedCnt == 1)
			break;
	}
}

void cut_startup(void)
{
	testMonitoringServerInfo.id = 100;
	testMonitoringServerInfo.type = NUM_MONITORING_SYSTEMS;
	testMonitoringServerInfo.port = 0;
	testMonitoringServerInfo.pollingIntervalSec = 30;
	testMonitoringServerInfo.retryIntervalSec = 30;
}

void cut_teardown(void)
{
	deleteDataStore(g_dataStore);
	g_dataStore = NULL;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_addAndRemove(void)
{
	const uint32_t storeId = 50;

	g_dataStore = new TestDataStore();
	cppcut_assert_equal(1, g_dataStore->getUsedCount());
	DataStoreManager mgr;
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(2, g_dataStore->getUsedCount());
	mgr.remove(storeId);
	cppcut_assert_equal(1, g_dataStore->getUsedCount());
}

void test_hasDataStore(void)
{
	const uint32_t storeId = 50;

	g_dataStore = new TestDataStore();
	DataStoreManager mgr;
	cppcut_assert_equal(false, mgr.hasDataStore(storeId));
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(true, mgr.hasDataStore(storeId));
	mgr.remove(storeId);
	cppcut_assert_equal(false, mgr.hasDataStore(storeId));
}

void test_dataStoreEventProc(void)
{
	struct TestDataEventProc : public DataStoreEventProc {
		DataStore *added;
		DataStore *removed;

		TestDataEventProc(void)
		: added(NULL),
		  removed(NULL)
		{
		}

		virtual void onAdded(DataStore *dataStore) override
		{
			added = dataStore;
		}

		virtual void onRemoved(DataStore *dataStore) override
		{
			removed = dataStore;
		}
	} testProc;

	const uint32_t storeId = 100;
	g_dataStore = new TestDataStore();

	DataStoreManager mgr;
	mgr.registEventProc(&testProc);

	cppcut_assert_null(testProc.added);
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(g_dataStore, testProc.added);

	cppcut_assert_null(testProc.removed);
	mgr.remove(storeId);
	cppcut_assert_equal(g_dataStore, testProc.removed);
}

} // namespace testDataStoreManager

