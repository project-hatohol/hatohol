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

	/**
	 * get an aceess informatin of the column in the table.
	 *
	 * @columnName A target column name.
	 * @columnDef A pointer of ColumnDef object.
	 * @tableStaticInfo A pointer of SQLTableStaticInfo object for
	 *                  the table to which the column belongs.
	 * @return A referece of ColumnAcessInfo of the column in
	 *         the table on success.
	 *         When an error, SQLProcessorException is thrown.
	 */
	static const ColumnAccessInfo &getColumnAccessInfo
	  (const string &columnName, const SQLTableStaticInfo *tableStaticInfo);

	/**
	 * get an index of the column in the table.
	 *
	 * @columnName A target column name.
	 * @columnDef A pointer of ColumnDef object.
	 * @tableStaticInfo A pointer of SQLTableStaticInfo object for
	 *                  the table to which the column belongs.
	 * @return An index of the column in the table on success.
	 *         When an error, SQLProcessorException is thrown.
	 */
	static int getColumnIndex(const string &columnName,
	                          const SQLTableStaticInfo *staticInfo);

	static ItemDataPtr createDefaultItemData(const ColumnDef *columnDef);

	/**
	 * get a pointer of ColumnDef form the column name.
	 *
	 * @columnName A target column name.
	 * @tableStaticInfo A pointer of SQLTableStaticInfo object for
	 *                  the table to which the column belongs.
	 * @return A pointer of ColumnDef is returned on success.
	 *         When an error, SQLProcessorException is thrown.
	 */
	static const ColumnDef *
	  getColumnDef(const string &columnName,
	               const SQLTableStaticInfo *tableStaticInfo);

	/**
	 * create ItemDataPtr from a string.
	 *
	 * @columnDef A pointer of ColumnDef object.
	 * @value A string of the value.
	 * @return An ItemDataPtr that refers an ItemData object on success.
	 *         When an error, the returned ItemDataPtr doesn't
	 *         have a reference (i.e. hasData() returns falase).
	 */
	static ItemDataPtr createItemData(const ColumnDef *columnDef,
	                                  string &value);

	/**
	 * get ItemDataPtr form an ItemGroup with a column name.
	 *
	 * @columnName A columnName of the target.
	 * @tableStaticInfo A pointer of SQLTableStaticInfo object for
	 *                  the table to which the column belongs.
	 * @return An ItemDataPtr that refers an ItemData object on success.
	 *         When an error, the returned ItemDataPtr doesn't
	 *         have a reference (i.e. hasData() returns falase).
	 */
	static ItemDataPtr
	  getItemDataFromItemGroupWithColumnName
	    (string &columnName, const SQLTableStaticInfo *tableStaticInfo,
	     const ItemGroup *itemGroup);

	static void decomposeTableAndColumn(const string &fieldName,
	                                    string &tableName,
	                                    string &columnName,
	                                    bool allowNoTableName = false);
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

// NOTE (TODO):
//
// We were going to use ItemDataCaster for improving type safety in the code
// that get the native value from ItemData.
//
// E.g. DBClient sub-classes typically have lines like below.
// 
//    arg.pushColumn(COLUMN_DEF_USERS[IDX_USERS_ID]);
//    ...
//    select(arg); 
//    ...
//    DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemUserId);
//
// A developper has to choose the type such as 'ItemInt' in DEFINE_AND_ASSERT().
// This may make a wrong choice. In principle, the type can be selected
// automatically, because COLUMN_DEF_USERS is statically defined and
// COLUMN_DEF_USERS[IDX_USERS_ID].type is fixed at a build time.
//
// However, C++99 (03) only accept a constant-expression as a template
// parameter. We cannot use ItemDatCaster for the purpose like
//
//    ItemDataCaster<COLUMN_DEF_USERS[IDX_USERS_ID].type>::cast().
//
// 'constexpr', one of the new features of C++11, will resolve this problem.
// So we will use the following classes when our target platforms support
// the feature. As for g++, it supports constexpr since 4.6 according to
// http://gcc.gnu.org/projects/cxx0x.html.

template<SQLColumnType type> 
struct ItemDataCasterBase {
	template<typename ITEM_TYPE>
	static const ITEM_TYPE *cast(const ItemData *item)
	{
		const ITEM_TYPE *downObj =
		  dynamic_cast<const ITEM_TYPE *>(item);
		HATOHOL_ASSERT(downObj != NULL,
		               "Failed to dynamic cast: %s -> %s",
		               DEMANGLED_TYPE_NAME(*item),
		               DEMANGLED_TYPE_NAME(ITEM_TYPE));
		return downObj;
	}
};

template <SQLColumnType type>
struct ItemDataCaster : public ItemDataCasterBase<type>
{
};

template <>
struct ItemDataCaster<SQL_COLUMN_TYPE_INT>
  : public ItemDataCasterBase<SQL_COLUMN_TYPE_INT>
{
	static const ItemInt *cast(const ItemData *item);
};

template <>
struct ItemDataCaster<SQL_COLUMN_TYPE_BIGUINT>
  : public ItemDataCasterBase<SQL_COLUMN_TYPE_BIGUINT>
{
	static const ItemUint64 *cast(const ItemData *item);
};

template <>
struct ItemDataCaster<SQL_COLUMN_TYPE_VARCHAR>
  : public ItemDataCasterBase<SQL_COLUMN_TYPE_VARCHAR>
{
	static const ItemString *cast(const ItemData *item);
};

#endif // SQLUtils_h

