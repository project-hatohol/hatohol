#ifndef SQLProcessor_h
#define SQLProcessor_h

#include <string>
#include <vector>
using namespace std;

#include <glib.h>

struct SQLSelectStruct {
	vector<string> selectRows;
	string table;
	string tableVar;
	string where;
	vector<string> orderBy;
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
	bool parseSelectStatement(SQLSelectStruct &selectStruct,
	                          vector<string> &words);

private:
};

#endif // SQLProcessor_h
