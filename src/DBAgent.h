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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
using namespace std;

#include <glib.h>
#include <stdint.h>

#include "SQLProcessorTypes.h"

struct DBAgentTableCreationArg {
	string              tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
};

struct DBAgentInsertArg {
	string tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
	const ItemGroupPtr  row;
};

struct DBAgentUpdateArg {
	string tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;
	const ItemGroupPtr  row;
};

struct DBAgentSelectArg {
	string tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;

	// output
	ItemTablePtr        dataTable;
};

struct DBAgentSelectExArg {
	string tableName;
	vector<string>        statements;
	vector<SQLColumnType> columnTypes;
	string condition;
	string orderBy;
	size_t limit;
	size_t offset;

	// output
	ItemTablePtr        dataTable;

	// constructor
	DBAgentSelectExArg(void);
};

struct DBAgentDeleteArg {
	string tableName;
	string condition;
};

typedef uint32_t DBDomainId;
typedef void (*DBSetupFunc)(DBDomainId domainId, void *data);
static const DBDomainId DEFAULT_DB_DOMAIN_ID = 0;

class DBAgent {
public:
	static void addSetupFunction(DBDomainId domainId,
	                             DBSetupFunc setupFunc, void *data = NULL);

	DBAgent(DBDomainId = DEFAULT_DB_DOMAIN_ID, bool skipSetup = false);
	virtual ~DBAgent();

	// virtual methods
	virtual bool isTableExisting(const string &tableName) = 0;
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition) = 0;
	virtual void begin(void) = 0;
	virtual void commit(void) = 0;
	virtual void rollback(void) = 0;
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg) = 0;
	virtual void insert(DBAgentInsertArg &insertArg) = 0;
	virtual void update(DBAgentUpdateArg &updateArg) = 0;
	virtual void select(DBAgentSelectArg &selectArg) = 0;
	virtual void select(DBAgentSelectExArg &selectExArg) = 0;
	virtual void deleteRows(DBAgentDeleteArg &deleteArg) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgent_h
