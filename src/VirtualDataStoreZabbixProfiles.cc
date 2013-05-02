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

#include "VirtualDataStoreZabbix.h"
#include "ItemGroupEnum.h"
#include "ItemEnum.h"
#include "VirtualDataStoreZabbixMacro.h"

static void register01(ItemTable *table);
static void register02(ItemTable *table);
static void register03(ItemTable *table);
static void register04(ItemTable *table);
static void register05(ItemTable *table);
static void register06(ItemTable *table);
static void register07(ItemTable *table);
static void register08(ItemTable *table);
static void register09(ItemTable *table);
static void register10(ItemTable *table);
static void register11(ItemTable *table);

void VirtualDataStoreZabbix::registerProfiles(ItemTable *table)
{
	register01(table);
	register02(table);
	register03(table);
	register04(table);
	register05(table);
	register06(table);
	register07(table);
	register08(table);
	register09(table);
	register10(table);
	register11(table);
}

void register01(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 1));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.view.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "tr_status.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 2));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.paging.lastpage"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "tr_status.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 3));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.cm.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "hostinventoriesoverview.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 4));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostinventoriesoverview.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "host_count"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 5));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostinventoriesoverview.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 6));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.cm.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 7));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 8));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discovery.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ip"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 9));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discovery.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 10));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.druleid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);
}

void register02(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 11));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.druleid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 12));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 13));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.reports.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "report1.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 14));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.admin.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "adm.gui.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 15));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.proxies.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "host"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.proxies.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 17));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.paging.page"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 18));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.menu.config.last"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "templates.php"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 19));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostgroups.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 20));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hostgroups.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register03(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 21));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hosts.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 22));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.hosts.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 23));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.config.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  4));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 24));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.overview.view.style"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 25));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.overview.type"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 26));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.httpmon.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "wt.name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 27));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.httpmon.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 28));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "i.name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 29));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 30));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.slideconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register04(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 31));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.slideconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 32));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 33));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 34));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screenconf.config"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 35));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discoveryconf.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 36));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.discoveryconf.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 37));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.usergrps.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 38));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.usergrps.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 39));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "alias"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 40));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register05(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 41));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.users.filter.usrgrpid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 42));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.tr_status.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "lastchange"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 43));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.tr_status.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "DESC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 44));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.events.source"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 45));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 46));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 47));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    21600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 48));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229103154"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 49));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23298));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register06(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 50));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    7200));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 51));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229162033"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 52));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23316));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 53));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.history.filter.state"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 54));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    86400));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 55));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121228162426"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 56));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23299));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 57));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.avail_report.config"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 58));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.reports.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 59));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.reports.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);
}

void register07(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 60));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 61));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004045746"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 62));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23310));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 63));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.view.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  524));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 64));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.latest.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  524));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 65));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 66));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004113120"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 67));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      523));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 68));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.graphid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  525));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 69));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);
}

void register08(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 70));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152939"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 71));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      524));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 72));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.graphs.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 73));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.graphs.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 74));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.config.hostid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  10084));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 75));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 76));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121245"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 77));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23307));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 78));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 79));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121257"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register09(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 80));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23308));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 81));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 82));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121323"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 83));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23335));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 84));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 85));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004121344"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 86));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23342));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 87));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 88));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004125509"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 89));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23338));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register10(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 90));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 91));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121004150157"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 92));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23337));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 93));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 94));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152759"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 95));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.item.graph.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      23274));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 96));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.elementid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  16));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 97));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.period"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    3600));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         2));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 98));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.stime"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "20121229152943"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 99));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.screens.isnow"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      16));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "1"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);
}

void register11(ItemTable *table)
{
	VariableItemGroupPtr grp;
	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 100));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.sort"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "name"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 101));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.sortorder"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, "ASC"));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         3));
	table->add(grp);

	grp = VariableItemGroupPtr();
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, 102));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    1));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, "web.templates.php.groupid"));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      0));
	ADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  0));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    0));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, ""));
	ADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    ""));
	ADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         1));
	table->add(grp);
}

