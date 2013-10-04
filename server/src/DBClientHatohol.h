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

#ifndef DBClientHatohol_h
#define DBClientHatohol_h

#include <list>
#include "DBClient.h"
#include "DataQueryOption.h"
#include "DBClientUser.h"

enum TriggerStatusType {
	TRIGGER_STATUS_OK,
	TRIGGER_STATUS_PROBLEM,
	TRIGGER_STATUS_UNKNOWN,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_UNKNOWN,
	TRIGGER_SEVERITY_INFO,
	TRIGGER_SEVERITY_WARNING,
	TRIGGER_SEVERITY_ERROR,     // Average  in ZABBIX API
	TRIGGER_SEVERITY_CRITICAL,  // High     in ZABBIX API
	TRIGGER_SEVERITY_EMERGENCY, // Disaster in ZABBIX API
	NUM_TRIGGER_SEVERITY,
};

static const uint32_t ALL_SERVERS = -1;
static const uint64_t ALL_HOSTS   = -1;
static const uint64_t ALL_TRIGGERS = -1;
static const uint64_t ALL_HOST_GROUPS = -1;

struct HostInfo {
	uint32_t            serverId;
	uint64_t            id;
	string              hostName;

	// The follwong members are currently not used.
	string              ipAddr;
	string              nickname;
};

static const uint64_t INVALID_HOST_ID = -1;

typedef list<HostInfo>               HostInfoList;
typedef HostInfoList::iterator       HostInfoListIterator;
typedef HostInfoList::const_iterator HostInfoListConstIterator;

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
	EVENT_TYPE_GOOD,
	EVENT_TYPE_BAD,
	EVENT_TYPE_UNKNOWN,
};

struct EventInfo {
	uint32_t            serverId;
	uint64_t            id;
	timespec            time;
	EventType           type;
	uint64_t            triggerId;

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

class DBClientHatohol : public DBClient {
public:
	static uint64_t EVENT_NOT_FOUND;
	static int HATOHOL_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static void init(void);

	DBClientHatohol(void);
	virtual ~DBClientHatohol();

	void getHostInfoList(HostInfoList &hostInfoList,
	                     uint32_t targetServerId = ALL_SERVERS,
	                     uint64_t targetHostId = ALL_HOSTS);

	void addTriggerInfo(TriggerInfo *triggerInfo);
	void addTriggerInfoList(const TriggerInfoList &triggerInfoList);

	/**
	 * Get the trigger information with the specified server ID and
	 * the trigger ID.
	 *
	 * @param triggerInfo
	 * Obtained information is copied to this instance.
	 *
	 * @param serverId  A server ID to be obtained.
	 * @param triggerId A trigger ID to be obtained.
	 *
	 * @return true if the trigger information is found. Otherwise false.
	 *
	 */
	bool getTriggerInfo(TriggerInfo &triggerInfo,
	                    uint32_t serverId, uint64_t triggerId);
	void getTriggerInfoList(TriggerInfoList &triggerInfoList,
	                        uint32_t targetServerId = ALL_SERVERS,
	                        uint64_t targetHostId = ALL_HOSTS,
	                        uint64_t targetTriggerId = ALL_TRIGGERS);
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
	void getEventInfoList(EventInfoList &eventInfoList,
	                      DataQueryOption &option);
	void setEventInfoList(const EventInfoList &eventInfoList,
	                      uint32_t serverId);

	/**
	 * get the last (maximum) event ID of the event that belongs to
	 * the specified server
	 * @param serverId A target server ID.
	 * @return
	 * The last event ID. If there is no event data, EVENT_NOT_FOUND
	 * is returned.
	 */
	uint64_t getLastEventId(uint32_t serverId);

	void addItemInfo(ItemInfo *itemInfo);
	void addItemInfoList(const ItemInfoList &itemInfoList);
	void getItemInfoList(ItemInfoList &itemInfoList,
	                     uint32_t targetServerId = ALL_SERVERS);

	/**
	 * get the number of triggers with the given server ID, host group ID,
	 * the severity. The triggers with status: TRIGGER_STATUS_OK is NOT
	 * counted.
	 * @param serverId A target server Id.
	 * @param hostGroupId A target host group ID.
	 * @param severity    A target severity.
	 * @return The number matched triggers.
	 */
	size_t getNumberOfTriggers(uint32_t serverId, uint64_t hostGroupId,
	                           TriggerSeverityType severity);
	size_t getNumberOfHosts(uint32_t serverId, uint64_t hostGroupId);
	size_t getNumberOfGoodHosts(uint32_t serverId, uint64_t hostGroupId);
	size_t getNumberOfBadHosts(uint32_t serverId, uint64_t hostGroupId);

protected:
	void addTriggerInfoBare(const TriggerInfo &triggerInfo);
	void addEventInfoBare(const EventInfo &eventInfo);
	void addItemInfoBare(const ItemInfo &itemInfo);

	void getTriggerInfoList(TriggerInfoList &triggerInfoList,
	                        const string &condition);

	static void appendCondition(string &cond, const string &newCond);
	static string makeConditionHostGroup(
	  const HostGrpAccessInfoMap *hostGrpAccessInfoMap,
	  const string &hostGroupIdColumnName);
	static string makeCondition(const ServerAccessInfoMap &srvAccessInfoMap,
	                            const string &serverIdColumnName,
	                            const string &hostGroupIdColumnName);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientHatohol_h
