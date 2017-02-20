/*
 * Copyright (C) 2014 Project Hatohol
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

#include <StringUtils.h>
#include "SQLUtils.h"
#include "DBClientJoinBuilder.h"
#include "HostResourceQueryOption.h"

using namespace std;
using namespace mlpl;

static const char *JOIN_OPERATORS[] = {
	"INNER JOIN",
	"LEFT JOIN",
	"RIGHT JOIN",
};
static const size_t NUM_JOIN_OPERATORS = ARRAY_SIZE(JOIN_OPERATORS);

struct DBClientJoinBuilder::Impl {
	DBAgent::SelectExArg selectExArg;
	vector<const DBAgent::TableProfile *> tables;
	const DataQueryOption *option;

	Impl(
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
: m_impl(new Impl(table, option))
{
	if (option)
		option->setTableNameAlways();

	// TODO: consider more ligher way w/o a dynamic cast.
	const HostResourceQueryOption *hrqOption =
	  dynamic_cast<const HostResourceQueryOption *>(option);
	if (hrqOption) {
		m_impl->selectExArg.tableField
		  = hrqOption->getPrimaryTableName();
		m_impl->selectExArg.useDistinct
		  = hrqOption->isHostgroupUsed();
	} else {
		m_impl->selectExArg.tableField = table.name;
	}
	if (option)
		m_impl->selectExArg.condition = option->getCondition();
}

DBClientJoinBuilder::~DBClientJoinBuilder()
{
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const size_t &index0, const size_t &index1)
{
	const DBAgent::TableProfile &table0 = *m_impl->selectExArg.tableProfile;
	addTableCommon(table, type, table0, index0, index1);
	m_impl->selectExArg.tableField += StringUtils::sprintf(
	  "%s=%s",
	  table0.getFullColumnName(index0).c_str(),
	  table.getFullColumnName(index1).c_str());
}

void DBClientJoinBuilder::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const DBAgent::TableProfile &tableC, const size_t &indexL,
  const size_t &indexR)
{
	addTableCommon(table, type, tableC, indexL, indexR);
	m_impl->selectExArg.tableField += StringUtils::sprintf(
	  "%s=%s",
	  tableC.getFullColumnName(indexL).c_str(),
	  table.getFullColumnName(indexR).c_str());
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
	m_impl->selectExArg.tableField += StringUtils::sprintf(
	  "(%s=%s AND %s=%s)",
	  tableC0.getFullColumnName(index0L).c_str(),
	  table.getFullColumnName(index0R).c_str(),
	  tableC1.getFullColumnName(index1L).c_str(),
	  table.getFullColumnName(index1R).c_str());
}

void DBClientJoinBuilder::add(const size_t &columnIndex)
{
	m_impl->selectExArg.add(columnIndex);
}

DBAgent::SelectExArg &DBClientJoinBuilder::getSelectExArg(void)
{
	const HostResourceQueryOption *hrqOption =
	  dynamic_cast<const HostResourceQueryOption *>(m_impl->option);
	if (hrqOption) {
		string clause = hrqOption->getJoinClause();
		if (!clause.empty()) {
			m_impl->selectExArg.tableField += " ";
			m_impl->selectExArg.tableField += clause;
		}
	}
	return m_impl->selectExArg;
}

DBAgent::SelectExArg &DBClientJoinBuilder::build(void)
{
	return getSelectExArg();
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

	m_impl->selectExArg.tableProfile = &table;
	m_impl->tables.push_back(&table);

	m_impl->selectExArg.tableField += StringUtils::sprintf(
	  " %s %s ON ", joinOperatorString, table.name);
}
