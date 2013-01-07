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
#include "ItemData.h"
#include "SQLWhereElement.h"

struct SQLSelectInfo {
	// input information
	ParsableString   query;

	// parsed matter
	vector<string>   columns;
	string           table;
	string           tableVar;
	SQLWhereElement *whereElem;
	vector<string>   orderedColumns;

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
	vector<string> rows;
	bool useIndex;

	// constructor
	SQLSelectResult(void);
};

class SQLProcessor
{
public:
	SQLProcessor(void);
	virtual ~SQLProcessor();
	virtual bool select(SQLSelectResult &result,
	                    SQLSelectInfo   &selectInfo) = 0;

protected:
	typedef list<ColumnBaseDefinition>  ColumnBaseDefList;
	typedef ColumnBaseDefList::iterator ColumnBaseDefListIterator;
	typedef map<int, ColumnBaseDefList> TableIdColumnBaseDefListMap;
	typedef TableIdColumnBaseDefListMap::iterator
	                                    TableIdColumnBaseDefListMapIterator;
	typedef map<int, const char *>      TableIdNameMap;
	typedef TableIdNameMap::iterator    TableIdNameMapIterator;

	struct SelectParserContext;
	typedef bool (SQLProcessor::*SelectSubParser)(SelectParserContext &ctx);

	bool parseSelectStatement(SQLSelectInfo &selectInfo);
	bool selectedAllColumns(SQLSelectInfo &selectInfo);

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
	map<string, SelectSubParser> m_selectRegionParserMap;
};

#endif // SQLProcessor_h
