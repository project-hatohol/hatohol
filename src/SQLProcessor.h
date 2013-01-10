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

typedef map<const SQLTableInfo *, ItemIdVector> SQLTableInfoItemIdVectorMap;
typedef SQLTableInfoItemIdVectorMap::iterator
  SQLTableInfoItemIdVectorMapIterator;
typedef SQLTableInfoItemIdVectorMap::const_iterator
  SQLTableInfoItemIdVectorMapConstIterator;

struct SQLSelectInfo {
	// input statement
	ParsableString   query;

	// parsed matter
	vector<SQLColumnInfo> columns;
	vector<SQLTableInfo>  tables;
	// The value (const SQLTableInfo *) in the following map points
	// an instance in 'tables' in this struct.
	map<string, const SQLTableInfo *> tableMap;

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
	bool checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo &columnInfo) const;

	void addColumnDefs(SQLSelectInfo &selectInfo,
	                   const SQLTableInfo &tableInfo,
	                   const ColumnBaseDefinition &columnBaseDef);
	bool addAllColumnDefs(SQLSelectInfo &selectInfo,
	                      const SQLTableInfo &tableInfo);
	/*bool makeTable(SQLSelectInfo &selectInfo,
	               SQLTableInfo &tableInfo,
	               const ItemTablePtr itemTablePtr);*/

	// methods for foreach
	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	//struct AddItemGroupArg;
	//static bool
	//addItemGroup(const ItemGroup *imteGroup, AddItemGroupArg &arg);


	static bool
	pickupMatchingRows(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	static bool
	makeTextRows(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	//
	// Select status parsers
	//
	bool parseRegionFrom(SelectParserContext &ctx);
	bool parseRegionWhere(SelectParserContext &ctx);
	bool parseRegionOrder(SelectParserContext &ctx);
	bool parseRegionGroup(SelectParserContext &ctx);

	//
	// Select statment parsers
	//
	bool parseSelectedColumns(SelectParserContext &ctx);
	bool parseGroupBy(SelectParserContext &ctx);
	bool parseFrom(SelectParserContext &ctx);
	bool parseWhere(SelectParserContext &ctx);
	bool parseOrderBy(SelectParserContext &ctx);

	string readNextWord(SelectParserContext &ctx,
	                    ParsingPosition *position = NULL);

private:
	static const SelectSubParser m_selectSubParsers[];

	enum SelectParseRegion {
		SELECT_PARSING_REGION_SELECT,
		SELECT_PARSING_REGION_GROUP_BY,
		SELECT_PARSING_REGION_FROM,
		SELECT_PARSING_REGION_WHERE,
		SELECT_PARSING_REGION_ORDER_BY,
		NUM_SELECT_PARSING_REGION,
	};

	SeparatorChecker *m_selectSeprators[NUM_SELECT_PARSING_REGION];
	SeparatorChecker            m_separatorSpaceComma;
	SeparatorCheckerWithCounter m_separatorCountSpaceComma;

	// These members are typically allocated in sub classes.
	TableNameStaticInfoMap &m_tableNameStaticInfoMap;
	map<string, SelectSubParser> m_selectRegionParserMap;
};

#endif // SQLProcessor_h
