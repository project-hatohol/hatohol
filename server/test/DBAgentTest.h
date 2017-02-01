/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#pragma once
#include <cppcutter.h>
#include "SQLProcessorTypes.h"
#include "DBAgent.h"

// TODO: Replace these with tableProfileTest
extern const char *TABLE_NAME_TEST;
extern const ColumnDef COLUMN_DEF_TEST[];
extern const size_t NUM_COLUMNS_TEST;

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_AGE,
	IDX_TEST_TABLE_NAME,
	IDX_TEST_TABLE_HEIGHT,
	IDX_TEST_TABLE_TIME,
	NUM_IDX_TEST_TABLE,
};

extern const DBAgent::TableProfile tableProfileTest;
extern const DBAgent::TableProfile tableProfileTestAutoInc;

extern const size_t NUM_TEST_DATA;
extern const uint64_t ID[];
extern const int AGE[];
extern const char *NAME[];
extern const double HEIGHT[];
extern const int    TIME[];

extern const int MAX_ALLOWD_CURR_TIME_ERROR;

class DBAgentChecker {
public:
	virtual void assertTable(
	  DBAgent &dbAgent, const DBAgent::TableProfile &tableProfile) = 0;

	// create and drop of index
	virtual void assertMakeCreateIndexStatement(
	  const std::string sql, const DBAgent::TableProfile &tableProfile,
	  const DBAgent::IndexDef &indexDef) = 0;
	virtual void assertMakeDropIndexStatement(
	  const std::string sql,
	  const std::string &name, const std::string &tableName) = 0;
	virtual void assertFixupIndexes(
	  DBAgent &dbAgent, const DBAgent::TableProfile &tableProfile) = 0;

	virtual void assertExistingRecord(
	               DBAgent &dbAgent,
	               uint64_t id, int age, const char *name, double height,
	               int datetime, size_t numColumns,
	               const ColumnDef *columnDefs,
	               const std::set<size_t> *nullIndexes = NULL) = 0;
	virtual void getIDStringVector(
	  DBAgent &dbAgent, const DBAgent::TableProfile &tableProfile,
	  const size_t &columnIdIdx, std::vector<std::string> &actualIds) = 0;

	static void createTable(DBAgent &dbAgent);
	static void insert(DBAgent &dbAgent, uint64_t id, int age,
	                   const char *name, double height, int time);
	static void makeTestData(DBAgent &dbAgent);
	static void makeTestData(
	  DBAgent &dbAgent, std::map<uint64_t, size_t> &testDataIdIndexMap);
protected:
	static void assertExistingRecordEachWord
	              (uint64_t id, int age, const char *name, double height,
	               int datetime, size_t numColumns,
	               const ColumnDef *columnDefs, const std::string &line,
	               const char splitChar,
	               const std::set<size_t> *nullIndexes,
	               const std::string &expectedNullNotation,
	               const char *U64fmt = "%" PRIu64);
};

void dbAgentTestExecSql(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestCreateTable(DBAgent &dbAgent, DBAgentChecker &checker);

void dbAgentDataMakeCreateIndexStatement(void);
void dbAgentTestMakeCreateIndexStatement(
  DBAgent &dbAgent, DBAgentChecker &checker, gconstpointer data);
void dbAgentTestMakeDropIndexStatement(
  DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestFixupIndexes(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestFixupSameNameIndexes(DBAgent &dbAgent, DBAgentChecker &checker);

void dbAgentTestInsert(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestInsertUint64
  (DBAgent &dbAgent, DBAgentChecker &checker, uint64_t id);
void dbAgentTestInsertNull(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpsert(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpsertWithPrimaryKeyAutoInc(
  DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdate(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdateBigUint(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdateCondition(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestSelect(DBAgent &dbAgent);
void dbAgentTestSelectEx(DBAgent &dbAgent);
void dbAgentTestSelectExWithCond(DBAgent &dbAgent);
void dbAgentTestSelectExWithCondAllColumns(DBAgent &dbAgent);
void dbAgentTestSelectHeightOrder
 (DBAgent &dbAgent, size_t limit = 0, size_t offset = 0,
  size_t forceExpectedRows = (size_t)-1);
void dbAgentTestDelete(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestAddColumns(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestRenameTable(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestDropTable(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestIsTableExisting(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestAutoIncrement(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestAutoIncrementWithDel(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentUpdateIfExistEleseInsert(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentGetLastInsertId(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentGetNumberOfAffectedRows(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentUpsertBySameData(DBAgent &dbAgent, DBAgentChecker &checker);

