#ifndef SQLProcessorZabbix_h
#define SQLProcessorZabbix_h

#include <map>
using namespace std;

#include "SQLProcessor.h"

class SQLProcessorZabbix;
typedef bool (SQLProcessorZabbix::*TableProcFunc)(SQLSelectStruct &arg);

class SQLProcessorZabbix : public SQLProcessor
{
public:
	static SQLProcessor *createInstance(void);
	SQLProcessorZabbix(void);
	~SQLProcessorZabbix();

	// virtual methods
	bool select(SQLSelectResult &result,
	            string &query, vector<string> &words);

protected:
	bool tableProcNodes(SQLSelectStruct &selStruct);

private:
	map<string, TableProcFunc> m_tableProcFuncMap;
};

#endif // SQLProcessorZabbix_h

