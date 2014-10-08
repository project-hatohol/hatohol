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

#ifndef DB_h
#define DB_h

#include <memory>
#include <Mutex.h>
#include <typeinfo>
#include "Params.h"
#include "DBAgent.h"

class DB {
public:
	enum {
		IDX_TABLES_VERSION_TABLES_ID,
		IDX_TABLES_VERSION_VERSION,
		NUM_IDX_TABLES,
	};

	static const DBAgent::TableProfile &
	  getTableProfileTablesVersion(void);
	static const std::string &getAlwaysFalseCondition(void);
	static bool isAlwaysFalseCondition(const std::string &condition);

	virtual ~DB(void);
	DBAgent &getDBAgent(void);

protected:
	struct SetupContext {
		const std::type_info &dbClassType;
		DBConnectInfo connectInfo;
		bool          initialized;
		hfl::Mutex   lock;

		SetupContext(const std::type_info &dbClassType);
	};

	DB(SetupContext &setupCtx);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DB_h
