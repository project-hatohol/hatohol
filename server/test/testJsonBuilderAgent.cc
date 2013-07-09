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
#include <StringUtils.h>
#include "JsonBuilderAgent.h"

using namespace mlpl;

namespace testJsonBuilderAgent {

// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_emptyObject(void)
{
	JsonBuilderAgent agent;
	agent.startObject();
	agent.endObject();
	string expected = "{}";
	cppcut_assert_equal(expected, agent.generate());
}

void test_array(void)
{
	string arrayName = "foo";
	const int numArray = 5;
	JsonBuilderAgent agent;
	agent.startObject();
	agent.startArray(arrayName);
	for (int i = 0; i < numArray; i++)
		agent.add(i);
	agent.endArray();
	agent.endObject();

	// check
	string expected = StringUtils::sprintf("{\"%s\":[", arrayName.c_str());
	for (int i = 0; i < numArray; i++) {
		if (i < numArray - 1)
			expected += StringUtils::sprintf("%d,", i);
		else
			expected += StringUtils::sprintf("%d]", i);
	}
	expected += "}";
	cppcut_assert_equal(expected, agent.generate());
}

} //namespace testJsonBuilderAgent


