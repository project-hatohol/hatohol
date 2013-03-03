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

#include <stdexcept>

#include "Utils.h"
#include "VirtualDataStoreZabbix.h"
#include "ItemGroupEnum.h"
#include "ItemEnum.h"

#define ADD(X) grp->add(X, false)

#define ADD_AS_NULL(X) \
do { \
  grp->add(X, false); \
  size_t num = grp->getNumberOfItems(); \
  grp->getItemAt(num-1)->setNull(); \
} while (0)

GMutex VirtualDataStoreZabbix::m_mutex;
VirtualDataStoreZabbix *VirtualDataStoreZabbix::m_instance = NULL;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix *VirtualDataStoreZabbix::getInstance(void)
{
	if (m_instance)
		return m_instance;

	g_mutex_lock(&m_mutex);
	if (!m_instance)
		m_instance = new VirtualDataStoreZabbix();
	g_mutex_unlock(&m_mutex);

	return m_instance;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
const ItemTablePtr
VirtualDataStoreZabbix::getItemTable(ItemGroupId groupId) const
{
	ItemGroupIdTableMapConstIterator it;
	ItemTable *table = NULL;
	m_staticItemTableMapLock.readLock();
	it = m_staticItemTableMap.find(groupId);
	if (it != m_staticItemTableMap.end())
		table = it->second;
	m_staticItemTableMapLock.readUnlock();
	return ItemTablePtr(table);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTable *VirtualDataStoreZabbix::createStaticItemTable(ItemGroupId groupId)
{
	pair<ItemGroupIdTableMapIterator, bool> result;
	ItemTable *table = new ItemTable();

	m_staticItemTableMapLock.writeLock();;
	result = m_staticItemTableMap.insert
	         (pair<ItemGroupId, ItemTable *>(groupId, table));
	m_staticItemTableMapLock.writeUnlock();;
	if (!result.second) {
		string msg;
		TRMSG(msg, "Failed: insert: groupId: %"PRIx_ITEM_GROUP". "
		           "The element with the same ID may exisit.",
		      groupId);
		throw invalid_argument(msg);
	}
	return table;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix::VirtualDataStoreZabbix(void)
{
	ItemTable *table;
	ItemGroup *grp;

	//
	// nodes
	//
	table = createStaticItemTable(GROUP_ID_ZBX_NODES);

	//
	// config
	//
	table = createStaticItemTable(GROUP_ID_ZBX_CONFIG);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_CONFIG_CONFIGID, 1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_ALERT_HISTORY, 365));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_HISTORY, 365));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED, 600));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_WORK_PERIOD, "1-5,09:00-18:00;"));
	ADD(new ItemUint64(ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID, 7));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE, 1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,     7));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,   100));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_DEFAULT_THEME, "originalblue"));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,   0));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_HOST, ""));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_LDAP_PORT, 389));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN, ""));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN, ""));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,    ""));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE, ""));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,    1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER, 1));
	ADD(new ItemUint64(ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID, 5));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_MAX_INTABLE,          50));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,        100));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0, "DBDBDB"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1, "D6F6FF"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2, "FFF6A5"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3, "FFB689"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4, "FF9999"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5, "FF3838"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0, "Not classified"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1, "Information"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2, "Warning"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3, "Average"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4, "High"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5, "Disaster"));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_PERIOD,    1800));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_BLINK_PERIOD, 1800));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR, "DC0000"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,   "DC0000"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR, "00AA00"));
	ADD(new ItemString(ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,   "00AA00"));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE, 1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,   1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,      1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,        1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,    1));
	ADD(new ItemInt(ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL, 10));

	//
	// users
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USERS);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_USERID,  1));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_ALIAS,   "Admin"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_NAME,    "Zabbix"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_SURNAME, "Administrator"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_PASSWD,  "5fce1b3e34b520afeffb37ce08c7cd66"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_URL,     ""));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_AUTOLOGIN,  1));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_AUTOLOGOUT, 0));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_LANG,    "en_GB"));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_REFRESH,    30));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_TYPE,       3));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_THEME,   "default"));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ATTEMPT_FAILED, 0));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_ATTEMPT_IP,  ""));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,  0 ));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,  50 ));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_USERID,  2));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_ALIAS,   "guest"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_NAME,    "Default"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_SURNAME, "User"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_PASSWD,  "d41d8cd98f00b204e9800998ecf8427e"));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_URL,     ""));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_AUTOLOGIN,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_AUTOLOGOUT, 900));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_LANG,    "en_GB"));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_REFRESH,    30));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_TYPE,       1));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_THEME,   "default"));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ATTEMPT_FAILED, 0));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_ATTEMPT_IP,  ""));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ATTEMPT_CLOCK,  0 ));
	ADD(new ItemInt(ITEM_ID_ZBX_USERS_ROWS_PER_PAGE,  50 ));

	//
	// usrgrp
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USRGRP);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  7));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Zabbix administrators"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  8));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Guests"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  9));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Disabled"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 1));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  11));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Enabled debug mode"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  11));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "No access to the frontend"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   2));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));

	//
	// users_groups
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USERS_GROUPS);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_ID,       4));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USRGRPID, 7));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USERID,   1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_ID,       2));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USRGRPID, 8));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USERID,   2));

	//
	// sessions
	//
	table = createStaticItemTable(GROUP_ID_ZBX_SESSIONS);

	//
	// profiles
	//
	table = createStaticItemTable(GROUP_ID_ZBX_PROFILES);
	registerProfiles(table);

	//
	// user_history
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USER_HISTORY);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_USER_HISTORY_USERHISTORYID, 1));
	ADD(new ItemUint64(ITEM_ID_ZBX_USER_HISTORY_USERID,        1));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_TITLE1, "History"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_URL1,
	                   "history.php?itemid[0]=23316&action=showgraph"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_TITLE2, "Dashboard"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_URL2, "dashboard.php"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_TITLE3,
	                   "Configuration of templates"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_URL3,
	                   "templates.php?groupid=0"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_TITLE4, "Dashboard"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_URL4, "dashboard.php"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_TITLE5,
	                   "Status of triggers"));
	ADD(new ItemString(ITEM_ID_ZBX_USER_HISTORY_URL5,
	                   "tr_status.php?groupid=0&hostid=0"));

	//
	// triggers
	//
	table = createStaticItemTable(GROUP_ID_ZBX_TRIGGERS);

	//
	// functions
	//
	table = createStaticItemTable(GROUP_ID_ZBX_FUNCTIONS);

	//
	// items
	//
	table = createStaticItemTable(GROUP_ID_ZBX_ITEMS);

	//
	// hosts
	//
	table = createStaticItemTable(GROUP_ID_ZBX_HOSTS);

	//
	// groups
	//
	table = createStaticItemTable(GROUP_ID_ZBX_GROUPS);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  1));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Templates"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  1));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Linux servers"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  4));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Zabbix servers"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  5));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Discovered hosts"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));

	//
	// hosts_groups
	//
	table = createStaticItemTable(GROUP_ID_ZBX_HOSTS_GROUPS);

	//
	// trigger_depends
	//
	table = createStaticItemTable(GROUP_ID_ZBX_TRIGGER_DEPENDS);

	//
	// events
	//
	table = createStaticItemTable(GROUP_ID_ZBX_EVENTS);

	//
	// scripts
	//
	table = createStaticItemTable(GROUP_ID_ZBX_SCRIPTS);
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_SCRIPTID,     1));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_NAME,         "Ping"));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_COMMAND,
	                   "/bin/ping -c 3 {HOST.CONN} 2>&1"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_HOST_ACCESS,  2));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_USRGRPID, 0));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_GROUPID,  0));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_DESCRIPTION,  ""));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_CONFIRMATION, ""));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_TYPE,         0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_EXECUTE_ON,   1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_SCRIPTID,     2));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_NAME,         "Traceroute"));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_COMMAND,
	                   "/usr/bin/traceroute {HOST.CONN} 2>&1"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_HOST_ACCESS,  2));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_USRGRPID, 0));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_GROUPID,  0));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_DESCRIPTION,  ""));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_CONFIRMATION, ""));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_TYPE,         0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_EXECUTE_ON,   1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_SCRIPTID,     3));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_NAME,
	                   "Detect operating system"));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_COMMAND,
	                   "sudo /usr/bin/nmap -O {HOST.CONN} 2>&1"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_HOST_ACCESS,  2));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_USRGRPID,     7));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCRIPTS_GROUPID, 0));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_DESCRIPTION,  ""));
	ADD(new ItemString(ITEM_ID_ZBX_SCRIPTS_CONFIRMATION, ""));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_TYPE,         0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCRIPTS_EXECUTE_ON,   1));

	//
	// host_inventory
	//
	table = createStaticItemTable(GROUP_ID_ZBX_HOST_INVENTORY);

}

VirtualDataStoreZabbix::~VirtualDataStoreZabbix()
{
}

void VirtualDataStoreZabbix::registerProfiles(ItemTable *table)
{
	ItemGroup *grp;
	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 1));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.view.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "tr_status.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 2));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.paging.lastpage"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "tr_status.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 3));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.cm.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "hostinventoriesoverview.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 4));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostinventoriesoverview.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "host_count"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 5));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostinventoriesoverview.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 6));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.cm.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 7));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 8));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discovery.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ip"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 9));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discovery.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 10));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.druleid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 11));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.druleid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 12));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 13));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.reports.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "report1.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 14));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.admin.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "adm.gui.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 15));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.proxies.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "host"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.proxies.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 17));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.paging.page"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 18));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.config.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "templates.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 19));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostgroups.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 20));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostgroups.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 21));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hosts.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 22));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hosts.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 23));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.config.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  4));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 24));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.overview.view.style"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 25));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.overview.type"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 26));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.httpmon.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "wt.name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 27));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.httpmon.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 28));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "i.name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 29));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 30));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.slideconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 31));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.slideconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 32));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 33));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 34));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.config"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 35));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discoveryconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 36));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discoveryconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 37));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.usergrps.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 38));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.usergrps.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 39));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "alias"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 40));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 41));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.filter.usrgrpid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 42));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.tr_status.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "lastchange"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 43));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.tr_status.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 44));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.events.source"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 45));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 46));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 47));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    21600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 48));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229103154"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 49));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 50));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    7200));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 51));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229162033"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 52));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 53));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.history.filter.state"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 54));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    86400));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 55));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121228162426"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 56));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 57));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.avail_report.config"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 58));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.reports.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 59));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.reports.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 60));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 61));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004045746"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 62));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 63));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  524));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 64));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  524));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 65));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 66));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004113120"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 67));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 68));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  525));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 69));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 70));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152939"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 71));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 72));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.graphs.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 73));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.graphs.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 74));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.config.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  10084));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 75));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 76));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121245"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 77));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 78));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 79));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121257"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 80));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 81));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 82));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121323"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 83));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 84));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 85));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121344"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 86));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 87));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 88));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004125509"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 89));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 90));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 91));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004150157"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 92));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 93));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 94));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152759"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 95));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 96));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.elementid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  16));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 97));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 98));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152943"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 99));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 100));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 101));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));

	grp = table->addNewGroup();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 102));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
}
