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

SQLProcessor::TableNameStaticInfoMap
  SQLProcessorZabbix::m_tableNameStaticInfoMap;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLProcessorZabbix::init(void)
{
#define MAKE_FUNC(G) \
static_cast<SQLTableMakeFunc>(&SQLProcessorZabbix::tableMakeFuncTemplate<G>)

	static SQLTableStaticInfo *staticInfo;
	staticInfo =
	  defineTable(TABLE_ID_NODES, TABLE_NAME_NODES,
	              MAKE_FUNC(GROUP_ID_ZBX_NODES));
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NODEID,
	             TABLE_ID_NODES, "nodeid",   SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NAME,
	             TABLE_ID_NODES, "name",     SQL_COLUMN_TYPE_VARCHAR, 64);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_IP,
	             TABLE_ID_NODES, "ip",       SQL_COLUMN_TYPE_VARCHAR, 39);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_PORT,
	             TABLE_ID_NODES, "port",     SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NODETYPE,
	             TABLE_ID_NODES, "nodetype", SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_MASTERID,
	             TABLE_ID_NODES, "masterid", SQL_COLUMN_TYPE_INT, 11);

	staticInfo =
	  defineTable(TABLE_ID_CONFIG, TABLE_NAME_CONFIG,
	              MAKE_FUNC(GROUP_ID_ZBX_CONFIG));
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_CONFIGID,
	             TABLE_ID_CONFIG, "configid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_ALERT_HISTORY,
	             TABLE_ID_CONFIG, "alert_history",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_HISTORY,
	             TABLE_ID_CONFIG, "event_history",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED,
	             TABLE_ID_CONFIG, "refresh_unsupoorted",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_WORK_PERIOD,
	             TABLE_ID_CONFIG, "work_period",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID,
	             TABLE_ID_CONFIG, "alert_usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE,
	             TABLE_ID_CONFIG, "event_ack_enable",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,
	             TABLE_ID_CONFIG, "event_expire",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,
	             TABLE_ID_CONFIG, "event_show_max",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DEFAULT_THEME,
	             TABLE_ID_CONFIG, "default_theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,
	             TABLE_ID_CONFIG, "authentication_type",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_HOST,
	             TABLE_ID_CONFIG, "ldap_host",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_PORT,
	             TABLE_ID_CONFIG, "ldap_port",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN,
	             TABLE_ID_CONFIG, "ldap_base_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN,
	             TABLE_ID_CONFIG, "ldap_bind_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,
	             TABLE_ID_CONFIG, "ldap_bind_password",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE,
	             TABLE_ID_CONFIG, "ldap_search_attribute",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,
	             TABLE_ID_CONFIG, "dropdown_first_entry",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER,
	             TABLE_ID_CONFIG, "dropdown_first_remember",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID,
	             TABLE_ID_CONFIG, "discovery_groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_MAX_INTABLE,
	             TABLE_ID_CONFIG, "max_in_table",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,
	             TABLE_ID_CONFIG, "search_limit",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0,
	             TABLE_ID_CONFIG, "severity_color_0",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1,
	             TABLE_ID_CONFIG, "severity_color_1",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2,
	             TABLE_ID_CONFIG, "severity_color_2",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3,
	             TABLE_ID_CONFIG, "severity_color_3",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4,
	             TABLE_ID_CONFIG, "severity_color_4",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5,
	             TABLE_ID_CONFIG, "severity_color_5",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0,
	             TABLE_ID_CONFIG, "severity_name_0",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1,
	             TABLE_ID_CONFIG, "severity_name_1",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2,
	             TABLE_ID_CONFIG, "severity_name_2",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3,
	             TABLE_ID_CONFIG, "severity_name_3",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4,
	             TABLE_ID_CONFIG, "severity_name_4",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5,
	             TABLE_ID_CONFIG, "severity_name_5",
	             SQL_COLUMN_TYPE_VARCHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_PERIOD,
	             TABLE_ID_CONFIG, "ok_period",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_BLINK_PERIOD,
	             TABLE_ID_CONFIG, "blink_period",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR,
	             TABLE_ID_CONFIG, "problem_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,
	             TABLE_ID_CONFIG, "problem_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR,
	             TABLE_ID_CONFIG, "ok_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,
	             TABLE_ID_CONFIG, "ok_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE,
	             TABLE_ID_CONFIG, "problem_unack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,
	             TABLE_ID_CONFIG, "problem_ack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,
	             TABLE_ID_CONFIG, "ok_unack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,
	             TABLE_ID_CONFIG, "ok_ack_style",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,
	             TABLE_ID_CONFIG, "snmptrap_logging",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL,
	             TABLE_ID_CONFIG, "server_check_interval",
	             SQL_COLUMN_TYPE_INT, 11);

	staticInfo =
	  defineTable(TABLE_ID_USERS, TABLE_NAME_USERS,
	              MAKE_FUNC(GROUP_ID_ZBX_USERS));
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_USERID,
	             TABLE_ID_USERS, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ALIAS,
	             TABLE_ID_USERS, "alias",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_NAME,
	             TABLE_ID_USERS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_SURNAME,
	             TABLE_ID_USERS, "surname",
	             SQL_COLUMN_TYPE_VARCHAR, 100);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_PASSWD,
	             TABLE_ID_USERS, "passwd",
	             SQL_COLUMN_TYPE_CHAR, 32);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_URL,
	             TABLE_ID_USERS, "url",
	             SQL_COLUMN_TYPE_VARCHAR, 256);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_AUTOLOGIN,
	             TABLE_ID_USERS, "autologin",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_AUTOLOGOUT,
	             TABLE_ID_USERS, "autologout",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_LANG,
	             TABLE_ID_USERS, "lang",
	             SQL_COLUMN_TYPE_VARCHAR, 5);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_REFRESH,
	             TABLE_ID_USERS, "refresh",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_TYPE,
	             TABLE_ID_USERS, "type",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_THEME,
	             TABLE_ID_USERS, "theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_FAILED,
	             TABLE_ID_USERS, "attempt_failed",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_IP,
	             TABLE_ID_USERS, "attempt_ip",
	             SQL_COLUMN_TYPE_VARCHAR, 39);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,
	             TABLE_ID_USERS, "attempt_clock",
	             SQL_COLUMN_TYPE_INT, 11);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,
	             TABLE_ID_USERS, "rows_per_page",
	             SQL_COLUMN_TYPE_INT, 11);
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
: SQLProcessor(m_tableNameStaticInfoMap)
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
template<ItemGroupId GROUP_ID>
bool SQLProcessorZabbix::tableMakeFuncTemplate
     (SQLSelectInfo &selectInfo, SQLTableInfo &tableInfo)
{
	const ItemGroupId itemGroupId = GROUP_ID;
	const ItemTablePtr tablePtr = m_VDSZabbix->getItemTable(itemGroupId);
	if (!tablePtr.hasData()) {
		MLPL_BUG("Failed to get ItemTable: %"PRIx_ITEM_GROUP"\n",
		         itemGroupId);
		return false;
	}
	return makeTable(selectInfo, tableInfo, tablePtr);
}

// ---------------------------------------------------------------------------
// Private static methods
// ---------------------------------------------------------------------------
SQLTableStaticInfo *
SQLProcessorZabbix::defineTable(int tableId, const char *tableName,
                                SQLTableMakeFunc tableMakeFunc)
{
	SQLTableStaticInfo *staticInfo = new SQLTableStaticInfo();
	staticInfo->tableId = tableId;
	staticInfo->tableName = tableName;
	staticInfo->tableMakeFunc = tableMakeFunc;
	m_tableNameStaticInfoMap[tableName] = staticInfo;
	return staticInfo;
}

void SQLProcessorZabbix::defineColumn(SQLTableStaticInfo *staticInfo,
                                      ItemId itemId,
                                      int tableId, const char *columnName,
                                      SQLColumnType type, size_t columnLength)
{
	ColumnBaseDefList &list =
	  const_cast<ColumnBaseDefList &>(staticInfo->columnBaseDefList);
	list.push_back(ColumnBaseDefinition());
	ColumnBaseDefinition &colBaseDef = list.back();
	colBaseDef.itemId       = itemId;
	colBaseDef.tableName    = staticInfo->tableName;
	colBaseDef.columnName   = columnName;
	colBaseDef.type         = type;
	colBaseDef.columnLength = columnLength;

	ItemNameColumnBaseDefRefMap &map = 
	  const_cast<ItemNameColumnBaseDefRefMap &>
	            (staticInfo->columnBaseDefMap);
	map[columnName] = &colBaseDef;
}
