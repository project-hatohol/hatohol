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
#include <sys/time.h>
#include <errno.h>
#include <cmath>
#include "SmartTime.h"

using namespace mlpl;

namespace testSmartTime {

static timespec getSampleTimespec(void)
{
	timespec ts;
	ts.tv_sec = 1379641056;
	ts.tv_nsec = 987654321;
	return ts;
}

static double getCurrTimeAsSec(void)
{
	timeval tv;
	cppcut_assert_equal(0, gettimeofday(&tv, NULL),
	                    cut_message("errno: %d", errno)); 
	return tv.tv_sec + tv.tv_usec/1e6;
}

static void _assertCurrentTime(const SmartTime &smtime)
{
	static const double ALLOWED_ERROR = 1e-3;
	double diff = getCurrTimeAsSec();
	diff -= smtime.getAsSec();
	cppcut_assert_equal(true, fabs(diff) < ALLOWED_ERROR,
	                    cut_message("smtime: %e, diff: %e",
	                                smtime.getAsSec(), diff)); 
}
#define assertCurrentTime(SMT) cut_trace(_assertCurrentTime(SMT))

static void _assertEqualTimespec(const timespec &expect, const timespec &actual)
{
	cppcut_assert_equal(expect.tv_sec, actual.tv_sec);
	cppcut_assert_equal(expect.tv_nsec, actual.tv_nsec);
}
#define assertEqualTimespec(E,A) cut_trace(_assertEqualTimespec(E,A))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getCurrTime(void)
{
	SmartTime smtime = SmartTime::getCurrTime();
	assertCurrentTime(smtime);
}

void test_constructorNone(void)
{
	SmartTime smtime;
	cppcut_assert_equal(0.0, smtime.getAsSec());
}

void test_constructorNoneExplicit(void)
{
	SmartTime smtime(SmartTime::INIT_NONE);
	cppcut_assert_equal(0.0, smtime.getAsSec());
}

void test_constructorCurrTime(void)
{
	SmartTime smtime(SmartTime::INIT_CURR_TIME);
	assertCurrentTime(smtime);
}

void test_setCurrTime(void)
{
	SmartTime smtime;
	smtime.setCurrTime();
	assertCurrentTime(smtime);
}

void test_getAsTimespec(void)
{
	timespec ts = getSampleTimespec();
	SmartTime smtime(ts);
	assertEqualTimespec(ts, smtime.getAsTimespec());
}

void test_getAsMSec(void)
{
	SmartTime smtime(getSampleTimespec());
	cppcut_assert_equal(1e3*smtime.getAsSec(), smtime.getAsMSec());
}

} // namespace testSmartTime
