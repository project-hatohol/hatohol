/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DBClient_h
#define DBClient_h

#include "DBAgent.h"

class DBClient {
public:
	DBClient(void);
	virtual ~DBClient();

protected:
	typedef void (*CreateTableInitializer)(DBAgent *, void *);

	// static methods
	static void createTable
	  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs,
	   CreateTableInitializer initializer = NULL, void *data = NULL);

	// non-static methods
	void setDBAgent(DBAgent *dbAgent);
	DBAgent *getDBAgent(void) const;

	void begin(void);
	void rollback(void);
	void commit(void);

	void insert(DBAgentInsertArg &insertArg);
	void update(DBAgentUpdateArg &updateArg);
	void select(DBAgentSelectArg &selectArg);
	void select(DBAgentSelectExArg &selectExArg);
	void deleteRows(DBAgentDeleteArg &deleteArg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClient_h
