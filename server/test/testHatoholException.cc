/*
 * Copyright (C) 2013 Project Hatohol
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

#include <gcutter.h>
#include <cppcutter.h>
#include "Hatohol.h"
#include "Params.h"
#include "HatoholException.h"
#include "ExceptionTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testHatoholException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	struct Check {
		static void errCode(const HatoholException &e) {
			cppcut_assert_equal(HTERR_UNKNOWN_REASON,
			                    e.getErrCode());
		}
	};
	assertThrow(HatoholException, HatoholException, Check::errCode);
}

void test_throwAsException(void)
{
	assertThrow(HatoholException, exception);
}

void test_hatoholAssertPass(void)
{
	bool gotException = false;
	int a = 1;
	try {
		HATOHOL_ASSERT(a == 1, "a == 1 @ %s", __func__);
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void test_hatoholAssertFail(void)
{
	bool gotException = false;
	int a = 2;
	try {
		HATOHOL_ASSERT(a == 1, "a = %d", a);
	} catch (const HatoholException &e) {
		cut_notify("<<MSG>>\n%s", e.getFancyMessage().c_str());
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

// ---------------------------------------------------------------------------
// Test cases for ExceptionCathable
// ---------------------------------------------------------------------------
static void setupDataForCatchException(void)
{
	gcut_add_datum("Not throw", "throw", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Throw",     "throw", G_TYPE_BOOLEAN, TRUE,  NULL);
}

void data_catchHatoholException(void)
{
	setupDataForCatchException();
}

void test_catchHatoholException(gconstpointer data)
{
	struct : public ExceptionCatchable {
		bool throwException;
		virtual void operator ()(void) override
		{
			if (throwException)
				THROW_HATOHOL_EXCEPTION("A test exception.");
		}
	} catcher;

	catcher.throwException = gcut_data_get_boolean(data, "throw");
	cppcut_assert_equal(catcher.throwException, catcher.exec());
}

void data_catchStdException(void)
{
	setupDataForCatchException();
}

void test_catchStdException(gconstpointer data)
{
	struct : public ExceptionCatchable {
		bool throwException;
		virtual void operator ()(void) override
		{
			if (throwException)
				throw exception();
		}
	} catcher;

	catcher.throwException = gcut_data_get_boolean(data, "throw");
	cppcut_assert_equal(catcher.throwException, catcher.exec());
}

void data_catchUnknownException(void)
{
	setupDataForCatchException();
}

void test_catchUnknownException(gconstpointer data)
{
	struct : public ExceptionCatchable {
		bool throwException;
		virtual void operator ()(void) override
		{
			if (throwException)
				throw "Unknown exception";
		}
	} catcher;

	catcher.throwException = gcut_data_get_boolean(data, "throw");
	cppcut_assert_equal(catcher.throwException, catcher.exec());
}

void data_THROW_HATOHOL_EXCEPTION_IF_NOT_OK(void)
{
	gcut_add_datum("HTERR_OK",
	               "code", G_TYPE_INT, HTERR_OK,
	               "expectException", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("HTERR_UNKNOWN_REASON",
	               "code", G_TYPE_INT, HTERR_UNKNOWN_REASON,
	               "expectException", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_THROW_HATOHOL_EXCEPTION_IF_NOT_OK(gconstpointer data)
{
	hatoholInit();

	const HatoholErrorCode errCode =
	  static_cast<HatoholErrorCode>(gcut_data_get_int(data, "code"));
	const char *optMsg = "Option message!";
	HatoholError err(errCode, optMsg);
	const bool expectException =
	  gcut_data_get_boolean(data, "expectException");

	bool gotException = false;
	HatoholErrorCode gotErrCode = HTERR_UNINITIALIZED;
	int gotLineNum = -1;
	string gotMessage;
	int expectLineNum = -1;
	try {
		// The following tow
		expectLineNum = __LINE__; THROW_HATOHOL_EXCEPTION_IF_NOT_OK(err);
	} catch (HatoholException &e) {
		gotException = true;
		gotErrCode = e.getErrCode();
		gotLineNum = e.getLineNumber();
		gotMessage = e.getFancyMessage();
	}

	cppcut_assert_equal(expectException, gotException);
	if (!expectException)
		return;
	cppcut_assert_equal(errCode, gotErrCode);
	cppcut_assert_equal(expectLineNum, gotLineNum);

	const string expectMsg = StringUtils::sprintf(
	  "<%s:%d> %s : %s\n", __FILE__, expectLineNum,
	  err.getMessage().c_str(), optMsg);
	cppcut_assert_equal(expectMsg, gotMessage);
}

} // namespace testHatoholException
