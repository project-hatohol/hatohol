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
#include "SmartBuffer.h"
using namespace mlpl;

namespace testSmartBuffer {

static SmartBuffer *g_sbuf = NULL;

void cut_teardown(void)
{
	if (g_sbuf) {
		delete g_sbuf;
		g_sbuf = NULL;
	}
}

// ----------------------------------------------------------------------------
// test cases
// ----------------------------------------------------------------------------
void test_watermarkInit(void)
{
	SmartBuffer sbuf;
	cppcut_assert_equal((size_t)0, sbuf.watermark());
}

void test_watermarkAdd(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.add32(3);
	cppcut_assert_equal((size_t)5, sbuf.watermark());
}

void test_watermarkResetIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.resetIndex();
	cppcut_assert_equal((size_t)0, sbuf.watermark());
	sbuf.add32(3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_takeOver(void)
{
	static const size_t buflen = 5;
	SmartBuffer sbuf(buflen);
	for (size_t i = 0; i < buflen; i++)
		sbuf.add8(i);
	const char *ptr = sbuf;
	size_t size  = sbuf.size();
	size_t index = sbuf.index();

	g_sbuf = sbuf.takeOver();
	cppcut_assert_equal(ptr, (const char *)(*g_sbuf));
	cppcut_assert_equal(size, g_sbuf->size());
	cppcut_assert_equal(index, g_sbuf->index());

	cppcut_assert_equal(NULL, (const char *)sbuf);
	cppcut_assert_equal((size_t)0, sbuf.size());
	cppcut_assert_equal((size_t)0, sbuf.index());
}

} // namespace testSmartBuffer
