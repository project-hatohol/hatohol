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
const ItemGroup *VirtualDataStoreZabbix::getItemGroup(ItemGroupId groupId)
{
	ItemGroupMapIterator it = m_itemGroupMap.find(groupId);
	if (it == m_itemGroupMap.end())
		return NULL;
	return it->second;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup *VirtualDataStoreZabbix::createItemGroup(ItemGroupId groupId)
{
	ItemGroup *grp = new ItemGroup(groupId);
	pair<ItemGroupMapIterator, bool> result;
	result = m_itemGroupMap.insert
	         (pair<ItemGroupId, ItemGroup *>(groupId, grp));
	if (!result.second) {
		string msg =
		  AMSG("Failed: insert: groupId: %"PRIx_ITEM_GROUP"\n",
		       groupId);
		throw invalid_argument(msg);
	}
	return grp;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix::VirtualDataStoreZabbix(void)
{
	ItemGroup *grp;
	grp = createItemGroup(GROUP_ID_ZBX_CONFIG);
	grp->add(new ItemUint64(ITEM_ID_ZBX_CONFIG_CONFIGID, 1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_ALERT_HISTORY, 365));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_HISTORY, 365));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_REFRESH_UNSUPORTED, 600));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_WORK_PERIOD, "1-5,09:00-18:00;"));
	grp->add(new ItemUint64(ITEM_ID_ZBX_CONFIG_ALERT_USRGRPID, 7));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_ACK_ENABLE, 1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_EXPIRE,     7));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_EVENT_SHOW_MAX,   100));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_DEFAULT_THEME, "originalblue"));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_AUTHENTICATION_TYPE,   0));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_HOST, ""));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_LDAP_PORT, 389));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BASE_DN, ""));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BIND_DN, ""));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_BIND_PASSWORD,    ""));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_LDAP_SEARCH_ATTRIBUTE, ""));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_ENTRY,    1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_DROPDOWN_FIRST_REMEMBER, 1));
	grp->add(new ItemUint64(ITEM_ID_ZBX_CONFIG_DISCOVERY_GROUPID, 5));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_MAX_INTABLE,          50));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_SEARCH_LIMITE,        100));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_0, "DBDBDB"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_1, "D6F6FF"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_2, "FFF6A5"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_3, "FFB689"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_4, "FF9999"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_COLOR_5, "FF3838"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_0, "Not classified"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_1, "Information"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_2, "Warning"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_3, "Average"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_4, "High"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_SEVERITY_NAME_5, "Disaster"));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_PERIOD,    1800));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_BLINK_PERIOD, 1800));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_COLOR, "DC0000"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_COLOR,   "DC0000"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_OK_UNACK_COLOR, "00AA00"));
	grp->add(new ItemString(ITEM_ID_ZBX_CONFIG_OK_ACK_COLOR,   "00AA00"));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_PROBLEM_UNACK_STYLE, 1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_PROBLEM_ACK_STYLE,   1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_UNACK_STYLE,      1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_OK_ACK_STYLE,        1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_SNMPTRAP_LOGGING,    1));
	grp->add(new ItemInt(ITEM_ID_ZBX_CONFIG_SERVER_CHECK_INTERVAL, 10));
}

VirtualDataStoreZabbix::~VirtualDataStoreZabbix()
{
}

