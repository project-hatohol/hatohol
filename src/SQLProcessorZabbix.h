#ifndef SQLProcessorZabbix_h
#define SQLProcessorZabbix_h

#include "SQLProcessor.h"

class SQLProcessorZabbix : public SQLProcessor
{
public:
	// static methods
	static void init(void);
	static SQLProcessor *createInstance(void);
	static const char *getDBName(void);

	// constructor and desctructor
	SQLProcessorZabbix(void);
	~SQLProcessorZabbix();

	// virtual methods
	bool select(SQLSelectResult &result,
	            string &query, vector<string> &words);

protected:
	typedef bool (SQLProcessorZabbix::*TableProcFunc)(SQLSelectResult &result, SQLSelectStruct &selectStruct);

	void addColumnDefs(SQLSelectResult &result,
	                   const ColumnBaseDefinition &columnBaseDef,
	                   SQLSelectStruct &selectStruct);
	void addAllColumnDefs(SQLSelectResult &result, int tableId,
	                      SQLSelectStruct &selectStruct);
	bool tableProcNodes(SQLSelectResult &result,
	                    SQLSelectStruct &selectStruct);

private:
	map<string, TableProcFunc> m_tableProcFuncMap;

	static TableIdColumnBaseDefListMap m_tableColumnBaseDefListMap;
	static TableIdNameMap              m_tableIdNameMap;

	static void defineTable(int tableId, const char *tableName);
	static void defineColumn(int tableId, const char *columnName,
	                         SQLColumnType, size_t columnLength);
	static const char *getTableName(int tableId);
};

#endif // SQLProcessorZabbix_h

