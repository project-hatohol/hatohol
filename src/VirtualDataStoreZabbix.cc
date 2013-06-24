/* Hatohol
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
#include "VirtualDataStoreZabbixMacro.h"
#include "DBClientConfig.h"
#include "DBClientHatohol.h"

MutexLock    VirtualDataStoreZabbix::m_mutex;
VirtualDataStoreZabbix *VirtualDataStoreZabbix::m_instance = NULL;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix *VirtualDataStoreZabbix::getInstance(void)
{
	if (m_instance)
		return m_instance;

	m_mutex.lock();
	if (!m_instance)
		m_instance = new VirtualDataStoreZabbix();
	m_mutex.unlock();

	return m_instance;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTablePtr VirtualDataStoreZabbix::getItemTable(ItemGroupId groupId)
{
	// search from DataGenerators
	DataGeneratorMapIterator generatorIt = 
	  m_dataGeneratorMap.find(groupId);
	if (generatorIt != m_dataGeneratorMap.end()) {
		DataGenerator generator = generatorIt->second;
		return (this->*generator)();
	}

	// search from built-in on memory database
	ItemGroupIdTableMapConstIterator it;
	ItemTable *table = NULL;
	m_staticItemTableMapLock.readLock();
	it = m_staticItemTableMap.find(groupId);
	if (it != m_staticItemTableMap.end())
		table = it->second;
	m_staticItemTableMapLock.unlock();
	return ItemTablePtr(table);
}

void VirtualDataStoreZabbix::start(void)
{
	VirtualDataStore::start<DataStoreZabbix>(MONITORING_SYSTEM_ZABBIX);
}

void VirtualDataStoreZabbix::getTriggerList(TriggerInfoList &triggerList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList);
}

void VirtualDataStoreZabbix::getEventList(EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getEventInfoList(eventList);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTable *VirtualDataStoreZabbix::createStaticItemTable(ItemGroupId groupId)
{
	pair<ItemGroupIdTableMapIterator, bool> result;
	ItemTable *table = new ItemTable();
	m_staticItemTableMapLock.writeLock();
	result = m_staticItemTableMap.insert
	         (pair<ItemGroupId, ItemTable *>(groupId, table));
	m_staticItemTableMapLock.unlock();
	if (!result.second) {
		string msg;
		TRMSG(msg, "Failed: insert: groupId: %"PRIx_ITEM_GROUP". "
		           "The element with the same ID may exisit.",
		      groupId);
		throw invalid_argument(msg);
	}
	return table;
}

ItemTablePtr VirtualDataStoreZabbix::getTriggers(void)
{
	// TODO: merge data of all stores. Now just call a stub function.
	DataStoreVector &dataStoreVect = getDataStoreVector();
	for (size_t i = 0; i < dataStoreVect.size(); i++) {
		DataStore *dataStore = dataStoreVect[i];
		DataStoreZabbix *dataStoreZabbix = 
		  dynamic_cast<DataStoreZabbix *>(dataStore);
		// return on the first time (here is an experimental)
		return dataStoreZabbix->getTriggers();
	}
	return ItemTablePtr();
}

ItemTablePtr VirtualDataStoreZabbix::getFunctions(void)
{
	// TODO: merge data of all stores. Now just call a stub function.
	DataStoreVector &dataStoreVect = getDataStoreVector();
	for (size_t i = 0; i < dataStoreVect.size(); i++) {
		DataStore *dataStore = dataStoreVect[i];
		DataStoreZabbix *dataStoreZabbix = 
		  dynamic_cast<DataStoreZabbix *>(dataStore);
		// return on the first time (here is an experimental)
		return dataStoreZabbix->getFunctions();
	}
	THROW_HATOHOL_EXCEPTION("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemTablePtr();
}

ItemTablePtr VirtualDataStoreZabbix::getItems(void)
{
	// TODO: merge data of all stores. Now just call a stub function.
	DataStoreVector &dataStoreVect = getDataStoreVector();
	for (size_t i = 0; i < dataStoreVect.size(); i++) {
		DataStore *dataStore = dataStoreVect[i];
		DataStoreZabbix *dataStoreZabbix = 
		  dynamic_cast<DataStoreZabbix *>(dataStore);
		// return on the first time (here is an experimental)
		return dataStoreZabbix->getItems();
	}
	THROW_HATOHOL_EXCEPTION("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemTablePtr();
}

ItemTablePtr VirtualDataStoreZabbix::getHosts(void)
{
	// TODO: merge data of all stores. Now just call a stub function.
	DataStoreVector &dataStoreVect = getDataStoreVector();
	for (size_t i = 0; i < dataStoreVect.size(); i++) {
		DataStore *dataStore = dataStoreVect[i];
		DataStoreZabbix *dataStoreZabbix = 
		  dynamic_cast<DataStoreZabbix *>(dataStore);
		// return on the first time (here is an experimental)
		return dataStoreZabbix->getHosts();
	}
	THROW_HATOHOL_EXCEPTION("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemTablePtr();
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix::VirtualDataStoreZabbix(void)
{
	ItemTable *table;
	VariableItemGroupPtr grp;

	//
	// nodes
	//
	table = createStaticItemTable(GROUP_ID_ZBX_NODES);

	//
	// config
	//
	table = createStaticItemTable(GROUP_ID_ZBX_CONFIG);
	grp = VariableItemGroupPtr();
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
	table->add(grp);

	//
	// users
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USERS);
	grp = VariableItemGroupPtr();
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
	table->add(grp);

	grp = VariableItemGroupPtr();
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
	table->add(grp);

	//
	// usrgrp
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USRGRP);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  7));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Zabbix administrators"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  8));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Guests"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  9));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Disabled"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 1));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  11));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "Enabled debug mode"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USRGRP_USRGRPID,  12));
	ADD(new ItemString(ITEM_ID_ZBX_USRGRP_NAME, "No access to the frontend"));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_GUI_ACCESS,   2));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_USERS_STATUS, 0));
	ADD(new ItemInt(ITEM_ID_ZBX_USRGRP_DEBUG_MODE,   0));
	table->add(grp);

	//
	// users_groups
	//
	table = createStaticItemTable(GROUP_ID_ZBX_USERS_GROUPS);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_ID,       4));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USRGRPID, 7));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USERID,   1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_ID,       2));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USRGRPID, 8));
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_GROUPS_USERID,   2));
	table->add(grp);

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
	grp = VariableItemGroupPtr();
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
	table->add(grp);

	//
	// triggers
	//
	m_dataGeneratorMap[GROUP_ID_ZBX_TRIGGERS] = 
	  &VirtualDataStoreZabbix::getTriggers;

	//
	// functions
	//
	m_dataGeneratorMap[GROUP_ID_ZBX_FUNCTIONS] = 
	  &VirtualDataStoreZabbix::getFunctions;

	//
	// items
	//
	m_dataGeneratorMap[GROUP_ID_ZBX_ITEMS] = 
	  &VirtualDataStoreZabbix::getItems;

	//
	// hosts
	//
	m_dataGeneratorMap[GROUP_ID_ZBX_HOSTS] = 
	  &VirtualDataStoreZabbix::getHosts;

	//
	// groups
	//
	table = createStaticItemTable(GROUP_ID_ZBX_GROUPS);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  1));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Templates"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  2));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Linux servers"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  4));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Zabbix servers"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 0));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_GROUPS_GROUPID,  5));
	ADD(new ItemString(ITEM_ID_ZBX_GROUPS_NAME,     "Discovered hosts"));
	ADD(new ItemInt   (ITEM_ID_ZBX_GROUPS_INTERNAL, 1));
	table->add(grp);

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
	grp = VariableItemGroupPtr();
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
	table->add(grp);

	grp = VariableItemGroupPtr();
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
	table->add(grp);

	grp = VariableItemGroupPtr();
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
	table->add(grp);

	//
	// host_inventory
	//
	table = createStaticItemTable(GROUP_ID_ZBX_HOST_INVENTORY);

	//
	// rights
	//
	table = createStaticItemTable(GROUP_ID_ZBX_RIGHTS);

	//
	// screens
	//
	table = createStaticItemTable(GROUP_ID_ZBX_SCREENS);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     3));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        2));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10001));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     4));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "Zabbix server health"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        2));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10047));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     5));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        1));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10076));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     6));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        1));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10077));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     7));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        1));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10075));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     9));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        2));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10074));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     10));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "System performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        1));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10078));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     15));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "MySQL performance"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        1));
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID,   10073));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SCREENS_SCREENID,     16));
	ADD(new ItemString(ITEM_ID_ZBX_SCREENS_NAME, "Zabbix server"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_HSIZE,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SCREENS_VSIZE,        2));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SCREENS_TEMPLATEID, 0));
	table->add(grp);

	//
	// graphs
	//
	table = createStaticItemTable(GROUP_ID_ZBX_GRAPHS);

	//
	// graphs_items
	//
	table = createStaticItemTable(GROUP_ID_ZBX_GRAPHS_ITEMS);

	//
	// sysmaps
	//
	table = createStaticItemTable(GROUP_ID_ZBX_SYSMAPS);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_SYSMAPS_SYSMAPID,     1));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_NAME, "Local network"));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_WIDTH,        680));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_HEIGHT,       200));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SYSMAPS_BACKGROUNDID, 0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE,     0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_LOCATION, 0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_HIGHLIGHT,      1));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_EXPANDPROBLEM,  1));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_MARKELEMENTS,   1));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_SHOW_UNACK,     0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_GRID_SIZE,      50));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_GRID_SHOW,      1));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_GRID_ALIGN,     1));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_FORMAT,   0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE_HOST,        2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE_HOSTGROUP,   2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE_TRIGGER,     2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE_MAP,         2));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_LABEL_TYPE_IMAGE,       2));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_LABEL_STRING_HOST,      ""));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_LABEL_STRING_HOSTGROUP, ""));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_LABEL_STRING_TRIGGER,   ""));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_LABEL_STRING_MAP,       ""));
	ADD(new ItemString(ITEM_ID_ZBX_SYSMAPS_LABEL_STRING_IMAGE,     ""));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_SYSMAPS_ICONMAPID, 0));
	ADD(new ItemInt   (ITEM_ID_ZBX_SYSMAPS_EXPAND_MACROS,          1));
	table->add(grp);

	//
	// sysmap_url
	//
	table = createStaticItemTable(GROUP_ID_ZBX_SYSMAP_URL);

	//
	// drules
	//
	table = createStaticItemTable(GROUP_ID_ZBX_DRULES);
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_DRULES_DRULEID,   2));
	ADD_AS_NULL(new ItemUint64(ITEM_ID_ZBX_DRULES_PROXY_HOSTID, 0));
	ADD(new ItemString(ITEM_ID_ZBX_DRULES_NAME,      "Local network"));
	ADD(new ItemString(ITEM_ID_ZBX_DRULES_IPRANGE,   "192.168.1.1-255"));
	ADD(new ItemInt   (ITEM_ID_ZBX_DRULES_DELAY,     3600));
	ADD(new ItemInt   (ITEM_ID_ZBX_DRULES_NEXTCHECK, 0));
	ADD(new ItemInt   (ITEM_ID_ZBX_DRULES_STATUS,    1));
	table->add(grp);
}

VirtualDataStoreZabbix::~VirtualDataStoreZabbix()
{
}
