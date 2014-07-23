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
#include "DBClient.h"
#include "DBClientJoinBuilder.h"
#include "HostResourceQueryOption.h"

using namespace std;
using namespace mlpl;

namespace testDBClientJoinBuilder {

static string getFullName(const DBAgent::TableProfile &tableProfile,
                          const size_t &columnIndex)
{
	string name = StringUtils::sprintf("%s.%s",
	   tableProfile.columnDefs[columnIndex].tableName,
	   tableProfile.columnDefs[columnIndex].columnName);
	return name;
}

static const char *TEST_TABLE_NAME0 = "test_table0";
static const char *TEST_TABLE_NAME1 = "test_table1";

static const ColumnDef COLUMN_DEF_TEST0[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME0,                  // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME0,                  // tableName
	"age",                             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME0,                  // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_TEST_TABLE0_ID,
	IDX_TEST_TABLE0_AGE,
	IDX_TEST_TABLE0_NAME,
	NUM_IDX_TEST_TABLE0,
};

const size_t NUM_COLUMNS_TEST0 = sizeof(COLUMN_DEF_TEST0) / sizeof(ColumnDef);
const DBAgent::TableProfile tableProfileTest0(
  TEST_TABLE_NAME0, COLUMN_DEF_TEST0,
  sizeof(COLUMN_DEF_TEST0), NUM_IDX_TEST_TABLE0
);

static const ColumnDef COLUMN_DEF_TEST1[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME1,                  // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME1,                  // tableName
	"tbl0_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_TABLE_NAME1,                  // tableName
	"my_age",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_TEST_TABLE1_ID,
	IDX_TEST_TABLE1_TBL0_ID,
	IDX_TEST_TABLE1_MY_AGE,
	NUM_IDX_TEST_TABLE1,
};

const size_t NUM_COLUMNS_TEST1 = sizeof(COLUMN_DEF_TEST1) / sizeof(ColumnDef);
const DBAgent::TableProfile tableProfileTest1(
  TEST_TABLE_NAME1, COLUMN_DEF_TEST1,
  sizeof(COLUMN_DEF_TEST1), NUM_IDX_TEST_TABLE1
);

class DBClientJoinBuilderTest : public DBClientJoinBuilder {
public:
	static const char *callGetJoinOperatorString(const JoinType &type)
	{
		return getJoinOperatorString(type);
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	DBClientJoinBuilder builder(tableProfileTest0);
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	cppcut_assert_equal(&tableProfileTest0, exArg.tableProfile);
	cppcut_assert_equal(true, exArg.useFullName);
}

void test_constructorWithOption(void)
{
	class MyOption : public DataQueryOption {
	public:
		virtual string getCondition(void) const override
		{
			return "A=1";
		}
	} option;

	DBClientJoinBuilder builder(tableProfileTest0, &option);
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	cppcut_assert_equal(&tableProfileTest0, exArg.tableProfile);
	cppcut_assert_equal(true, exArg.useFullName);
	cppcut_assert_equal(true, option.getTableNameAlways());
	cppcut_assert_not_equal(string(""),        exArg.condition);
	cppcut_assert_equal(option.getCondition(), exArg.condition);
}

void data_constructorWithHostResourceQueryOption(void)
{
	gcut_add_datum("With hostgroup", "used", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("W/O  hostgroup", "used", G_TYPE_BOOLEAN, FALSE, NULL);
}

void test_constructorWithHostResourceQueryOption(gconstpointer data)
{
	// Define synapse to pass the build. So the paramters are dummy and
	// the object won't work well.
	HostResourceQueryOption::Synapse
	   synapse(tableProfileTest0, 0, 0, 0, 0, tableProfileTest1, 0, 0, 0);
	class MyOption : public HostResourceQueryOption {
	public:
		bool hostgroupIsUsed;

		MyOption(const Synapse &synapse)
		: HostResourceQueryOption(synapse),
		  hostgroupIsUsed(false)
		{
		}

		virtual string getFromClause(void) const override
		{
			return "t0 INNER JOIN t1 ON t0.c0=t1.c1";
		}

		virtual string getCondition(void) const override
		{
			return "A=1";
		}

		virtual bool isHostgroupUsed(void) const override
		{
			return hostgroupIsUsed;
		}
	} option(synapse);
	option.hostgroupIsUsed = gcut_data_get_int(data, "used");

	DBClientJoinBuilder builder(tableProfileTest0, &option);
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	cppcut_assert_equal(&tableProfileTest0, exArg.tableProfile);
	cppcut_assert_equal(true, exArg.useFullName);
	cppcut_assert_equal(true, option.getTableNameAlways());
	cppcut_assert_not_equal(string(""),        exArg.condition);
	cppcut_assert_equal(option.getCondition(), exArg.condition);
	cppcut_assert_not_equal(string(""),         exArg.tableField);
	cppcut_assert_equal(option.getFromClause(), exArg.tableField);
	cppcut_assert_equal(option.hostgroupIsUsed, exArg.useDistinct);
}

void test_basicUse(void)
{
	DBClientJoinBuilder builder(tableProfileTest0);
	builder.add(IDX_TEST_TABLE0_NAME);

	builder.addTable(tableProfileTest1, DBClientJoinBuilder::INNER_JOIN,
	  IDX_TEST_TABLE0_ID, IDX_TEST_TABLE1_TBL0_ID);
	builder.add(IDX_TEST_TABLE1_ID);

	// check where clause
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	const string expectTableField = StringUtils::sprintf(
	  "%s INNER JOIN %s ON %s=%s",
	  TEST_TABLE_NAME0, TEST_TABLE_NAME1,
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_ID).c_str(),
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_TBL0_ID).c_str()
	);
	cppcut_assert_equal(expectTableField, exArg.tableField);

	// columns
	cppcut_assert_equal((size_t)2, exArg.statements.size());
	cppcut_assert_equal(
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_NAME),
	  exArg.statements[0]);
	cppcut_assert_equal(
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_ID),
	  exArg.statements[1]);
}

void test_addTableWithTableC(void)
{
	DBClientJoinBuilder builder(tableProfileTest0);
	builder.add(IDX_TEST_TABLE0_NAME);

	builder.addTable(tableProfileTest1, DBClientJoinBuilder::INNER_JOIN,
	  tableProfileTest0, IDX_TEST_TABLE0_ID, IDX_TEST_TABLE1_TBL0_ID);
	builder.add(IDX_TEST_TABLE1_ID);

	// check where clause
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	const string expectTableField = StringUtils::sprintf(
	  "%s INNER JOIN %s ON %s=%s",
	  TEST_TABLE_NAME0, TEST_TABLE_NAME1,
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_ID).c_str(),
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_TBL0_ID).c_str()
	);
	cppcut_assert_equal(expectTableField, exArg.tableField);

	// columns
	cppcut_assert_equal((size_t)2, exArg.statements.size());
	cppcut_assert_equal(
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_NAME),
	  exArg.statements[0]);
	cppcut_assert_equal(
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_ID),
	  exArg.statements[1]);
}

void test_useFourIndexes(void)
{
	DBClientJoinBuilder builder(tableProfileTest0);
	builder.add(IDX_TEST_TABLE0_NAME);

	builder.addTable(tableProfileTest1, DBClientJoinBuilder::INNER_JOIN,
	  tableProfileTest0, IDX_TEST_TABLE0_ID, IDX_TEST_TABLE1_TBL0_ID,
	  tableProfileTest0, IDX_TEST_TABLE0_AGE, IDX_TEST_TABLE1_MY_AGE);
	builder.add(IDX_TEST_TABLE1_ID);

	// check where clause
	const DBAgent::SelectExArg &exArg = builder.getSelectExArg();
	const string expectTableField = StringUtils::sprintf(
	  "%s INNER JOIN %s ON (%s=%s AND %s=%s)",
	  TEST_TABLE_NAME0, TEST_TABLE_NAME1,
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_ID).c_str(),
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_TBL0_ID).c_str(),
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_AGE).c_str(),
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_MY_AGE).c_str()
	);
	cppcut_assert_equal(expectTableField, exArg.tableField);

	// columns
	cppcut_assert_equal((size_t)2, exArg.statements.size());
	cppcut_assert_equal(
	  getFullName(tableProfileTest0, IDX_TEST_TABLE0_NAME),
	  exArg.statements[0]);
	cppcut_assert_equal(
	  getFullName(tableProfileTest1, IDX_TEST_TABLE1_ID),
	  exArg.statements[1]);
}

void data_getJoinOperatorString(void)
{
	gcut_add_datum("INNER JOIN",
	               "type", G_TYPE_INT, DBClientJoinBuilder::INNER_JOIN,
	               "expect", G_TYPE_STRING, "INNER JOIN", NULL);
	gcut_add_datum("LEFT JOIN",
	               "type", G_TYPE_INT, DBClientJoinBuilder::LEFT_JOIN,
	               "expect", G_TYPE_STRING, "LEFT JOIN", NULL);
	gcut_add_datum("RIGHT JOIN",
	               "type", G_TYPE_INT, DBClientJoinBuilder::RIGHT_JOIN,
	               "expect", G_TYPE_STRING, "RIGHT JOIN", NULL);
}

void test_getJoinOperatorString(gconstpointer data)
{
	const DBClientJoinBuilder::JoinType type =
	  static_cast<const DBClientJoinBuilder::JoinType>(gcut_data_get_int(data, "type"));
	const char *expect = gcut_data_get_string(data, "expect");
	cut_assert_equal_string(
	  expect, DBClientJoinBuilderTest::callGetJoinOperatorString(type));
}

} // namespace testDBClientJoinBuilder
