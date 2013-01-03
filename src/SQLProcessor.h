#ifndef SQLProcessor_h
#define SQLProcessor_h

#include <string>
#include <vector>
#include <map>
#include <list>
using namespace std;

#include <glib.h>

struct SQLSelectStruct {
	vector<string> columns;
	string         table;
	string         tableVar;
	string         where;
	vector<string> orderedColumns;
};

enum SQLColumnType {
	SQL_COLUMN_TYPE_INT,
	SQL_COLUMN_TYPE_VARCHAR,
};

struct ColumnBaseDefinition {
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
};

struct SQLColumnDefinition
{
	string schema;
	string table;
	string tableVar;
	string column;
	string columnVar;
	SQLColumnType type;
	size_t columnLength;
};

struct SQLRow
{
};

struct SQLSelectResult {
	vector<SQLColumnDefinition> columnDefs;
	vector<SQLRow> rows;
};

class SQLProcessor
{
public:
	SQLProcessor(void);
	virtual ~SQLProcessor();
	virtual bool select(SQLSelectResult &result,
	                    string &query, vector<string> &words) = 0;

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

	bool parseSelectStatement(SQLSelectStruct &selectStruct,
	                          vector<string> &words);
	bool selectedAllColumns(SQLSelectStruct &selectStruct);

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

private:
	static const SelectSubParser m_selectSubParsers[];
	map<string, SelectSubParser> m_selectRegionParserMap;
};

#endif // SQLProcessor_h
