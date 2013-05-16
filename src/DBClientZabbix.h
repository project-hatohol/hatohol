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

#ifndef DBClientZabbix_h
#define DBClientZabbix_h

#include "DBClient.h"
#include "DBClientAsura.h"

class DBClientZabbix : public DBClient {
public:
	static const int ZABBIX_DB_VERSION;
	static const uint64_t EVENT_ID_NOT_FOUND;

	static void init(void);
	static void reset(void);
	static DBDomainId getDBDomainId(int zabbixServerId);
	static void resetDBInitializedFlags(void);
	static void transformEventsToAsuraFormat(EventInfoList &eventInfoList,
	                                         const ItemTablePtr events,
	                                         uint32_t serverId);
	static bool transformEventItemGroupToEventInfo(EventInfo &eventInfo,
	                                               const ItemGroup *event);
	static void transformItemsToAsuraFormat(ItemInfoList &eventInfoList,
	                                        const ItemTablePtr events,
	                                        uint32_t serverId);
	static bool transformItemItemGroupToItemInfo(ItemInfo &itemInfo,
	                                             const ItemGroup *item);

	DBClientZabbix(size_t zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);
	void addItemsRaw2_0(ItemTablePtr tablePtr);
	void addHostsRaw2_0(ItemTablePtr tablePtr);
	void addEventsRaw2_0(ItemTablePtr tablePtr);
	void addApplicationRaw2_0(ItemTablePtr tablePtr);

	void getTriggersAsAsuraFormat(TriggerInfoList &triggerInfoList);
	void getEventsAsAsuraFormat(EventInfoList &eventInfoList);

	/**
	 * get the last (biggest) event ID in the database.
	 * @return
	 * The last event ID. If the database has no event data,
	 * EVENT_ID_NOT_FOUND is returned.
	 */
	uint64_t getLastEventId(void);

protected:
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	static void updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data);

	void prepareSetupFuncCallback(size_t zabbixServerId);
	void addItems(
	  ItemTablePtr tablePtr, const string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs);
	void makeSelectExArgForTriggerAsAsuraFormat(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
