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

#ifndef SQLProcessorTypes_h
#define SQLProcessorTypes_h

#include <list>
using namespace std;

#include "ParsableString.h"
using namespace mlpl;

#include "ItemTablePtr.h"

enum SQLColumnType {
	SQL_COLUMN_TYPE_INT,
	SQL_COLUMN_TYPE_BIGUINT,
	SQL_COLUMN_TYPE_VARCHAR,
	SQL_COLUMN_TYPE_CHAR,
	NUM_SQL_COLUMN_TYPES,
};

enum SQLJoinType {
	SQL_JOIN_TYPE_UNKNOWN,
	SQL_JOIN_TYPE_INNER,
	SQL_JOIN_TYPE_LEFT_OUTER,
	SQL_JOIN_TYPE_RIGHT_OUTER,
	SQL_JOIN_TYPE_FULL_OUTER,
	SQL_JOIN_TYPE_CROSS,
};

struct SQLProcessorInfo {
	// input statement
	ParsableString   statement;

	// error information
	string           errorMessage;

	//
	// constructor and destructor
	//
	SQLProcessorInfo(const ParsableString &_statement);
	virtual ~SQLProcessorInfo();
};

struct ColumnBaseDefinition {
	ItemId         itemId;
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
	uint16_t       flags;
};

typedef list<ColumnBaseDefinition>        ColumnBaseDefList;
typedef ColumnBaseDefList::iterator       ColumnBaseDefListIterator;
typedef ColumnBaseDefList::const_iterator ColumnBaseDefListConstIterator;

typedef map<string, ColumnBaseDefinition *>    ItemNameColumnBaseDefRefMap;
typedef ItemNameColumnBaseDefRefMap::iterator
  ItemNameColumnBaseDefRefMapIterator;
typedef ItemNameColumnBaseDefRefMap::const_iterator
  ItemNameColumnBaseDefRefMapConstIterator;

struct SQLSelectInfo;
struct SQLTableInfo;
class SQLProcessor;
typedef const ItemTablePtr
(SQLProcessor::*SQLTableMakeFunc)(SQLSelectInfo &selectInfo,
                                  const SQLTableInfo &tableInfo);
typedef ItemTablePtr (*SQLTableGetFunc)(void);

struct SQLTableStaticInfo {
	int                        tableId;
	const char                *tableName;
	// TODO: Consider that 'tableMakeFunc' can be removed
	//       and use tableGetFunc instead.
	SQLTableMakeFunc           tableMakeFunc;
	SQLTableGetFunc            tableGetFunc;
	const ColumnBaseDefList    columnBaseDefList;

	// The value (ColumnBaseDefinition *) points an instance in
	// 'columnBaseDefList' in this struct.
	// So we must not explicitly free it.
	const ItemNameColumnBaseDefRefMap columnBaseDefMap;
};

typedef map<string, const SQLTableStaticInfo *> TableNameStaticInfoMap;
typedef TableNameStaticInfoMap::iterator TableNameStaticInfoMapIterator;

class SQLColumnIndexResoveler {
public:
	virtual int getIndex(const string &tableName,
	                     const string &columnName) const = 0;
};

#endif // SQLProcessorTypes_h
