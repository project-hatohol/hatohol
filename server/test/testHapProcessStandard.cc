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

struct StartAcquisitionTester {
	TestHapProcessStandard m_hapProc;
	MonitoringServerInfo   m_serverInfo;

	StartAcquisitionTester(void)
	{
		initServerInfo(m_serverInfo);

		const ArmStatus &armStatus =  m_hapProc.callGetArmStatus();
		ArmInfo armInfo = armStatus.getArmInfo();
		cppcut_assert_equal(ARM_WORK_STAT_INIT, armInfo.stat);
	}

	void run(void)
	{
		m_hapProc.callOnReady(m_serverInfo);
	}

	void assert(const HatoholErrorCode &expectHatoholErrorCode,
	            const HatoholArmPluginWatchType &expectWatchType,
	            const ArmWorkingStatus &expectArmWorkingStat)
	{
		cppcut_assert_equal(true, m_hapProc.m_calledAquireData);
		assertHatoholError(expectHatoholErrorCode,
		                    m_hapProc.m_errorOnComplated);
		cppcut_assert_equal(expectWatchType,
		                     m_hapProc.m_watchTypeOnComplated);

		const ArmStatus &armStatus =  m_hapProc.callGetArmStatus();
		ArmInfo armInfo = armStatus.getArmInfo();
		cppcut_assert_equal(expectArmWorkingStat, armInfo.stat);
		if (expectArmWorkingStat == ARM_WORK_STAT_FAILURE) {
			cppcut_assert_equal(false,
			                    armInfo.failureComment.empty());
		}
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_startAcquisition(void)
{
	StartAcquisitionTester tester;
	tester.run();
	tester.assert(HTERR_OK, COLLECT_OK, ARM_WORK_STAT_OK);
}

void test_startAcquisitionWithError(void)
{
	StartAcquisitionTester tester;
	tester.m_hapProc.m_returnValueOfAcquireData = HTERR_UNKNOWN_REASON;
	tester.run();
	tester.assert(HTERR_UNKNOWN_REASON, COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	              ARM_WORK_STAT_FAILURE);
}

void test_startAcquisitionWithError(void)
{
	StartAcquisitionTester tester;
	tester.m_hapProc.m_returnValueOfAcquireData = HTERR_UNKNOWN_REASON;
	tester.run();
	tester.assert(HTERR_UNKNOWN_REASON, COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	              ARM_WORK_STAT_FAILURE);
}

} // namespace testHapProcessStandard
