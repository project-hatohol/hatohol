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
#include "HatoholDBUtils.h"
#include "Helpers.h"
#include "DBTablesTest.h"

using namespace std;
using namespace hfl;

namespace testHatoholDBUtils {

class HatoholDBUtilsTest : public HatoholDBUtils {
public:
	static void extractItemKeys(StringVector &params, const string &key)
	{
		HatoholDBUtils::extractItemKeys(params, key);
	}

	static int getItemVariable(const string &word) 
	{
		return HatoholDBUtils::getItemVariable(word);
	}

	static void testMakeItemBrief(const string &name, const string &key,
	                              const string &expected)
	{
		VariableItemGroupPtr itemGroup;
		itemGroup->addNewItem(ITEM_ID_ZBX_ITEMS_NAME, name);
		itemGroup->addNewItem(ITEM_ID_ZBX_ITEMS_KEY_, key);
		cppcut_assert_equal(
		  expected, HatoholDBUtilsTest::makeItemBrief(itemGroup));
	}

	static void assertTransformItemItemGroupToItemInfo(
	  const bool useItemCategoryNameMap)
	{
		const ItemCategoryIdType itemCategoryId =
		  useItemCategoryNameMap ? 1234 : NO_ITEM_CATEGORY_ID;
		ItemInfo expect = testItemInfo[0]; // make a copy
		VariableItemGroupPtr item = convert(expect, itemCategoryId);

		ItemInfo actual;
		// actual.serverId shall not be changed in the target method.
		actual.serverId = expect.serverId;

		ItemCategoryNameMap itemCategoryNameMap;
		if (useItemCategoryNameMap) {
			itemCategoryNameMap[itemCategoryId] =
			  expect.itemGroupName;
		} else {
			expect.itemGroupName = "N/A";
		}

		bool succeeded =
		   HatoholDBUtils::transformItemItemGroupToItemInfo(
		     actual, item, itemCategoryNameMap);
		cppcut_assert_equal(true, succeeded);

		cppcut_assert_equal(expect.serverId, actual.serverId);
		cppcut_assert_equal(expect.id, actual.id);
		cppcut_assert_equal(expect.hostId, actual.hostId);
		cppcut_assert_equal(expect.brief, actual.brief);
		cppcut_assert_equal(expect.lastValueTime.tv_sec,
		                    actual.lastValueTime.tv_sec);
		cppcut_assert_equal(expect.lastValueTime.tv_nsec,
		                    actual.lastValueTime.tv_nsec);
		cppcut_assert_equal(expect.lastValue, actual.lastValue);
		cppcut_assert_equal(expect.prevValue, actual.prevValue);
		cppcut_assert_equal(expect.itemGroupName, actual.itemGroupName);
		cppcut_assert_equal(expect.delay, actual.delay);
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_extractItemKeys(void)
{
	StringVector vect;
	HatoholDBUtilsTest::extractItemKeys
	  (vect, "vm.memory.size[available]");
	assertStringVectorVA(vect, "available", NULL);
}

void test_extractItemKeysNoBracket(void)
{
	StringVector vect;
	HatoholDBUtilsTest::extractItemKeys(vect, "system.uname");
	assertStringVectorVA(vect, NULL);
}

void test_extractItemKeysNullParams(void)
{
	StringVector vect;
	HatoholDBUtilsTest::extractItemKeys(vect, "proc.num[]");
	assertStringVectorVA(vect, "", NULL);
}

void test_extractItemKeysTwo(void)
{
	StringVector vect;
	HatoholDBUtilsTest::extractItemKeys(vect, "vfs.fs.size[/boot,free]");
	assertStringVectorVA(vect, "/boot", "free", NULL);
}

void test_extractItemKeysWithEmptyParams(void)
{
	StringVector vect;
	HatoholDBUtilsTest::extractItemKeys(vect, "proc.num[,,run]");
	assertStringVectorVA(vect, "", "", "run", NULL);
}

void test_getItemVariable(void)
{
	cppcut_assert_equal(1, HatoholDBUtilsTest::getItemVariable("$1"));
}

void test_getItemVariableMultipleDigits(void)
{
	cppcut_assert_equal(123, HatoholDBUtilsTest::getItemVariable("$123"));
}

void test_getItemVariableWord(void)
{
	cppcut_assert_equal(-1, HatoholDBUtilsTest::getItemVariable("abc"));
}

void test_getItemVariableDoubleDollar(void)
{
	cppcut_assert_equal(-1, HatoholDBUtilsTest::getItemVariable("$$"));
}

void test_makeItemBrief(void)
{
	HatoholDBUtilsTest::testMakeItemBrief(
	  "CPU $2 time", "system.cpu.util[,idle]",
	  "CPU idle time");
}

void test_makeItemBriefOverNumVariable10(void)
{
	HatoholDBUtilsTest::testMakeItemBrief(
	  "ABC $12", "foo[P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13]",
	  "ABC P12");
}

void test_makeItemBriefTwoParam(void)
{
	HatoholDBUtilsTest::testMakeItemBrief(
	  "Free disk space on $1", "vfs.fs.size[/,free]",
	  "Free disk space on /");
}

void test_makeItemBriefTwoKeys(void)
{
	HatoholDBUtilsTest::testMakeItemBrief(
	  "Zabbix $4 $2 processes, in %",
	  "zabbix[process,unreachable poller,avg,busy]",
	  "Zabbix busy unreachable poller processes, in %");
}

void test_makeItemBriefNoVariables(void)
{
	HatoholDBUtilsTest::testMakeItemBrief(
	  "Host name", "system.hostname", "Host name");
}

void test_transformItemItemGroupToItemInfoWithoutItemCategory(void)
{
	cut_trace(
	  HatoholDBUtilsTest::assertTransformItemItemGroupToItemInfo(false));
}

void test_transformItemItemGroupToItemInfoWithItemCategory(void)
{
	cut_trace(
	  HatoholDBUtilsTest::assertTransformItemItemGroupToItemInfo(true));
}

} // namespace testHatoholDBUtils
