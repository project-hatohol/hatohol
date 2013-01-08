#include <stdexcept>

#include "Utils.h"
#include "VirtualDataStoreZabbix.h"
#include "ItemGroupEnum.h"
#include "ItemEnum.h"

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
const ItemTable *VirtualDataStoreZabbix::getItemTable(ItemGroupId groupId) const
{
	ItemGroupIdTableMapConstIterator it;
	ItemTable *table = NULL;
	m_staticItemTableMapLock.readLock();
	it = m_staticItemTableMap.find(groupId);
	if (it != m_staticItemTableMap.end())
		table = it->second;
	m_staticItemTableMapLock.readUnlock();
	return table;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTable *VirtualDataStoreZabbix::createStaticItemTable(ItemGroupId groupId)
{
	pair<ItemGroupIdTableMapIterator, bool> result;
	ItemTable *table = new ItemTable(groupId);

	m_staticItemTableMapLock.writeLock();;
	result = m_staticItemTableMap.insert
	         (pair<ItemGroupId, ItemTable *>(groupId, table));
	m_staticItemTableMapLock.writeUnlock();;
	if (!result.second) {
		string msg =
		  AMSG("Failed: insert: groupId: %"PRIx_ITEM_GROUP". "
		       "The element with the same ID may exisit.\n",
		       groupId);
		throw invalid_argument(msg);
	}
	return table;
}

ItemGroup *VirtualDataStoreZabbix::createStaticItemGroup(ItemTable *itemTable)
{
	ItemGroupId groupId = itemTable->getItemGroupId();
	ItemGroup *grp = new ItemGroup(groupId);
	itemTable->add(grp, false);
	return grp;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix::VirtualDataStoreZabbix(void)
{
	ItemTable *table;
	ItemGroup *grp;

#define ADD(X) grp->add(X, false)
	table = createStaticItemTable(GROUP_ID_ZBX_NODES);

	table = createStaticItemTable(GROUP_ID_ZBX_CONFIG);
	grp = createStaticItemGroup(table);
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

	table = createStaticItemTable(GROUP_ID_ZBX_USERS);
	grp = createStaticItemGroup(table);
	ADD(new ItemUint64(ITEM_ID_ZBX_USERS_USERID, 1));
	ADD(new ItemString(ITEM_ID_ZBX_USERS_ALIAS, "Admin"));
#undef ADD
}

VirtualDataStoreZabbix::~VirtualDataStoreZabbix()
{
}

