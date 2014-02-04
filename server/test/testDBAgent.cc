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
#include "DBAgent.h"

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

	template <typename T>
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
			const T actual = *elem->dataPtr;
			cppcut_assert_equal(i, elem->columnIndex);
			cppcut_assert_equal(vals[i], actual);
		}
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
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg) {}
	virtual void insert(DBAgentInsertArg &insertArg) {}
	virtual void update(DBAgentUpdateArg &updateArg) {}
	virtual void update(const UpdateArg &updateArg) {}
	virtual void select(DBAgentSelectArg &selectArg) {}
	virtual void select(DBAgentSelectExArg &selectExArg) {}
	virtual void deleteRows(DBAgentDeleteArg &deleteArg) {}
	virtual void addColumns(DBAgentAddColumnsArg &addColumnsArg) {}

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

void test_createUpdateArg(void)
{
	TestDBAgent dbAgent;
	dbAgent.assertCreateUpdateArg();
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

} // namespace testDBAgent
