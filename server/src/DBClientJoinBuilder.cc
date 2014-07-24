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

#include <StringUtils.h>
#include <SQLUtils.h>
#include "DBClientJoinBuilder.h"
#include "HostResourceQueryOption.h"

using namespace std;
using namespace mlpl;

static const char *JOIN_OPERATORS[] = {
	"INNER JOIN",
	"LEFT JOIN",
	"RIGHT JOIN",
};
static const size_t NUM_JOIN_OPERATORS = 
  sizeof(JOIN_OPERATORS) / sizeof(const char *);

struct DBClientJoinBuilder::PrivateContext {
	DBAgent::SelectExArg selectExArg;
	vector<const DBAgent::TableProfile *> tables;
	const DataQueryOption *option;

	PrivateContext(
	  const DBAgent::TableProfile &table, const DataQueryOption *_option)
	: selectExArg(table),
	  option(_option)
	{
		selectExArg.useFullName = true;
		tables.push_back(&table);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClientJoinBuilder::DBClientJoinBuilder(
  const DBAgent::TableProfile &table, const DataQueryOption *option)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(table, option);
	if (option)
		option->setTableNameAlways();

	// TODO: consider more ligher way w/o a dynamic cast.
	const HostResourceQueryOption *hrqOption =
	  dynamic_cast<const HostResourceQueryOption *>(option);
	if (hrqOption) {
		m_ctx->selectExArg.tableField = hrqOption->getFromClause();
		m_ctx->selectExArg.useDistinct = hrqOption->isHostgroupUsed();
	} else {
		m_ctx->selectExArg.tableField = table.name;
	}
	if (option)
		m_ctx->selectExArg.condition = option->getCondition();
}

DBClientJoinBuilder::~DBClientJoinBuilder()
{
	delete m_ctx;
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const size_t &index0, const size_t &index1)
{
	const DBAgent::TableProfile &table0 = *m_ctx->selectExArg.tableProfile;
	addTableCommon(table, type, table0, index0, index1);
	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  "%s=%s",
	  SQLUtils::getFullName(table0.columnDefs, index0).c_str(),
	  SQLUtils::getFullName(table.columnDefs, index1).c_str());
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const DBAgent::TableProfile &tableC, const size_t &indexL,
  const size_t &indexR)
{
	addTableCommon(table, type, tableC, indexL, indexR);
	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  "%s=%s",
	  SQLUtils::getFullName(tableC.columnDefs, indexL).c_str(),
	  SQLUtils::getFullName(table.columnDefs, indexR).c_str());
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const DBAgent::TableProfile &tableC0, const size_t &index0L,
  const size_t &index0R,
  const DBAgent::TableProfile &tableC1, const size_t &index1L,
  const size_t &index1R)
{
	HATOHOL_ASSERT(
	  index1L < tableC1.numColumns,
	  "Invalid column index1L: %zd (%zd)", index1L, tableC1.numColumns);
	HATOHOL_ASSERT(
	  index1R < table.numColumns,
	  "Invalid column index1R: %zd (%zd)", index1R, table.numColumns);

	addTableCommon(table, type, tableC0, index0L, index0R);
	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  "(%s=%s AND %s=%s)",
	  SQLUtils::getFullName(tableC0.columnDefs, index0L).c_str(),
	  SQLUtils::getFullName(table.columnDefs,   index0R).c_str(),
	  SQLUtils::getFullName(tableC1.columnDefs, index1L).c_str(),
	  SQLUtils::getFullName(table.columnDefs,   index1R).c_str());
}

void DBClientJoinBuilder::add(const size_t &columnIndex)
{
	m_ctx->selectExArg.add(columnIndex);
}

DBAgent::SelectExArg &DBClientJoinBuilder::getSelectExArg(void)
{
	return m_ctx->selectExArg;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
const char *DBClientJoinBuilder::getJoinOperatorString(const JoinType &type)
{
	HATOHOL_ASSERT(type < NUM_TYPE_JOIN, "Invalid join type: %d", type);
	HATOHOL_ASSERT(type < NUM_JOIN_OPERATORS,
	               "Invalid join type: %d", type);
	return JOIN_OPERATORS[type];
}

void DBClientJoinBuilder::addTableCommon(
  const DBAgent::TableProfile &table, const JoinType &type,
  const DBAgent::TableProfile &tableC0, const size_t &index0L,
  const size_t &index0R)
{
	HATOHOL_ASSERT(
	  index0L < tableC0.numColumns,
	  "Invalid column index0L: %zd (%zd)", index0L, tableC0.numColumns);
	HATOHOL_ASSERT(
	  index0R < table.numColumns,
	  "Invalid column index0R: %zd (%zd)", index0R, table.numColumns);
	const char *joinOperatorString = getJoinOperatorString(type);

	m_ctx->selectExArg.tableProfile = &table;
	m_ctx->tables.push_back(&table);

	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  " %s %s ON ", joinOperatorString, table.name);
}
