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

#ifndef DBClientAsura_h
#define DBClientAsura_h

#include <list>
#include "DBClient.h"

enum TriggerStatusType {
	TRIGGER_STATUS_OK,
	TRIGGER_STATUS_PROBLEM,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_INFO,
	TRIGGER_SEVERITY_WARN,
	TRIGGER_SEVERITY_CRITICAL,
	TRIGGER_SEVERITY_UNKNOWN,
};

struct TriggerInfo {
	uint32_t            serverId;
	uint64_t            id;
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangeTime;
	uint64_t            hostId;
	string              hostName;
	string              brief;
};

typedef list<TriggerInfo>               TriggerInfoList;
typedef TriggerInfoList::iterator       TriggerInfoListIterator;
typedef TriggerInfoList::const_iterator TriggerInfoListConstIterator;

enum EventType {
	TRIGGER_ACTIVATED,
	TRIGGER_DEACTIVATED,
};

struct EventInfo {
	uint32_t            serverId;
	uint64_t            id;
	timespec            time;
	EventType           type;
	uint64_t            triggerId;

	// TODO: The member 'triggerInfo' will be removed,
	//       after we modify the way to process ZABBIX events.
	TriggerInfo         triggerInfo;

	// status and type are basically same information.
	// so they should be unified.
	TriggerStatusType   status;
	TriggerSeverityType severity;
	uint64_t            hostId;
	string              hostName;
	string              brief;
};

typedef list<EventInfo>               EventInfoList;
typedef EventInfoList::iterator       EventInfoListIterator;
typedef EventInfoList::const_iterator EventInfoListConstIterator;

struct ItemInfo {
	uint32_t            serverId;
	uint64_t            id;
	uint64_t            hostId;
	string              brief;
	timespec            lastValueTime;
	string              lastValue;
	string              prevValue;
	string              itemGroupName;
};

typedef list<ItemInfo>               ItemInfoList;
typedef ItemInfoList::iterator       ItemInfoListIterator;
typedef ItemInfoList::const_iterator ItemInfoListConstIterator;

class DBClientAsura : public DBClient {
public:
	static int ASURA_DB_VERSION;
	static void init(void);
	static void reset(void);

	DBClientAsura(void);
	virtual ~DBClientAsura();

	void addTriggerInfo(TriggerInfo *triggerInfo);
	void getTriggerInfoList(TriggerInfoList &triggerInfoList);
	void setTriggerInfoList(const TriggerInfoList &triggerInfoList,
	                        uint32_t serverId);
	/**
	 * get the last change time of the trigger that belongs to
	 * the specified server
	 * @param serverId A target server ID.
	 * @return
	 * The last change time of tringer in unix time. If there is no
	 * trigger information, 0 is returned.
	 */
	int  getLastChangeTimeOfTrigger(uint32_t serverId);

	void addEventInfo(EventInfo *eventInfo);
	void addEventInfoList(const EventInfoList &eventInfoList);
	void getEventInfoList(EventInfoList &eventInfoList);
	void setEventInfoList(const EventInfoList &eventInfoList,
	                      uint32_t serverId);

	void addItemInfo(ItemInfo *itemInfo);
	void addItemInfoList(const ItemInfoList &itemInfoList);
	void getItemInfoList(ItemInfoList &itemInfoList);

protected:
	static void resetDBInitializedFlags(void);
	void prepareSetupFunction(void);
	void addTriggerInfoBare(const TriggerInfo &triggerInfo);
	void addEventInfoBare(const EventInfo &eventInfo);
	void addItemInfoBare(const ItemInfo &eventInfo);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientAsura_h
