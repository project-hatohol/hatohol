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

struct SQLColumnInfo {
	vector<string>   names;
	string           table;
	string           tableVar;
};

struct SQLSelectInfo {
	// input information
	ParsableString   query;

	// parsed matter
	vector<SQLColumnInfo> columnInfo;
	SQLWhereElement      *whereElem;
	vector<string>        orderedColumns;

	// constructor and destructor
	SQLSelectInfo(ParsableString &_query);
	virtual ~SQLSelectInfo();
};

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

struct SQLSelectResult {
	vector<SQLColumnDefinition> columnDefs;
	
	// list of item tables
	ItemTablePtrList itemTablePtrList;

	// list of item tables
	ItemTablePtr unifiedTable;
	ItemTablePtr selecteTable;

	// text output
	vector<string> textRows;

	bool useIndex;

	// constructor
	SQLSelectResult(void);
};

class SQLProcessor
{
public:
	virtual bool select(SQLSelectResult &result,
	                    SQLSelectInfo   &selectInfo);
	virtual const char *getDBName(void) = 0;

protected:
	typedef bool
	(SQLProcessor::*TableProcFunc)(SQLSelectResult &result,
	                               SQLSelectInfo &selectInfo,
	                               const SQLColumnInfo &columnInfo);
	typedef map<string, TableProcFunc>  TableProcFuncMap;
	typedef TableProcFuncMap::iterator  TableProcFuncIterator;;

	typedef list<ColumnBaseDefinition>  ColumnBaseDefList;
	typedef ColumnBaseDefList::iterator ColumnBaseDefListIterator;
	typedef map<int, ColumnBaseDefList> TableIdColumnBaseDefListMap;
	typedef TableIdColumnBaseDefListMap::iterator
	                                    TableIdColumnBaseDefListMapIterator;
	typedef map<int, const char *>      TableIdNameMap;
	typedef TableIdNameMap::iterator    TableIdNameMapIterator;

	struct SelectParserContext;
	typedef bool (SQLProcessor::*SelectSubParser)(SelectParserContext &ctx);

	SQLProcessor(
	  TableProcFuncMap            &tableProcFuncMap,
	  TableIdColumnBaseDefListMap &tableColumnBaseDefListMap,
	  TableIdNameMap              &tableIdNameMap);
	virtual ~SQLProcessor();

	bool parseSelectStatement(SQLSelectInfo &selectInfo);
	bool selectedAllColumns(const SQLColumnInfo &columns);

	static bool
	setSelectResult(const ItemGroup *itemGroup, SQLSelectResult &result);

	void addColumnDefs(SQLSelectResult &result,
	                   const ColumnBaseDefinition &columnBaseDef,
	                   const SQLColumnInfo &columnInfo);
	void addAllColumnDefs(SQLSelectResult &result, int tableId,
	                      const SQLColumnInfo &columnInfo);
	bool generalSelect(SQLSelectResult &result, SQLSelectInfo &selectInfo,
	                   const SQLColumnInfo &columnInfo,
	                   const ItemTable *itemTable, int tableId);

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
	static SeparatorChecker *m_selectSeprators[];
	static SeparatorChecker m_separatorSpaceComma;

	// These members are typically allocated in sub classes.
	TableProcFuncMap            &m_tableProcFuncMap;
	TableIdColumnBaseDefListMap &m_tableColumnBaseDefListMap;
	TableIdNameMap              &m_tableIdNameMap;

	map<string, SelectSubParser> m_selectRegionParserMap;
};

#endif // SQLProcessor_h
