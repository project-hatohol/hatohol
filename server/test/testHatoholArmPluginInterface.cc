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

#include <gcutter.h>
#include <cppcutter.h>
#include <SimpleSemaphore.h>
#include "DataSamples.h"
#include "Helpers.h"
#include "HatoholArmPluginInterface.h"
#include "HatoholArmPluginInterfaceTest.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginInterface {

static const size_t TIMEOUT = 5000;
static const size_t HAPI_ITEM_INT_BODY_SIZE = 8;

void _assertHapiItemDataHeader(
  const HapiItemDataHeader *header,
  const ItemDataType &type, const ItemId &itemId, const bool &isNull = false)
{
	cppcut_assert_equal(isNull, (bool)EndianConverter::LtoN(header->flags));
	cppcut_assert_equal(type,
	                    (ItemDataType)EndianConverter::LtoN(header->type));
	cppcut_assert_equal(itemId,
	                    (ItemId)EndianConverter::LtoN(header->itemId));
}
#define assertHapiItemDataHeader(H,T,I,...) \
  cut_trace(_assertHapiItemDataHeader(H,T,I,##__VA_ARGS__))

template<typename NativeType, typename BodyType>
void _assertHapiItemDataBody(const void *body, const NativeType &expect)
{
	const BodyType *valuePtr = static_cast<const BodyType *>(body);
	cppcut_assert_equal(expect,
	                    (NativeType)EndianConverter::LtoN(*valuePtr));
}
#define assertHapiItemDataBody(NT,BT,BP,E) \
  cut_trace((_assertHapiItemDataBody<NT,BT>)(BP,E))

template <typename NativeType, typename ItemDataClass, typename BodyType>
void _assertAppendItemData(
  const NativeType &value, const size_t &expectBodySize,
  const ItemDataType &itemDataType,
  void (*bodyAssertFunc)(const void *body, void *userData) = NULL,
  void *assertFuncUserData = NULL)
{
	SmartBuffer sbuf;
	const ItemId itemId = 12345678;
	ItemDataPtr itemData(new ItemDataClass(itemId, value), false);

	const size_t expectSize = sizeof(HapiItemDataHeader) + expectBodySize;

	HatoholArmPluginInterface::appendItemData(sbuf, itemData);
	cppcut_assert_equal(expectSize, sbuf.index());
	const HapiItemDataHeader *header =
	  sbuf.getPointer<HapiItemDataHeader>(0);
	assertHapiItemDataHeader(header, itemDataType, itemId);
	if (bodyAssertFunc) {
		(*bodyAssertFunc)(header + 1, assertFuncUserData);
		return;
	}
	assertHapiItemDataBody(NativeType, BodyType, header + 1, value);
}
#define assertAppendItemData(NT,IDC,BT,VAL,BODY_SZ,IT,...) \
  cut_trace((_assertAppendItemData<NT,IDC,BT>)(VAL,BODY_SZ,IT,##__VA_ARGS__))

template<typename NativeType, typename ItemDataClass>
static void _assertCreateItemData(
  const NativeType &value,
  const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
{
	SmartBuffer sbuf;
	const ItemId itemId = 12345678;
	ItemDataPtr srcItemData(new ItemDataClass(itemId, value, nullFlag), false);
	HatoholArmPluginInterface::appendItemData(sbuf, srcItemData);
	sbuf.resetIndex();

	ItemDataPtr actual = HatoholArmPluginInterface::createItemData(sbuf);
	cppcut_assert_equal(itemId, actual->getId());
	cppcut_assert_equal(nullFlag == ITEM_DATA_NULL, actual->isNull());
	cppcut_assert_equal(*srcItemData, *actual);
	cppcut_assert_equal(1, actual->getUsedCount());
}
#define assertCreateItemData(NT,IDC,VAL,...) \
  cut_trace((_assertCreateItemData<NT,IDC>)(VAL,##__VA_ARGS__))

static ItemGroupPtr createTestItemGroup(void)
{
	const size_t NUM_ITEMS = 3;
	SmartBuffer sbuf;
	VariableItemGroupPtr itemGrpPtr(new ItemGroup());
	for (size_t i = 0; i < NUM_ITEMS; i++) {
		const int value = i * 5;
		ItemData *itemData = new ItemInt(value);
		itemGrpPtr->add(itemData, false);
	}
	return (ItemGroupPtr)itemGrpPtr;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	HatoholArmPluginInterface hapi;
}

void test_onConnected(void)
{
	HatoholArmPluginInterfaceTest hapi;
	hapi.assertStartAndWaitConnected();
}

void test_sendAndonReceived(void)
{
	const string testMessage = "FOO";

	// start HAPI pair
	HatoholArmPluginInterfaceTest hapiSv;
	hapiSv.assertStartAndWaitConnected();

	HatoholArmPluginInterfaceTest hapiCl(hapiSv);
	hapiCl.assertStartAndWaitConnected();

	// wait for the completion of the initiation
	hapiSv.assertWaitInitiated();

	// send the message and receive it
	hapiSv.setMessageIntercept();
	hapiCl.send(testMessage);
	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapiSv.getRcvSem().timedWait(TIMEOUT));
	cppcut_assert_equal(testMessage, hapiSv.getMessage());
}

void test_registCommandHandler(void)
{
	struct Hapi : public HatoholArmPluginInterfaceTest {
		const HapiCommandCode testCmdCode;
		HapiCommandCode       gotCmdCode;
		SimpleSemaphore       handledSem;

		Hapi(void)
		: testCmdCode((HapiCommandCode)5),
		  gotCmdCode((HapiCommandCode)0),
		  handledSem(0)
		{
			registerCommandHandler(
			  testCmdCode, (CommandHandler)&Hapi::handler);
		}

		void handler(const HapiCommandHeader *header)
		{
			gotCmdCode = (HapiCommandCode)header->code;
			handledSem.post();
		}
	} hapiSv;
	hapiSv.assertStartAndWaitConnected();

	HatoholArmPluginInterfaceTest hapiCl(hapiSv);
	hapiCl.assertStartAndWaitConnected();

	// wait for the completion of the initiation
	hapiSv.assertWaitInitiated();

	// send command code and wait for the callback.
	SmartBuffer cmdBuf;
	hapiCl.callSetupCommandHeader<void>(cmdBuf, hapiSv.testCmdCode);
	hapiCl.send(cmdBuf);

	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapiSv.handledSem.timedWait(TIMEOUT));
	cppcut_assert_equal(hapiSv.testCmdCode, hapiSv.gotCmdCode);
}

void test_onGotResponse(void)
{
	struct Hapi : public HatoholArmPluginInterfaceTest {
		HapiResponseCode      gotResCode;
		SimpleSemaphore       gotResSem;

		Hapi(void)
		: gotResCode(NUM_HAPI_CMD_RES),
		  gotResSem(0)
		{
		}
		
		void onGotResponse(const HapiResponseHeader *header,
		                   SmartBuffer &resBuf) override
		{
			gotResCode =
			  static_cast<HapiResponseCode>(header->code);
			gotResSem.post();
		}
	} hapiSv;
	hapiSv.assertStartAndWaitConnected();

	HatoholArmPluginInterfaceTest hapiCl(hapiSv);
	hapiCl.assertStartAndWaitConnected();

	// wait for the completion of the initiation
	hapiSv.assertWaitInitiated();

	// send a command code and wait for the callback.
	SmartBuffer resBuf;
	hapiCl.callSetupResponseBuffer<void>(resBuf);
	HapiResponseHeader *header = resBuf.getPointer<HapiResponseHeader>(0);
	header->sequenceId = 0;
	hapiCl.send(resBuf);

	cppcut_assert_equal(SimpleSemaphore::STAT_OK,
	                    hapiSv.gotResSem.timedWait(TIMEOUT));
	cppcut_assert_equal(HAPI_RES_OK, hapiSv.gotResCode);
}

void test_getString(void)
{
	SmartBuffer buf(10);
	char *head = buf.getPointer<char>(1);
	const size_t offset = 3;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = '\0';
	cut_assert_equal_string(
	  "AB", HatoholArmPluginInterface::getString(buf, head, offset, 2));
}

void test_getStringWithTooLongParam(void)
{
	SmartBuffer buf(3);
	char *head = buf.getPointer<char>(0);
	const size_t offset = 0;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = '\0';
	cut_assert_equal_string(
	  NULL,
	  HatoholArmPluginInterface::getString(buf, head, offset, 3));
}

void test_getStringWithoutNullTerm(void)
{
	SmartBuffer buf(3);
	char *head = buf.getPointer<char>(0);
	const size_t offset = 0;
	head[offset+0] = 'A';
	head[offset+1] = 'B';
	head[offset+2] = 'C';
	cut_assert_equal_string(
	  NULL,
	  HatoholArmPluginInterface::getString(buf, head, offset, 2));
}

void test_putString(void)
{
	char _buf[50];
	char *refAddr = &_buf[15];
	char *buf = &_buf[20];
	string str = "Cat";
	uint16_t offset;
	uint16_t length;
	char *next = HatoholArmPluginInterface::putString(buf, refAddr, str,
	                                                  &offset, &length);
	cppcut_assert_equal((uint16_t)5, offset);
	cppcut_assert_equal((uint16_t)3, length);
	cppcut_assert_equal(&_buf[24], next);
	cut_assert_equal_string("Cat", buf);
}

void test_getIncrementedSequenceId(void)
{
	HatoholArmPluginInterfaceTest plugin;
	cppcut_assert_equal(1u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(2u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(3u, plugin.callGetIncrementedSequenceId());
}

void test_getIncrementedSequenceIdAroundMax(void)
{
	HatoholArmPluginInterfaceTest plugin;
	plugin.callSetSequenceId(HatoholArmPluginInterface::SEQ_ID_MAX-1);
	cppcut_assert_equal(
	  HatoholArmPluginInterface::SEQ_ID_MAX,
	  plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(0u, plugin.callGetIncrementedSequenceId());
	cppcut_assert_equal(1u, plugin.callGetIncrementedSequenceId());
}

void data_appendItemBool(void)
{
	addDataSamplesForGCutBool();
}

void test_appendItemBool(gconstpointer data)
{
	bool value = gcut_data_get_boolean(data, "val");
	assertAppendItemData(bool, ItemBool, uint8_t, value, 1, ITEM_TYPE_BOOL);
}

void data_appendItemInt(void)
{
	addDataSamplesForGCutInt();
}

void test_appendItemInt(gconstpointer data)
{
	int value = gcut_data_get_int(data, "val");
	assertAppendItemData(int, ItemInt, uint64_t, value, 8, ITEM_TYPE_INT);
}

void data_appendItemUint64(void)
{
	addDataSamplesForGCutUint64();
}

void test_appendItemUint64(gconstpointer data)
{
	uint64_t value = gcut_data_get_uint64(data, "val");
	assertAppendItemData(uint64_t, ItemUint64, uint64_t,
	                     value, 8, ITEM_TYPE_UINT64);
}

void data_appendItemDouble(void)
{
	addDataSamplesForGCutDouble();
}

void test_appendItemDouble(gconstpointer data)
{
	double value = gcut_data_get_double(data, "val");
	assertAppendItemData(double, ItemDouble, double,
	                     value, 8, ITEM_TYPE_DOUBLE);
}

void data_appendItemString(void)
{
	addDataSamplesForGCutString();
}

void test_appendItemString(gconstpointer data)
{
	struct Gadget {
		string testStr;
		uint32_t strLen;

		static void assertBody(const void *body, void *userData)
		{
			Gadget *obj = static_cast<Gadget *>(userData);
			const uint32_t *size =
			  static_cast<const uint32_t *>(body);
			cppcut_assert_equal(obj->strLen,
			                    EndianConverter::LtoN(*size));
			const char *actual =
			  reinterpret_cast<const char *>(size + 1);
			cut_assert_equal_string(obj->testStr.c_str(), actual);
		}
	} gadget;

	gadget.testStr = gcut_data_get_string(data, "val");
	gadget.strLen = gadget.testStr.size();
	const size_t expectBodySize = sizeof(uint32_t) + gadget.strLen + 1;
	assertAppendItemData(string, ItemString, string,
	                     gadget.testStr, expectBodySize, ITEM_TYPE_STRING,
	                     Gadget::assertBody, &gadget);
}

void test_appendItemDataWithNull(void)
{
	SmartBuffer sbuf;
	ItemDataPtr itemData(new ItemBool(1234, true, ITEM_DATA_NULL), false);
	HatoholArmPluginInterface::appendItemData(sbuf, itemData);
	assertHapiItemDataHeader(
	  sbuf.getPointer<HapiItemDataHeader>(0), ITEM_TYPE_BOOL, 1234, true);
}

void data_createItemBool(void)
{
	addDataSamplesForGCutBool();
}

void test_createItemBool(gconstpointer data)
{
	bool value = gcut_data_get_boolean(data, "val");
	assertCreateItemData(bool, ItemBool, value);
}

void data_createItemInt(void)
{
	addDataSamplesForGCutInt();
}

void test_createItemInt(gconstpointer data)
{
	int value = gcut_data_get_int(data, "val");
	assertCreateItemData(int, ItemInt, value);
}

void data_createItemUint64(void)
{
	addDataSamplesForGCutUint64();
}

void test_createItemUint64(gconstpointer data)
{
	uint64_t value = gcut_data_get_uint64(data, "val");
	assertCreateItemData(uint64_t, ItemUint64, value);
}

void data_createItemDouble(void)
{
	addDataSamplesForGCutDouble();
}

void test_createItemDouble(gconstpointer data)
{
	double value = gcut_data_get_double(data, "val");
	assertCreateItemData(double, ItemDouble, value);
}

void data_createItemString(void)
{
	addDataSamplesForGCutString();
}

void test_createItemString(gconstpointer data)
{
	string value = gcut_data_get_string(data, "val");
	assertCreateItemData(string, ItemString, value);
}

void test_createItemDataOfNull(void)
{
	assertCreateItemData(bool, ItemBool, true, ITEM_DATA_NULL);
}

void test_appendItemGroup(void)
{
	// create test samples
	SmartBuffer sbuf;
	ItemGroupPtr itemGrpPtr = createTestItemGroup();
	HatoholArmPluginInterface::appendItemGroup(sbuf, itemGrpPtr);
	const size_t numItems = itemGrpPtr->getNumberOfItems();

	// check the entirely written size
	const size_t HAPI_ITEM_INT_SIZE =
	  sizeof(HapiItemDataHeader) + HAPI_ITEM_INT_BODY_SIZE;
	const size_t expectedSize =
	  sizeof(HapiItemGroupHeader) + numItems * HAPI_ITEM_INT_SIZE;
	cppcut_assert_equal(expectedSize, sbuf.index());

	// check the header content
	const HapiItemGroupHeader *grpHeader =
	  sbuf.getPointer<HapiItemGroupHeader>(0);
	const uint16_t expectedFlags = 0;
	cppcut_assert_equal(expectedFlags,
	                    EndianConverter::LtoN(grpHeader->flags));
	cppcut_assert_equal(numItems,
	                    (size_t)EndianConverter::LtoN(grpHeader->numItems));
	cppcut_assert_equal(expectedSize,
	                    (size_t)EndianConverter::LtoN(grpHeader->length));

	// check each item data
	for (size_t i = 0; i < itemGrpPtr->getNumberOfItems(); i++) {
		assertAppendItemData(int, ItemInt, uint64_t,
		                     *itemGrpPtr->getItemAt(i),
		                     HAPI_ITEM_INT_BODY_SIZE, ITEM_TYPE_INT);
	}
}

void test_createItemGroup(void)
{
	// Make a test data.
	SmartBuffer sbuf;
	ItemGroupPtr srcItemGrpPtr = createTestItemGroup();
	HatoholArmPluginInterface::appendItemGroup(sbuf, srcItemGrpPtr);

	sbuf.resetIndex();
	ItemGroupPtr createdItemGrpPtr =
	  HatoholArmPluginInterface::createItemGroup(sbuf);

	// Check the created ItemGroup
	cppcut_assert_equal(srcItemGrpPtr->getNumberOfItems(),
	                    createdItemGrpPtr->getNumberOfItems());

	// check each item data
	for (size_t i = 0; i < srcItemGrpPtr->getNumberOfItems(); i++) {
		cppcut_assert_equal(*srcItemGrpPtr->getItemAt(i),
		                    *createdItemGrpPtr->getItemAt(i));
	}
}

} // namespace testHatoholArmPluginInterface
