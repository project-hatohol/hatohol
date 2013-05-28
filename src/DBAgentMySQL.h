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

#ifndef DBAgentMySQL_h
#define DBAgentMySQL_h

#include <mysql.h>
#include "DBAgent.h"

class DBAgentMySQL : public DBAgent {
public:
	// constructor and destructor
	DBAgentMySQL(const char *db,            // When the parameter is NULL,
	             const char *user = NULL,   //   current user name is used
	             const char *passwd = NULL, //   passwd is not checked
	             const char *host = NULL,   //   localhost is used
	             unsigned int port = 0);    //   default port is used
	virtual ~DBAgentMySQL();

	// virtual methods
	virtual bool isTableExisting(const string &tableName);
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition);
	virtual void begin(void);
	virtual void commit(void);
	virtual void rollback(void);
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg);
	virtual void insert(DBAgentInsertArg &insertArg);
	virtual void update(DBAgentUpdateArg &updateArg);
	virtual void select(DBAgentSelectArg &selectArg);
	virtual void select(DBAgentSelectExArg &selectExArg);
	virtual void deleteRows(DBAgentDeleteArg &deleteArg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgentMySQL_h


