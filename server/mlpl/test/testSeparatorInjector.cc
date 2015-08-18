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

#include <string>
#include <vector>
using namespace std;

#include <cutter.h>
#include <cppcutter.h>
#include <gcutter.h>

#include "SeparatorInjector.h"
using namespace mlpl;

namespace testSeparatorInejector {

void test_basicFeature(void)
{
	string str = "ABC";
	SeparatorInjector sepInj(",");
	sepInj(str);
	cppcut_assert_equal(string("ABC"), str);

	sepInj(str);
	cppcut_assert_equal(string("ABC,"), str);

	str += "DOG!";
	sepInj(str);
	cppcut_assert_equal(string("ABC,DOG!,"), str);
}

void test_clear(void)
{
	string str = "ABC";
	SeparatorInjector sepInj(",");
	sepInj(str);
	cppcut_assert_equal(string("ABC"), str);

	sepInj.clear();
	sepInj(str);
	cppcut_assert_equal(string("ABC"), str);
}

} // namespace testSeparatorInejector
