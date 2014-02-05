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
#include <string>
#include <StringUtils.h>
#include "DBAgent.h"
#include "DBAgentTest.h"
using namespace std;
using namespace mlpl;

namespace testDBAgent {

class TestDBAgent : public DBAgent {
public:

	void assertCreateTableProfile(void)
	{
		const char *name = "foo bar";
		TableProfile tblProf(name, m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		cppcut_assert_equal(name, tblProf.name);
		cppcut_assert_equal(m_testColumnDefs, tblProf.columnDefs);
		cppcut_assert_equal(m_numTestColumns, tblProf.numColumns);
	}

	void assertCreateTableProfileWithInvalidNumIndexes(void)
	{
		bool gotException = false;
		try {
			TableProfile tblProf("name", m_testColumnDefs,
			                     sizeof(m_testColumnDefs),
			                     m_numTestColumns+1);
		} catch (const HatoholException &e) {
			gotException = true;
		}
		cppcut_assert_equal(true, gotException);
	}

	void assertCreateUpdateArg(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		UpdateArg arg(tblProf);
		cppcut_assert_equal(&tblProf, &arg.tableProfile);
		cppcut_assert_equal(true, arg.condition.empty());
		cppcut_assert_equal(true, arg.rows.empty());
	}

	void assertCreateInsertArg(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		InsertArg arg(tblProf);
		cppcut_assert_equal(&tblProf, &arg.tableProfile);
		cppcut_assert_equal((size_t)0, arg.row->getNumberOfItems());
	}

	void assertCreateSelectArg(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		SelectArg arg(tblProf);
		cppcut_assert_equal(&tblProf, &arg.tableProfile);
		cppcut_assert_equal(true, arg.columnIndexes.empty());
		cppcut_assert_equal((size_t)0,
		                    arg.dataTable->getNumberOfRows());
	}

	void assertCreateSelectExArg(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		SelectExArg arg(tblProf);
		cppcut_assert_equal(&tblProf, arg.tableProfile);
		cppcut_assert_equal(true, arg.statements.empty());
		cppcut_assert_equal(true, arg.columnTypes.empty());
		cppcut_assert_equal(true, arg.condition.empty());
		cppcut_assert_equal(true, arg.orderBy.empty());
		cppcut_assert_equal((size_t)0, arg.limit);
		cppcut_assert_equal((size_t)0, arg.offset);
		cppcut_assert_equal(true, arg.tableField.empty());
		cppcut_assert_equal((size_t)0,
		                    arg.dataTable->getNumberOfRows());
	}

	void assertSelectExArgAdd(void)
	{
		SelectExArg arg(tableProfileTest);
		vector<string> expectStmts;
		for (size_t i = 0; i < tableProfileTest.numColumns; i++) {
			string v;
			string expect;
			if (i % 2) {
				v = StringUtils::sprintf("%c", 'a' + (int)i);
				arg.add(i, v);
				expect = v;
				expect += ".";
			} else {
				arg.add(i);
			}
			expect += tableProfileTest.columnDefs[i].columnName;
			expectStmts.push_back(expect);
		}

		// check
		cppcut_assert_equal(tableProfileTest.numColumns,
		                    arg.statements.size());
		cppcut_assert_equal(tableProfileTest.numColumns,
		                    arg.columnTypes.size());
		for (size_t i = 0; i < tableProfileTest.numColumns; i++) {
			const string &actualStmt = arg.statements[i];
			cppcut_assert_equal(expectStmts[i], actualStmt);

			const SQLColumnType &actualType = arg.columnTypes[i];
			const SQLColumnType &expectType =
			  tableProfileTest.columnDefs[i].type;
			cppcut_assert_equal(expectType, actualType);
		}
	}

	void assertSelectExArgAddDirectStatement(void)
	{
		SelectExArg arg(tableProfileTest);
		arg.add("count(*)", SQL_COLUMN_TYPE_INT);
		arg.add("avg(foo)", SQL_COLUMN_TYPE_DOUBLE);

		// check
		cppcut_assert_equal((size_t)2, arg.statements.size());
		cppcut_assert_equal((size_t)2, arg.columnTypes.size());
		cppcut_assert_equal(string("count(*)"), arg.statements[0]);
		cppcut_assert_equal(string("avg(foo)"), arg.statements[1]);
		cppcut_assert_equal(SQL_COLUMN_TYPE_INT, arg.columnTypes[0]);
		cppcut_assert_equal(SQL_COLUMN_TYPE_DOUBLE, arg.columnTypes[1]);
	}

	void assertCreateSelectMultiTableArg(void)
	{
		TableProfileEx profiles[] = {
		  {&tableProfileTest, "t"}, {&tableProfileTestAutoInc, "inc"}
		};
		const size_t numTables =
		  sizeof(profiles) / sizeof(TableProfileEx);
		SelectMultiTableArg arg(profiles, numTables);
		cppcut_assert_equal(profiles, arg.profileExArray);
		cppcut_assert_equal(numTables, arg.numTables);
		cppcut_assert_equal(profiles, arg.currProfile);
	}

	void assertSelectMultiTableArgSetProfile(void)
	{
		TableProfileEx profiles[] = {
		  {&tableProfileTest, "t"}, {&tableProfileTestAutoInc, "inc"}
		};
		const size_t numTables =
		  sizeof(profiles) / sizeof(TableProfileEx);
		SelectMultiTableArg arg(profiles, numTables);
		arg.setProfile(1);
		cppcut_assert_equal(&profiles[1], arg.currProfile);
		cppcut_assert_equal(&tableProfileTestAutoInc, arg.tableProfile);
	}

	void assertSelectMultiTableArgAdd(void)
	{
		TableProfileEx profiles[] = {
		  {&tableProfileTest, "t"}, {&tableProfileTestAutoInc, "inc"}
		};
		const size_t numTables =
		  sizeof(profiles) / sizeof(TableProfileEx);
		SelectMultiTableArg arg(profiles, numTables);

		// 1st add()
		size_t columnIdx = 2;
		arg.add(columnIdx);

		cppcut_assert_equal((size_t)1, arg.statements.size());
		cppcut_assert_equal((size_t)1, arg.columnTypes.size());

		const ColumnDef *def = &tableProfileTest.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("t.%s", def->columnName),
		  arg.statements[0]);
		cppcut_assert_equal(def->type, arg.columnTypes[0]);

		// 2nd add() after the change of the profile
		arg.setProfile(1);
		columnIdx = 0;
		arg.add(columnIdx);

		cppcut_assert_equal((size_t)2, arg.statements.size());
		cppcut_assert_equal((size_t)2, arg.columnTypes.size());

		def = &tableProfileTestAutoInc.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("inc.%s", def->columnName),
		  arg.statements[1]);
		cppcut_assert_equal(def->type, arg.columnTypes[1]);
	}

	void assertSelectMultiTableArgGetFullName(void)
	{
		TableProfileEx profiles[] = {
		  {&tableProfileTest, "t"}, {&tableProfileTestAutoInc, "inc"}
		};
		const size_t numTables =
		  sizeof(profiles) / sizeof(TableProfileEx);
		SelectMultiTableArg arg(profiles, numTables);

		const size_t columnIdx = 3;
		const string actualName = arg.getFullName(0, columnIdx);

		const ColumnDef *def = &tableProfileTest.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("t.%s", def->columnName),
		  actualName);
	}

	void assertCreateDeleteArg(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		DeleteArg arg(tblProf);
		cppcut_assert_equal(&tblProf, &arg.tableProfile);
		cppcut_assert_equal(true, arg.condition.empty());
	}

	template <typename T, typename T_READ>
	void assertUpdateArgAdd(const T *vals, const size_t &numVals)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		UpdateArg arg(tblProf);
		for (size_t i = 0; i < numVals; i++)
			arg.add(i, vals[i]);

		// check
		cppcut_assert_equal(numVals, arg.rows.size());
		for (size_t i = 0; i < numVals; i++) {
			const RowElement *elem = arg.rows[i];
			const T_READ actual = *elem->dataPtr;
			cppcut_assert_equal(i, elem->columnIndex);
			cppcut_assert_equal(vals[i], static_cast<T>(actual));
			cppcut_assert_equal(1, elem->dataPtr->getUsedCount());
		}
	}

	template <typename T>
	void assertUpdateArgAdd(const T *vals, const size_t &numVals)
	{
		assertUpdateArgAdd<T,T>(vals, numVals);
	}

private:
	static const size_t m_numTestColumns = 5;
	ColumnDef m_testColumnDefs[m_numTestColumns];

	// stub implementations
	virtual bool isTableExisting(const std::string &tableName)
	{
		return false;
	}

	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition)
	{
		return false;
	}

	virtual void begin(void) {}
	virtual void commit(void) {}
	virtual void rollback(void) {}
	virtual void createTable(const DBAgent::TableProfile &tableProfile) {}
	virtual void insert(const InsertArg &insertArg) {}
	virtual void update(const UpdateArg &updateArg) {}
	virtual void select(const SelectArg &selectArg) {}
	virtual void select(DBAgentSelectExArg &selectExArg) {} // TODO: remove
	virtual void select(const SelectExArg &selectExArg) {}
	virtual void deleteRows(const DeleteArg &deleteArg) {}
	virtual void addColumns(const AddColumnsArg &addColumnsArg) {}

	virtual uint64_t getLastInsertId(void)
	{
		return 0;
	}

	virtual uint64_t getNumberOfAffectedRows(void)
	{
		return 0;
	}
};

const size_t TestDBAgent::m_numTestColumns;

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createTableProfile(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateTableProfile();
}

void test_createTableProfileWithIvalidNumIndexes(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateTableProfileWithInvalidNumIndexes();
}

void test_createInsertArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateInsertArg();
}

void test_createUpdateArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateUpdateArg();
}

void test_createSelectArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateSelectArg();
}

void test_createSelectExArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateSelectExArg();
}

void test_selectExArgAdd(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectExArgAdd();
}

void test_selectExArgAddDirectStatement(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectExArgAddDirectStatement();
}

void test_createSelectMultiTableArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateSelectMultiTableArg();
}

void test_selectMultiTableArgSetProfile(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectMultiTableArgSetProfile();
}

void test_selectMultiTableArgAdd(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectMultiTableArgAdd();
}

void test_selectMultiTableArgGetFullName(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectMultiTableArgGetFullName();
}

void test_createDeleteArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateDeleteArg();
}

void test_updateArgAddInt(void)
{
	TestDBAgent dbAgent;
	const int vals[] = {3, -1, 500};
	const size_t numVals = sizeof(vals) / sizeof(int);
	dbAgent.assertUpdateArgAdd<int>(vals, numVals);
}

void test_updateArgAddUint64(void)
{
	TestDBAgent dbAgent;
	const uint64_t vals[] = {0, 0xfedcba987654321, 0x0123456789abcdef};
	const size_t numVals = sizeof(vals) / sizeof(uint64_t);
	dbAgent.assertUpdateArgAdd<uint64_t>(vals, numVals);
}

void test_updateArgAddDouble(void)
{
	TestDBAgent dbAgent;
	const double vals[] = {0.8, -0.53432e237, 234.43243e8};
	const size_t numVals = sizeof(vals) / sizeof(double);
	dbAgent.assertUpdateArgAdd<double>(vals, numVals);
}

void test_updateArgAddString(void)
{
	TestDBAgent dbAgent;
	const string vals[] = {"booo", "v.v;", "Ueno Zoo"};
	const size_t numVals = sizeof(vals) / sizeof(string);
	dbAgent.assertUpdateArgAdd<string>(vals, numVals);
}

void test_updateArgAddTime_t(void)
{
	TestDBAgent dbAgent;
	const time_t vals[] = {0, 0x7fffffff, 1391563132};
	const size_t numVals = sizeof(vals) / sizeof(string);
	dbAgent.assertUpdateArgAdd<time_t, int>(vals, numVals);
}

} // namespace testDBAgent
