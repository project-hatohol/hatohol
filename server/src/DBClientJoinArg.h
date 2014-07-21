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

class DBClientJoinArg {
public:
	enum JoinType {
		INNER_JOIN,
		LEFT_JOIN,
		RIGHT_JOIN,
		NUM_TYPE_JOIN,
	};

	DBClientJoinArg(const DBAgent::TableProfile &table);
	virtual ~DBClientJoinArg();

	void addTable(const DBAgent::TableProfile &table, const JoinType &type,
	              const size_t &index0, const size_t &index1);
	void add(const size_t &index);
	const DBAgent::SelectExArg &getSelectExArg(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientJoin_h
