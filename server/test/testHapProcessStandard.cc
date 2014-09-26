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
#include "HapProcessStandard.h"
#include "Helpers.h"

namespace testHapProcessStandard {

class TestHapProcessStandard : public HapProcessStandard {
public:
	bool                      m_calledAquireData;
	HatoholError              m_errorOnComplated;
	HatoholArmPluginWatchType m_watchTypeOnComplated;

	// These are input paramters from the test cases
	HatoholError              m_returnValueOfAcquireData;

	TestHapProcessStandard(void)
	: HapProcessStandard(0, NULL),
	  m_calledAquireData(false),
	  m_errorOnComplated(NUM_HATOHOL_ERROR_CODE),
	  m_watchTypeOnComplated(NUM_COLLECT_NG_KIND),
	  m_returnValueOfAcquireData(HTERR_OK)
	{
	}

	virtual HatoholError acquireData(void) override
	{
		m_calledAquireData = true;
		return m_returnValueOfAcquireData;
	}

	virtual void onCompletedAcquistion(
	  const HatoholError &err,
	  const HatoholArmPluginWatchType &watchType) override
	{
		m_errorOnComplated = err;
		m_watchTypeOnComplated  = watchType;
	}

	void callOnReady(const MonitoringServerInfo &serverInfo)
	{
		onReady(serverInfo);
	}

	const ArmStatus &callGetArmStatus(void)
	{
		return getArmStatus();
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_startAcquisition(void)
{
	TestHapProcessStandard hapProc;
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	hapProc.callOnReady(serverInfo);
	cppcut_assert_equal(true,       hapProc.m_calledAquireData);
	assertHatoholError(HTERR_OK,    hapProc.m_errorOnComplated);
	cppcut_assert_equal(COLLECT_OK, hapProc.m_watchTypeOnComplated);

	const ArmStatus &armStatus = hapProc.callGetArmStatus();
	ArmInfo armInfo = armStatus.getArmInfo();
	cppcut_assert_equal(ARM_WORK_STAT_OK, armInfo.stat);
}

void test_startAcquisitionWithError(void)
{
	TestHapProcessStandard hapProc;
	hapProc.m_returnValueOfAcquireData = HTERR_UNKNOWN_REASON;
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	hapProc.callOnReady(serverInfo);
	cppcut_assert_equal(true,       hapProc.m_calledAquireData);
	assertHatoholError(HTERR_UNKNOWN_REASON, hapProc.m_errorOnComplated);
	cppcut_assert_equal(COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	                    hapProc.m_watchTypeOnComplated);

	const ArmStatus &armStatus = hapProc.callGetArmStatus();
	ArmInfo armInfo = armStatus.getArmInfo();
	cppcut_assert_equal(ARM_WORK_STAT_FAILURE, armInfo.stat);
}

} // namespace testHapProcessStandard
