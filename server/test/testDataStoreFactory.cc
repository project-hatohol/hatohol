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

#include <cppcutter.h>
#include <gcutter.h>
#include <memory>
#include "Hatohol.h"
#include "DBTablesConfig.h"
#include "DataStoreFactory.h"
#include "DataStoreFake.h"
#include "HatoholArmPluginGateHAPI2.h"
#include "Helpers.h"
#include "Reaper.h"
#include "DBTablesTest.h"

using namespace std;
using namespace mlpl;

namespace testDataStoreFactory {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void data_createWithInvalidTypes(void)
{
	gcut_add_datum("MONITORING_SYSTEM_UNKNOWN",
	               "type", G_TYPE_INT, MONITORING_SYSTEM_UNKNOWN, NULL);
	gcut_add_datum("MONITORING_SYSTEM_UNKNOWN-1",
	               "type", G_TYPE_INT, MONITORING_SYSTEM_UNKNOWN-1, NULL);
	gcut_add_datum("NUM_MONITORING_SYSTEMS",
	               "type", G_TYPE_INT, NUM_MONITORING_SYSTEMS, NULL);
	gcut_add_datum("NUM_MONITORING_SYSTEMS+1",
	               "type", G_TYPE_INT, NUM_MONITORING_SYSTEMS+1, NULL);
}

void test_createWithInvalidTypes(gconstpointer data)
{
	MonitoringServerInfo svInfo;
	initServerInfo(svInfo);
	svInfo.type =
	  static_cast<MonitoringSystemType>(gcut_data_get_int(data, "type"));
	shared_ptr<DataStore> dataStore = DataStoreFactory::create(svInfo);
	// to free the instance automatically
	cppcut_assert_null(dataStore.get());
}

void data_create(void)
{
	gcut_add_datum("MONITORING_SYSTEM_FAKE",
	               "type", G_TYPE_INT, MONITORING_SYSTEM_FAKE,
	               "type-name", G_TYPE_STRING, typeid(DataStoreFake).name(),
	               NULL);
	gcut_add_datum("MONITORING_SYSTEM_HAPI2",
	               "type", G_TYPE_INT, MONITORING_SYSTEM_HAPI2,
	               "type-name", G_TYPE_STRING,
	                 typeid(HatoholArmPluginGateHAPI2).name(), NULL);
}

void test_create(gconstpointer data)
{
	MonitoringServerInfo svInfo;
	initServerInfo(svInfo);
	svInfo.type =
	  static_cast<MonitoringSystemType>(gcut_data_get_int(data, "type"));
	string typeName = gcut_data_get_string(data, "type-name");
	shared_ptr<DataStore> dataStore = DataStoreFactory::create(svInfo);
	// to free the instance automatically
	cppcut_assert_not_null(dataStore.get());
	cppcut_assert_equal(typeName, string(typeid(*dataStore).name()));
}

} // namespace testDataStoreFactory

