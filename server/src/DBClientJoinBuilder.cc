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
	addTableCommon(table, type, index0, index1);
	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  "%s=%s",
	  SQLUtils::getFullName(table0.columnDefs, index0).c_str(),
	  SQLUtils::getFullName(table.columnDefs, index1).c_str());
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const size_t &index0, const size_t &index1,
  const size_t &index2, const size_t &index3)
{
	const DBAgent::TableProfile &table0 = *m_ctx->selectExArg.tableProfile;
	HATOHOL_ASSERT(
	  index2 < table0.numColumns,
	  "Invalid column index2: %zd (%zd)", index2, table0.numColumns);
	HATOHOL_ASSERT(
	  index3 < table.numColumns,
	  "Invalid column index3: %zd (%zd)", index3, table.numColumns);

	addTableCommon(table, type, index0, index1);
	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  "(%s=%s AND %s=%s)",
	  SQLUtils::getFullName(table0.columnDefs, index0).c_str(),
	  SQLUtils::getFullName(table.columnDefs,  index1).c_str(),
	  SQLUtils::getFullName(table0.columnDefs, index2).c_str(),
	  SQLUtils::getFullName(table.columnDefs,  index3).c_str());
}

void DBClientJoinBuilder::add(const size_t &columnIndex)
{
	m_ctx->selectExArg.add(columnIndex);
}

const DBAgent::SelectExArg &DBClientJoinBuilder::getSelectExArg(void)
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
  const size_t &index0, const size_t &index1)
{
	const DBAgent::TableProfile &table0 = *m_ctx->selectExArg.tableProfile;
	HATOHOL_ASSERT(
	  index0 < table0.numColumns,
	  "Invalid column index0: %zd (%zd)", index0, table0.numColumns);
	HATOHOL_ASSERT(
	  index1 < table.numColumns,
	  "Invalid column index1: %zd (%zd)", index1, table.numColumns);
	const char *joinOperatorString = getJoinOperatorString(type);

	m_ctx->selectExArg.tableProfile = &table;
	m_ctx->tables.push_back(&table);

	m_ctx->selectExArg.tableField += StringUtils::sprintf(
	  " %s %s ON ", joinOperatorString, table.name);
}
