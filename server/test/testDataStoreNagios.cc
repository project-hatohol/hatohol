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
#include "DataStoreNagios.h"
#include "Hatohol.h"
#include "ArmNagiosNDOUtils.h"

using namespace mlpl;

namespace testDataStoreNagios {

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getArmBase(void)
{
	MonitoringServerInfo serverInfo;
	serverInfo.id = 5;
	UsedCountablePtr<DataStoreNagios>
	  dataStoreNagiosPtr(new DataStoreNagios(serverInfo, false), false);
	ArmBase *armBase = dataStoreNagiosPtr->getArmBase();
	cppcut_assert_not_null(armBase);
	cppcut_assert_equal(typeid(ArmNagiosNDOUtils), typeid(*armBase));
}

} // namespace testDataStoreManager


