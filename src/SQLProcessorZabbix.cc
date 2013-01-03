#include <Logger.h>
using namespace mlpl;

#include "SQLProcessorZabbix.h"

enum TableID {
	TABLE_ID_NODES,
};

static const char *TABLE_NAME_NODES = "nodes";

map<string, SQLProcessorZabbix::TableProcFunc>
  SQLProcessorZabbix::m_tableProcFuncMap;

SQLProcessor::TableIdColumnBaseDefListMap
  SQLProcessorZabbix::m_tableColumnBaseDefListMap;

SQLProcessor::TableIdNameMap
  SQLProcessorZabbix::m_tableIdNameMap;

/*
static const char *COLUMN_NAME_NODEID = "nodeid";
static const char *COLUMN_NAME_NAME   = "name";
static const char *COLUMN_NAME_IP     = "ip";
static const char *COLUMN_NAME_PORT   = "port";
static const char *COLUMN_NAME_NODETYPE  = "nodetype";
static const char *COLUMN_NAME_MASTERID  = "masterid";
*/

/*
+----------+-------------+------+-----+---------+-------+
| Field    | Type        | Null | Key | Default | Extra |
+----------+-------------+------+-----+---------+-------+
| nodeid   | int(11)     | NO   | PRI | NULL    |       |
| name     | varchar(64) | NO   |     | 0       |       |
| ip       | varchar(39) | NO   |     |         |       |
| port     | int(11)     | NO   |     | 10051   |       |
| nodetype | int(11)     | NO   |     | 0       |       |
| masterid | int(11)     | YES  | MUL | NULL    |       |
+----------+-------------+------+-----+---------+-------+
*/

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLProcessorZabbix::init(void)
{
	defineTable(TABLE_ID_NODES, TABLE_NAME_NODES);
	defineColumn(TABLE_ID_NODES, "nodeid",   SQL_COLUMN_TYPE_INT, 11);
	defineColumn(TABLE_ID_NODES, "name",     SQL_COLUMN_TYPE_VARCHAR, 64);
	defineColumn(TABLE_ID_NODES, "ip",       SQL_COLUMN_TYPE_VARCHAR, 39);
	defineColumn(TABLE_ID_NODES, "port",     SQL_COLUMN_TYPE_INT, 11);
	defineColumn(TABLE_ID_NODES, "nodetype", SQL_COLUMN_TYPE_INT, 11);
	defineColumn(TABLE_ID_NODES, "masterid", SQL_COLUMN_TYPE_INT, 11);

	m_tableProcFuncMap[TABLE_NAME_NODES]
	  = &SQLProcessorZabbix::tableProcNodes;
}

SQLProcessor *SQLProcessorZabbix::createInstance(void)
{
	return new SQLProcessorZabbix();
}

const char *SQLProcessorZabbix::getDBName(void)
{
	return "zabbix";
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLProcessorZabbix::SQLProcessorZabbix(void)
{
	MLPL_INFO("created: %s\n", __func__);
}

SQLProcessorZabbix::~SQLProcessorZabbix()
{
}

bool SQLProcessorZabbix::select(SQLSelectResult &result,
                                string &query, vector<string> &words)
{
	SQLSelectStruct selectStruct;
	if (!parseSelectStatement(selectStruct, words))
		return false;

	map<string, TableProcFunc>::iterator it;
	it = m_tableProcFuncMap.find(selectStruct.table);
	if (it == m_tableProcFuncMap.end())
		return false;

	TableProcFunc func = it->second;
	return (this->*func)(result, selectStruct);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void
SQLProcessorZabbix::addColumnDefs(SQLSelectResult &result,
                                  const ColumnBaseDefinition &columnBaseDef,
                                  SQLSelectStruct &selectStruct)
{
	result.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = result.columnDefs.back();
	colDef.schema = getDBName();
	colDef.table = selectStruct.table;
	colDef.tableVar = selectStruct.tableVar;
	colDef.column = selectStruct.table;
	colDef.tableVar = selectStruct.tableVar;
	colDef.type     = columnBaseDef.type;
	colDef.columnLength = columnBaseDef.columnLength;
}

void
SQLProcessorZabbix::addAllColumnDefs(SQLSelectResult &result, int tableId,
                                     SQLSelectStruct &selectStruct)
{
	TableIdColumnBaseDefListMap::iterator it;
	it = m_tableColumnBaseDefListMap.find(tableId);
	if (it == m_tableColumnBaseDefListMap.end()) {
		MLPL_BUG("Not found table: %d\n", tableId);
		return;
	}

	ColumnBaseDefList &baseDefList = it->second;;
	ColumnBaseDefListIterator baseDef = baseDefList.begin();
	for (; baseDef != baseDefList.end(); ++baseDef)
		addColumnDefs(result, *baseDef, selectStruct);
}

bool SQLProcessorZabbix::tableProcNodes(SQLSelectResult &result,
                                        SQLSelectStruct &selectStruct)
{
	MLPL_DBG("***** %s\n", __func__);
	if (selectStruct.columns.empty())
		return false;

	if (selectedAllColumns(selectStruct)) {
		addAllColumnDefs(result, TABLE_ID_NODES, selectStruct);
		return true;

	} else {
		MLPL_DBG("Not supported: columns: %s\n",
		         selectStruct.columns[0].c_str());
	}
	
	return false;
}

// ---------------------------------------------------------------------------
// Private static methods
// ---------------------------------------------------------------------------
void SQLProcessorZabbix::defineTable(int tableId, const char *tableName)
{
	m_tableColumnBaseDefListMap[tableId] = ColumnBaseDefList();
	m_tableIdNameMap[tableId] = tableName;
}

void SQLProcessorZabbix::defineColumn(int tableId, const char *columnName,
                                      SQLColumnType type, size_t columnLength)
{
	TableIdColumnBaseDefListMapIterator it;
	it = m_tableColumnBaseDefListMap.find(tableId);
	if (it == m_tableColumnBaseDefListMap.end()) {
		MLPL_BUG("Not found table: %d\n", tableId);
		return;
	}
	ColumnBaseDefList &list = it->second;
	list.push_back(ColumnBaseDefinition());
	ColumnBaseDefinition &colDef = list.back();
	colDef.tableName    = getTableName(tableId);
	colDef.columnName   = columnName;
	colDef.type         = type;
	colDef.columnLength = columnLength;
}

const char *SQLProcessorZabbix::getTableName(int tableId)
{
	TableIdNameMapIterator it = m_tableIdNameMap.find(tableId);
	if (it == m_tableIdNameMap.end())
		return NULL;
	return it->second;
}
