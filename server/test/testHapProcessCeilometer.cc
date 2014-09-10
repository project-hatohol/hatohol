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
#include <gcutter.h>
#include "HapProcessCeilometer.h"

using namespace std;
using namespace mlpl;

namespace testHapProcessCeilometer {

class TestHapProcessCeilometer : public HapProcessCeilometer {
public:
	TestHapProcessCeilometer(void)
	: HapProcessCeilometer(0, NULL)
	{
	}

	TriggerStatusType callParseAlarmState(const std::string &state)
	{
		return parseAlarmState(state);
	}

	SmartTime callParseStateTimestamp(const std::string &stateTimestamp)
	{
		return parseStateTimestamp(stateTimestamp);
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void data_parseAlarmState(void)
{
	gcut_add_datum("ok",
	               "msg", G_TYPE_STRING, "ok",
	               "expect", G_TYPE_INT, TRIGGER_STATUS_OK, NULL);
	gcut_add_datum("alarm",
	               "msg", G_TYPE_STRING, "alarm",
	               "expect", G_TYPE_INT, TRIGGER_STATUS_PROBLEM, NULL);
	gcut_add_datum("insufficient data",
	               "msg", G_TYPE_STRING, "insufficient data",
	               "expect", G_TYPE_INT, TRIGGER_STATUS_UNKNOWN, NULL);
	gcut_add_datum("unexpected string",
	               "msg", G_TYPE_STRING, "HOGE TA HOGE ZOOO",
	               "expect", G_TYPE_INT, TRIGGER_STATUS_UNKNOWN, NULL);
}

void test_parseAlarmState(gconstpointer data)
{
	TestHapProcessCeilometer hap;
	cppcut_assert_equal(
	  hap.callParseAlarmState(gcut_data_get_string(data, "msg")),
	  static_cast<TriggerStatusType>(gcut_data_get_int(data, "expect")));
}

void test_parseStateTimestamp()
{
	TestHapProcessCeilometer hap;
	const string ts = "2014-09-10T09:54:03.012345";
	SmartTime actual = hap.callParseStateTimestamp(ts);
	timespec _expect;
	_expect.tv_sec = 1410342843;
	_expect.tv_nsec = 12345 * 1000;
	SmartTime expect(_expect);
	cppcut_assert_equal(expect, actual);
}

void test_parseStateTimestampWithoutUSec()
{
	TestHapProcessCeilometer hap;
	const string ts = "2014-09-10T09:54:03";
	SmartTime actual = hap.callParseStateTimestamp(ts);
	timespec _expect;
	_expect.tv_sec = 1410342843;
	_expect.tv_nsec = 0;
	SmartTime expect(_expect);
	cppcut_assert_equal(expect, actual);
}

} // namespace testHapProcessCeilometer
