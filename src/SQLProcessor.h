#ifndef SQLProcessor_h
#define SQLProcessor_h

#include <string>
#include <vector>
using namespace std;

#include <glib.h>

struct SQLSelectStruct {
	vector<string> selectedColumns;
	string table;
	string tableVar;
	string where;
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
	typedef size_t (SQLProcessor::*SelectSubParser)
	  (size_t idx, size_t indexInTheStatus,
	   SQLSelectStruct &selectStruct, vector<string> &words);

	bool parseSelectStatement(SQLSelectStruct &selectStruct,
	                          vector<string> &words);
	size_t parseSelectedColumns(size_t idx, size_t indexInTheStatus,
	                            SQLSelectStruct &selectStruct,
	                            vector<string> &words);
	size_t parseGroupBy(size_t idx, size_t indexInTheStatus,
	                    SQLSelectStruct &selectStruct,
	                    vector<string> &words);
	size_t parseFrom(size_t idx, size_t indexInTheStatus,
	                 SQLSelectStruct &selectStruct,
	                 vector<string> &words);
	size_t parseWhere(size_t idx, size_t indexInTheStatus,
	                  SQLSelectStruct &selectStruct,
	                  vector<string> &words);
	size_t parseOrderBy(size_t idx, size_t indexInTheStatus,
	                    SQLSelectStruct &selectStruct,
	                    vector<string> &words);

private:
	static const SelectSubParser m_selectSubParsers[];
};

#endif // SQLProcessor_h
