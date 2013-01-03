#include <Logger.h>
using namespace mlpl;

#include "SQLProcessorZabbix.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLProcessor *SQLProcessorZabbix::createInstance(void)
{
	//return static_cast<SQLProcessor *>(new SQLProcessorZabbix());
	return new SQLProcessorZabbix();
}

SQLProcessorZabbix::SQLProcessorZabbix(void)
{
	MLPL_INFO("created: %s\n", __func__);

	m_tableProcFuncMap["nodes"] = &SQLProcessorZabbix::tableProcNodes;
}

SQLProcessorZabbix::~SQLProcessorZabbix()
{
}

bool SQLProcessorZabbix::select(SQLSelectResult &result,
                                string &query, vector<string> &words)
{
	SQLSelectStruct selStruct;
	if (!parseSelectStatement(selStruct, words))
		return false;

	map<string, TableProcFunc>::iterator it;
	it = m_tableProcFuncMap.find(selStruct.table);
	if (it != m_tableProcFuncMap.end())
		return false;

	TableProcFunc func = it->second;
	return (this->*func)(selStruct);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool SQLProcessorZabbix::tableProcNodes(SQLSelectStruct &selStruct)
{
	return false;
}
