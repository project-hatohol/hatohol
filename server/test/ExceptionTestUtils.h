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

#ifndef ExceptionTestUtils_h
#define ExceptionTestUtils_h

#include <cppcutter.h>

template<class ThrowExceptionType, class CaughtExceptionType>
void _assertThrow(void)
{
	const char *brief = "foo";
	bool exceptionReceived = false;
	try {
		throw ThrowExceptionType(brief);
	} catch (const CaughtExceptionType &e) {
		exceptionReceived = true;
		string expected = "<:-1> ";
		expected += brief;
		expected += "\n";
		cppcut_assert_equal(expected, string(e.what()));
	}
	cppcut_assert_equal(true, exceptionReceived);
}
#define assertThrow(T,C) cut_trace((_assertThrow<T,C>()))

#endif // ExceptionTestUtils_h
