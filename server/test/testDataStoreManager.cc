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
#include "DataStoreManager.h"

namespace testDataStoreManager {

DataStore *g_dataStore = NULL;

static void deleteDataStore(DataStore *dataStore)
{
	while (true) {
		int usedCnt = g_dataStore->getUsedCount();
		g_dataStore->unref();
		if (usedCnt == 1)
			break;
	}
}

void cut_teardown(void)
{
	if (g_dataStore) {
		deleteDataStore(g_dataStore);
		g_dataStore = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_addAndRemove(void)
{
	const uint32_t storeId = 50;

	g_dataStore = new DataStore();
	cppcut_assert_equal(1, g_dataStore->getUsedCount());
	DataStoreManager mgr;
	mgr.add(storeId, g_dataStore);
	cppcut_assert_equal(2, g_dataStore->getUsedCount());
	mgr.remove(storeId);
	cppcut_assert_equal(1, g_dataStore->getUsedCount());
}

} // namespace testDataStoreManager

