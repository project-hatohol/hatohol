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

#ifndef SQLUtils_h
#define SQLUtils_h

#include "ItemDataPtr.h"
#include "SQLProcessorTypes.h"

class SQLUtils {
public:
	static const int COLUMN_NOT_FOUND = -1;

	static void init(void);

	static ItemDataPtr createFromString(const char *str,
	                                    SQLColumnType type);

protected:
	typedef ItemDataPtr (*ItemDataCreator)
	  (const ColumnDef *columnDef, const char *value);
	
	static ItemDataPtr creatorItemInt
	  (const ColumnDef *columnDef, const char *value);
	static ItemDataPtr creatorItemBiguint
	  (const ColumnDef *columnDef, const char *value);
	static ItemDataPtr creatorVarchar
	  (const ColumnDef *columnDef, const char *value);
	static ItemDataPtr creatorChar
	  (const ColumnDef *columnDef, const char *value);
	static ItemDataPtr creatorDouble
	  (const ColumnDef *columnDef, const char *value);
	static ItemDataPtr creatorDatetime
	  (const ColumnDef *columnDef, const char *value);

private:
	static ItemDataCreator m_itemDataCreators[];
	static size_t          m_numItemDataCreators;
};

#endif // SQLUtils_h

