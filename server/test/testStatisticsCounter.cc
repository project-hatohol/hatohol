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

#include "SmartTime.h"
#include "StatisticsCounter.h"
#include <cppcutter.h>
#include <unistd.h>

using namespace std;
using namespace mlpl;

namespace testStatisticsCounter {

static void _assertBetween(
  const SmartTime &min, const SmartTime &target, const SmartTime &max)
{
	cppcut_assert_equal(true, target > min,
	   cut_message("min: %s, target:%s",
	               static_cast<string>(min).c_str(),
	               static_cast<string>(target).c_str()));
	cppcut_assert_equal(true, target < max,
	   cut_message("target: %s, max:%s",
	               static_cast<string>(target).c_str(),
	               static_cast<string>(max).c_str()));
}
#define assertBetween(MIN, TARGET, MAX) \
  cut_trace(_assertBetween(MIN, TARGET, MAX))

static SmartTime add(const SmartTime &ref, const int &sec)
{
	timespec ts = ref.getAsTimespec();
	ts.tv_sec += sec;
	return ts;
}

void test_constructor(void)
{
	constexpr int intervalSec = 1;
	SmartTime t0 = SmartTime(SmartTime::INIT_CURR_TIME);
	StatisticsCounter counter(intervalSec);
	SmartTime t1 = SmartTime(SmartTime::INIT_CURR_TIME);

	StatisticsCounter::Slot prevSlot, currSlot;
	counter.get(&prevSlot, &currSlot);

	assertBetween(add(t0, -intervalSec), prevSlot.startTime, t0);
	assertBetween(t0, prevSlot.endTime, t1);

	cppcut_assert_equal(prevSlot.endTime, currSlot.startTime);
	assertBetween(currSlot.startTime, currSlot.endTime,
	              SmartTime(SmartTime::INIT_CURR_TIME));

	cppcut_assert_equal(static_cast<uint64_t>(0), prevSlot.number);
	cppcut_assert_equal(static_cast<uint64_t>(0), currSlot.number);
}

void test_add(void)
{
	constexpr int intervalSec = 1;
	StatisticsCounter counter(intervalSec);
	counter.add(5);
	counter.add(3);
	StatisticsCounter::Slot prevSlot, currSlot;
	counter.get(&prevSlot, &currSlot);
	cppcut_assert_equal(static_cast<uint64_t>(0), prevSlot.number);
	cppcut_assert_equal(static_cast<uint64_t>(8), currSlot.number);
}

void test_add_both_slot(void)
{
	constexpr int intervalUSec = 10*1000;
	StatisticsCounter counter(0, intervalUSec*1000);
	counter.add(5);
	counter.add(3);
	usleep(intervalUSec);
	StatisticsCounter::Slot prevSlot, currSlot;
	counter.get(&prevSlot, &currSlot);
	cppcut_assert_equal(static_cast<uint64_t>(8), prevSlot.number);
	cppcut_assert_equal(static_cast<uint64_t>(0), currSlot.number);
	cppcut_assert_equal(prevSlot.endTime, currSlot.startTime);
	assertBetween(currSlot.startTime, currSlot.endTime,
	              SmartTime(SmartTime::INIT_CURR_TIME));
}

void test_get_after_double_interval(void)
{
	constexpr int intervalUSec = 10*1000;
	StatisticsCounter counter(0, intervalUSec*1000);
	counter.add(5);
	usleep(2*intervalUSec);
	StatisticsCounter::Slot prevSlot, currSlot;
	counter.get(&prevSlot, &currSlot);
	cppcut_assert_equal(static_cast<uint64_t>(0), prevSlot.number);
	cppcut_assert_equal(static_cast<uint64_t>(0), currSlot.number);
	assertBetween(prevSlot.startTime, prevSlot.endTime,
	              SmartTime(SmartTime::INIT_CURR_TIME));
	cppcut_assert_equal(prevSlot.endTime, currSlot.startTime);
	cppcut_assert_equal(currSlot.endTime, currSlot.startTime);
}

void test_add_after_double_interval(void)
{
	constexpr int intervalUSec = 10*1000;
	StatisticsCounter counter(0, intervalUSec*1000);
	counter.add(5);
	usleep(2*intervalUSec);
	counter.add(4);
	counter.add(6);
	StatisticsCounter::Slot prevSlot, currSlot;
	counter.get(&prevSlot, &currSlot);
	cppcut_assert_equal(static_cast<uint64_t>(0), prevSlot.number);
	cppcut_assert_equal(static_cast<uint64_t>(10), currSlot.number);
	assertBetween(prevSlot.startTime, prevSlot.endTime, currSlot.endTime);
	cppcut_assert_equal(prevSlot.endTime, currSlot.startTime);
}

} // testStatisticsCounter
