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
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testFaceRest {

static JSONParserAgent *g_parser = NULL;

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

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_testPost(void)
{
	TestModeStone stone;
	startFaceRest();
	StringMap parameters;
	parameters["AB"] = "Foo Goo N2";
	parameters["<!>"] = "? @ @ v '";
	parameters["O.O -v@v-"] = "'<< x234 >>'";
	RequestArg arg("/test", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJSONParser(arg);
	assertStartObject(g_parser, "queryData");
	StringMapIterator it = parameters.begin();
	for (; it != parameters.end(); ++it)
		assertValueInParser(g_parser, it->first, it->second);
}

void test_testError(void)
{
	TestModeStone stone;
	startFaceRest();
	RequestArg arg("/test/error");
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser, HTERR_ERROR_TEST);
}

} // namespace testFaceRest
