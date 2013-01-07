#include <Logger.h>
using namespace mlpl;

#include "ItemDataPtr.h"
#include "ItemEnum.h"
#include "ItemGroupEnum.h"
#include "SQLProcessorZabbix.h"

enum TableID {
	TABLE_ID_NODES,
	TABLE_ID_CONFIG,
};

static const char *TABLE_NAME_NODES  = "nodes";
static const char *TABLE_NAME_CONFIG = "config";

map<string, SQLProcessorZabbix::TableProcFunc>
  SQLProcessorZabbix::m_tableProcFuncMap;

SQLProcessor::TableIdColumnBaseDefListMap
  SQLProcessorZabbix::m_tableColumnBaseDefListMap;

SQLProcessor::TableIdNameMap
  SQLProcessorZabbix::m_tableIdNameMap;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLProcessorZabbix::init(void)
{
	defineTable(TABLE_ID_NODES, TABLE_NAME_NODES);
	defineColumn(ITEM_ID_ZBX_NODES_NODEID,
	             TABLE_ID_NODES, "nodeid",   SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_NODES_NAME,
	             TABLE_ID_NODES, "name",     SQL_COLUMN_TYPE_VARCHAR, 64);
	defineColumn(ITEM_ID_ZBX_NODES_IP,
	             TABLE_ID_NODES, "ip",       SQL_COLUMN_TYPE_VARCHAR, 39);
	defineColumn(ITEM_ID_ZBX_NODES_PORT,
	             TABLE_ID_NODES, "port",     SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_NODES_NODETYPE,
	             TABLE_ID_NODES, "nodetype", SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_NODES_MASTERID,
	             TABLE_ID_NODES, "masterid", SQL_COLUMN_TYPE_INT, 11);
	m_tableProcFuncMap[TABLE_NAME_NODES]
	  = &SQLProcessorZabbix::tableProcNodes;

	defineTable(TABLE_ID_CONFIG, TABLE_NAME_CONFIG);
	defineColumn(ITEM_ID_ZBX_CONFIG_CONFIGID,
	             TABLE_ID_CONFIG, "configid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(ITEM_ID_ZBX_CONFIG_ALERT_HISTORY,
	             TABLE_ID_CONFIG, "alert_history",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_EVENT_HISTORY,
	             TABLE_ID_CONFIG, "event_history",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED,
	             TABLE_ID_CONFIG, "refresh_unsupoorted",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_WORK_PERIOD,
	             TABLE_ID_CONFIG, "work_period",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID,
	             TABLE_ID_CONFIG, "alert_usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE,
	             TABLE_ID_CONFIG, "event_ack_enable",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,
	             TABLE_ID_CONFIG, "event_expire",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,
	             TABLE_ID_CONFIG, "event_show_max",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_DEFAULT_THEME,
	             TABLE_ID_CONFIG, "default_theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,
	             TABLE_ID_CONFIG, "authentication_type",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_HOST,
	             TABLE_ID_CONFIG, "ldap_host",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_PORT,
	             TABLE_ID_CONFIG, "ldap_port",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN,
	             TABLE_ID_CONFIG, "ldap_base_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN,
	             TABLE_ID_CONFIG, "ldap_bind_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,
	             TABLE_ID_CONFIG, "ldap_bind_password",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE,
	             TABLE_ID_CONFIG, "ldap_search_attribute",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,
	             TABLE_ID_CONFIG, "dropdown_first_entry",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER,
	             TABLE_ID_CONFIG, "dropdown_first_remember",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID,
	             TABLE_ID_CONFIG, "discovery_groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(ITEM_ID_ZBX_CONFIG_MAX_INTABLE,
	             TABLE_ID_CONFIG, "max_in_table",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,
	             TABLE_ID_CONFIG, "search_limit",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0,
	             TABLE_ID_CONFIG, "severity_color_0",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1,
	             TABLE_ID_CONFIG, "severity_color_1",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2,
	             TABLE_ID_CONFIG, "severity_color_2",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3,
	             TABLE_ID_CONFIG, "severity_color_3",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4,
	             TABLE_ID_CONFIG, "severity_color_4",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5,
	             TABLE_ID_CONFIG, "severity_color_5",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0,
	             TABLE_ID_CONFIG, "severity_name_0",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1,
	             TABLE_ID_CONFIG, "severity_name_1",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2,
	             TABLE_ID_CONFIG, "severity_name_2",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3,
	             TABLE_ID_CONFIG, "severity_name_3",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4,
	             TABLE_ID_CONFIG, "severity_name_4",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5,
	             TABLE_ID_CONFIG, "severity_name_5",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(ITEM_ID_ZBX_CONFIG_OK_PERIOD,
	             TABLE_ID_CONFIG, "ok_period",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_BLINK_PERIOD,
	             TABLE_ID_CONFIG, "blink_period",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR,
	             TABLE_ID_CONFIG, "problem_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,
	             TABLE_ID_CONFIG, "problem_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR,
	             TABLE_ID_CONFIG, "ok_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,
	             TABLE_ID_CONFIG, "ok_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE,
	             TABLE_ID_CONFIG, "problem_unack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,
	             TABLE_ID_CONFIG, "problem_ack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,
	             TABLE_ID_CONFIG, "ok_unack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,
	             TABLE_ID_CONFIG, "ok_ack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,
	             TABLE_ID_CONFIG, "snmptrap_logging",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL,
	             TABLE_ID_CONFIG, "server_check_interval",
	             SQL_COLUMN_TYPE_INT, 11);
	m_tableProcFuncMap[TABLE_NAME_CONFIG]
	  = &SQLProcessorZabbix::tableProcConfig;
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
	m_VDSZabbix = VirtualDataStoreZabbix::getInstance();
	MLPL_INFO("created: %s\n", __func__);
}

SQLProcessorZabbix::~SQLProcessorZabbix()
{
}

bool SQLProcessorZabbix::select(SQLSelectResult &result,
                                SQLSelectInfo   &selectInfo)
{
	if (!parseSelectStatement(selectInfo))
		return false;

	map<string, TableProcFunc>::iterator it;
	it = m_tableProcFuncMap.find(selectInfo.table);
	if (it == m_tableProcFuncMap.end())
		return false;

	TableProcFunc func = it->second;
	return (this->*func)(result, selectInfo);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void
SQLProcessorZabbix::addColumnDefs(SQLSelectResult &result,
                                  const ColumnBaseDefinition &columnBaseDef,
                                  SQLSelectInfo &selectInfo)
{
	result.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = result.columnDefs.back();
	colDef.baseDef      = &columnBaseDef;
	colDef.schema       = getDBName();
	colDef.table        = selectInfo.table;
	colDef.tableVar     = selectInfo.tableVar;
	colDef.column       = columnBaseDef.columnName;
	colDef.columnVar    = columnBaseDef.columnName;
}

void
SQLProcessorZabbix::addAllColumnDefs(SQLSelectResult &result, int tableId,
                                     SQLSelectInfo &selectInfo)
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
		addColumnDefs(result, *baseDef, selectInfo);
}

bool SQLProcessorZabbix::tableProcNodes
     (SQLSelectResult &result, SQLSelectInfo &selectInfo)
{
	if (selectInfo.columns.empty())
		return false;

	if (selectedAllColumns(selectInfo)) {
		addAllColumnDefs(result, TABLE_ID_NODES, selectInfo);
		return true;

	} else {
		MLPL_DBG("Not supported: columns: %s\n",
		         selectInfo.columns[0].c_str());
	}
	
	return false;
}

bool SQLProcessorZabbix::tableProcConfig
     (SQLSelectResult &result, SQLSelectInfo &selectInfo)
{
	if (selectInfo.columns.empty())
		return false;

	// Set column definition
	if (selectedAllColumns(selectInfo)) {
		addAllColumnDefs(result, TABLE_ID_CONFIG, selectInfo);
		return true;

	} else {
		MLPL_DBG("Not supported: columns: %s\n",
		         selectInfo.columns[0].c_str());
	}

	// Set row data
	const ItemGroupId itemGroupId = GROUP_ID_ZBX_CONFIG;
	const ItemGroup *itemGroup = m_VDSZabbix->getItemGroup(itemGroupId);
	if (!itemGroup) {
		MLPL_BUG("Failed to get ItemGroup: %"PRIx_ITEM_GROUP"\n",
		         itemGroupId);
		return false;
	}

	for (size_t i = 0; i < result.columnDefs.size(); i++) {
		const SQLColumnDefinition &colDef = result.columnDefs[i];
		ItemDataPtr dataPtr(colDef.baseDef->itemId, itemGroup);
		if (!dataPtr.hasData()) {
			MLPL_BUG("Failed to get ItemData: %"PRIx_ITEM
			         "from ItemGroup: %"PRIx_ITEM_GROUP"\n",
			         colDef.baseDef->itemId, itemGroup);
			return false;
		}
		result.rows.push_back(dataPtr->getString());
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

void SQLProcessorZabbix::defineColumn(ItemId itemId,
                                      int tableId, const char *columnName,
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
	colDef.itemId       = itemId;
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
