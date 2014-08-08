/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef SQLProcessorTypes_h
#define SQLProcessorTypes_h

#include <list>
#include "ItemTablePtr.h"

enum SQLColumnType {
	SQL_COLUMN_TYPE_INT,
	SQL_COLUMN_TYPE_BIGUINT,
	SQL_COLUMN_TYPE_VARCHAR,
	SQL_COLUMN_TYPE_CHAR,
	SQL_COLUMN_TYPE_TEXT,
	SQL_COLUMN_TYPE_DOUBLE,
	SQL_COLUMN_TYPE_DATETIME,
	NUM_SQL_COLUMN_TYPES,
};

enum SQLKeyType {
	SQL_KEY_NONE,
	SQL_KEY_PRI,
	SQL_KEY_UNI,
	SQL_KEY_IDX, // an index only for the column
};

enum SQLSubQueryMode {
	SQL_SUB_QUERY_NONE,
	SQL_SUB_QUERY_EXISTS,
	SQL_SUB_QUERY_NOT_EXISTS,
};

enum SQLColumnFlags {
	SQL_COLUMN_FLAG_AUTO_INC = (1 << 0),
};

struct ColumnDef {
	ItemId         itemId;
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
	size_t         decFracLength;
	bool           canBeNull;
	SQLKeyType     keyType;
	uint16_t       flags;

	// If there's no default value, 'defalutValue' is "".
	// If the default is NULL, 'defalutValue' shoud be NULL.
	const char    *defaultValue;
};

typedef std::list<ColumnDef>          ColumnDefList;
typedef ColumnDefList::iterator       ColumnDefListIterator;
typedef ColumnDefList::const_iterator ColumnDefListConstIterator;

#endif // SQLProcessorTypes_h
