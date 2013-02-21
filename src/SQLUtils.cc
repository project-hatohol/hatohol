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

#include "SQLUtils.h"
#include "SQLProcessorTypes.h"
#include "AsuraException.h"

SQLUtils::ItemDataCreator SQLUtils::m_itemDataCreators[] =
{
	&SQLUtils::creatorItemInt,
	&SQLUtils::creatorItemBiguint,
	&SQLUtils::creatorVarchar,
	&SQLUtils::creatorChar,
};

size_t SQLUtils::m_numItemDataCreators
  = sizeof(m_itemDataCreators)/sizeof(SQLUtils::ItemDataCreator);

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLUtils::init(void)
{
	// check
	if (m_numItemDataCreators != NUM_SQL_COLUMN_TYPES) {
		THROW_ASURA_EXCEPTION(
		  "The number of m_itemDataCreator is wrong: "
		  "expected/acutual: %zd/%zd",
		  NUM_SQL_COLUMN_TYPES, m_numItemDataCreators);
	}
}

ItemDataPtr SQLUtils::createDefaultItemData(const ColumnBaseDefinition *baseDef)
{
	string msg;
	TRMSG(msg, "Not implemented: %s\n", __PRETTY_FUNCTION__);
	throw AsuraException(msg);
	return ItemDataPtr();
}

ItemDataPtr SQLUtils::createItemData(const ColumnBaseDefinition *baseDef,
                                     string &value)
{
	if (baseDef->type >= NUM_SQL_COLUMN_TYPES) {
		THROW_ASURA_EXCEPTION(
		  "baseDef->type: illegal value: %d, table: %s, column: %s",
		  baseDef->type, baseDef->tableName, baseDef->columnName);
	}
	return (*m_itemDataCreators[baseDef->type])(baseDef, value.c_str());
}

ColumnBaseDefinition *
SQLUtils::getColumnBaseDefinition(string &columnName,
                                  const SQLTableStaticInfo *tableStaticInfo)
{
	ItemNameColumnBaseDefRefMapConstIterator it =
	  tableStaticInfo->columnBaseDefMap.find(columnName);
	if (it == tableStaticInfo->columnBaseDefMap.end()) {
		MLPL_DBG("Not found: column: %s from table: %s\n",
		         columnName.c_str(), tableStaticInfo->tableName);
		return NULL;
	}
	return it->second;
}

ItemDataPtr SQLUtils::getItemDataFromItemGroupWithColumnName
  (string &columnName, const SQLTableStaticInfo *tableStaticInfo,
   ItemGroup *itemGroup)
{
	ColumnBaseDefinition *colBaseDef =
	  getColumnBaseDefinition(columnName, tableStaticInfo);
	if (!colBaseDef)
		return ItemDataPtr();
	ItemId itemId = colBaseDef->itemId;
	ItemDataPtr dataPtr = itemGroup->getItem(itemId);
	if (!dataPtr) {
		MLPL_DBG("Not found: item: %s (%"PRIu_ITEM"), "
		         "table: %s\n",
		         columnName.c_str(), itemId,
		         tableStaticInfo->tableName);
		return ItemDataPtr();
	}
	return dataPtr;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemDataPtr SQLUtils::creatorItemInt
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	bool isFloat;
	if (!StringUtils::isNumber(value, &isFloat)) {
		MLPL_DBG("Not number: %s\n", value);
		return ItemDataPtr();
	}
	if (isFloat) {
		MLPL_WARN("Floating point is specified. "
		          "Precision may be lost: %s\n", value);
	}
	ItemId itemId = columnBaseDef->itemId;
	return ItemDataPtr(new ItemInt(itemId, atoi(value)), false);
}

ItemDataPtr SQLUtils::creatorItemBiguint
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	bool isFloat;
	if (!StringUtils::isNumber(value, &isFloat)) {
		MLPL_DBG("Not number: %s\n", value);
		return ItemDataPtr();
	}
	if (isFloat) {
		MLPL_WARN("Floating point is specified. "
		          "Precision may be lost: %s\n", value);
	}
	uint64_t valUint64;
	if (sscanf(value, "%"PRIu64, &valUint64) != 1) {
		MLPL_DBG("Not number: %s\n", value);
		return ItemDataPtr();
	}
	ItemId itemId = columnBaseDef->itemId;
	return ItemDataPtr(new ItemUint64(itemId, valUint64), false);
}

ItemDataPtr SQLUtils::creatorVarchar
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	ItemId itemId = columnBaseDef->itemId;
	return ItemDataPtr(new ItemString(itemId, value), false);
}

ItemDataPtr SQLUtils::creatorChar
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	ItemId itemId = columnBaseDef->itemId;
	return ItemDataPtr(new ItemString(itemId, value), false);
}
