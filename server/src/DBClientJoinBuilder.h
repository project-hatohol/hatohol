/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#ifndef DBClientJoin_h
#define DBClientJoin_h

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
	 * t0 JOIN t1 ON (t0.column0=t1.column1 AND t0.column2=t1.column3)
	 *
	 * @param table A tableProfile for the table placed at the right side.
	 * @param type A join type.
	 * @param index0 An index of the left table for the condition.
	 * @param index1 An index of the right table for the condition.
	 * @param index2 An index of the left table for the condition.
	 * @param index3 An index of the right table for the condition.
	 */
	void addTable(const DBAgent::TableProfile &table, const JoinType &type,
	              const size_t &index0, const size_t &index1,
	              const size_t &index2, const size_t &index3);
	/**
	 * Add a column.
	 *
	 * @param columnIndex
	 * A column index of the table that is passed most recently in
	 * either the constructor or addTable().
	 */
	void add(const size_t &columnIndex);

	const DBAgent::SelectExArg &getSelectExArg(void);

protected:
	static const char *getJoinOperatorString(const JoinType &type);
	void addTableCommon(
	  const DBAgent::TableProfile &table, const JoinType &type,
	  const size_t &index0, const size_t &index1);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientJoin_h
