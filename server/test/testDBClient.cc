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
#include "DBClient.h"

namespace testDBClient {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_isAlwaysFalseCondition(void)
{
	bool actual = DBClient::isAlwaysFalseCondition(
	                DBClient::getAlwaysFalseCondition());
	cppcut_assert_equal(true, actual);
}

void test_isAlwaysFalseConditionReturnFalse(void)
{
	bool actual = DBClient::isAlwaysFalseCondition("1");
	cppcut_assert_equal(false, actual);
}

} // namespace testDBClient
