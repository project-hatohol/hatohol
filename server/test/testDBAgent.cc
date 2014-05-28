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

	void assertSelectArgAdd(void)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		SelectArg arg(tblProf);
		cppcut_assert_equal(true, arg.columnIndexes.empty());
		arg.add(5);
		arg.add(3);
		arg.add(18);
		cppcut_assert_equal((size_t)3, arg.columnIndexes.size());
		cppcut_assert_equal((size_t)5, arg.columnIndexes[0]);
		cppcut_assert_equal((size_t)3, arg.columnIndexes[1]);
		cppcut_assert_equal((size_t)18, arg.columnIndexes[2]);
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

	void assertSelectExArgAdd(const bool &useFullName = false)
	{
		SelectExArg arg(tableProfileTest);
		if (useFullName)
			arg.useFullName = true;
		for (size_t i = 0; i < tableProfileTest.numColumns; i++)
			arg.add(i);

		// check
		cppcut_assert_equal(tableProfileTest.numColumns,
		                    arg.statements.size());
		cppcut_assert_equal(tableProfileTest.numColumns,
		                    arg.columnTypes.size());
		for (size_t i = 0; i < tableProfileTest.numColumns; i++) {
			string expectStmt;
			const ColumnDef &columnDef =
			  tableProfileTest.columnDefs[i];
			const string &actualStmt = arg.statements[i];
			if (useFullName) {
				expectStmt = columnDef.tableName;
				expectStmt += ".";
			}
			expectStmt += columnDef.columnName;
			cppcut_assert_equal(expectStmt, actualStmt);

			const SQLColumnType &actualType = arg.columnTypes[i];
			const SQLColumnType &expectType = columnDef.type;
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
		const TableProfile *tableProfiles[] = {
		  &tableProfileTest, &tableProfileTestAutoInc,
		};
		const size_t numTableProfiles =
		  sizeof(tableProfiles) / sizeof(TableProfile *);
		SelectMultiTableArg arg(tableProfiles, numTableProfiles);
		cppcut_assert_equal(tableProfiles, arg.profiles);
		cppcut_assert_equal(numTableProfiles, arg.numTables);
	}

	void assertSelectMultiTableArgSetProfile(void)
	{
		const TableProfile *tableProfiles[] = {
		  &tableProfileTest, &tableProfileTestAutoInc,
		};
		const size_t numTableProfiles =
		  sizeof(tableProfiles) / sizeof(TableProfile *);
		SelectMultiTableArg arg(tableProfiles, numTableProfiles);
		arg.setTable(1);
		cppcut_assert_equal(tableProfiles[1], arg.tableProfile);
		cppcut_assert_equal(&tableProfileTestAutoInc, arg.tableProfile);
	}

	void assertSelectMultiTableArgAdd(void)
	{
		const TableProfile *tableProfiles[] = {
		  &tableProfileTest, &tableProfileTestAutoInc,
		};
		const size_t numTableProfiles =
		  sizeof(tableProfiles) / sizeof(TableProfile *);
		SelectMultiTableArg arg(tableProfiles, numTableProfiles);

		// 1st add()
		size_t columnIdx = 2;
		arg.add(columnIdx);

		cppcut_assert_equal((size_t)1, arg.statements.size());
		cppcut_assert_equal((size_t)1, arg.columnTypes.size());

		const ColumnDef *def = &tableProfileTest.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("%s.%s",
		                       def->tableName, def->columnName),
		  arg.statements[0]);
		cppcut_assert_equal(def->type, arg.columnTypes[0]);

		// 2nd add() after the change of the profile
		arg.setTable(1);
		columnIdx = 0;
		arg.add(columnIdx);

		cppcut_assert_equal((size_t)2, arg.statements.size());
		cppcut_assert_equal((size_t)2, arg.columnTypes.size());

		def = &tableProfileTestAutoInc.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("%s.%s",
		                       def->tableName, def->columnName),
		  arg.statements[1]);
		cppcut_assert_equal(def->type, arg.columnTypes[1]);
	}

	void assertSelectMultiTableArgGetFullName(void)
	{
		const TableProfile *tableProfiles[] = {
		  &tableProfileTest, &tableProfileTestAutoInc,
		};
		const size_t numTableProfiles =
		  sizeof(tableProfiles) / sizeof(TableProfile *);
		SelectMultiTableArg arg(tableProfiles, numTableProfiles);

		const size_t columnIdx = 3;
		const string actualName = arg.getFullName(0, columnIdx);

		const ColumnDef *def = &tableProfileTest.columnDefs[columnIdx];
		cppcut_assert_equal(
		  StringUtils::sprintf("%s.%s",
		                       tableProfileTest.name, def->columnName),
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
	void assertInsertArgAdd(const T *vals, const size_t &numVals)
	{
		TableProfile tblProf("name", m_testColumnDefs,
		                     sizeof(m_testColumnDefs),
		                     m_numTestColumns);
		InsertArg arg(tblProf);
		for (size_t i = 0; i < numVals; i++) {
			if (i % 2 == 0)
				arg.add(vals[i]);
			else
				arg.add(vals[i], ITEM_DATA_NULL);
		}

		// check
		cppcut_assert_equal(numVals, arg.row->getNumberOfItems());
		for (size_t i = 0; i < numVals; i++) {
			const ItemData *itemData = arg.row->getItemAt(i);
			const T_READ actual = *itemData;
			cppcut_assert_equal(vals[i], static_cast<T>(actual));
			const bool expectedNull = (i % 2 == 1);
			cppcut_assert_equal(expectedNull, itemData->isNull());
			cppcut_assert_equal(1, itemData->getUsedCount());
		}
	}

	template <typename T>
	void assertInsertArgAdd(const T *vals, const size_t &numVals)
	{
		assertInsertArgAdd<T,T>(vals, numVals);
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

	void assertMakeSelectStatementDistinct(const bool &distinct) 
	{
		SelectExArg arg(tableProfileTest);
		const size_t cIdx = 0;
		const ColumnDef &columnDef = tableProfileTest.columnDefs[cIdx];
		arg.add(cIdx);
		arg.useDistinct = distinct;
		string expect = StringUtils::sprintf(
		  "SELECT %s%s FROM %s",
		  distinct ? "DISTINCT " : "",
		  columnDef.columnName, tableProfileTest.name);
		cppcut_assert_equal(expect, makeSelectStatement(arg));
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
	virtual void execSql(const string &sql) {}
	virtual void createTable(const DBAgent::TableProfile &tableProfile) {}
	virtual void insert(const InsertArg &insertArg) {}
	virtual void update(const UpdateArg &updateArg) {}
	virtual void select(const SelectArg &selectArg) {}
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

	virtual void fixupIndexes(const TableProfile &tableProfile,
	                          const IndexDef *indexDefArray) override
	{
	}

	virtual string
	makeCreateIndexStatement(const IndexDef &indexDef) override
	{
		return "";
	}

	virtual string
	makeDropIndexStatement(const string &name,
	                       const string &tableName) override
	{
		return "";
	}

	virtual void getIndexInfoVect(
	  std::vector<IndexInfo> &indexInfoVect,
	  const TableProfile &tableProfile) override
	{
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

void test_selectArgAdd(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectArgAdd();
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

void test_selectExArgAddFullName(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertSelectExArgAdd(true);
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

void test_insertArgAddInt(void)
{
	TestDBAgent dbAgent;
	const int vals[] = {3, -1, 500};
	const size_t numVals = sizeof(vals) / sizeof(int);
	dbAgent.assertInsertArgAdd<int>(vals, numVals);
}

void test_insertArgAddUint64(void)
{
	TestDBAgent dbAgent;
	const uint64_t vals[] = {0, 0xfedcba987654321, 0x0123456789abcdef};
	const size_t numVals = sizeof(vals) / sizeof(uint64_t);
	dbAgent.assertInsertArgAdd<uint64_t>(vals, numVals);
}

void test_insertArgAddDouble(void)
{
	TestDBAgent dbAgent;
	const double vals[] = {0.8, -0.53432e237, 234.43243e8};
	const size_t numVals = sizeof(vals) / sizeof(double);
	dbAgent.assertInsertArgAdd<double>(vals, numVals);
}

void test_insertArgAddString(void)
{
	TestDBAgent dbAgent;
	const string vals[] = {"booo", "v.v;", "Ueno Zoo"};
	const size_t numVals = sizeof(vals) / sizeof(string);
	dbAgent.assertInsertArgAdd<string>(vals, numVals);
}

void test_insertArgAddTime_t(void)
{
	TestDBAgent dbAgent;
	const time_t vals[] = {0, 0x7fffffff, 1391563132};
	const size_t numVals = sizeof(vals) / sizeof(string);
	dbAgent.assertInsertArgAdd<time_t, int>(vals, numVals);
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

void test_makeSelectStatementDistinct(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertMakeSelectStatementDistinct(false);
	dbAgent.assertMakeSelectStatementDistinct(true);
}

} // namespace testDBAgent
