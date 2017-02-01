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
#include "DBAgent.h"
#include "DataQueryOption.h"

class DBClientJoinBuilder {
public:
	enum JoinType {
		INNER_JOIN,
		LEFT_JOIN,
		RIGHT_JOIN,
		NUM_TYPE_JOIN,
	};

	/**
	 * Constructor.
	 *
	 * @param table A tableProfile for the most left table.
	 * @param option A DataQueryOption for the query.
	 */
	DBClientJoinBuilder(const DBAgent::TableProfile &table,
	                    const DataQueryOption *option = NULL);
	virtual ~DBClientJoinBuilder();

	/**
	 * Add a table for the JOIN and the make the where clause like
	 * the following .
	 *
	 * table0 JOIN table ON table0.column0=table1.column1
	 *
	 * @param table A tableProfile for the table placed at the right side.
	 * @param type A join type.
	 * @param index0 An index of the left table for the condition.
	 * @param index1 An index of the right table for the condition.
	 */
	void addTable(const DBAgent::TableProfile &table, const JoinType &type,
	              const size_t &index0, const size_t &index1);

	/**
	 * Add a table for the JOIN and the make the where clause like
	 * the following .
	 *
	 * table0 JOIN table ON tableC.columnL=table.columnR
	 *
	 * @param table A tableProfile for the table placed at the right side.
	 * @param type A join type.
	 * @param tableC A table profile for the LHS index of the condition.
	 * @param indexL An index of the left table for the condition.
	 * @param indexR An index of the right table for the condition.
	 */
	void addTable(
	  const DBAgent::TableProfile &table, const JoinType &type,
	  const DBAgent::TableProfile &tableC, const size_t &indexL,
	  const size_t &indexR);

	/**
	 * Add a table for the JOIN and the make the where clause like
	 * the following .
	 *
	 * t0 JOIN t1 ON (L0.column0L=t1.column0R AND L1.column1L=t1.column1R)
	 *
	 * @param table A tableProfile for the table placed at the right side.
	 * @param type A join type.
	 * @param tableC0 A table profile for the 1st LHS index.
	 * @param index0L An index of the LHS of the 1st comparison.
	 * @param index0R An index of the RHS of the 1st comparison.
	 * @param tableC1 A table profile for the 1st LHS index.
	 * @param index1L An index of the LHS of the 2nd comparison.
	 * @param index1R An index of the RHS of the 2nd comparison.
	 */
	void addTable(
	  const DBAgent::TableProfile &table, const JoinType &type,
	  const DBAgent::TableProfile &tableC0, const size_t &index0L,
	  const size_t &index0R,
	  const DBAgent::TableProfile &tableC1, const size_t &index1L,
	  const size_t &index1R);
	/**
	 * Add a column.
	 *
	 * @param columnIndex
	 * A column index of the table that is passed most recently in
	 * either the constructor or addTable().
	 */
	void add(const size_t &columnIndex);

	DBAgent::SelectExArg &getSelectExArg(void);
	DBAgent::SelectExArg &build(void);

protected:
	static const char *getJoinOperatorString(const JoinType &type);
	void addTableCommon(
	  const DBAgent::TableProfile &table, const JoinType &type,
	  const DBAgent::TableProfile &tableC0, const size_t &index0L,
	  const size_t &index0R);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

