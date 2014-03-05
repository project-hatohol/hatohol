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
	cppcut_assert_equal(ARM_WORK_STAT_INIT, armInfo.stat);
	cppcut_assert_equal(true, armInfo.failureComment.empty());
	cppcut_assert_equal(initTime, armInfo.statUpdateTime);
	cppcut_assert_equal(initTime, armInfo.lastSuccessTime);
	cppcut_assert_equal(initTime, armInfo.lastFailureTime);
	cppcut_assert_equal((size_t)0, armInfo.numTryToGet);
	cppcut_assert_equal((size_t)0, armInfo.numFailure);
}

} // namespace testArmStatus
