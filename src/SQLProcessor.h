#ifndef SQLProcessor_h
#define SQLProcessor_h

#include <string>
#include <vector>
#include <map>
using namespace std;

#include <glib.h>

struct SQLSelectStruct {
	vector<string> selectedColumns;
	string         table;
	string         tableVar;
	string         where;
	vector<string> orderedColumns;
};

struct SQLSelectResult {
};

class SQLProcessor
{
public:
	SQLProcessor(void);
	virtual ~SQLProcessor();
	virtual bool select(SQLSelectResult &result,
	                    string &query, vector<string> &words) = 0;

protected:
	struct SelectParserContext;
	typedef bool (SQLProcessor::*SelectSubParser)(SelectParserContext &ctx);

	bool parseSelectStatement(SQLSelectStruct &selectStruct,
	                          vector<string> &words);

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
