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

#include <cstring>
#include "SQLUtils.h"
#include "SQLProcessorTypes.h"
#include "SQLProcessorException.h"
#include "HatoholException.h"
using namespace std;
using namespace mlpl;

SQLUtils::ItemDataCreator SQLUtils::m_itemDataCreators[] =
{
	&SQLUtils::creatorItemInt,
	&SQLUtils::creatorItemBiguint,
	&SQLUtils::creatorVarchar,
	&SQLUtils::creatorChar,
	&SQLUtils::creatorVarchar, // SQL_COLUMN_TYPE_TEXT
	&SQLUtils::creatorDouble,
	&SQLUtils::creatorDatetime
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
		THROW_HATOHOL_EXCEPTION(
		  "The number of m_itemDataCreator is wrong: "
		  "expected/acutual: %d/%zd",
		  NUM_SQL_COLUMN_TYPES, m_numItemDataCreators);
	}
}

string SQLUtils::getFullName(const ColumnDef *columnDefs, const size_t &index)
{
	const ColumnDef &def = columnDefs[index];
	string fullName = def.tableName;
	fullName += ".";
	fullName += def.columnName;
	return fullName;
}

const ColumnAccessInfo &
SQLUtils::getColumnAccessInfo(const string &columnName,
                              const SQLTableStaticInfo *tableStaticInfo)
{
	ColumnNameAccessInfoMapConstIterator it =
	  tableStaticInfo->columnAccessInfoMap.find(columnName);
	if (it == tableStaticInfo->columnAccessInfoMap.end()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found: column: %s from table: %s",
		  columnName.c_str(), tableStaticInfo->tableName);
	}
	const ColumnAccessInfo &accessInfo = it->second;
	return accessInfo;
}

int SQLUtils::getColumnIndex(const string &columnName,
                             const SQLTableStaticInfo *tableStaticInfo)
{
	const ColumnAccessInfo &accessInfo =
	  getColumnAccessInfo(columnName, tableStaticInfo);
	return accessInfo.index;
}

const ColumnDef * SQLUtils::getColumnDef
  (const string &columnName, const SQLTableStaticInfo *tableStaticInfo)
{
	const ColumnAccessInfo &accessInfo =
	  getColumnAccessInfo(columnName, tableStaticInfo);
	return accessInfo.columnDef;
}

ItemDataPtr SQLUtils::createDefaultItemData(const ColumnDef *columnDef)
{
	string msg;
	TRMSG(msg, "Not implemented: %s\n", __PRETTY_FUNCTION__);
	throw HatoholException(msg);
	return ItemDataPtr();
}

ItemDataPtr SQLUtils::createItemData(const ColumnDef *columnDef,
                                     const string &value)
{
	if (columnDef->type >= NUM_SQL_COLUMN_TYPES) {
		THROW_HATOHOL_EXCEPTION(
		  "columnDef->type: illegal value: %d, table: %s, column: %s",
		  columnDef->type, columnDef->tableName, columnDef->columnName);
	}
	return (*m_itemDataCreators[columnDef->type])(columnDef, value.c_str());
}

ItemDataPtr SQLUtils::getItemDataFromItemGroupWithColumnName
  (const string &columnName, const SQLTableStaticInfo *tableStaticInfo,
   const ItemGroup *itemGroup)
{
	const ColumnDef *columnDef = getColumnDef(columnName, tableStaticInfo);
	if (!columnDef)
		return ItemDataPtr();
	ItemId itemId = columnDef->itemId;
	ItemDataPtr dataPtr = itemGroup->getItem(itemId);
	if (!dataPtr) {
		MLPL_DBG("Not found: item: %s (%" PRIu_ITEM "), "
		         "table: %s\n",
		         columnName.c_str(), itemId,
		         tableStaticInfo->tableName);
		return ItemDataPtr();
	}
	return dataPtr;
}

void SQLUtils::decomposeTableAndColumn(const string &fieldName,
                                       string &tableName, string &columnName,
                                       bool allowNoTableName)
{
	size_t dotPosition = fieldName.find(".");
	if (dotPosition == string::npos) {
		if (allowNoTableName) {
			columnName = fieldName;
			return;
		}
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "'dot' is not found in join field: %s", fieldName.c_str());
	}
	if (dotPosition == 0 || dotPosition == fieldName.size() - 1) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "The position of 'dot' is invalid: %s (%zd)",
		  fieldName.c_str(), dotPosition);
	}
	tableName = string(fieldName, 0, dotPosition);
	columnName = string(fieldName, dotPosition + 1);
}

ItemDataPtr SQLUtils::createFromString(const char *str, SQLColumnType type)
{
	ItemData *itemData = NULL;
	bool strIsNull = false;
	if (!str)
		strIsNull = true;

	switch(type) {
	case SQL_COLUMN_TYPE_INT:
		if (strIsNull)
			itemData = new ItemInt(0, ITEM_DATA_NULL);
		else
			itemData = new ItemInt(atoi(str));
		break;
	case SQL_COLUMN_TYPE_BIGUINT:
		if (strIsNull) {
			itemData = new ItemUint64(0, ITEM_DATA_NULL);
		} else {
			uint64_t val;
			sscanf(str, "%" PRIu64, &val);
			itemData = new ItemUint64(val);
		}
		break;
	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
		if (strIsNull)
			itemData = new ItemString("", ITEM_DATA_NULL);
		else
			itemData = new ItemString(str);
		break;
	case SQL_COLUMN_TYPE_DOUBLE:
		if (strIsNull)
			itemData = new ItemDouble(0, ITEM_DATA_NULL);
		else
			itemData = new ItemDouble(atof(str));
		break;
	case SQL_COLUMN_TYPE_DATETIME:
	{ // brace is needed to avoid the error: jump to case label
		if (strIsNull) {
			itemData = new ItemInt(0, ITEM_DATA_NULL);
			break;
		}

		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		int numVal = sscanf(str,
		                    "%04d-%02d-%02d %02d:%02d:%02d",
		                    &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		                    &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
		static const int EXPECT_NUM_VAL = 6;
		if (numVal != EXPECT_NUM_VAL) {
			MLPL_WARN(
			  "Probably, parse of the time failed: %d, %s\n",
			  numVal, str);
		}
		tm.tm_year -= 1900;
		tm.tm_mon--; // tm_mon is counted from 0 in POSIX time APIs.
		time_t time = mktime(&tm);
		itemData = new ItemInt((int)time);
		break;
	}
	case NUM_SQL_COLUMN_TYPES:
	default:
		THROW_HATOHOL_EXCEPTION("Unknown column type: %d\n", type);
	}
	return itemData;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemDataPtr SQLUtils::creatorItemInt(const ColumnDef *columnDef,
                                     const char *value)
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
	ItemId itemId = columnDef->itemId;
	return ItemDataPtr(new ItemInt(itemId, atoi(value)), false);
}

ItemDataPtr SQLUtils::creatorItemBiguint(const ColumnDef *columnDef,
                                         const char *value)
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
	if (sscanf(value, "%" PRIu64, &valUint64) != 1) {
		MLPL_DBG("Not number: %s\n", value);
		return ItemDataPtr();
	}
	ItemId itemId = columnDef->itemId;
	return ItemDataPtr(new ItemUint64(itemId, valUint64), false);
}

ItemDataPtr SQLUtils::creatorVarchar(const ColumnDef *columnDef,
                                     const char *value)
{
	ItemId itemId = columnDef->itemId;
	return ItemDataPtr(new ItemString(itemId, value), false);
}

ItemDataPtr SQLUtils::creatorChar(const ColumnDef *columnDef,
                                  const char *value)
{
	ItemId itemId = columnDef->itemId;
	return ItemDataPtr(new ItemString(itemId, value), false);
}

ItemDataPtr SQLUtils::creatorDouble(const ColumnDef *columnDef,
                                    const char *value)
{
	bool isFloat;
	if (!StringUtils::isNumber(value, &isFloat)) {
		MLPL_DBG("Not number: %s\n", value);
		return ItemDataPtr();
	}
	ItemId itemId = columnDef->itemId;
	return ItemDataPtr(new ItemDouble(itemId, atof(value)), false);
}

ItemDataPtr SQLUtils::creatorDatetime(const ColumnDef *columnDef,
                                      const char *value)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// ItemDataCaster
// ---------------------------------------------------------------------------
const ItemInt *ItemDataCaster<SQL_COLUMN_TYPE_INT>::cast(const ItemData *item)
{
	return ItemDataCasterBase<SQL_COLUMN_TYPE_INT>::cast<ItemInt>(item);
}

const ItemUint64 *
ItemDataCaster<SQL_COLUMN_TYPE_BIGUINT>::cast(const ItemData *item)
{
	return ItemDataCasterBase<SQL_COLUMN_TYPE_BIGUINT>::cast<ItemUint64>(item);
}

const ItemString *
ItemDataCaster<SQL_COLUMN_TYPE_VARCHAR>::cast(const ItemData *item)
{
	return ItemDataCasterBase<SQL_COLUMN_TYPE_VARCHAR>::cast<ItemString>(item);
}
