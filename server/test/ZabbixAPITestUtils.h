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

#ifndef ZabbixAPITestUtils_h
#define ZabbixAPITestUtils_h

#include <string>
#include <cppcutter.h>
#include "ZabbixAPI.h"
#include "ZabbixAPIEmulator.h"

class ZabbixAPITestee :  public ZabbixAPI {
public:
	static const size_t NUM_TEST_READ_TIMES = 10;
	enum GetTestType {
		GET_TEST_TYPE_API_VERSION,
	};

	ZabbixAPITestee(const MonitoringServerInfo &serverInfo);
	virtual ~ZabbixAPITestee();

	const std::string &errorMessage(void) const;
	bool run(const GetTestType &type,
	         const ZabbixAPIEmulator::APIVersion &expectedVersion =
	           ZabbixAPIEmulator::API_VERSION_2_0_4);
	void testCheckAPIVersion(
	       bool expected, int major, int minor, int micro);
	void testOpenSession(void);

	static void initServerInfoWithDefaultParam(
	              MonitoringServerInfo &serverInfo);

protected:
	typedef bool (ZabbixAPITestee::*TestProc)(void);

	bool launch(TestProc testProc, size_t numRepeat = NUM_TEST_READ_TIMES);
	bool defaultTestProc(void);
	bool testProcVersion(void);

private:
	std::string m_errorMessage;
};

void _assertTestGet(
  const ZabbixAPITestee::GetTestType &testType,
  const ZabbixAPIEmulator::APIVersion &expectedVersion =
    ZabbixAPIEmulator::API_VERSION_2_0_4);
#define assertTestGet(TYPE, ...) cut_trace(_assertTestGet(TYPE, ##__VA_ARGS__))

void _assertReceiveData(
  const ZabbixAPITestee::GetTestType &testType, const ServerIdType &svId,
  const ZabbixAPIEmulator::APIVersion &expectedVersion =
    ZabbixAPIEmulator::API_VERSION_2_0_4);
#define assertReceiveData(TYPE, SERVER_ID, ...)	\
  cut_trace(_assertReceiveData(TYPE, SERVER_ID, ##__VA_ARGS__))

void _assertCheckAPIVersion(bool expected, int major, int minor , int micro);
#define assertCheckAPIVersion(EXPECTED,MAJOR,MINOR,MICRO) \
  cut_trace(_assertCheckAPIVersion(EXPECTED,MAJOR,MINOR,MICRO))

#endif // ZabbixAPITestUtils_h
