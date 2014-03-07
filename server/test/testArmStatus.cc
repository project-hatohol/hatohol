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
#include <SmartTime.h>
#include "ArmStatus.h"

using namespace std;
using namespace mlpl;

namespace testArmStatus {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getArmInfoInitial(void)
{
	ArmStatus armStatus;
	const ArmInfo armInfo = armStatus.getArmInfo();
	const SmartTime initTime;
	cppcut_assert_equal(false, armInfo.running);
	cppcut_assert_equal(ARM_WORK_STAT_INIT, armInfo.stat);
	cppcut_assert_equal(true, armInfo.failureComment.empty());
	cppcut_assert_equal(initTime, armInfo.statUpdateTime);
	cppcut_assert_equal(initTime, armInfo.lastSuccessTime);
	cppcut_assert_equal(initTime, armInfo.lastFailureTime);
	cppcut_assert_equal((size_t)0, armInfo.numUpdated);
	cppcut_assert_equal((size_t)0, armInfo.numFailure);
}

void test_logSuccess(void)
{
	ArmStatus armStatus;
	const SmartTime time0(SmartTime::INIT_CURR_TIME);
	armStatus.logSuccess();
	ArmInfo armInfo = armStatus.getArmInfo();
	const SmartTime time1(SmartTime::INIT_CURR_TIME);

	cppcut_assert_equal(ARM_WORK_STAT_OK, armInfo.stat);
	cppcut_assert_equal(true, armInfo.statUpdateTime >= time0);
	cppcut_assert_equal(true, time1 >= armInfo.statUpdateTime);
	cppcut_assert_equal(armInfo.statUpdateTime, armInfo.lastSuccessTime);
	cppcut_assert_equal((size_t)1, armInfo.numUpdated);

	// The remaining members should be unchanged.
	const SmartTime initTime;
	cppcut_assert_equal(false, armInfo.running);
	cppcut_assert_equal(true, armInfo.failureComment.empty());
	cppcut_assert_equal(initTime, armInfo.lastFailureTime);
	cppcut_assert_equal((size_t)0, armInfo.numFailure);
}

void test_logFailure(void)
{
	ArmStatus armStatus;
	const SmartTime time0(SmartTime::INIT_CURR_TIME);
	const string comment = "Test failure comment (;_;)";
	armStatus.logFailure(comment);
	ArmInfo armInfo = armStatus.getArmInfo();
	const SmartTime time1(SmartTime::INIT_CURR_TIME);

	cppcut_assert_equal(ARM_WORK_STAT_FAILURE, armInfo.stat);
	cppcut_assert_equal(true, armInfo.statUpdateTime >= time0);
	cppcut_assert_equal(true, time1 >= armInfo.statUpdateTime);
	cppcut_assert_equal(armInfo.statUpdateTime, armInfo.lastFailureTime);
	cppcut_assert_equal(comment, armInfo.failureComment);
	cppcut_assert_equal((size_t)1, armInfo.numUpdated);
	cppcut_assert_equal((size_t)1, armInfo.numFailure);

	// The remaining members should be unchanged.
	const SmartTime initTime;
	cppcut_assert_equal(false, armInfo.running);
	cppcut_assert_equal(initTime, armInfo.lastSuccessTime);
}

} // namespace testArmStatus
