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

#ifndef SQLProcessorZabbix_h
#define SQLProcessorZabbix_h

#include "ItemDataPtr.h"
#include "SQLProcessor.h"
#include "VirtualDataStoreZabbix.h"

class SQLProcessorZabbix : public SQLProcessor
{
public:
	// static methods
	static void init(void);
	static SQLProcessor *createInstance(void);
	static const char *getDBNameStatic(void);

	// constructor and desctructor
	SQLProcessorZabbix(void);
	~SQLProcessorZabbix();

	// virtual methods
	const char *getDBName(void);

private:
	static TableNameStaticInfoMap m_tableNameStaticInfoMap;

	static SQLTableStaticInfo *
	defineTable(int tableId, const char *tableName,
	            SQLTableGetFunc tableGetFunc);
	static void defineColumn(SQLTableStaticInfo *staticInfo,
	                         ItemId itemId,
	                         int tableId, const char *columnName,
	                         SQLColumnType, size_t columnLength,
	                         bool canBeNull, SQLKeyType keyType,
	                         const char *defaultValue);
	static void defineColumn(SQLTableStaticInfo *staticInfo,
	                         ItemId itemId,
	                         int tableId, const char *columnName,
	                         SQLColumnType type,
	                         size_t columnLength, size_t decFracLength,
	                         bool canBeNull, SQLKeyType keyType,
	                         const char *defaultValue);
	VirtualDataStoreZabbix *m_VDSZabbix;

	template<ItemGroupId GROUP_ID>
	static ItemTablePtr tableGetFuncTemplate(void);
};

#endif // SQLProcessorZabbix_h
