#include <Logger.h>
using namespace mlpl;

#include "ItemEnum.h"
#include "ItemGroupEnum.h"
#include "ItemDataPtr.h"
#include "ItemTablePtr.h"
#include "SQLProcessorZabbix.h"

enum TableID {
	TABLE_ID_NODES,
	TABLE_ID_CONFIG,
	TABLE_ID_USERS,
};

static const char *TABLE_NAME_NODES  = "nodes";
static const char *TABLE_NAME_CONFIG = "config";
static const char *TABLE_NAME_USERS  = "users";

SQLProcessor::TableProcFuncMap
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
#define ADD_FUNC(N, G, T) m_tableProcFuncMap[N] = \
static_cast<TableProcFunc>(&SQLProcessorZabbix::tableProcTemplate<G, T>);

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
	ADD_FUNC(TABLE_NAME_NODES, GROUP_ID_ZBX_NODES, TABLE_ID_NODES);

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
	ADD_FUNC(TABLE_NAME_CONFIG, GROUP_ID_ZBX_CONFIG, TABLE_ID_CONFIG);

	defineTable(TABLE_ID_USERS, TABLE_NAME_USERS);
	defineColumn(ITEM_ID_ZBX_USERS_USERID,
	             TABLE_ID_USERS, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(ITEM_ID_ZBX_USERS_ALIAS,
	             TABLE_ID_USERS, "alias",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(ITEM_ID_ZBX_USERS_NAME,
	             TABLE_ID_USERS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(ITEM_ID_ZBX_USERS_SURNAME,
	             TABLE_ID_USERS, "surname",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(ITEM_ID_ZBX_USERS_PASSWD,
	             TABLE_ID_USERS, "passwd",
	             SQL_COLUMN_TYPE_CHAR, 32);
	defineColumn(ITEM_ID_ZBX_USERS_URL,
	             TABLE_ID_USERS, "url",
	             SQL_COLUMN_TYPE_VARCHAR, 256);
	defineColumn(ITEM_ID_ZBX_USERS_AUTOLOGIN,
	             TABLE_ID_USERS, "autologin",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_AUTOLOGOUT,
	             TABLE_ID_USERS, "autologout",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_LANG,
	             TABLE_ID_USERS, "lang",
	             SQL_COLUMN_TYPE_VARCHAR, 5);
	defineColumn(ITEM_ID_ZBX_USERS_REFRESH,
	             TABLE_ID_USERS, "refresh",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_TYPE,
	             TABLE_ID_USERS, "type",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_THEME,
	             TABLE_ID_USERS, "theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(ITEM_ID_ZBX_USERS_ATTEMPT_FAILED,
	             TABLE_ID_USERS, "attempt_failed",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_ATTEMPT_IP,
	             TABLE_ID_USERS, "attempt_ip",
	             SQL_COLUMN_TYPE_VARCHAR, 39);
	defineColumn(ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,
	             TABLE_ID_USERS, "attempt_clock",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,
	             TABLE_ID_USERS, "rows_per_page",
	             SQL_COLUMN_TYPE_INT, 11);
	ADD_FUNC(TABLE_NAME_USERS, GROUP_ID_ZBX_USERS, TABLE_ID_USERS);
}

SQLProcessor *SQLProcessorZabbix::createInstance(void)
{
	return new SQLProcessorZabbix();
}

const char *SQLProcessorZabbix::getDBNameStatic(void)
{
	return "zabbix";
}

const char *SQLProcessorZabbix::getDBName(void)
{
	return getDBNameStatic();
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLProcessorZabbix::SQLProcessorZabbix(void)
: SQLProcessor(m_tableProcFuncMap,
               m_tableColumnBaseDefListMap,
               m_tableIdNameMap)
{
	m_VDSZabbix = VirtualDataStoreZabbix::getInstance();
	MLPL_INFO("created: %s\n", __func__);
}

SQLProcessorZabbix::~SQLProcessorZabbix()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
template<ItemGroupId GROUP_ID, int TABLE_ID> 
bool SQLProcessorZabbix::tableProcTemplate
     (SQLSelectResult &result, SQLSelectInfo &selectInfo)
{
	const ItemGroupId itemGroupId = GROUP_ID;
	const ItemTablePtr tablePtr(m_VDSZabbix->getItemTable(itemGroupId));
	if (!tablePtr.hasData()) {
		MLPL_BUG("Failed to get ItemTable: %"PRIx_ITEM_GROUP"\n",
		         itemGroupId);
		return false;
	}
	return generalSelect(result, selectInfo, tablePtr, TABLE_ID);
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
