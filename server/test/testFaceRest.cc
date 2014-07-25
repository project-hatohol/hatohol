/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include "Hatohol.h"
#include "FaceRest.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRest {

static JsonParserAgent *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;
}

static void _assertTestMode(const bool expectedMode = false,
                            const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg("/test");
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "testMode", expectedMode);
}
#define assertTestMode(E,...) cut_trace(_assertTestMode(E,##__VA_ARGS__))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_isTestModeDefault(void)
{
	cppcut_assert_equal(false, FaceRest::isTestMode());
	assertTestMode(false);
}

void test_isTestModeSet(void)
{
	CommandLineArg arg;
	arg.push_back("--test-mode");
	hatoholInit(&arg);
	cppcut_assert_equal(true, FaceRest::isTestMode());
	assertTestMode(true);
}

void test_isTestModeReset(void)
{
	test_isTestModeSet();
	cut_teardown();
	hatoholInit();
	cppcut_assert_equal(false, FaceRest::isTestMode());
	assertTestMode(false);
}

void test_testPost(void)
{
	setupTestMode();
	startFaceRest();
	StringMap parameters;
	parameters["AB"] = "Foo Goo N2";
	parameters["<!>"] = "? @ @ v '";
	parameters["O.O -v@v-"] = "'<< x234 >>'";
	RequestArg arg("/test", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	assertStartObject(g_parser, "queryData");
	StringMapIterator it = parameters.begin();
	for (; it != parameters.end(); ++it)
		assertValueInParser(g_parser, it->first, it->second);
}

void test_testError(void)
{
	setupTestMode();
	startFaceRest();
	RequestArg arg("/test/error");
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_ERROR_TEST);
}

} // namespace testFaceRest
