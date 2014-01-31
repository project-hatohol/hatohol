/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DBClientZabbix_h
#define DBClientZabbix_h

#include "DBClient.h"
#include "DBClientHatohol.h"

class DBClientZabbix : public DBClient {
public:
	typedef void (DBClientZabbix::*AbsentItemPicker)
	               (std::vector<uint64_t> &absentHostIdVector,
	                const std::vector<uint64_t> &hostIdVector);
	typedef void (DBClientZabbix::*TableSaver)(ItemTablePtr tablePtr);

	static const int ZABBIX_DB_VERSION;
	static const uint64_t EVENT_ID_NOT_FOUND;
	static const int TRIGGER_CHANGE_TIME_NOT_FOUND;

	static void init(void);
	static DBDomainId getDBDomainId(const ServerIdType zabbixServerId);
	static void transformEventsToHatoholFormat(
	  EventInfoList &eventInfoList, const ItemTablePtr events,
	  const ServerIdType &serverId);
	static bool transformEventItemGroupToEventInfo(EventInfo &eventInfo,
	                                               const ItemGroup *event);
	static void transformItemsToHatoholFormat(ItemInfoList &eventInfoList,
	                                          const ItemTablePtr events,
	                                          const ServerIdType &serverId);
	static bool transformItemItemGroupToItemInfo(ItemInfo &itemInfo,
	                                             const ItemGroup *item,
	                                             DBClientZabbix &dbZabbix);
	static void transformGroupItemGroupToHostgroupInfo(HostgroupInfo &groupInfo,
	                                               const ItemGroup *groupItemGroup);
	static void transformGroupsToHatoholFormat(HostgroupInfoList &groupInfoList,
	                                           const ItemTablePtr groups,
	                                           uint32_t serverId);
	static void transformHostsGroupsItemGroupToHatoholFormat
	  (HostgroupElement &hostgroupElement,
	   const ItemGroup *groupHostsGroups);
	static void transformHostsGroupsToHatoholFormat
	  (HostgroupElementList &hostgroupElementList,
	   const ItemTablePtr mapHostHostgroups,
	   uint32_t serverId);
	static void transformHostsItemGroupToHatoholFormat(HostInfo &hostInfo,
	                                                   const ItemGroup *groupHosts);
	static void transformHostsToHatoholFormat(HostInfoList &hostInfoList,
	                                          const ItemTablePtr hosts,
	                                          uint32_t serverId);

	/**
	 * create a DBClientZabbix instance.
	 *
	 * Different from other DBClient sub classes, instances of this class 
	 * have to be created with the factory function.
	 *
	 * @param A server ID of the zabbix server.
	 * @return A new DBclientZabbix instance.
	 */
	static DBClientZabbix *create(const ServerIdType zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);
	void addItemsRaw2_0(ItemTablePtr tablePtr);
	void addHostsRaw2_0(ItemTablePtr tablePtr);
	void addEventsRaw2_0(ItemTablePtr tablePtr);
	void addApplicationsRaw2_0(ItemTablePtr tablePtr);
	void addGroupsRaw2_0(ItemTablePtr tablePtr);
	void addHostsGroupsRaw2_0(ItemTablePtr tablePtr);

	void getTriggersAsHatoholFormat(TriggerInfoList &triggerInfoList);
	void getEventsAsHatoholFormat(EventInfoList &eventInfoList);

	/**
	 * get the last (biggest) event ID in the database.
	 * @return
	 * The last event ID. If the database has no event data,
	 * EVENT_ID_NOT_FOUND is returned.
	 */
	uint64_t getLastEventId(void);

	/**
	 * get the last trigger change time in the database.
	 * @return
	 * The last change time. If the database has no event data,
	 * TRIGGER_CHANGE_TIME_NOT_FOUND is returned.
	 */
	int getTriggerLastChange(void);

	std::string getApplicationName(uint64_t applicationId);

	void pickupAbsentHostIds(std::vector<uint64_t> &absentHostIdVector,
	                         const std::vector<uint64_t> &hostIdVector);
	void pickupAbsentApplcationIds(
	  std::vector<uint64_t> &absentAppIdVector,
	  const std::vector<uint64_t> &appIdVector);

protected:
	static std::string getDBName(const ServerIdType zabbixServerId);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static void updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data);
	static std::string makeItemBrief(const ItemGroup *itemItemGroup);

	/**
	 * check if the given word is a variable (e.g. $1, $2, ...).
	 * @params word.
	 * A word to be checked.
	 * @return 
	 * A variable number if the given word is a variable. Otherwise
	 * -1 is returned.
	 */
	static int  getItemVariable(const std::string &word);
	static void extractItemKeys(mlpl::StringVector &params,
	                            const std::string &key);

	DBClientZabbix(const ServerIdType zabbixServerId);
	void addItems(
	  ItemTablePtr tablePtr, const std::string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs,
	  int updateCheckIndex);
	void makeSelectExArgForTriggerAsHatoholFormat(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
