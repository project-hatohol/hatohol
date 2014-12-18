/*
 * Copyright (C) 2014 Project Hatohol
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
#include "JSONParser.h"
using namespace std;

namespace testJSONParserRewinder {

// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_pushObject(void)
{
	int64_t val;
	JSONParser parser("{\"obj1\":{\"elem10\":5}, \"elem00\":8}");
	cppcut_assert_equal(false, parser.hasError());
	{
		JSONParser::PositionStack posStack(parser);
		cppcut_assert_equal(true, posStack.pushObject("obj1"));
		cppcut_assert_equal(true, parser.read("elem10", val));
		cppcut_assert_equal((int64_t)5, val);
	}
	cppcut_assert_equal(true, parser.read("elem00", val));
	cppcut_assert_equal((int64_t)8, val);
}

void test_pushElement(void)
{
	int64_t val;
	JSONParser parser(
	  "{\"elem1\":[{\"elem10\":7},{\"elem11\":15}], \"elem00\":8}");
	cppcut_assert_equal(false, parser.hasError());
	{
		JSONParser::PositionStack posStack(parser);
		cppcut_assert_equal(true, posStack.pushObject("elem1"));
		cppcut_assert_equal(true, posStack.pushElement(0));
		cppcut_assert_equal(true, parser.read("elem10", val));
		cppcut_assert_equal((int64_t)7, val);
		posStack.pop();

		cppcut_assert_equal(true, posStack.pushElement(1));
		cppcut_assert_equal(true, parser.read("elem11", val));
		cppcut_assert_equal((int64_t)15, val);
	}
	cppcut_assert_equal(true, parser.read("elem00", val));
	cppcut_assert_equal((int64_t)8, val);
}

} //namespace testJSONParserRewinder
