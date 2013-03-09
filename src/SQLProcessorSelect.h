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

#ifndef SQLProcessorSelect_h
#define SQLProcessorSelect_h

#include <string>
#include <vector>
#include <map>
#include <list>
using namespace std;

#include "StringUtils.h"
#include "ParsableString.h"
using namespace mlpl;

#include <glib.h>
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

	string schema;
	string table;
	string tableVar;
	string column;
	string columnVar;

	// constructor and methods
	SQLOutputColumn(SQLFormulaInfo *_formulaInfo);
	SQLOutputColumn(const SQLColumnInfo *_columnInfo);
	~SQLOutputColumn();
	ItemDataPtr getItem(const ItemGroup *itemGroup) const;
};

struct SQLColumnInfo;
struct SQLTableInfo {
	string name;
	string varName;

	// A pointer in 'columnList' points an instance in 
	// SelectInfo::columns in the SQLSelectInfo instance including this.
	// It is added in associateColumnWithTable().
	// So we must not explicitly free it.
	list<const SQLColumnInfo *> columnList;

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

typedef list<SQLTableInfo *>         SQLTableInfoList;
typedef SQLTableInfoList::iterator   SQLTableInfoListIterator;

struct SQLColumnInfo {
	string name;         // ex.) tableVarName.column1
	string baseName;     // ex.) column1
	string tableVar;     // ex.) tableVarName

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
	SQLColumnInfo(const string &_name);
	void associate(SQLTableInfo *tableInfo);
	void setColumnType(void);
};

typedef map<string, SQLColumnInfo *>    SQLColumnNameMap;
typedef SQLColumnNameMap::iterator      SQLColumnNameMapIterator;

typedef map<const SQLTableInfo *, ItemIdVector> SQLTableInfoItemIdVectorMap;
typedef SQLTableInfoItemIdVectorMap::iterator
  SQLTableInfoItemIdVectorMapIterator;
typedef SQLTableInfoItemIdVectorMap::const_iterator
  SQLTableInfoItemIdVectorMapConstIterator;

typedef map<string, SQLTableInfo *>       SQLTableVarNameInfoMap;
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
	SQLTableVarNameInfoMap tableVarInfoMap;
	SQLWhereParser         whereParser;
	vector<string>         orderedColumns;

	// definition of output Columns
	vector<SQLOutputColumn>     outputColumnVector;

	// unified table
	ItemTablePtr joinedTable;
	ItemTablePtr selectedTable;

	// grouped tables
	ItemTablePtrList groupedTables;

	// output
	vector<StringVector> textRows;

	// flags
	bool useIndex;

	// constants
	const ItemDataPtr itemFalsePtr;

	//
	// constructor and destructor
	//
	SQLSelectInfo(ParsableString &_statement);
	virtual ~SQLSelectInfo();
};

class SQLProcessorSelect
{
public:
	static void init(void);
	SQLProcessorSelect(const string &dbName,
	                   TableNameStaticInfoMap &tableNameStaticInfoMap);
	SQLProcessorSelect(const SQLProcessorSelect *parent);
	virtual ~SQLProcessorSelect();
	virtual bool select(SQLSelectInfo &selectInfo);

protected:
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
	string readNextWord(ParsingPosition *position = NULL);
	static void parseColumnName(const string &name,
	                            string &baseName, string &tableVar);
	SQLTableInfo *
	  getTableInfoFromColumnInfo(SQLColumnInfo *columnInfo) const;
	SQLTableInfo *
	  getTableInfoWithScanTables(SQLColumnInfo *columnInfo) const;
	SQLTableInfo *
	  getTableInfoFromVarName(const string &tableVar) const;
	static FormulaVariableDataGetter *
	  formulaColumnDataGetterFactory(const string &name, void *priv);
	void makeGroupedTableForColumn(const string &columnName);
	bool checkSectionParserChange(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // SQLProcessorSelect_h
