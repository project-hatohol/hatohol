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

struct BufferParams {
	const char *ptr;
	size_t size;
	size_t index;
	size_t watermark;

	BufferParams(SmartBuffer &sbuf)
	{
		ptr   = sbuf;
		size  = sbuf.size();
		index = sbuf.index();
		watermark = sbuf.watermark();
	}
};

static void _assertEqual(
  const BufferParams &srcParams, SmartBuffer &src, SmartBuffer &dest)
{
	cppcut_assert_equal(srcParams.ptr,       (const char *)dest);
	cppcut_assert_equal(srcParams.size,      dest.size());
	cppcut_assert_equal(srcParams.index,     dest.index());
	cppcut_assert_equal(srcParams.watermark, dest.watermark());

	cppcut_assert_equal(NULL, (const char *)src);
	cppcut_assert_equal((size_t)0, src.size());
	cppcut_assert_equal((size_t)0, src.index());
	cppcut_assert_equal((size_t)0, src.watermark());
}
#define assertEqual(P,S,D) cut_trace(_assertEqual(P,S,D))

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

void test_watermarkIncIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.incIndex(5);
	cppcut_assert_equal((size_t)6, sbuf.watermark());
}

void test_watermarkResetIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	// resetIndex() doesn't reset the watermark
	sbuf.resetIndex();
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.add32(3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_watermarkResetIndexDeep(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.resetIndexDeep();
	cppcut_assert_equal((size_t)0, sbuf.watermark());
	sbuf.add32(3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_watermarkAlloc(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.alloc(20, true);
	cppcut_assert_equal((size_t)0, sbuf.watermark());
}

void test_watermarkAllocNotDeep(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.alloc(20, false);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
}

void test_watermarkSetAt(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add32(3);
	sbuf.setAt(50, 3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_getPointer(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	cppcut_assert_equal((char)10, *sbuf.getPointer<char>(5));
}

void test_getPointerDefaultParam(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	sbuf.resetIndex();
	sbuf.incIndex(3);
	cppcut_assert_equal((char)6, *sbuf.getPointer<char>());
}

void test_getPointerWithType(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	sbuf.resetIndex();
	cppcut_assert_equal((uint32_t)0x06040200, *sbuf.getPointer<uint32_t>());
}

void test_takeOver(void)
{
	static const size_t buflen = 5;
	SmartBuffer src(buflen);
	for (size_t i = 0; i < buflen; i++)
		src.add8(i);

	BufferParams srcParams(src);
	g_sbuf = src.takeOver();
	assertEqual(srcParams, src, *g_sbuf);
}

void test_handOver(void)
{
	static const size_t buflen = 5;
	SmartBuffer src(buflen);
	for (size_t i = 0; i < buflen; i++)
		src.add8(i);

	BufferParams srcParams(src);
	SmartBuffer dest;
	src.handOver(dest);
	assertEqual(srcParams, src, dest);
}

void test_setIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.setIndex(5);
	cppcut_assert_equal((size_t)5, sbuf.index());
}

} // namespace testSmartBuffer
