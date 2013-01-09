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

struct ColumnBaseDefinition {
	ItemId         itemId;
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
	uint16_t       flags;
};

struct SQLColumnDefinition
{
	const ColumnBaseDefinition *baseDef;
	string schema;
	string table;
	string tableVar;
	string column;
	string columnVar;
};

typedef list<ColumnBaseDefinition>  ColumnBaseDefList;
typedef ColumnBaseDefList::iterator ColumnBaseDefListIterator;

typedef map<int, ColumnBaseDefinition *>    ItemIdColumnBaseDefRefMap;
typedef ItemIdColumnBaseDefRefMap::iterator ItemIdColumnBaseDefRefMapIterator;

struct SQLSelectInfo;
struct SQLTableInfo;
class SQLProcessor;
typedef bool
(SQLProcessor::*SQLTableMakeFunc)(SQLSelectInfo &selectInfo,
                                  SQLTableInfo &tableInfo);

struct SQLTableStaticInfo {
	int                        tableId;
	const char                *tableName;
	SQLTableMakeFunc           tableMakeFunc;
	const ColumnBaseDefList    columnBaseDefList;
	ItemIdColumnBaseDefRefMap  columnBaseDefMap;
};

struct SQLColumnInfo;
struct SQLTableInfo {
	const SQLTableStaticInfo *staticInfo;
	string name;
	string varName;
	list<const SQLColumnInfo *> columnList;

	// constructor and methods
	SQLTableInfo(void);
};

struct SQLColumnInfo {
	string name;         // ex.) tableVarName.column1
	string baseName;     // ex.) column1
	string tableVar;     // ex.) tableVarName
	const SQLTableInfo *tableInfo;

	// constructor and methods
	SQLColumnInfo(void);
	void associate(SQLTableInfo *tableInfo);
};


struct SQLSelectInfo {
	// input statement
	ParsableString   query;

	// parsed matter
	vector<SQLColumnInfo> columns;
	vector<SQLTableInfo>  tables;
	map<string, const SQLTableInfo *> tableMap;
	SQLWhereElement            *whereElem;
	vector<string>              orderedColumns;

	// list of target tables
	ItemTablePtrList itemTablePtrList;

	// unified tables
	ItemTablePtr joinedTable;

	bool useIndex;
	ItemTablePtr selectedTable;

	// output
	vector<SQLColumnDefinition> columnDefs;
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
	bool makeColumnDefs(SQLSelectInfo &selectInfo);
	bool makeItemTables(SQLSelectInfo &selectInfo);
	bool checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
	                             const SQLColumnInfo &columnInfo) const;

	void addColumnDefs(SQLSelectInfo &selectInfo,
	                   const ColumnBaseDefinition &columnBaseDef,
	                   const SQLColumnInfo &columnInfo);
	void addAllColumnDefs(SQLSelectInfo &selectInfo,
	                      const SQLTableInfo &tableInfo, int tableId);
	bool makeTable(SQLSelectInfo &selectInfo,
	               SQLTableInfo &tableInfo,
	               const ItemTablePtr itemTablePtr);

	// methods for foreach
	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectInfo &selectInfo);

	struct AddItemGroupArg;
	static bool
	addItemGroup(const ItemGroup *imteGroup, AddItemGroupArg &arg);


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
