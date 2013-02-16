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
		THROW_ASURA_EXCEPTION_WITH_LOG(BUG,
		  "The number of m_itemDataCreator is wrong: "
		  "expected/acutual: %zd/%zd\n",
		  NUM_SQL_COLUMN_TYPES, m_numItemDataCreators);
	}
}

ItemDataPtr SQLUtils::createDefaultItemData(const ColumnBaseDefinition *baseDef)
{
	string msg;
	TRMSG(msg, "Not implemented: %s\n", __PRETTY_FUNCTION__);
	throw new AsuraException(msg);
	return ItemDataPtr();
}

ItemDataPtr SQLUtils::createItemData(const ColumnBaseDefinition *baseDef,
                                     string &value)
{
	if (baseDef->type >= NUM_SQL_COLUMN_TYPES) {
		THROW_ASURA_EXCEPTION_WITH_LOG(BUG,
		  "baseDef->type: illegal value: %d, table: %s, column: %s\n",
		  baseDef->type, baseDef->tableName, baseDef->columnName);
	}
	return (*m_itemDataCreators[baseDef->type])(baseDef, value.c_str());
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
	return ItemDataPtr(new ItemInt(atoi(value)), false);
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
	return ItemDataPtr(new ItemUint64(valUint64), false);
}

ItemDataPtr SQLUtils::creatorVarchar
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	return ItemDataPtr(new ItemString(value), false);
}

ItemDataPtr SQLUtils::creatorChar
  (const ColumnBaseDefinition *columnBaseDef, const char *value)
{
	return ItemDataPtr(new ItemString(value), false);
}
