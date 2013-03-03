/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
	TABLE_ID_USRGRP,
	TABLE_ID_USERS_GROUPS,
	TABLE_ID_SESSIONS,
	TABLE_ID_PROFILES,
	TABLE_ID_USER_HISTORY,
	TABLE_ID_TRIGGERS,
	TABLE_ID_FUNCTIONS,
	TABLE_ID_ITEMS,
	TABLE_ID_HOSTS,
	TABLE_ID_GROUPS,
	TABLE_ID_HOSTS_GROUPS,
	TABLE_ID_TRIGGER_DEPENDS,
	TABLE_ID_EVENTS,
	TABLE_ID_SCRIPTS,
	TABLE_ID_HOST_INVENTORY,
	TABLE_ID_RIGHTS,
	TABLE_ID_SCREENS,
};

static const char *TABLE_NAME_NODES  = "nodes";
static const char *TABLE_NAME_CONFIG = "config";
static const char *TABLE_NAME_USERS  = "users";
static const char *TABLE_NAME_USRGRP = "usrgrp";
static const char *TABLE_NAME_USERS_GROUPS = "users_groups";
static const char *TABLE_NAME_SESSIONS = "sessions";
static const char *TABLE_NAME_PROFILES = "profiles";
static const char *TABLE_NAME_USER_HISTORY = "user_history";
static const char *TABLE_NAME_TRIGGERS = "triggers";
static const char *TABLE_NAME_FUNCTIONS = "functions";
static const char *TABLE_NAME_ITEMS = "items";
static const char *TABLE_NAME_HOSTS = "hosts";
static const char *TABLE_NAME_GROUPS = "groups";
static const char *TABLE_NAME_HOSTS_GROUPS = "hosts_groups";
static const char *TABLE_NAME_TRIGGER_DEPENDS = "trigger_depends";
static const char *TABLE_NAME_EVENTS = "events";
static const char *TABLE_NAME_SCRIPTS = "scripts";
static const char *TABLE_NAME_HOST_INVENTORY = "host_inventory";
static const char *TABLE_NAME_RIGHTS = "rights";
static const char *TABLE_NAME_SCREENS = "screens";

TableNameStaticInfoMap SQLProcessorZabbix::m_tableNameStaticInfoMap;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLProcessorZabbix::init(void)
{
#define MAKE_FUNC(G) tableGetFuncTemplate<G>

	static SQLTableStaticInfo *staticInfo;
	staticInfo =
	  defineTable(TABLE_ID_NODES, TABLE_NAME_NODES,
	              MAKE_FUNC(GROUP_ID_ZBX_NODES));
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NODEID,
	             TABLE_ID_NODES, "nodeid",   SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NAME,
	             TABLE_ID_NODES, "name",     SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_IP,
	             TABLE_ID_NODES, "ip",       SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_PORT,
	             TABLE_ID_NODES, "port",     SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "10051");
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_NODETYPE,
	             TABLE_ID_NODES, "nodetype", SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_NODES_MASTERID,
	             TABLE_ID_NODES, "masterid", SQL_COLUMN_TYPE_INT, 11,
	             true, SQL_KEY_MUL, NULL);

	staticInfo =
	  defineTable(TABLE_ID_CONFIG, TABLE_NAME_CONFIG,
	              MAKE_FUNC(GROUP_ID_ZBX_CONFIG));
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_CONFIGID,
	             TABLE_ID_CONFIG, "configid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_ALERT_HISTORY,
	             TABLE_ID_CONFIG, "alert_history",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_HISTORY,
	             TABLE_ID_CONFIG, "event_history",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED,
	             TABLE_ID_CONFIG, "refresh_unsupoorted",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_WORK_PERIOD,
	             TABLE_ID_CONFIG, "work_period",
	             SQL_COLUMN_TYPE_VARCHAR, 100,
	             false, SQL_KEY_NONE, "1-5,00:00-24:00");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID,
	             TABLE_ID_CONFIG, "alert_usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE,
	             TABLE_ID_CONFIG, "event_ack_enable",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,
	             TABLE_ID_CONFIG, "event_expire",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "7");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,
	             TABLE_ID_CONFIG, "event_show_max",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "100");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DEFAULT_THEME,
	             TABLE_ID_CONFIG, "default_theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "originalblue");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,
	             TABLE_ID_CONFIG, "authentication_type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_HOST,
	             TABLE_ID_CONFIG, "ldap_host",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_PORT,
	             TABLE_ID_CONFIG, "ldap_port",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "389");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN,
	             TABLE_ID_CONFIG, "ldap_base_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN,
	             TABLE_ID_CONFIG, "ldap_bind_dn",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,
	             TABLE_ID_CONFIG, "ldap_bind_password",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE,
	             TABLE_ID_CONFIG, "ldap_search_attribute",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,
	             TABLE_ID_CONFIG, "dropdown_first_entry",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER,
	             TABLE_ID_CONFIG, "dropdown_first_remember",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID,
	             TABLE_ID_CONFIG, "discovery_groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_MAX_INTABLE,
	             TABLE_ID_CONFIG, "max_in_table",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "50");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,
	             TABLE_ID_CONFIG, "search_limit",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1000");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0,
	             TABLE_ID_CONFIG, "severity_color_0",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "DBDBDB");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1,
	             TABLE_ID_CONFIG, "severity_color_1",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "D6F6FF");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2,
	             TABLE_ID_CONFIG, "severity_color_2",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "FFF6A5");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3,
	             TABLE_ID_CONFIG, "severity_color_3",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "FFB689");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4,
	             TABLE_ID_CONFIG, "severity_color_4",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "FF9999");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5,
	             TABLE_ID_CONFIG, "severity_color_5",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "FF3838");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0,
	             TABLE_ID_CONFIG, "severity_name_0",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "Not classified");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1,
	             TABLE_ID_CONFIG, "severity_name_1",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "Information");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2,
	             TABLE_ID_CONFIG, "severity_name_2",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "Warning");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3,
	             TABLE_ID_CONFIG, "severity_name_3",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "Average");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4,
	             TABLE_ID_CONFIG, "severity_name_4",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "High");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5,
	             TABLE_ID_CONFIG, "severity_name_5",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "Disaster");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_PERIOD,
	             TABLE_ID_CONFIG, "ok_period",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1800");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_BLINK_PERIOD,
	             TABLE_ID_CONFIG, "blink_period",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1800");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR,
	             TABLE_ID_CONFIG, "problem_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "DC0000");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,
	             TABLE_ID_CONFIG, "problem_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "DC0000");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR,
	             TABLE_ID_CONFIG, "ok_unack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "00AA00");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,
	             TABLE_ID_CONFIG, "ok_ack_color",
	             SQL_COLUMN_TYPE_VARCHAR, 6,
	             false, SQL_KEY_NONE, "00AA00");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE,
	             TABLE_ID_CONFIG, "problem_unack_style",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,
	             TABLE_ID_CONFIG, "problem_ack_style",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,
	             TABLE_ID_CONFIG, "ok_unack_style",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,
	             TABLE_ID_CONFIG, "ok_ack_style",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,
	             TABLE_ID_CONFIG, "snmptrap_logging",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL,
	             TABLE_ID_CONFIG, "server_check_interval",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "10");

	staticInfo =
	  defineTable(TABLE_ID_USERS, TABLE_NAME_USERS,
	              MAKE_FUNC(GROUP_ID_ZBX_USERS));
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_USERID,
	             TABLE_ID_USERS, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ALIAS,
	             TABLE_ID_USERS, "alias",
	             SQL_COLUMN_TYPE_VARCHAR, 100,
	             false, SQL_KEY_MUL, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_NAME,
	             TABLE_ID_USERS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 100,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_SURNAME,
	             TABLE_ID_USERS, "surname",
	             SQL_COLUMN_TYPE_VARCHAR, 100,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_PASSWD,
	             TABLE_ID_USERS, "passwd",
	             SQL_COLUMN_TYPE_CHAR, 32,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_URL,
	             TABLE_ID_USERS, "url",
	             SQL_COLUMN_TYPE_VARCHAR, 256,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_AUTOLOGIN,
	             TABLE_ID_USERS, "autologin",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_AUTOLOGOUT,
	             TABLE_ID_USERS, "autologout",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "900");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_LANG,
	             TABLE_ID_USERS, "lang",
	             SQL_COLUMN_TYPE_VARCHAR, 5,
	             false, SQL_KEY_NONE, "en_GB");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_REFRESH,
	             TABLE_ID_USERS, "refresh",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "30");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_TYPE,
	             TABLE_ID_USERS, "type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_THEME,
	             TABLE_ID_USERS, "theme",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "default");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_FAILED,
	             TABLE_ID_USERS, "attempt_failed",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_IP,
	             TABLE_ID_USERS, "attempt_ip",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,
	             TABLE_ID_USERS, "attempt_clock",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,
	             TABLE_ID_USERS, "rows_per_page",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "50");

	staticInfo =
	  defineTable(TABLE_ID_USRGRP, TABLE_NAME_USRGRP,
	              MAKE_FUNC(GROUP_ID_ZBX_USRGRP));
	defineColumn(staticInfo, ITEM_ID_ZBX_USRGRP_USRGRPID,
	             TABLE_ID_USRGRP, "usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USRGRP_NAME,
	             TABLE_ID_USRGRP, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_MUL, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USRGRP_GUI_ACCESS,
	             TABLE_ID_USRGRP, "gui_access",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USRGRP_USERS_STATUS,
	             TABLE_ID_USRGRP, "users_status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_USRGRP_DEBUG_MODE,
	             TABLE_ID_USRGRP, "debug_mode",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_USERS_GROUPS, TABLE_NAME_USERS_GROUPS,
	              MAKE_FUNC(GROUP_ID_ZBX_USERS_GROUPS));
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_GROUPS_ID,
	             TABLE_ID_USERS_GROUPS, "id",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_GROUPS_USRGRPID,
	             TABLE_ID_USERS_GROUPS, "usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USERS_GROUPS_USERID,
	             TABLE_ID_USERS_GROUPS, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);

	staticInfo =
	  defineTable(TABLE_ID_SESSIONS, TABLE_NAME_SESSIONS,
	              MAKE_FUNC(GROUP_ID_ZBX_SESSIONS));
	defineColumn(staticInfo, ITEM_ID_ZBX_SESSIONS_SESSIONID,
	             TABLE_ID_SESSIONS, "sessionid",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_PRI, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_SESSIONS_USERID,
	             TABLE_ID_SESSIONS, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_SESSIONS_LASTACCESS,
	             TABLE_ID_SESSIONS, "lastaccess",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0"); 
	defineColumn(staticInfo, ITEM_ID_ZBX_SESSIONS_STATUS,
	             TABLE_ID_SESSIONS, "status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0"); 

	staticInfo =
	  defineTable(TABLE_ID_PROFILES, TABLE_NAME_PROFILES,
	              MAKE_FUNC(GROUP_ID_ZBX_PROFILES));
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_PROFILEID,
	             TABLE_ID_PROFILES, "profileid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_USERID,
	             TABLE_ID_PROFILES, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_IDX,
	             TABLE_ID_PROFILES, "idx",
	             SQL_COLUMN_TYPE_VARCHAR, 96,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_IDX2,
	             TABLE_ID_PROFILES, "idx2",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_VALUE_ID,
	             TABLE_ID_PROFILES, "value_id",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_VALUE_INT,
	             TABLE_ID_PROFILES, "value_int",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_VALUE_STR,
	             TABLE_ID_PROFILES, "value_str",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_SOURCE,
	             TABLE_ID_PROFILES, "source",
	             SQL_COLUMN_TYPE_VARCHAR, 96,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_PROFILES_TYPE,
	             TABLE_ID_PROFILES, "type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_USER_HISTORY, TABLE_NAME_USER_HISTORY,
	              MAKE_FUNC(GROUP_ID_ZBX_USER_HISTORY));
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_USERHISTORYID,
	             TABLE_ID_USER_HISTORY, "userhistoryid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_USERID,
	             TABLE_ID_USER_HISTORY, "userid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_TITLE1,
	             TABLE_ID_USER_HISTORY, "title1",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_URL1,
	             TABLE_ID_USER_HISTORY, "url1",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_TITLE2,
	             TABLE_ID_USER_HISTORY, "title2",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_URL2,
	             TABLE_ID_USER_HISTORY, "url2",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_TITLE3,
	             TABLE_ID_USER_HISTORY, "title3",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_URL3,
	             TABLE_ID_USER_HISTORY, "url3",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_TITLE4,
	             TABLE_ID_USER_HISTORY, "title4",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_URL4,
	             TABLE_ID_USER_HISTORY, "url4",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_TITLE5,
	             TABLE_ID_USER_HISTORY, "title5",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_USER_HISTORY_URL5,
	             TABLE_ID_USER_HISTORY, "url5",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");

	staticInfo =
	  defineTable(TABLE_ID_TRIGGERS, TABLE_NAME_TRIGGERS,
	              MAKE_FUNC(GROUP_ID_ZBX_TRIGGERS));
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_TRIGGERID,
	             TABLE_ID_TRIGGERS, "triggerid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_EXPRESSION,
	             TABLE_ID_TRIGGERS, "expression",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_DESCRIPTION,
	             TABLE_ID_TRIGGERS, "description",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_URL,
	             TABLE_ID_TRIGGERS, "url",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_STATUS,
	             TABLE_ID_TRIGGERS, "status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_VALUE,
	             TABLE_ID_TRIGGERS, "value",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_PRIORITY,
	             TABLE_ID_TRIGGERS, "priority",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_LASTCHANGE,
	             TABLE_ID_TRIGGERS, "lastchange",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_COMMENTS,
	             TABLE_ID_TRIGGERS, "comments",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_ERROR,
	             TABLE_ID_TRIGGERS, "error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_TEMPLATEID,
	             TABLE_ID_TRIGGERS, "templateid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_TYPE,
	             TABLE_ID_TRIGGERS, "type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS,
	             TABLE_ID_TRIGGERS, "value_flags",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGERS_FLAGS,
	             TABLE_ID_TRIGGERS, "flags",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_FUNCTIONS, TABLE_NAME_FUNCTIONS,
	              MAKE_FUNC(GROUP_ID_ZBX_FUNCTIONS));
	defineColumn(staticInfo, ITEM_ID_ZBX_FUNCTIONS_FUNCTIONID,
	             TABLE_ID_FUNCTIONS, "functionid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_FUNCTIONS_ITEMID,
	             TABLE_ID_FUNCTIONS, "itemid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_FUNCTIONS_TRIGGERID,
	             TABLE_ID_FUNCTIONS, "triggerid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL); 
	defineColumn(staticInfo, ITEM_ID_ZBX_FUNCTIONS_FUNCTION,
	             TABLE_ID_FUNCTIONS, "function",
	             SQL_COLUMN_TYPE_VARCHAR, 12,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_FUNCTIONS_PARAMETER,
	             TABLE_ID_FUNCTIONS, "parameters",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_ITEMS, TABLE_NAME_ITEMS,
	              MAKE_FUNC(GROUP_ID_ZBX_ITEMS));
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_ITEMID,
	             TABLE_ID_ITEMS, "itemid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_TYPE,
	             TABLE_ID_ITEMS, "type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMP_COMMUNITY,
	             TABLE_ID_ITEMS, "snmp_community",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMP_OID,
	             TABLE_ID_ITEMS, "snmp_oid",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_HOSTID,
	             TABLE_ID_ITEMS, "hostid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_NAME,
	             TABLE_ID_ITEMS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_KEY_,
	             TABLE_ID_ITEMS, "key_",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_DELAY,
	             TABLE_ID_ITEMS, "delay",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_HISTORY,
	             TABLE_ID_ITEMS, "history",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "90");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_TRENDS,
	             TABLE_ID_ITEMS, "trends",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "365");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LASTVALUE,
	             TABLE_ID_ITEMS, "lastvalue",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             true, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LASTCLOCK,
	             TABLE_ID_ITEMS, "lastclock",
	             SQL_COLUMN_TYPE_INT, 11,
	             true, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PREVVALUE,
	             TABLE_ID_ITEMS, "prevvalue",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             true, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_STATUS,
	             TABLE_ID_ITEMS, "status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_VALUE_TYPE,
	             TABLE_ID_ITEMS, "value_type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_TRAPPER_HOSTS,
	             TABLE_ID_ITEMS, "trapper_hosts",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_UNITS,
	             TABLE_ID_ITEMS, "units",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_MULTIPLIER,
	             TABLE_ID_ITEMS, "multiplier",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_DELTA,
	             TABLE_ID_ITEMS, "delta",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PREVORGVALUE,
	             TABLE_ID_ITEMS, "prevorgvalue",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             true, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYNAME,
	             TABLE_ID_ITEMS, "snmpv3_securityname",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYLEVEL,
	             TABLE_ID_ITEMS, "snmpv3_securitylevel",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMPV3_AUTHPASSPHRASE,
	             TABLE_ID_ITEMS, "snmpv3_authpassphrase",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_SNMPV3_PRIVPASSPRASE,
	             TABLE_ID_ITEMS, "snmpv3_privpassphrase",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_FORMULA,
	             TABLE_ID_ITEMS, "formula",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_ERROR,
	             TABLE_ID_ITEMS, "error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LASTLOGSIZE,
	             TABLE_ID_ITEMS, "lastlogsize",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LOGTIMEFMT,
	             TABLE_ID_ITEMS, "logtimefmt",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_TEMPLATEID,
	             TABLE_ID_ITEMS, "templateid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_VALUEMAPID,
	             TABLE_ID_ITEMS, "valuemapid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_DELAY_FLEX,
	             TABLE_ID_ITEMS, "delay_flex",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PARAMS,
	             TABLE_ID_ITEMS, "params",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_IPMI_SENSOR,
	             TABLE_ID_ITEMS, "ipmi_sensor",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_DATA_TYPE,
	             TABLE_ID_ITEMS, "data_type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_AUTHTYPE,
	             TABLE_ID_ITEMS, "authtype",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_USERNAME,
	             TABLE_ID_ITEMS, "username",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PASSWORD,
	             TABLE_ID_ITEMS, "passowrd",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PUBLICKEY,
	             TABLE_ID_ITEMS, "publickey",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PRIVATEKEY,
	             TABLE_ID_ITEMS, "privatekey",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_MTIME,
	             TABLE_ID_ITEMS, "mtime",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LASTNS,
	             TABLE_ID_ITEMS, "lastns",
	             SQL_COLUMN_TYPE_INT, 11,
	             true, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_FLAGS,
	             TABLE_ID_ITEMS, "flags",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_FILTER,
	             TABLE_ID_ITEMS, "filter",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_INTERFACEID,
	             TABLE_ID_ITEMS, "interfaceid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_PORT,
	             TABLE_ID_ITEMS, "port",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_DESCRIPTION,
	             TABLE_ID_ITEMS, "description",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_INVENTORY_LINK,
	             TABLE_ID_ITEMS, "inventory_link",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_LIFETIME,
	             TABLE_ID_ITEMS, "lifetime",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "30");

	staticInfo =
	  defineTable(TABLE_ID_HOSTS, TABLE_NAME_HOSTS,
	              MAKE_FUNC(GROUP_ID_ZBX_HOSTS));
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_HOSTID,
	             TABLE_ID_HOSTS, "hostid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_PROXY_HOSTID,
	             TABLE_ID_HOSTS, "proxy_hostid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_HOST,
	             TABLE_ID_HOSTS, "host",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_MUL, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_STATUS,
	             TABLE_ID_HOSTS, "status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_DISABLE_UNTIL,
	             TABLE_ID_HOSTS, "disable_until",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_ERROR,
	             TABLE_ID_HOSTS, "error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_AVAILABLE,
	             TABLE_ID_HOSTS, "available",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_ERRORS_FROM,
	             TABLE_ID_HOSTS, "errors_from",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_LASTACCESS,
	             TABLE_ID_HOSTS, "lastaccess",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_AUTHTYPE,
	             TABLE_ID_HOSTS, "ipmi_authtype",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_PRIVILEGE,
	             TABLE_ID_HOSTS, "ipmi_privilege",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "2");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_USERNAME,
	             TABLE_ID_HOSTS, "ipmi_username",
	             SQL_COLUMN_TYPE_VARCHAR, 16,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_PASSWORD,
	             TABLE_ID_HOSTS, "ipmi_password",
	             SQL_COLUMN_TYPE_VARCHAR, 20,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_DISABLE_UNTIL,
	             TABLE_ID_HOSTS, "ipmi_disable_until",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_AVAILABLE,
	             TABLE_ID_HOSTS, "ipmi_available",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_SNMP_DISABLE_UNTIL,
	             TABLE_ID_HOSTS, "snmp_disable_until",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_SNMP_AVAILABLE,
	             TABLE_ID_HOSTS, "snmp_available",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_MAINTENANCEID,
	             TABLE_ID_HOSTS, "maintenanceid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_MAINTENANCE_STATUS,
	             TABLE_ID_HOSTS, "maintenance_status",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_MAINTENANCE_TYPE,
	             TABLE_ID_HOSTS, "maintenance_type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_MAINTENANCE_FROM,
	             TABLE_ID_HOSTS, "maintenance_from",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_ERRORS_FROM,
	             TABLE_ID_HOSTS, "ipmi_errors_from",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_SNMP_ERRORS_FROM,
	             TABLE_ID_HOSTS, "snmp_errors_from",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_IPMI_ERROR,
	             TABLE_ID_HOSTS, "ipmi_error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_SNMP_ERROR,
	             TABLE_ID_HOSTS, "snmp_error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_JMX_DISABLE_UNTIL,
	             TABLE_ID_HOSTS, "jmx_disable_until",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_JMX_AVAILABLE,
	             TABLE_ID_HOSTS, "jmx_available",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_JMX_ERRORS_FROM,
	             TABLE_ID_HOSTS, "jmx_errors_from",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_JMX_ERROR,
	             TABLE_ID_HOSTS, "jmx_error",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_NAME,
	             TABLE_ID_HOSTS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_MUL, "");

	staticInfo =
	  defineTable(TABLE_ID_GROUPS, TABLE_NAME_GROUPS,
	              MAKE_FUNC(GROUP_ID_ZBX_GROUPS));
	defineColumn(staticInfo, ITEM_ID_ZBX_GROUPS_GROUPID,
	             TABLE_ID_GROUPS, "groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_GROUPS_NAME,
	             TABLE_ID_GROUPS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_GROUPS_INTERNAL,
	             TABLE_ID_GROUPS, "internal",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_HOSTS_GROUPS, TABLE_NAME_HOSTS_GROUPS,
	              MAKE_FUNC(GROUP_ID_ZBX_HOSTS_GROUPS));
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_GROUPS_HOSTGROUPID,
	             TABLE_ID_HOSTS_GROUPS, "hostgroupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_GROUPS_HOSTID,
	             TABLE_ID_HOSTS_GROUPS, "hostid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOSTS_GROUPS_GROUPID,
	             TABLE_ID_HOSTS_GROUPS, "groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);

	staticInfo =
	  defineTable(TABLE_ID_TRIGGER_DEPENDS, TABLE_NAME_TRIGGER_DEPENDS,
	              MAKE_FUNC(GROUP_ID_ZBX_TRIGGER_DEPENDS));
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGER_DEPENDS_TRIGGERDEPID,
	             TABLE_ID_TRIGGER_DEPENDS, "triggerdepid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGER_DEPENDS_TRIGGERID_DOWN,
	             TABLE_ID_TRIGGER_DEPENDS, "triggerid_down",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_TRIGGER_DEPENDS_TRIGGERID_UP,
	             TABLE_ID_GROUPS, "triggerid_up",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);

	staticInfo =
	  defineTable(TABLE_ID_EVENTS, TABLE_NAME_EVENTS,
	              MAKE_FUNC(GROUP_ID_ZBX_EVENTS));
	defineColumn(staticInfo, ITEM_ID_EVENTS_EVENTID,
	             TABLE_ID_EVENTS, "eventid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_EVENTS_SOURCE,
	             TABLE_ID_EVENTS, "source",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_OBJECT,
	             TABLE_ID_EVENTS, "object",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_OBJECTID,
	             TABLE_ID_EVENTS, "objectid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_CLOCK,
	             TABLE_ID_EVENTS, "clock",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_MUL, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_VALUE,
	             TABLE_ID_EVENTS, "value",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_ACKNOWLEDGED,
	             TABLE_ID_EVENTS, "acknowledged",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_NS,
	             TABLE_ID_EVENTS, "ns",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_EVENTS_VALUE_CHANGED,
	             TABLE_ID_EVENTS, "value_changed",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");

	staticInfo =
	  defineTable(TABLE_ID_SCRIPTS, TABLE_NAME_SCRIPTS,
	              MAKE_FUNC(GROUP_ID_ZBX_SCRIPTS));
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_SCRIPTID,
	             TABLE_ID_SCRIPTS, "scriptid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_NAME,
	             TABLE_ID_SCRIPTS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_COMMAND,
	             TABLE_ID_SCRIPTS, "command",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_HOST_ACCESS,
	             TABLE_ID_SCRIPTS, "host_access",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "2");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_USRGRPID,
	             TABLE_ID_SCRIPTS, "usrgrpid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_GROUPID,
	             TABLE_ID_SCRIPTS, "groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_DESCRIPTION,
	             TABLE_ID_SCRIPTS, "description",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_CONFIRMATION,
	             TABLE_ID_SCRIPTS, "confirmation",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_TYPE,
	             TABLE_ID_SCRIPTS, "type",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCRIPTS_EXECUTE_ON,
	             TABLE_ID_SCRIPTS, "execute_on",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");

	staticInfo =
	  defineTable(TABLE_ID_HOST_INVENTORY, TABLE_NAME_HOST_INVENTORY,
	              MAKE_FUNC(GROUP_ID_ZBX_HOST_INVENTORY));
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HOSTID,
	             TABLE_ID_HOST_INVENTORY, "hostid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_INVENTORY_MODE,
	             TABLE_ID_HOST_INVENTORY, "inventory_mode",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_TYPE,
	             TABLE_ID_HOST_INVENTORY, "type",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_TYPE_FULL,
	             TABLE_ID_HOST_INVENTORY, "type_full",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_NAME,
	             TABLE_ID_HOST_INVENTORY, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_ALIAS,
	             TABLE_ID_HOST_INVENTORY, "alias",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OS,
	             TABLE_ID_HOST_INVENTORY, "os",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OS_FULL,
	             TABLE_ID_HOST_INVENTORY, "os_full",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OS_SHORT,
	             TABLE_ID_HOST_INVENTORY, "os_short",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SERIALNO_A,
	             TABLE_ID_HOST_INVENTORY, "serialno_a",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SERIALNO_B,
	             TABLE_ID_HOST_INVENTORY, "serialno_b",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_TAG,
	             TABLE_ID_HOST_INVENTORY, "tag",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_ASSET_TAG,
	             TABLE_ID_HOST_INVENTORY, "asset_tag",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_MACADDRESS_A,
	             TABLE_ID_HOST_INVENTORY, "macaddress_a",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_MACADDRESS_B,
	             TABLE_ID_HOST_INVENTORY, "macaddress_b",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HARDWARE,
	             TABLE_ID_HOST_INVENTORY, "hardware",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HARDWARE_FULL,
	             TABLE_ID_HOST_INVENTORY, "hardware_full",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE,
	             TABLE_ID_HOST_INVENTORY, "software",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_FULL,
	             TABLE_ID_HOST_INVENTORY, "software_full",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_APP_A,
	             TABLE_ID_HOST_INVENTORY, "software_app_a",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_APP_B,
	             TABLE_ID_HOST_INVENTORY, "software_app_b",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_APP_C,
	             TABLE_ID_HOST_INVENTORY, "software_app_c",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_APP_D,
	             TABLE_ID_HOST_INVENTORY, "software_app_d",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SOFTWARE_APP_E,
	             TABLE_ID_HOST_INVENTORY, "software_app_e",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_CONTACT,
	             TABLE_ID_HOST_INVENTORY, "contact",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_LOCATION,
	             TABLE_ID_HOST_INVENTORY, "location",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_LOCATION_LAT,
	             TABLE_ID_HOST_INVENTORY, "location_lat",
	             SQL_COLUMN_TYPE_VARCHAR, 16,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_LOCATION_LON,
	             TABLE_ID_HOST_INVENTORY, "location_lon",
	             SQL_COLUMN_TYPE_VARCHAR, 16,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_NOTES,
	             TABLE_ID_HOST_INVENTORY, "notes",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_CHASSIS,
	             TABLE_ID_HOST_INVENTORY, "chassis",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_MODEL,
	             TABLE_ID_HOST_INVENTORY, "model",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HW_ARCH,
	             TABLE_ID_HOST_INVENTORY, "hw_arch",
	             SQL_COLUMN_TYPE_VARCHAR, 32,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_VENDOR,
	             TABLE_ID_HOST_INVENTORY, "vendor",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_CONTRACT_NUMBER,
	             TABLE_ID_HOST_INVENTORY, "contract_number",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_INSTALLER_NAME,
	             TABLE_ID_HOST_INVENTORY, "installer_name",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_DEPLOYMENT_STATUS,
	             TABLE_ID_HOST_INVENTORY, "deployment_status",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_URL_A,
	             TABLE_ID_HOST_INVENTORY, "url_a",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_URL_B,
	             TABLE_ID_HOST_INVENTORY, "url_b",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_URL_C,
	             TABLE_ID_HOST_INVENTORY, "url_c",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HOST_NETWORKS,
	             TABLE_ID_HOST_INVENTORY, "host_networks",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HOST_NETMASK,
	             TABLE_ID_HOST_INVENTORY, "host_netmask",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_HOST_ROUTER,
	             TABLE_ID_HOST_INVENTORY, "host_router",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OOB_IP,
	             TABLE_ID_HOST_INVENTORY, "oob_ip",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OOB_NETMASK,
	             TABLE_ID_HOST_INVENTORY, "oob_netmask",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_OOB_ROUTER,
	             TABLE_ID_HOST_INVENTORY, "oob_router",
	             SQL_COLUMN_TYPE_VARCHAR, 39,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_DATE_HW_PURCHASE,
	             TABLE_ID_HOST_INVENTORY, "date_hw_purchase",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_DATE_HW_INSTALL,
	             TABLE_ID_HOST_INVENTORY, "date_hw_install",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_DATE_HW_EXPIRY,
	             TABLE_ID_HOST_INVENTORY, "date_hw_expiry",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_DATE_HW_DECOMM,
	             TABLE_ID_HOST_INVENTORY, "date_hw_decomm",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_ADDRESS_A,
	             TABLE_ID_HOST_INVENTORY, "site_address_a",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_ADDRESS_B,
	             TABLE_ID_HOST_INVENTORY, "site_address_b",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_ADDRESS_C,
	             TABLE_ID_HOST_INVENTORY, "site_address_c",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_CITY,
	             TABLE_ID_HOST_INVENTORY, "site_city",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_STATE,
	             TABLE_ID_HOST_INVENTORY, "site_state",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_COUNTRY,
	             TABLE_ID_HOST_INVENTORY, "site_country",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_ZIP,
	             TABLE_ID_HOST_INVENTORY, "site_zip",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_RACK,
	             TABLE_ID_HOST_INVENTORY, "site_rack",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_SITE_NOTES,
	             TABLE_ID_HOST_INVENTORY, "site_notes",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_NAME,
	             TABLE_ID_HOST_INVENTORY, "poc_1_name",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_EMAIL,
	             TABLE_ID_HOST_INVENTORY, "poc_1_email",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_PHONE_A,
	             TABLE_ID_HOST_INVENTORY, "poc_1_phone_a",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_PHONE_B,
	             TABLE_ID_HOST_INVENTORY, "poc_1_phone_b",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_CELL,
	             TABLE_ID_HOST_INVENTORY, "poc_1_cell",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_SCREEN,
	             TABLE_ID_HOST_INVENTORY, "poc_1_screen",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_1_NOTES,
	             TABLE_ID_HOST_INVENTORY, "poc_1_notes",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_NAME,
	             TABLE_ID_HOST_INVENTORY, "poc_2_name",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_EMAIL,
	             TABLE_ID_HOST_INVENTORY, "poc_2_email",
	             SQL_COLUMN_TYPE_VARCHAR, 128,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_PHONE_A,
	             TABLE_ID_HOST_INVENTORY, "poc_2_phone_a",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_PHONE_B,
	             TABLE_ID_HOST_INVENTORY, "poc_2_phone_b",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_CELL,
	             TABLE_ID_HOST_INVENTORY, "poc_2_cell",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_SCREEN,
	             TABLE_ID_HOST_INVENTORY, "poc_2_screen",
	             SQL_COLUMN_TYPE_VARCHAR, 64,
	             false, SQL_KEY_NONE, "");
	defineColumn(staticInfo, ITEM_ID_ZBX_HOST_INVENTORY_POC_2_NOTES,
	             TABLE_ID_HOST_INVENTORY, "poc_2_notes",
	             SQL_COLUMN_TYPE_TEXT, 0,
	             false, SQL_KEY_NONE, NULL);

	staticInfo =
	  defineTable(TABLE_ID_RIGHTS, TABLE_NAME_RIGHTS,
	              MAKE_FUNC(GROUP_ID_ZBX_RIGHTS));
	defineColumn(staticInfo, ITEM_ID_ZBX_RIGHTS_RIGHTID,
	             TABLE_ID_RIGHTS, "rightid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_RIGHTS_GROUPID,
	             TABLE_ID_RIGHTS, "groupid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_RIGHTS_PERMISSION,
	             TABLE_ID_RIGHTS, "permission",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "0");
	defineColumn(staticInfo, ITEM_ID_ZBX_RIGHTS_ID,
	             TABLE_ID_RIGHTS, "id",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_MUL, NULL);

	staticInfo =
	  defineTable(TABLE_ID_SCREENS, TABLE_NAME_SCREENS,
	              MAKE_FUNC(GROUP_ID_ZBX_SCREENS));
	defineColumn(staticInfo, ITEM_ID_ZBX_SCREENS_SCREENID,
	             TABLE_ID_SCREENS, "screenid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             false, SQL_KEY_PRI, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCREENS_NAME,
	             TABLE_ID_SCREENS, "name",
	             SQL_COLUMN_TYPE_VARCHAR, 255,
	             false, SQL_KEY_NONE, NULL);
	defineColumn(staticInfo, ITEM_ID_ZBX_SCREENS_HSIZE,
	             TABLE_ID_SCREENS, "hsize",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCREENS_VSIZE,
	             TABLE_ID_SCREENS, "vsize",
	             SQL_COLUMN_TYPE_INT, 11,
	             false, SQL_KEY_NONE, "1");
	defineColumn(staticInfo, ITEM_ID_ZBX_SCREENS_TEMPLATEID,
	             TABLE_ID_SCREENS, "templateid",
	             SQL_COLUMN_TYPE_BIGUINT, 20,
	             true, SQL_KEY_MUL, NULL);
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
: SQLProcessor(getDBName(), m_tableNameStaticInfoMap)
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
ItemTablePtr SQLProcessorZabbix::tableGetFuncTemplate(void)
{
	const ItemGroupId itemGroupId = GROUP_ID;
	VirtualDataStoreZabbix *dataStore =
	  VirtualDataStoreZabbix::getInstance();
	return dataStore->getItemTable(itemGroupId);
}

// ---------------------------------------------------------------------------
// Private static methods
// ---------------------------------------------------------------------------
SQLTableStaticInfo *
SQLProcessorZabbix::defineTable(int tableId, const char *tableName,
                                SQLTableGetFunc tableGetFunc)
{
	SQLTableStaticInfo *staticInfo = new SQLTableStaticInfo();
	staticInfo->tableId = tableId;
	staticInfo->tableName = tableName;
	staticInfo->tableGetFunc = tableGetFunc;
	m_tableNameStaticInfoMap[tableName] = staticInfo;
	return staticInfo;
}

void SQLProcessorZabbix::defineColumn(SQLTableStaticInfo *staticInfo,
                                      ItemId itemId,
                                      int tableId, const char *columnName,
                                      SQLColumnType type, size_t columnLength,
                                      bool canBeNull, SQLKeyType keyType,
                                      const char *defaultValue)
{
	size_t decFracLength = 0;
	defineColumn(staticInfo, itemId, tableId, columnName, type,
	             columnLength, decFracLength, canBeNull,
	             keyType, defaultValue);
}

void SQLProcessorZabbix::defineColumn(SQLTableStaticInfo *staticInfo,
                                      ItemId itemId,
                                      int tableId, const char *columnName,
                                      SQLColumnType type,
                                      size_t columnLength, size_t decFracLength,
                                      bool canBeNull, SQLKeyType keyType,
                                      const char *defaultValue)
{
	ColumnDefList &list =
	  const_cast<ColumnDefList &>(staticInfo->columnDefList);
	int index = list.size();
	list.push_back(ColumnDef());
	ColumnDef &columnDef = list.back();
	columnDef.itemId       = itemId;
	columnDef.tableName    = staticInfo->tableName;
	columnDef.columnName   = columnName;
	columnDef.type         = type;
	columnDef.columnLength = columnLength;
	columnDef.decFracLength = decFracLength;
	columnDef.canBeNull    = canBeNull;
	columnDef.keyType      = keyType;
	columnDef.defaultValue = defaultValue;

	ColumnNameAccessInfoMap &map = staticInfo->columnAccessInfoMap;
	ColumnAccessInfo accessInfo = {index, &columnDef};
	map[columnName] = accessInfo;
}
