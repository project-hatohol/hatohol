/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <sys/time.h>
#include <errno.h>
#include <cmath>
#include <gcutter.h>
#include "SmartTime.h"

using namespace std;
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

static void _assertOperatorPlusSubst(bool carry)
{
	timespec ts0 = getSampleTimespec();
	SmartTime smtime0(ts0);

	long addedSec = 1050;
	long addedNSec = SmartTime::NANO_SEC_PER_SEC - ts0.tv_nsec;
	static const long EXPECTED_DECIMAL_VALUE = 2052;
	if (!carry)
		addedNSec -= 1;
	else
		addedNSec += EXPECTED_DECIMAL_VALUE;
	timespec ts1;
	ts1.tv_sec = addedSec;
	ts1.tv_nsec = addedNSec;
	smtime0 += ts1;

	long expectInt = ts0.tv_sec + addedSec;
	long expectDec = 0;
	if (!carry) {
		expectDec = 999999999;
	} else {
		expectInt += 1;
		expectDec = EXPECTED_DECIMAL_VALUE;
	}
	cppcut_assert_equal(expectInt, (long)smtime0.getAsTimespec().tv_sec);
	cppcut_assert_equal(expectDec, (long)smtime0.getAsTimespec().tv_nsec);
}
#define assertOperatorPlusSubst(C) cut_trace(_assertOperatorPlusSubst(C))

static void _assertOperatorMinusSubst(bool plusInt, bool plusDec)
{
	timespec ts0 = getSampleTimespec();
	SmartTime smtime0(ts0);

	static const int DiffSec = 1050;
	static const int DiffNSec = 237;
	timespec ts1;
	if (plusInt)
		ts1.tv_sec = ts0.tv_sec + DiffSec;
	else
		ts1.tv_sec = ts0.tv_sec - DiffSec;
	if (plusDec)
		ts1.tv_nsec = ts0.tv_nsec + DiffNSec;
	else
		ts1.tv_nsec = ts0.tv_nsec - DiffNSec;
	SmartTime smtime1(ts1);
	smtime1 -= smtime0;

	int expectInt = 0;
	int expectDec = 0;
	if (plusInt && plusDec) {
		expectInt = DiffSec;
		expectDec = DiffNSec;
	} else if (plusInt && !plusDec) {
		expectInt = DiffSec - 1;
		expectDec = 1000000000 - DiffNSec;
	} else if (!plusInt && plusDec) {
		expectInt = -DiffSec;
		expectDec = DiffNSec;
	} else {
		expectInt = -DiffSec - 1;
		expectDec = 1000000000 - DiffNSec;
	}
	cppcut_assert_equal(expectInt, (int)smtime1.getAsTimespec().tv_sec);
	cppcut_assert_equal(expectDec, (int)smtime1.getAsTimespec().tv_nsec);
}
#define assertOperatorMinusSubst(I,D) cut_trace(_assertOperatorMinusSubst(I,D))

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

void test_copyConstructor(void)
{
	SmartTime smtime0(SmartTime::INIT_CURR_TIME);
	SmartTime smtime1(smtime0);
	const timespec &ts0 = smtime0.getAsTimespec();
	const timespec &ts1 = smtime1.getAsTimespec();
	cppcut_assert_not_equal(&ts0, &ts1);
	cppcut_assert_equal(ts0.tv_sec, ts1.tv_sec);
	cppcut_assert_equal(ts0.tv_nsec, ts1.tv_nsec);
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

void test_operatorSubst(void)
{
	SmartTime src(getSampleTimespec());
	SmartTime dst(SmartTime::INIT_CURR_TIME);
	dst = src;
	assertEqualTimespec(src.getAsTimespec(), dst.getAsTimespec());
}

void test_operatorPlusSubst(void)
{
	assertOperatorPlusSubst(false);
}

void test_operatorPlusSubstCarry(void)
{
	assertOperatorPlusSubst(true);
}

void test_operatorMinusSubst(void)
{
	assertOperatorMinusSubst(true, true);
}

void test_operatorMinusSubstBorrow(void)
{
	assertOperatorMinusSubst(true, false);
}

void test_operatorMinusSubstResultMinus(void)
{
	assertOperatorMinusSubst(false, true);
}

void test_operatorMinusSubstBorrowResultMinus(void)
{
	assertOperatorMinusSubst(false, false);
}

void test_operatorEqual(void)
{
	timespec ts = {1393897643, 987654321};
	SmartTime stime0(ts);
	SmartTime stime1(ts);
	cppcut_assert_equal(true, stime0 == stime1);
}

void test_operatorEqualDiffNSec(void)
{
	timespec ts0 = {1393897643, 987654321};
	timespec ts1 = {1393897643, 123456789};
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(false, stime0 == stime1);
}

void test_operatorEqualDiffSec(void)
{
	timespec ts0 = {1393897643, 987654321};
	timespec ts1 = {1393897000, 987654321};
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(false, stime0 == stime1);
}

void data_operatorGreaterOrEqual(void)
{
	gcut_add_datum("Second of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897001, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654322,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Equal",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Second of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897001, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654320,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
}

void test_operatorGreaterOrEqual(gconstpointer data)
{
	timespec ts0 = {gcut_data_get_int(data, "secLhs"),
	                gcut_data_get_int(data, "nsLhs")};
	timespec ts1 = {gcut_data_get_int(data, "secRhs"),
	                gcut_data_get_int(data, "nsRhs")};
	const bool expect = gcut_data_get_boolean(data, "expect");
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(expect, stime0 >= stime1);
}

void data_operatorGreater(void)
{
	gcut_add_datum("Second of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897001, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654322,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Equal",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Second of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897001, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654320,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
}

void test_operatorGreater(gconstpointer data)
{
	timespec ts0 = {gcut_data_get_int(data, "secLhs"),
	                gcut_data_get_int(data, "nsLhs")};
	timespec ts1 = {gcut_data_get_int(data, "secRhs"),
	                gcut_data_get_int(data, "nsRhs")};
	const bool expect = gcut_data_get_boolean(data, "expect");
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(expect, stime0 > stime1);
}

void data_operatorLessOrEqual(void)
{
	gcut_add_datum("Second of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897001, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654322,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Equal",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Second of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897001, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654320,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
}

void test_operatorLessOrEqual(gconstpointer data)
{
	timespec ts0 = {gcut_data_get_int(data, "secLhs"),
	                gcut_data_get_int(data, "nsLhs")};
	timespec ts1 = {gcut_data_get_int(data, "secRhs"),
	                gcut_data_get_int(data, "nsRhs")};
	const bool expect = gcut_data_get_boolean(data, "expect");
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(expect, stime0 <= stime1);
}

void data_operatorLess(void)
{
	gcut_add_datum("Second of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897001, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is greater",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654322,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Equal",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("Second of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654321,
	               "secRhs", G_TYPE_INT, 1393897001, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("Nanosecond of LHS is smaller",
	               "secLhs", G_TYPE_INT, 1393897000, "nsLhs",  G_TYPE_INT,  987654320,
	               "secRhs", G_TYPE_INT, 1393897000, "nsRhs",  G_TYPE_INT,  987654321,
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
}

void test_operatorLess(gconstpointer data)
{
	timespec ts0 = {gcut_data_get_int(data, "secLhs"),
	                gcut_data_get_int(data, "nsLhs")};
	timespec ts1 = {gcut_data_get_int(data, "secRhs"),
	                gcut_data_get_int(data, "nsRhs")};
	const bool expect = gcut_data_get_boolean(data, "expect");
	SmartTime stime0(ts0);
	SmartTime stime1(ts1);
	cppcut_assert_equal(expect, stime0 < stime1);
}

void test_operatorOStream(void)
{
	timespec ts = {1234567890, 12345678};
	const SmartTime stime(ts);
	ostringstream ss; 
	ss << stime;
	cppcut_assert_equal(string("1234567890.012345678"), ss.str());
}

void test_operatorCastToString(void)
{
	timespec ts = {1234567890, 1234567};
	const SmartTime stime(ts);
	cppcut_assert_equal(string("1234567890.001234567"),
	                    static_cast<string>(stime));
}

void data_hasValidTime(void)
{
	gcut_add_datum("both: zero",
	               "sec", G_TYPE_INT, 0, "ns",  G_TYPE_INT, 0,
	               "expect", G_TYPE_BOOLEAN, FALSE,  NULL);
	gcut_add_datum("ns: non-zero",
	               "sec", G_TYPE_INT, 0, "ns",  G_TYPE_INT, 1,
	               "expect", G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("sec: non-zero",
	               "sec", G_TYPE_INT, 1, "ns",  G_TYPE_INT, 0,
	               "expect", G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("both: non-zero",
	               "sec", G_TYPE_INT, 1, "ns",  G_TYPE_INT, 1,
	               "expect", G_TYPE_BOOLEAN, TRUE,  NULL);
}

void test_hasValidTime(gconstpointer data)
{
	const timespec ts = {gcut_data_get_int(data, "sec"),
	                     gcut_data_get_int(data, "ns")};
	const bool expect = gcut_data_get_boolean(data, "expect");
	SmartTime stime(ts);
	cppcut_assert_equal(expect, stime.hasValidTime());
}

void test_hasValidTimeOfDefault(void)
{
	SmartTime stime;
	cppcut_assert_equal(false, stime.hasValidTime());
}

} // namespace testSmartTime
