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
#include "ItemTablePtr.h"
#include "SQLWhereElement.h"

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
struct SQLColumnDefinition
{
	// 'columnBaseDef' points an instance in
	// SQLTableStaticInfo::columnBaseDefList.
	// So we must not explicitly free it.
	const ColumnBaseDefinition *columnBaseDef;

	// 'tableInfo' points an instance in SQLSelectInfo::tables.
	// So we must not explicitly free it.
	const SQLTableInfo *tableInfo;

	string schema;
	string table;
	string tableVar;
	string column;
	string columnVar;

	// constructor
	SQLColumnDefinition(void);
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
                                  const SQLTableInfo &tableInfo,
                                  const ItemIdVector &itemIdVector);

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

typedef list<SQLTableInfo>         SQLTableInfoList;
typedef SQLTableInfoList::iterator SQLTableInfoListIterator;

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
	// This variable is not set when 'columnType' is COLUMN_TYPE_ALL.
	const ColumnBaseDefinition *columnBaseDef;

	enum {COLUMN_TYPE_UNKNOWN, COLUMN_TYPE_NORMAL, COLUMN_TYPE_ALL};
	int columnType;

	// constructor and methods
	SQLColumnInfo(void);
	void associate(SQLTableInfo *tableInfo);
	void setColumnType(void);
};

typedef list<SQLColumnInfo>         SQLColumnInfoList;
typedef SQLColumnInfoList::iterator SQLColumnInfoListIterator;

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

	// parsed matter
	SQLColumnInfoList columns;
	SQLTableInfoList  tables;
	// The value (const SQLTableInfo *) in the following map points
	// an instance in 'tables' in this struct.
	// Key is a table var name.
	SQLTableVarNameInfoMap tableVarInfoMap;

	// We must free 'whereElem' when no longer needed. Its destructor
	// causes the free chain of child 'whereElem' instances.
	SQLWhereElement            *whereElem;
	vector<string>              orderedColumns;

	vector<SQLColumnDefinition> columnDefs;

	// item list to be obtained
	SQLTableInfoItemIdVectorMap neededItemIdVectorMap;

	// list of obtained tables
	ItemTablePtrList itemTablePtrList;

	// unified table
	ItemTablePtr joinedTable;

	bool useIndex;
	ItemTablePtr selectedTable;

	// output
	vector<string> textRows;

	//
	// constructor and destructor
	//
	SQLSelectInfo(ParsableString &_query);
	virtual ~SQLSelectInfo();
};

class SQLProcessor
{
public:
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
	bool associateColumnWithTable(SQLSelectInfo &selectInfo);
	bool associateTableWithStaticInfo(SQLSelectInfo &selectInfo);
	bool setColumnTypeAndBaseDefInColumnInfo(SQLSelectInfo &selectInfo);
	bool makeColumnDefs(SQLSelectInfo &selectInfo);
	bool enumerateNeededItemIds(SQLSelectInfo &selectInfo);
	bool makeItemTables(SQLSelectInfo &selectInfo);
	bool doJoin(SQLSelectInfo &selectInfo);
	bool checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo &columnInfo) const;

	void addColumnDefs(SQLSelectInfo &selectInfo,
	                   const SQLTableInfo &tableInfo,
	                   const ColumnBaseDefinition &columnBaseDef);
	bool addAllColumnDefs(SQLSelectInfo &selectInfo,
	                      const SQLTableInfo &tableInfo);

	// methods for foreach
	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	static bool
	pickupMatchingRows(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	static bool
	makeTextRows(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	//
	// Select status parsers
	//
	bool parseSectionFrom(SelectParserContext &ctx);
	bool parseSectionWhere(SelectParserContext &ctx);
	bool parseSectionOrder(SelectParserContext &ctx);
	bool parseSectionGroup(SelectParserContext &ctx);
	bool parseSectionLimit(SelectParserContext &ctx);

	//
	// Select statment parsers
	//
	bool parseSelectedColumns(SelectParserContext &ctx);
	bool parseGroupBy(SelectParserContext &ctx);
	bool parseFrom(SelectParserContext &ctx);
	bool parseWhere(SelectParserContext &ctx);
	bool parseOrderBy(SelectParserContext &ctx);

	//
	// Callbacks for parsing 'where' section
	//
	static void whereCbEq(const char separator, void *arg);

	string readNextWord(SelectParserContext &ctx,
	                    ParsingPosition *position = NULL);

private:
	static const SelectSubParser m_selectSubParsers[];

	enum SelectParseSection {
		SELECT_PARSING_SECTION_SELECT,
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
	map<string, SelectSubParser> m_selectSectionParserMap;
};

#endif // SQLProcessor_h
