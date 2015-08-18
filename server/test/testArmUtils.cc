/*
 * Copyright (C) 2015 Project Hatohol
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
#include "ArmUtils.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "DBTablesTest.h"

namespace testArmUtils {

static void _assertArmTriggerStatus(
  const TriggerStatusType &expectedStatus,
  const ArmUtils::ArmTrigger *armTriggers,
  const size_t &startIdx, const size_t &_endIdx = SIZE_MAX)
{
	const size_t endIdx = (_endIdx != SIZE_MAX) ? _endIdx : (startIdx + 1);
	for (size_t i = startIdx; i < endIdx; i++) {
		cppcut_assert_equal(expectedStatus, armTriggers[i].status,
		                    cut_message("i: %zd", i));
	}
}

#define assertArmTriggerStatus(E,T,IDX,...) \
cut_trace(_assertArmTriggerStatus(E,T,IDX,##__VA_ARGS__))

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_updateTriggerStatus(void)
{
	MonitoringServerInfo svInfo;
	initServerInfo(svInfo);
	const size_t numArmTriggers = 5;
	ArmUtils::ArmTrigger armTriggers[numArmTriggers];
	ArmUtils armUtils(svInfo, armTriggers, numArmTriggers);
	for (size_t i = 0; i < numArmTriggers; i++) {
		armTriggers[i].triggerId = 100 + i;
		armTriggers[i].status = TRIGGER_STATUS_UNKNOWN;
	}

	// A case an error happens in the middle of the data path.
	const size_t errTriggerIdx = 2;
	armUtils.updateTriggerStatus(errTriggerIdx, TRIGGER_STATUS_PROBLEM);
	assertArmTriggerStatus(TRIGGER_STATUS_UNKNOWN, armTriggers,
	                       0, errTriggerIdx - 1);
	assertArmTriggerStatus(TRIGGER_STATUS_PROBLEM, armTriggers,
	                       errTriggerIdx);
	assertArmTriggerStatus(TRIGGER_STATUS_OK, armTriggers,
	                       errTriggerIdx + 1, numArmTriggers - 1);

	// When an Arm trigger is OK, all the triggers should be OK too.
	const size_t okTriggerIdx = numArmTriggers + 1;
	armUtils.updateTriggerStatus(okTriggerIdx, TRIGGER_STATUS_OK);
	assertArmTriggerStatus(TRIGGER_STATUS_OK, armTriggers,
	                       0, numArmTriggers - 1);
}

}; // namespace testArmUtils
