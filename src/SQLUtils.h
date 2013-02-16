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

#ifndef SQLUtils_h
#define SQLUtils_h

#include "ItemDataPtr.h"
#include "SQLProcessorTypes.h"

class SQLUtils {
public:
	static void init(void);
	static ItemDataPtr
	  createDefaultItemData(const ColumnBaseDefinition *columnBaseDef);
	static ItemDataPtr
	  createItemData(const ColumnBaseDefinition *columnBaseDef,
	                 string &value);
protected:
	typedef ItemDataPtr (*ItemDataCreator)
	    (const ColumnBaseDefinition *columnBaseDef, const char *value);
	
	static ItemDataPtr creatorItemInt
	  (const ColumnBaseDefinition *columnBaseDef, const char *value);
	static ItemDataPtr creatorItemBiguint
	  (const ColumnBaseDefinition *columnBaseDef, const char *value);
	static ItemDataPtr creatorVarchar
	  (const ColumnBaseDefinition *columnBaseDef, const char *value);
	static ItemDataPtr creatorChar
	  (const ColumnBaseDefinition *columnBaseDef, const char *value);

private:
	static ItemDataCreator m_itemDataCreators[];
	static size_t          m_numItemDataCreators;
};

#endif // SQLUtils_h

