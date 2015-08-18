/*
 * Copyright (C) 2013 Project Hatohol
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

#include <cstdio>
#include <cppcutter.h>
#include <string>
#include <gcutter.h>
#include "SmartBuffer.h"
using namespace std;
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
	delete g_sbuf;
	g_sbuf = NULL;
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

void test_getValue(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	sbuf.resetIndex();
	cppcut_assert_equal((char)0,  sbuf.getValue<char>()); // CURR_INDEX
	cppcut_assert_equal((char)10, sbuf.getValue<char>(5));
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

void test_addString(void)
{
	SmartBuffer sbuf(100);
	string foo = "foo X1234";
	sbuf.add<uint16_t>(foo);
	const size_t expectSizeLen = sizeof(uint16_t);
	const uint16_t expectStrLen = foo.size();
	cppcut_assert_equal(expectSizeLen + expectStrLen, sbuf.index());
	cppcut_assert_equal(expectStrLen, *sbuf.getPointer<const uint16_t>(0));
	cppcut_assert_equal(
	  foo, string(sbuf.getPointer<char>(expectSizeLen), expectStrLen));
}

void test_insertString(void)
{
	SmartBuffer sbuf(100);
	const string str = "foo X1234";
	const uint32_t dummyStuff = 0x12345678;
	const size_t bodyIndex = 50;
	sbuf.add32(dummyStuff);
	const size_t nextIndex = sbuf.insertString(str, bodyIndex);

	sbuf.setIndex(sizeof(dummyStuff));
	cppcut_assert_equal(static_cast<uint32_t>(str.size()),
	                    sbuf.getValueAndIncIndex<uint32_t>());
	int32_t expectOffset = bodyIndex - sizeof(dummyStuff);
	cppcut_assert_equal(expectOffset, sbuf.getValueAndIncIndex<int32_t>());
	cppcut_assert_equal(str, string(sbuf.getPointer<char>(bodyIndex)));
	cppcut_assert_equal(nextIndex, bodyIndex + str.size() + 1);
	cppcut_assert_equal(nextIndex, sbuf.watermark());
}

void data_insertStringWithLongString(void)
{
	gcut_add_datum("2 characters (Within the limit)",
	               "txt", G_TYPE_STRING, "A",
	               "expectException", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("3 characters (Over the limit)",
	               "txt", G_TYPE_STRING, "AB",
	               "expectException", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_insertStringWithLongString(gconstpointer data)
{
	const size_t bodyIndex = 10;
	SmartBuffer sbuf(bodyIndex + 2);
	const string txt = gcut_data_get_string(data, "txt");
	const bool expectException =
	  gcut_data_get_boolean(data, "expectException");

	sbuf.add8(1);
	bool gotException = false;
	try {
		sbuf.insertString(txt, bodyIndex);
	} catch (const out_of_range &e) {
		gotException = true;
	}
	cppcut_assert_equal(expectException, gotException);
}

void data_addTooLongString(void)
{
	gcut_add_datum("Maximum",
	               "len", G_TYPE_INT, 255,
	               "expectException", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Over",
	               "len", G_TYPE_INT, 256,
	               "expectException", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_addTooLongString(gconstpointer data)
{
	const int len = gcut_data_get_int(data, "len");
	const bool expectException =
	  gcut_data_get_boolean(data, "expectException");

	SmartBuffer sbuf(512);
	string foo;
	for (int i = 0; i < len; i++)
		foo += 'X';
	bool gotException = false;
	try {
		sbuf.add<uint8_t>(foo);
	} catch (const range_error &e) {
		gotException = true;
	}
	cppcut_assert_equal(expectException, gotException);
}

void test_getString(void)
{
	SmartBuffer sbuf(100);
	string material = "ABC 123 xyz";
	sbuf.add<uint16_t>(material);
	const size_t indexPos = 25;
	sbuf.setIndex(indexPos);
	cppcut_assert_equal(material, sbuf.getString<uint16_t>(0));
	cppcut_assert_equal(indexPos, sbuf.index());
}

void test_getStringAndIncIndex(void)
{
	SmartBuffer sbuf(100);
	string material = "ABC 123 xyz";
	sbuf.add<uint16_t>(material);
	sbuf.resetIndex();
	cppcut_assert_equal(material, sbuf.getStringAndIncIndex<uint16_t>());
	const size_t expectIndex = sizeof(uint16_t) + material.size();
	cppcut_assert_equal(expectIndex, sbuf.index());
}

void test_extractString(void)
{
	SmartBuffer sbuf(100);
	const string str = "foo X1234";
	const size_t bodyIndex = 50;
	sbuf.insertString(str, bodyIndex);
	sbuf.resetIndex(); // index -> 0
	const SmartBuffer::StringHeader *header =
	  sbuf.getPointer<SmartBuffer::StringHeader>();
	cppcut_assert_equal(str, sbuf.extractString(header));

	// check that the index is not changed.
	cppcut_assert_equal((size_t)0, sbuf.index());
}

void test_extractStringAndIncIndex(void)
{
	SmartBuffer sbuf(100);
	const string str = "foo X1234";
	const uint32_t dummyStuff = 0x12345678;
	const size_t bodyIndex = 50;
	sbuf.add32(dummyStuff);
	sbuf.insertString(str, bodyIndex);
	sbuf.resetIndex();
	cppcut_assert_equal(dummyStuff, sbuf.getValueAndIncIndex<uint32_t>());
	cppcut_assert_equal(str, sbuf.extractStringAndIncIndex());

	// check the index after the call
	const size_t expectIndex =
	  sizeof(dummyStuff) + sizeof(SmartBuffer::StringHeader);
	cppcut_assert_equal(expectIndex, sbuf.index());
}

} // namespace testSmartBuffer
