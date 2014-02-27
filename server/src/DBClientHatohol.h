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
#include "ItemGroupStream.h"

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

static const ServerIdType ALL_SERVERS = -1;
static const uint64_t ALL_HOSTS   = -1;
static const uint64_t ALL_TRIGGERS = -1;
static const uint64_t ALL_ITEMS    = -1;
static const uint64_t ALL_HOST_GROUPS = -1;

struct HostInfo {
	ServerIdType        serverId;
	uint64_t            id;
	std::string         hostName;

	// The follwong members are currently not used.
	std::string         ipAddr;
	std::string         nickname;
};

static const uint64_t INVALID_HOST_ID = -1;

typedef std::list<HostInfo>          HostInfoList;
typedef HostInfoList::iterator       HostInfoListIterator;
typedef HostInfoList::const_iterator HostInfoListConstIterator;

struct TriggerInfo {
	ServerIdType        serverId;
	uint64_t            id;
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangeTime;
	uint64_t            hostId;
	std::string         hostName;
	std::string         brief;

	// 'hostgroupId' variable is used when retrieve data from DB.
	uint64_t            hostgroupId;
	std::string         hostgroupName;
};

typedef std::list<TriggerInfo>          TriggerInfoList;
typedef TriggerInfoList::iterator       TriggerInfoListIterator;
typedef TriggerInfoList::const_iterator TriggerInfoListConstIterator;

enum EventType {
	EVENT_TYPE_GOOD,
	EVENT_TYPE_BAD,
	EVENT_TYPE_UNKNOWN,
};

struct EventInfo {
	// 'unifiedId' is the unique ID in the event table of Hatohol cache DB.
	// 'id' is the unique in a 'serverId'. It is typically the same as
	// the event ID of a monitroing system such as ZABBIX and Nagios.
	uint64_t            unifiedId;
	ServerIdType        serverId;
	EventIdType         id;
	timespec            time;
	EventType           type;
	uint64_t            triggerId;

	// status and type are basically same information.
	// so they should be unified.
	TriggerStatusType   status;
	TriggerSeverityType severity;
	uint64_t            hostId;
	std::string         hostName;
	std::string         brief;
	uint64_t            hostgroupId;
	std::string         hostgroupName;
};
void initEventInfo(EventInfo &eventInfo);

typedef std::list<EventInfo>          EventInfoList;
typedef EventInfoList::iterator       EventInfoListIterator;
typedef EventInfoList::const_iterator EventInfoListConstIterator;

struct ItemInfo {
	ServerIdType        serverId;
	ItemIdType          id;
	uint64_t            hostId;
	std::string         brief;
	timespec            lastValueTime;
	std::string         lastValue;
	std::string         prevValue;
	std::string         itemGroupName;

	uint64_t            hostgroupId;
	std::string         hostgroupName;
};

typedef std::list<ItemInfo>          ItemInfoList;
typedef ItemInfoList::iterator       ItemInfoListIterator;
typedef ItemInfoList::const_iterator ItemInfoListConstIterator;

struct HostgroupInfo {
	int                 id;
	ServerIdType        serverId;
	uint64_t            groupId;
	std::string         groupName;
};

typedef std::list<HostgroupInfo>               HostgroupInfoList;
typedef HostgroupInfoList::iterator       HostgroupInfoListIterator;
typedef HostgroupInfoList::const_iterator HostgroupInfoListConstIterator;

struct HostgroupElement {
	int                 id;
	ServerIdType        serverId;
	uint64_t            hostId;
	uint64_t            groupId;
};

typedef std::list<HostgroupElement> HostgroupElementList;
typedef HostgroupElementList::iterator HostgroupElementListIterator;
typedef HostgroupElementList::const_iterator HostgroupElementListConstIterator;

class HostResourceQueryOption : public DataQueryOption {
public:
	HostResourceQueryOption(UserIdType userId = INVALID_USER_ID);
	HostResourceQueryOption(const HostResourceQueryOption &src);
	virtual ~HostResourceQueryOption();

	// Overriding of virtual methods
	virtual std::string getCondition(const std::string &tableAlias = "") const;

	virtual ServerIdType getTargetServerId(void) const;
	virtual void setTargetServerId(const ServerIdType &targetServerId);
	virtual uint64_t getTargetHostId(void) const;
	virtual void setTargetHostId(uint64_t targetHostId);
	virtual uint64_t getTargetHostgroupId(void) const;
	virtual void setTargetHostgroupId(uint64_t targetHostGroupId);

protected:
	void setServerIdColumnName(const std::string &name) const;
	std::string getServerIdColumnName(
	  const std::string &tableAlias = "") const;
	void setHostGroupIdColumnName(const std::string &name) const;
	std::string getHostGroupIdColumnName(
	  const std::string &tableAlias = "") const;
	void setHostIdColumnName(const std::string &name) const;
	std::string getHostIdColumnName(
	  const std::string &tableAlias = "") const;
	static void appendCondition(std::string &cond,
	                            const std::string &newCond);
	static std::string makeCondition(
	  const ServerHostGrpSetMap &srvHostGrpSetMap,
	  const std::string &serverIdColumnName,
	  const std::string &hostGroupIdColumnName,
	  const std::string &hostIdColumnName,
	  ServerIdType targetServerId = ALL_SERVERS,
	  uint64_t targetHostgroup = ALL_HOST_GROUPS,
	  uint64_t targetHostId = ALL_HOSTS);
	static std::string makeConditionServer(
	  const ServerIdType &serverId,
	  const HostGroupSet &hostGroupSet,
	  const std::string &serverIdColumnName,
	  const std::string &hostGroupIdColumnName,
	  const uint64_t &hostgroupId = ALL_HOST_GROUPS);
	static std::string makeConditionHostGroup(
	  const HostGroupSet &hostGroupSet,
	  const std::string &hostGroupIdColumnName);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class EventsQueryOption : public HostResourceQueryOption {
public:
	EventsQueryOption(UserIdType userId = INVALID_USER_ID);

	void setSortDirection(SortDirection direction);
	SortDirection getSortDirection(void) const;
};

class TriggersQueryOption : public HostResourceQueryOption {
public:
	TriggersQueryOption(UserIdType userId = INVALID_USER_ID);
};

class ItemsQueryOption : public HostResourceQueryOption {
public:
	ItemsQueryOption(UserIdType userId = INVALID_USER_ID);
};

class HostsQueryOption : public HostResourceQueryOption {
public:
	HostsQueryOption(UserIdType userId = INVALID_USER_ID);
};

class HostgroupsQueryOption : public HostResourceQueryOption {
public:
	HostgroupsQueryOption(UserIdType userId = INVALID_USER_ID);
};

class HostgroupElementQueryOption: public HostResourceQueryOption {
public:
	HostgroupElementQueryOption(UserIdType userId = INVALID_USER_ID);
};

class DBClientHatohol : public DBClient {
public:
	static uint64_t EVENT_NOT_FOUND;
	static int HATOHOL_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static void init(void);

	DBClientHatohol(void);
	virtual ~DBClientHatohol();

	void getHostInfoList(HostInfoList &hostInfoList,
			     const HostsQueryOption &option);

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
	                    const ServerIdType &serverId, uint64_t triggerId);
	void getTriggerInfoList(TriggerInfoList &triggerInfoList,
				const TriggersQueryOption &option,
	                        uint64_t targetTriggerId = ALL_TRIGGERS);
	void setTriggerInfoList(const TriggerInfoList &triggerInfoList,
	                        const ServerIdType &serverId);
	/**
	 * get the last change time of the trigger that belongs to
	 * the specified server
	 * @param serverId A target server ID.
	 * @return
	 * The last change time of tringer in unix time. If there is no
	 * trigger information, 0 is returned.
	 */
	int  getLastChangeTimeOfTrigger(const ServerIdType &serverId);

	void addEventInfo(EventInfo *eventInfo);
	void addEventInfoList(const EventInfoList &eventInfoList);
	HatoholError getEventInfoList(EventInfoList &eventInfoList,
	                              const EventsQueryOption &option);
	void setEventInfoList(const EventInfoList &eventInfoList,
	                      const ServerIdType &serverId);

	void addHostgroupInfo(HostgroupInfo *eventInfo);
	void addHostgroupInfoList(const HostgroupInfoList &groupInfoList);
	HatoholError getHostgroupInfoList(HostgroupInfoList &hostgroupInfoList,
	                      const HostgroupsQueryOption &option);
	HatoholError getHostgroupElementList
	  (HostgroupElementList &hostgroupElementList,
	   const HostgroupElementQueryOption &option);

	void addHostgroupElement(HostgroupElement *mapHostHostgroupsInfo);
	void addHostgroupElementList
	  (const HostgroupElementList &mapHostHostgroupsInfoList);

	void addHostInfo(HostInfo *hostInfo);
	void addHostInfoList(const HostInfoList &hostInfoList);

	/**
	 * get the last (maximum) event ID of the event that belongs to
	 * the specified server
	 * @param serverId A target server ID.
	 * @return
	 * The last event ID. If there is no event data, EVENT_NOT_FOUND
	 * is returned.
	 */
	uint64_t getLastEventId(const ServerIdType &serverId);

	void addItemInfo(ItemInfo *itemInfo);
	void addItemInfoList(const ItemInfoList &itemInfoList);
	void getItemInfoList(ItemInfoList &itemInfoList,
			     const ItemsQueryOption &option,
			     uint64_t targetItemId = ALL_ITEMS);
	void getItemInfoList(ItemInfoList &itemInfoList,
			     const std::string &condition);

	/**
	 * get the number of triggers with the given server ID, host group ID,
	 * the severity. The triggers with status: TRIGGER_STATUS_OK is NOT
	 * counted. When the user doesn't have privilege to access to the
	 * server or host group, the trigger concerned with it isn't counted
	 * too.
	 *
	 * @param option
	 * A query option to specify user ID (or privilege), target server ID
	 * and host group ID.
	 *
	 * @param severity
	 * A target severity.
	 *
	 * @return The number matched triggers.
	 */
	size_t getNumberOfTriggers(const TriggersQueryOption &option,
	                           TriggerSeverityType severity);
	size_t getNumberOfHosts(const HostsQueryOption &option);
	size_t getNumberOfGoodHosts(const HostsQueryOption &option);
	size_t getNumberOfBadHosts(const HostsQueryOption &option);

	void pickupAbsentHostIds(std::vector<uint64_t> &absentHostIdVector,
	                         const std::vector<uint64_t> &hostIdVector);

protected:
	void addTriggerInfoWithoutTransaction(const TriggerInfo &triggerInfo);
	void addEventInfoWithoutTransaction(const EventInfo &eventInfo);
	void addItemInfoWithoutTransaction(const ItemInfo &itemInfo);
	void addHostgroupInfoWithoutTransaction(const HostgroupInfo &groupInfo);
	void addHostgroupElementWithoutTransaction(
	  const HostgroupElement &hostgroupElement);
	void addHostInfoWithoutTransaction(const HostInfo &hostInfo);

	void getTriggerInfoList(TriggerInfoList &triggerInfoList,
	                        const std::string &condition);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

void operator>>(ItemGroupStream &itemGroupStream, TriggerStatusType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, TriggerSeverityType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, EventType &rhs);

#endif // DBClientHatohol_h
