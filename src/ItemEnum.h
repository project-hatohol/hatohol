#ifndef ItemEnum_h
#define ItemEnum_h

enum
{
	ITEM_ID_NOT_SET,
	ITEM_ID_NOBODY,

	ITEM_ID_ZBX_NODES_NODEID,
	ITEM_ID_ZBX_NODES_NAME,
	ITEM_ID_ZBX_NODES_IP,
	ITEM_ID_ZBX_NODES_PORT,
	ITEM_ID_ZBX_NODES_NODETYPE,
	ITEM_ID_ZBX_NODES_MASTERID,

	ITEM_ID_ZBX_CONFIG_CONFIGID,
	ITEM_ID_ZBX_CONFIG_ALERT_HISTORY,
	ITEM_ID_ZBX_CONFIG_EVENT_HISTORY,
	ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED,
	ITEM_ID_ZBX_CONFIG_WORK_PERIOD,
	ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID,
	ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE,
	ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,
	ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,
	ITEM_ID_ZBX_CONFIG_DEFAULT_THEME,
	ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,
	ITEM_ID_ZBX_CONFIG_LDAP_HOST,
	ITEM_ID_ZBX_CONFIG_LDAP_PORT,
	ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN,
	ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN,
	ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,
	ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE,
	ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,
	ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER,
	ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID,
	ITEM_ID_ZBX_CONFIG_MAX_INTABLE,
	ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4,
	ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4,
	ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5,
	ITEM_ID_ZBX_CONFIG_OK_PERIOD,
	ITEM_ID_ZBX_CONFIG_BLINK_PERIOD,
	ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR,
	ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,
	ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR,
	ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,
	ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE,
	ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,
	ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,
	ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,
	ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,
	ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL,

	ITEM_ID_ZBX_USERS_USERID,
	ITEM_ID_ZBX_USERS_ALIAS,
	ITEM_ID_ZBX_USERS_NAME,
	ITEM_ID_ZBX_USERS_SURNAME,
	ITEM_ID_ZBX_USERS_PASSWD,
	ITEM_ID_ZBX_USERS_URL,
	ITEM_ID_ZBX_USERS_AUTOLOGIN,
	ITEM_ID_ZBX_USERS_AUTOLOGOUT,
	ITEM_ID_ZBX_USERS_LANG,
	ITEM_ID_ZBX_USERS_REFRESH,
	ITEM_ID_ZBX_USERS_TYPE,
	ITEM_ID_ZBX_USERS_THEME,
	ITEM_ID_ZBX_USERS_ATTEMPT_FAILED,
	ITEM_ID_ZBX_USERS_ATTEMPT_IP,
	ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,
	ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,

	ITEM_ID_ZBX_USRGRP_USRGRPID,
	ITEM_ID_ZBX_USRGRP_NAME,
	ITEM_ID_ZBX_USRGRP_GUI_ACCESS,
	ITEM_ID_ZBX_USRGRP_USERS_STATUS,
	ITEM_ID_ZBX_USRGRP_DEBUG_MODE,

	ITEM_ID_ZBX_USERS_GROUPS_ID,
	ITEM_ID_ZBX_USERS_GROUPS_USRGRPID,
	ITEM_ID_ZBX_USERS_GROUPS_USERID,

	ITEM_ID_ZBX_SESSIONS_SESSIONID,
	ITEM_ID_ZBX_SESSIONS_USERID,
	ITEM_ID_ZBX_SESSIONS_LASTACCESS,
	ITEM_ID_ZBX_SESSIONS_STATUS,
};

#endif // ItemEnum_h
