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

#include "DBClientConnectableBase.h"
#include "DBClientHatohol.h"

class DBClientZabbix : public DBClientConnectableBase {
public:
	typedef void (DBClientZabbix::*AbsentItemPicker)
	               (vector<uint64_t> &absentHostIdVector,
	                const vector<uint64_t> &hostIdVector);
	typedef void (DBClientZabbix::*TableSaver)(ItemTablePtr tablePtr);

	static const int ZABBIX_DB_VERSION;
	static const uint64_t EVENT_ID_NOT_FOUND;
	static const int TRIGGER_CHANGE_TIME_NOT_FOUND;

	static void init(void);
	static DBDomainId getDBDomainId(int zabbixServerId);
	static void transformEventsToHatoholFormat(EventInfoList &eventInfoList,
	                                         const ItemTablePtr events,
	                                         uint32_t serverId);
	static bool transformEventItemGroupToEventInfo(EventInfo &eventInfo,
	                                               const ItemGroup *event);
	static void transformItemsToHatoholFormat(ItemInfoList &eventInfoList,
	                                        const ItemTablePtr events,
	                                        uint32_t serverId);
	static bool transformItemItemGroupToItemInfo(ItemInfo &itemInfo,
	                                             const ItemGroup *item,
	                                             DBClientZabbix &dbZabbix);

	DBClientZabbix(size_t zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);
	void addItemsRaw2_0(ItemTablePtr tablePtr);
	void addHostsRaw2_0(ItemTablePtr tablePtr);
	void addEventsRaw2_0(ItemTablePtr tablePtr);
	void addApplicationsRaw2_0(ItemTablePtr tablePtr);

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

	string getApplicationName(uint64_t applicationId);

	void pickupAbsentHostIds(vector<uint64_t> &absentHostIdVector,
	                         const vector<uint64_t> &hostIdVector);
	void pickupAbsentApplcationIds(vector<uint64_t> &absentAppIdVector,
	                               const vector<uint64_t> &appIdVector);

protected:
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static void updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data);
	static string makeItemBrief(const ItemGroup *itemItemGroup);

	/**
	 * check if the given word is a variable (e.g. $1, $2, ...).
	 * @params word.
	 * A word to be checked.
	 * @return 
	 * A variable number if the given word is a variable. Otherwise
	 * -1 is returned.
	 */
	static int  getItemVariable(const string &word);
	static void extractItemKeys(StringVector &params, const string &key);

	void addItems(
	  ItemTablePtr tablePtr, const string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs,
	  int updateCheckIndex = -1);
	void updateItems(
	  const ItemGroup *itemGroup,
	  const string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs);
	void makeSelectExArgForTriggerAsHatoholFormat(void);

	bool isRecordExisting(
	  const ItemGroup *itemGroup, const string &tableName,
	  size_t numColumns, const ColumnDef &columnCheck);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
