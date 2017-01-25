/*
 * Copyright (C) 2013 Project Hatohol
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

#include <memory>
#include <cppcutter.h>
#include "ArmBase.h"
#include "DataStoreManager.h"

using namespace std;

namespace testDataStoreManager {

shared_ptr<DataStore> g_dataStore;

static MonitoringServerInfo testMonitoringServerInfo;

class TestArmBase : public ArmBase {
public:
	TestArmBase(void)
	: ArmBase("TestArmBase", testMonitoringServerInfo)
	{
	}

	virtual ArmPollingResult mainThreadOneProc(void) override
	{
		return COLLECT_OK;
	}
};

class TestDataStore : public DataStore {
	TestArmBase armBase;

	virtual const MonitoringServerInfo
	  &getMonitoringServerInfo(void) const override
	{
		return armBase.getServerInfo();
	}

	virtual const ArmStatus &getArmStatus(void) const override
	{
		return armBase.getArmStatus();
	}
};

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
	g_dataStore = nullptr;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_addAndRemove(void)
{
	const uint32_t storeId = 50;

	g_dataStore = make_shared<TestDataStore>();
	cppcut_assert_equal(1L, g_dataStore.use_count());
	DataStoreManager mgr;
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(2L, g_dataStore.use_count());
	mgr.remove(storeId);
	cppcut_assert_equal(1L, g_dataStore.use_count());
}

void test_hasDataStore(void)
{
	const uint32_t storeId = 50;

	g_dataStore = make_shared<TestDataStore>();
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
		shared_ptr<DataStore> added;
		shared_ptr<DataStore> removed;

		TestDataEventProc(void)
		: added(NULL),
		  removed(NULL)
		{
		}

		virtual void onAdded(shared_ptr<DataStore> dataStore) override
		{
			added = dataStore;
		}

		virtual void onRemoved(shared_ptr<DataStore> dataStore) override
		{
			removed = dataStore;
		}
	} testProc;

	const uint32_t storeId = 100;
	g_dataStore = make_shared<TestDataStore>();

	DataStoreManager mgr;
	mgr.registEventProc(&testProc);

	cppcut_assert_null(testProc.added.get());
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(g_dataStore, testProc.added);

	cppcut_assert_null(testProc.removed.get());
	mgr.remove(storeId);
	cppcut_assert_equal(g_dataStore, testProc.removed);
}

} // namespace testDataStoreManager

