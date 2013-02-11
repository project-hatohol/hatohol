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

#ifndef SQLProcessor_h
#define SQLProcessor_h

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
#include "SQLWhereParser.h"

enum SQLColumnType {
	SQL_COLUMN_TYPE_INT,
	SQL_COLUMN_TYPE_BIGUINT,
	SQL_COLUMN_TYPE_VARCHAR,
	SQL_COLUMN_TYPE_CHAR,
};

enum SQLJoinType {
	SQL_JOIN_TYPE_UNKNOWN,
	SQL_JOIN_TYPE_INNER,
	SQL_JOIN_TYPE_LEFT_OUTER,
	SQL_JOIN_TYPE_RIGHT_OUTER,
	SQL_JOIN_TYPE_FULL_OUTER,
	SQL_JOIN_TYPE_CROSS,
};

struct ColumnBaseDefinition {
	ItemId         itemId;
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
	uint16_t       flags;
};

struct SQLTableInfo;
struct SQLColumnInfo;
struct SQLOutputColumn
{
	const SQLFormulaInfo *formulaInfo;
	const SQLColumnInfo  *columnInfo; // when '*' is specified

	// When columnBaseDefDeleteFlag is false,
	// 'columnBaseDef' points an instance in
	// SQLTableStaticInfo::columnBaseDefList.
	// So we must not explicitly free it in that case.
	// When columnBaseDefDeleteFlag is true, it is created dynamically
	// We have to delete it in destructor.
	const ColumnBaseDefinition *columnBaseDef;
	bool  columnBaseDefDeleteFlag;

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

typedef list<ColumnBaseDefinition>        ColumnBaseDefList;
typedef ColumnBaseDefList::iterator       ColumnBaseDefListIterator;
typedef ColumnBaseDefList::const_iterator ColumnBaseDefListConstIterator;

typedef map<string, ColumnBaseDefinition *>    ItemNameColumnBaseDefRefMap;
typedef ItemNameColumnBaseDefRefMap::iterator
  ItemNameColumnBaseDefRefMapIterator;
typedef ItemNameColumnBaseDefRefMap::const_iterator
  ItemNameColumnBaseDefRefMapConstIterator;

struct SQLSelectInfo;
class SQLProcessor;
typedef const ItemTablePtr
(SQLProcessor::*SQLTableMakeFunc)(SQLSelectInfo &selectInfo,
                                  const SQLTableInfo &tableInfo);

struct SQLTableStaticInfo {
	int                        tableId;
	const char                *tableName;
	SQLTableMakeFunc           tableMakeFunc;
	const ColumnBaseDefList    columnBaseDefList;

	// The value (ColumnBaseDefinition *) points an instance in
	// 'columnBaseDefList' in this struct.
	// So we must not explicitly free it.
	const ItemNameColumnBaseDefRefMap columnBaseDefMap;
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

	// constructor and methods
	SQLTableInfo(void);
};

typedef list<SQLTableInfo *>         SQLTableInfoVector;
typedef SQLTableInfoVector::iterator SQLTableInfoVectorIterator;

struct SQLColumnInfo {
	string name;         // ex.) tableVarName.column1
	string baseName;     // ex.) column1
	string tableVar;     // ex.) tableVarName

	// 'tableInfo' points an instance in SQLSelectInfo::tables.
	// So we must not it free them.
	// It is set in associateColumnWithTable().
	const SQLTableInfo *tableInfo;

	// 'columnBaseDef' points an instance in
	// 'tableInfo->staticInfo->columnBaseDefList'.
	// So we must not explicitly free it.
	// It is set in setBaseDefAndColumnTypeInColumnInfo().
	// This variable is not set when 'columnType' is COLUMN_TYPE_ALL,
	// COLUMN_TYPE_ALL_OF_TABLE.
	const ColumnBaseDefinition *columnBaseDef;

	enum {COLUMN_TYPE_UNKNOWN, COLUMN_TYPE_NORMAL, COLUMN_TYPE_ALL,
	      COLUMN_TYPE_ALL_OF_TABLE};
	int columnType;

	// constructor and methods
	SQLColumnInfo(string &_name);
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

typedef map<string, const SQLTableInfo *> SQLTableVarNameInfoMap;
typedef SQLTableVarNameInfoMap::iterator  SQLTableVarNameInfoMapIterator;

struct SQLSelectInfo {
	// input statement
	ParsableString   query;

	// parsed matter (Elements in these two container have to be freed)
	SQLColumnParser         columnParser;
	SQLColumnNameMap        columnNameMap;
	SQLTableInfoVector      tables;

	// The value (const SQLTableInfo *) in the following map points
	// an instance in 'tables' in this struct.
	// Key is a table var name.
	SQLTableVarNameInfoMap tableVarInfoMap;
	SQLWhereParser         whereParser;
	vector<string>         orderedColumns;

	// definition of output Columns
	vector<SQLOutputColumn>     outputColumnVector;

	// list of obtained tables
	ItemTablePtrList itemTablePtrList;

	// unified table
	ItemTablePtr joinedTable;
	ItemTablePtr selectedTable;
	ItemTablePtr packedTable;

	// output
	vector<StringVector> textRows;

	// flags
	bool useIndex;
	size_t makeTextRowsWriteMaskCount;

	// temporay variable for selecting column
	ItemGroupPtr evalTargetItemGroup;

	// constants
	const ItemDataPtr itemFalsePtr;

	//
	// constructor and destructor
	//
	SQLSelectInfo(ParsableString &_query);
	virtual ~SQLSelectInfo();
};

class SQLProcessor
{
public:
	static void init(void);
	virtual bool select(SQLSelectInfo &selectInfo);
	virtual const char *getDBName(void) = 0;

protected:
	typedef map<string, const SQLTableStaticInfo *> TableNameStaticInfoMap;
	typedef TableNameStaticInfoMap::iterator TableNameStaticInfoMapIterator;

	struct SelectParserContext;
	typedef bool (SQLProcessor::*SelectSubParser)(SelectParserContext &ctx);

	SQLProcessor(TableNameStaticInfoMap &tableNameStaticInfoMap);
	virtual ~SQLProcessor();

	bool parseSelectStatement(SQLSelectInfo &selectInfo);
	bool checkParsedResult(const SQLSelectInfo &selectInfo) const;
	bool fixupColumnNameMap(SQLSelectInfo &selectInfo);
	bool associateColumnWithTable(SQLSelectInfo &selectInfo);
	bool associateTableWithStaticInfo(SQLSelectInfo &selectInfo);
	bool setColumnTypeAndBaseDefInColumnInfo(SQLSelectInfo &selectInfo);
	bool makeColumnDefs(SQLSelectInfo &selectInfo);
	bool enumerateNeededItemIds(SQLSelectInfo &selectInfo);
	bool makeItemTables(SQLSelectInfo &selectInfo);
	bool doJoin(SQLSelectInfo &selectInfo);
	bool selectMatchingRows(SQLSelectInfo &selectInfo);
	bool makeTextOutput(SQLSelectInfo &selectInfo);
	bool checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo &columnInfo) const;

	void addOutputColumn(SQLSelectInfo &selectInfo,
	                     const SQLColumnInfo *columnInfo,
	                     const ColumnBaseDefinition *columnBaseDef);
	void addOutputColumn(SQLSelectInfo &selectInfo,
	                     SQLFormulaInfo *formulaInfo);
	bool addOutputColumnWildcardAllTables(SQLSelectInfo &selectInfo);
	bool addOutputColumnWildcard(SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo *columnInfo);
	const ColumnBaseDefinition *
	  makeColumnBaseDefForFormula(SQLFormulaInfo *formulaInfo);

	// methods for foreach
	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	static bool pickupMatchingRows(const ItemGroup *itemGroup,
	                               SQLSelectInfo &selectInfo);
	static bool makeTextRows(const ItemGroup *itemGroup,
	                         SQLSelectInfo &selectInfo);

	//
	// Select status parsers
	//
	bool parseSectionColumn(SelectParserContext &ctx);
	bool parseSectionFrom(SelectParserContext &ctx);
	bool parseSectionWhere(SelectParserContext &ctx);
	bool parseSectionOrder(SelectParserContext &ctx);
	bool parseSectionGroup(SelectParserContext &ctx);
	bool parseSectionLimit(SelectParserContext &ctx);

	//
	// Select statement parsers
	//
	bool parseSelectedColumns(SelectParserContext &ctx);
	bool parseGroupBy(SelectParserContext &ctx);
	bool parseFrom(SelectParserContext &ctx);
	bool parseWhere(SelectParserContext &ctx);
	bool parseOrderBy(SelectParserContext &ctx);
	bool parseLimit(SelectParserContext &ctx);

	//
	// General sub routines
	//
	string readNextWord(SelectParserContext &ctx,
	                    ParsingPosition *position = NULL);
	static bool parseColumnName(const string &name,
	                            string &baseName, string &tableVar);
	static ColumnBaseDefinition *
	  getColumnBaseDefinitionFromColumnName(const SQLTableInfo *tableInfo,
                                                string &baseName);
	static const SQLTableInfo *
	  getTableInfoFromVarName(SQLSelectInfo &selectInfo, string &tableVar);
	static FormulaVariableDataGetter *
	  formulaColumnDataGetterFactory(string &name, void *priv);
	static bool getColumnItemId(SQLSelectInfo &selectInfo,
	                            string &name, ItemId &itemId);

private:
	static const SelectSubParser m_selectSubParsers[];
	static map<string, SelectSubParser> m_selectSectionParserMap;

	enum SelectParseSection {
		SELECT_PARSING_SECTION_COLUMN,
		SELECT_PARSING_SECTION_GROUP_BY,
		SELECT_PARSING_SECTION_FROM,
		SELECT_PARSING_SECTION_WHERE,
		SELECT_PARSING_SECTION_ORDER_BY,
		SELECT_PARSING_SECTION_LIMIT,
		NUM_SELECT_PARSING_SECTION,
	};

	SeparatorChecker *m_selectSeprators[NUM_SELECT_PARSING_SECTION];
	SeparatorChecker             m_separatorSpaceComma;
	SeparatorCheckerWithCounter  m_separatorCountSpaceComma;
	SeparatorCheckerWithCallback m_separatorCBForWhere;

	// These members are typically allocated in sub classes.
	TableNameStaticInfoMap &m_tableNameStaticInfoMap;
};

#endif // SQLProcessor_h
