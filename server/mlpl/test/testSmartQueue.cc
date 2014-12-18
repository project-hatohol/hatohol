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

struct Gadget {
	vector<int> vec;
	static void valueReceiver(int v, Gadget &obj)
	{
		obj.vec.push_back(v);
	}
};

void test_popAll(void)
{
	Gadget actual;
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

void test_popIfNonEmptyWithElement(void)
{
	SmartQueue<int> q;
	q.push(1);
	int val;
	cppcut_assert_equal(true, q.popIfNonEmpty(val));
	cppcut_assert_equal(1, val);
}

void test_popIfNonEmptyWithoutElement(void)
{
	SmartQueue<int> q;
	int val;
	cppcut_assert_equal(false, q.popIfNonEmpty(val));
}

void test_empty(void)
{
	SmartQueue<int> q;
	cppcut_assert_equal(true, q.empty());
	q.push(1);
	cppcut_assert_equal(false, q.empty());
	q.pop();
	cppcut_assert_equal(true, q.empty());
}

void test_size(void)
{
	SmartQueue<int> q;
	cppcut_assert_equal((size_t)0, q.size());
	q.push(1);
	cppcut_assert_equal((size_t)1, q.size());
	q.push(5);
	cppcut_assert_equal((size_t)2, q.size());
	q.pop();
	cppcut_assert_equal((size_t)1, q.size());
	q.pop();
	cppcut_assert_equal((size_t)0, q.size());
}

void test_front(void)
{
	SmartQueue<int> q;
	q.push(1);
	q.push(5);
	cppcut_assert_equal(1, q.front());
	q.pop();
	cppcut_assert_equal(5, q.front());
}

void test_frontReadTwice(void)
{
	SmartQueue<int> q;
	q.push(3);
	cppcut_assert_equal(3, q.front());
	cppcut_assert_equal(3, q.front());
}

} // namespace testSmartQueue
