/*
 * Copyright (C) 2014 Project Hatohol
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
#include <vector>
#include "SmartQueue.h"

using namespace std;
using namespace mlpl;

namespace testSmartQueue {

// ----------------------------------------------------------------------------
// test cases
// ----------------------------------------------------------------------------
void test_pushAndPop(void)
{
	SmartQueue<int> q;
	q.push(1);
	q.push(-5);
	q.push(8);
	cppcut_assert_equal(1, q.pop());
	cppcut_assert_equal(-5, q.pop());
	cppcut_assert_equal(8, q.pop());
}

void test_popAll(void)
{
	struct Gadget {
		vector<int> vec;
		static void valueReceiver(int v, Gadget &obj)
		{
			obj.vec.push_back(v);
		}
	} actual;

	SmartQueue<int> q;
	q.push(1);
	q.push(-5);
	q.push(8);
	q.popAll<Gadget &>(Gadget::valueReceiver, actual);
	cppcut_assert_equal((size_t)3, actual.vec.size());
	cppcut_assert_equal(1,  actual.vec[0]);
	cppcut_assert_equal(-5, actual.vec[1]);
	cppcut_assert_equal(8,  actual.vec[2]);
}

} // namespace testSmartQueue
