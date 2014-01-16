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

#ifndef SQLProcessorSelect_h
#define SQLProcessorSelect_h

#include <string>
#include <vector>
#include <map>
#include <list>
#include <glib.h>
#include "StringUtils.h"
#include "ParsableString.h"
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "FormulaElement.h"
#include "SQLColumnParser.h"
#include "SQLFromParser.h"
#include "SQLWhereParser.h"
#include "SQLProcessorTypes.h"
#include "SQLProcessorInsert.h"
#include "SQLProcessorUpdate.h"

struct SQLColumnInfo;
struct SQLTableInfo;
struct SQLOutputColumn
{
	const SQLFormulaInfo *formulaInfo;
	const SQLColumnInfo  *columnInfo; // when '*' is specified

	// When columnDefDeleteFlag is false,
	// 'columnDef' points an instance in
	// SQLTableStaticInfo::columnDefList.
	// So we must not explicitly free it in that case.
	// When columnDefDeleteFlag is true, it is created dynamically
	// We have to delete it in destructor.
	const ColumnDef *columnDef;
	bool  columnDefDeleteFlag;

	// 'tableInfo' points an instance in SQLSelectInfo::tables.
	// So we must not explicitly free it.
	const SQLTableInfo *tableInfo;

	std::string schema;
	std::string table;
	std::string tableVar;
	std::string column;
	std::string columnVar;

	// constructor and methods
	SQLOutputColumn(SQLFormulaInfo *_formulaInfo);
	SQLOutputColumn(const SQLColumnInfo *_columnInfo);
	~SQLOutputColumn();
	ItemDataPtr getItem(const ItemGroup *itemGroup) const;
	void resetStatistics(void);
};

struct SQLColumnInfo;
struct SQLTableInfo {
	std::string name;
	std::string varName;

	// A pointer in 'columnList' points an instance in 
	// SelectInfo::columns in the SQLSelectInfo instance including this.
	// It is added in associateColumnWithTable().
	// So we must not explicitly free it.
	std::list<const SQLColumnInfo *> columnList;

	// We assume that the body of 'staticInfo' that is given in
	// the constructor via m_tableNameStaticInfoMap exists
	// at least until SQLProcessor instance exists.
	// So we must not explicitly free them.
	const SQLTableStaticInfo *staticInfo;

	// The owner of the tableElement is SQLFormula in SQLFromParser.
	// So we must not explicitly free it.
	SQLTableElement *tableElement;

	// constructor and methods
	SQLTableInfo(void);
};

typedef std::list<SQLTableInfo *>    SQLTableInfoList;
typedef SQLTableInfoList::iterator   SQLTableInfoListIterator;

struct SQLColumnInfo {
	std::string name;         // ex.) tableVarName.column1
	std::string baseName;     // ex.) column1
	std::string tableVar;     // ex.) tableVarName

	// 'tableInfo' points an instance in SQLSelectInfo::tables.
	// So we must not it free them.
	// It is set in associateColumnWithTable().
	const SQLTableInfo *tableInfo;

	// 'columnDef' points an instance in
	// 'tableInfo->staticInfo->columnDefList'.
	// So we must not explicitly free it.
	// It is set in setDefAndColumnTypeInColumnInfo().
	// This variable is not set when 'columnType' is COLUMN_TYPE_ALL,
	// COLUMN_TYPE_ALL_OF_TABLE.
	const ColumnDef *columnDef;

	enum {COLUMN_TYPE_UNKNOWN, COLUMN_TYPE_NORMAL, COLUMN_TYPE_ALL,
	      COLUMN_TYPE_ALL_OF_TABLE};
	int columnType;

	// ItemGroup
	ItemGroupPtr *currTargetItemGroupAddr;

	// constructor and methods
	SQLColumnInfo(const std::string &_name);
	void associate(SQLTableInfo *tableInfo);
	void setColumnType(void);
};

typedef std::map<std::string, SQLColumnInfo *> SQLColumnNameMap;
typedef SQLColumnNameMap::iterator             SQLColumnNameMapIterator;

typedef std::map<const SQLTableInfo *, ItemIdVector>
   SQLTableInfoItemIdVectorMap;
typedef SQLTableInfoItemIdVectorMap::iterator
  SQLTableInfoItemIdVectorMapIterator;
typedef SQLTableInfoItemIdVectorMap::const_iterator
  SQLTableInfoItemIdVectorMapConstIterator;

typedef std::map<std::string, SQLTableInfo *>  SQLTableVarNameInfoMap;
typedef SQLTableVarNameInfoMap::iterator  SQLTableVarNameInfoMapIterator;

struct SQLSelectInfo : public SQLProcessorInfo {
	// parsed matter (Elements in these two container have to be freed)
	SQLColumnParser         columnParser;
	SQLColumnNameMap        columnNameMap;
	SQLTableInfoList        tables;
	SQLFromParser           fromParser;

	// The value (const SQLTableInfo *) in the following map points
	// an instance in 'tables' in this struct.
	// Key is a table var name.
	SQLTableVarNameInfoMap   tableVarInfoMap;
	SQLWhereParser           whereParser;
	std::vector<std::string> orderedColumns;

	// definition of output Columns
	std::vector<SQLOutputColumn> outputColumnVector;

	// unified table
	ItemTablePtr joinedTable;
	ItemTablePtr selectedTable;

	// grouped tables
	ItemTablePtrList groupedTables;

	// output
	std::vector<mlpl::StringVector> textRows;

	// flags
	bool useIndex;

	// constants
	const ItemDataPtr itemFalsePtr;

	//
	// constructor and destructor
	//
	SQLSelectInfo(const mlpl::ParsableString &_statement);
	virtual ~SQLSelectInfo();
};

class SQLProcessorSelect
{
public:
	static void init(void);
	SQLProcessorSelect(const std::string &dbName,
	                   TableNameStaticInfoMap &tableNameStaticInfoMap);
	SQLProcessorSelect(const SQLProcessorSelect *parent,
	                   SQLSubQueryMode subQueryMode);
	virtual ~SQLProcessorSelect();
	virtual bool select(SQLSelectInfo &selectInfo);
	
	/**
	 * Execute select operation for 'EXISTS'.
	 *
	 * @selectInfo An SQLSelectInfo instance. Similar to select(),
	 *             it is needed only to set statement for the first call.
	 *             Howerver, for the 2nd or more call, different from
	 *             select(), the instance should repeatedly be used
	 *             for the performance.
	 * @subQueryMode A mode of sub query.
	 * @return \true when at least one row is selected. Otherwise, \false
	 *         is returned.
	 */
	bool runForExists(SQLSelectInfo &selectInfo);

protected:
	void setupForSelect(SQLSelectInfo &selectInfo);
	void setSelectInfoToPrivateContext(SQLSelectInfo &selectInfo);
	void parseSelectStatement(void);
	void makeTableInfo(void);
	void checkParsedResult(void) const;
	void fixupColumnNameMap(void);
	void associateColumnWithTable(void);
	void associateTableWithStaticInfo(void);
	void setColumnTypeAndDefInColumnInfo(void);
	void makeColumnDefs(void);
	void makeItemTables(void);
	void optimizeFormula(void);
	void pickupColumnComparisons(void);
	void doJoinWithFromParser(void);
	void doJoin(void);
	void selectMatchingRows(void);
	void makeGroups(void);
	void makeTextOutputForAllGroups(void);
	void makeTextOutput(ItemTablePtr &tablePtr);
	bool checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo &columnInfo) const;

	void addOutputColumn(SQLSelectInfo *selectInfo,
	                     const SQLColumnInfo *columnInfo,
	                     const ColumnDef *columnDef,
	                     const SQLFormulaInfo *formulaInfo = NULL);
	void addOutputColumn(SQLSelectInfo *selectInfo,
	                     SQLFormulaInfo *formulaInfo);
	void addOutputColumnsOfAllTables(SQLSelectInfo *selectInfo,
	                                 const SQLColumnInfo *columnInfo);
	void addOutputColumnsOfOneTable(SQLSelectInfo *selectInfo,
	                                const SQLColumnInfo *columnInfo);
	const ColumnDef *
	  makeColumnDefForFormula(SQLFormulaInfo *formulaInfo);

	// methods for foreach
	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	static bool pickupMatchingRows(const ItemGroup *itemGroup,
	                               SQLProcessorSelect *sqlProcSelect);
	static bool makeGroupedTable(const ItemGroup *itemGroup,
	                             SQLProcessorSelect *sqlProcSelect);
	static bool makeTextRows(const ItemGroup *itemGroup,
	                         SQLProcessorSelect *sqlProcSelect);

	//
	// Select status parsers
	//
	bool parseSectionColumn(void);
	bool parseSectionFrom(void);
	bool parseSectionWhere(void);
	bool parseSectionOrder(void);
	bool parseSectionGroup(void);
	bool parseSectionLimit(void);

	//
	// Select statement sub parsers
	//
	void parseSelectedColumns(void);
	void parseGroupBy(void);
	void parseFrom(void);
	void parseWhere(void);
	void parseOrderBy(void);
	void parseLimit(void);

	//
	// General sub routines
	//
	void setup(void);
	std::string readNextWord(mlpl::ParsingPosition *position = NULL);
	static void parseColumnName(const std::string &name,
	                            std::string &baseName,
	                            std::string &tableVar);
	SQLTableInfo *
	  getTableInfoFromColumnInfo(SQLColumnInfo *columnInfo) const;
	SQLTableInfo *
	  getTableInfoWithScanTables(SQLColumnInfo *columnInfo) const;
	SQLTableInfo *
	  getTableInfoFromVarName(const std::string &tableVar) const;
	static FormulaVariableDataGetter *
	  formulaColumnDataGetterFactory(const std::string &name, void *priv);
	void makeGroupByTables(void);
	bool checkSectionParserChange(void);
	size_t getColumnIndexInJoinedTable(const std::string &columnName);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // SQLProcessorSelect_h
