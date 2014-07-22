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
#include "DBClientJoinArg.h"

using namespace std;
using namespace mlpl;

namespace testDBClientJoinArg {

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
},
};

enum {
	IDX_TEST_TABLE1_ID,
	IDX_TEST_TABLE1_TBL0_ID,
	NUM_IDX_TEST_TABLE1,
};

const size_t NUM_COLUMNS_TEST1 = sizeof(COLUMN_DEF_TEST1) / sizeof(ColumnDef);
const DBAgent::TableProfile tableProfileTest1(
  TEST_TABLE_NAME1, COLUMN_DEF_TEST1,
  sizeof(COLUMN_DEF_TEST1), NUM_IDX_TEST_TABLE1
);

class DBClientJoinArgTest : public DBClientJoinArg {
public:
	static const char *callGetJoinOperatorString(const JoinType &type)
	{
		return getJoinOperatorString(type);
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_basicUse(void)
{
	DBClientJoinArg arg(tableProfileTest0);
	arg.add(IDX_TEST_TABLE0_NAME);

	arg.addTable(tableProfileTest1, DBClientJoinArg::INNER_JOIN,
	  IDX_TEST_TABLE0_ID, IDX_TEST_TABLE1_TBL0_ID);
	arg.add(IDX_TEST_TABLE1_ID);

	// check where clause
	const DBAgent::SelectExArg &exArg = arg.getSelectExArg();
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

void data_getJoinOperatorString(void)
{
	gcut_add_datum("INNER JOIN",
	               "type", G_TYPE_INT, DBClientJoinArg::INNER_JOIN,
	               "expect", G_TYPE_STRING, "INNER JOIN", NULL);
	gcut_add_datum("LEFT JOIN",
	               "type", G_TYPE_INT, DBClientJoinArg::LEFT_JOIN,
	               "expect", G_TYPE_STRING, "LEFT JOIN", NULL);
	gcut_add_datum("RIGHT JOIN",
	               "type", G_TYPE_INT, DBClientJoinArg::RIGHT_JOIN,
	               "expect", G_TYPE_STRING, "RIGHT JOIN", NULL);
}

void test_getJoinOperatorString(gconstpointer data)
{
	const DBClientJoinArg::JoinType type =
	  static_cast<const DBClientJoinArg::JoinType>(gcut_data_get_int(data, "type"));
	const char *expect = gcut_data_get_string(data, "expect");
	cut_assert_equal_string(
	  expect, DBClientJoinArgTest::callGetJoinOperatorString(type));
}

} // namespace testDBClientJoinArg
