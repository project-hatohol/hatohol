/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include "HostResourceQueryOption.h"

enum TriggerStatusType {
	TRIGGER_STATUS_ALL = -1,
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

static const uint64_t ALL_TRIGGERS = -1;
static const uint64_t ALL_ITEMS    = -1;

struct HostInfo {
	ServerIdType        serverId;
	HostIdType          id;
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
	TriggerIdType       id;
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangeTime;
	HostIdType          hostId;
	std::string         hostName;
	std::string         brief;
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
	HostIdType          hostId;
	std::string         hostName;
	std::string         brief;
};
void initEventInfo(EventInfo &eventInfo);

typedef std::list<EventInfo>          EventInfoList;
typedef EventInfoList::iterator       EventInfoListIterator;
typedef EventInfoList::const_iterator EventInfoListConstIterator;

struct ItemInfo {
	ServerIdType        serverId;
	ItemIdType          id;
	HostIdType            hostId;
	std::string         brief;
	timespec            lastValueTime;
	std::string         lastValue;
	std::string         prevValue;
	std::string         itemGroupName;
	int                 delay;
};

typedef std::list<ItemInfo>          ItemInfoList;
typedef ItemInfoList::iterator       ItemInfoListIterator;
typedef ItemInfoList::const_iterator ItemInfoListConstIterator;

struct HostgroupInfo {
	int                 id;
	ServerIdType        serverId;
	HostgroupIdType     groupId;
	std::string         groupName;
};

typedef std::list<HostgroupInfo>               HostgroupInfoList;
typedef HostgroupInfoList::iterator       HostgroupInfoListIterator;
typedef HostgroupInfoList::const_iterator HostgroupInfoListConstIterator;

struct HostgroupElement {
	int                 id;
	ServerIdType        serverId;
	HostIdType          hostId;
	HostgroupIdType     groupId;
};

typedef std::list<HostgroupElement> HostgroupElementList;
typedef HostgroupElementList::iterator HostgroupElementListIterator;
typedef HostgroupElementList::const_iterator HostgroupElementListConstIterator;

struct MonitoringServerStatus {
	ServerIdType serverId;
	double       nvps;
};

typedef std::list<MonitoringServerStatus> MonitoringServerStatusList;
typedef MonitoringServerStatusList::iterator MonitoringServerStatusListIterator;
typedef MonitoringServerStatusList::const_iterator MonitoringServerStatusListConstIterator;

struct IssueInfo {
	IssueTrackerIdType trackerId;
	ServerIdType       serverId;
	EventIdType        eventId;
	TriggerIdType      triggerId;
	std::string        identifier;
	std::string        location;
	std::string        status;
	std::string        assignee;
	timespec           createdAt;
	timespec           updatedAt;
};

typedef std::vector<IssueInfo>        IssueInfoVect;
typedef IssueInfoVect::iterator       IssueInfoVectIterator;
typedef IssueInfoVect::const_iterator IssueInfoVectConstIterator;

class EventsQueryOption : public HostResourceQueryOption {
public:
	enum SortType {
		SORT_UNIFIED_ID,
		SORT_TIME,
		NUM_SORT_TYPES
	};

	EventsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	EventsQueryOption(DataQueryContext *dataQueryContext);
	EventsQueryOption(const EventsQueryOption &src);
	~EventsQueryOption();

	virtual std::string getCondition(void) const override;

	void setLimitOfUnifiedId(const uint64_t &unifiedId);
	uint64_t getLimitOfUnifiedId(void) const;

	void setSortType(const SortType &type, const SortDirection &direction);
	SortType getSortType(void) const;
	SortDirection getSortDirection(void) const;

	void setMinimumSeverity(const TriggerSeverityType &severity);
	TriggerSeverityType getMinimumSeverity(void) const;
	void setTriggerStatus(const TriggerStatusType &status);
	TriggerStatusType getTriggerStatus(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class TriggersQueryOption : public HostResourceQueryOption {
public:
	TriggersQueryOption(const UserIdType &userId = INVALID_USER_ID);
	TriggersQueryOption(DataQueryContext *dataQueryContext);
	TriggersQueryOption(const TriggersQueryOption &src);
	~TriggersQueryOption();

	virtual std::string getCondition(void) const override;

	void setTargetId(const TriggerIdType &id);
	TriggerIdType getTargetId(void) const;
	void setMinimumSeverity(const TriggerSeverityType &severity);
	TriggerSeverityType getMinimumSeverity(void) const;
	void setTriggerStatus(const TriggerStatusType &status);
	TriggerStatusType getTriggerStatus(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class ItemsQueryOption : public HostResourceQueryOption {
public:
	ItemsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	ItemsQueryOption(DataQueryContext *dataQueryContext);
	ItemsQueryOption(const ItemsQueryOption &src);
	~ItemsQueryOption();

	virtual std::string getCondition(void) const override;

	void setTargetId(const ItemIdType &id);
	ItemIdType getTargetId(void) const;
	void setTargetItemGroupName(const std::string &itemGroupName);
	const std::string &getTargetItemGroupName(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

class HostsQueryOption : public HostResourceQueryOption {
public:
	HostsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	HostsQueryOption(DataQueryContext *dataQueryContext);
};

class HostgroupsQueryOption : public HostResourceQueryOption {
public:
	HostgroupsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	HostgroupsQueryOption(DataQueryContext *dataQueryContext);
};

class HostgroupElementQueryOption: public HostResourceQueryOption {
public:
	HostgroupElementQueryOption(const UserIdType &userId = INVALID_USER_ID);
	HostgroupElementQueryOption(DataQueryContext *dataQueryContext);
};

class IssuesQueryOption : public DataQueryOption {
public:
	IssuesQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IssuesQueryOption(DataQueryContext *dataQueryContext);
};

class DBClientHatohol : public DBClient {
public:
	static const EventIdType EVENT_NOT_FOUND;
	static const int         HATOHOL_DB_VERSION;
	static const char       *DEFAULT_DB_NAME;
	static void init(void);

	static const char *TABLE_NAME_TRIGGERS;
	static const char *TABLE_NAME_EVENTS;
	static const char *TABLE_NAME_ITEMS;
	static const char *TABLE_NAME_HOSTS;
	static const char *TABLE_NAME_HOSTGROUPS;
	static const char *TABLE_NAME_MAP_HOSTS_HOSTGROUPS;
	static const char *TABLE_NAME_SERVERS;
	static const char *TABLE_NAME_ISSUES;

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
	                    const TriggersQueryOption &option);
	void getTriggerInfoList(TriggerInfoList &triggerInfoList,
				const TriggersQueryOption &option);
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
	                              const EventsQueryOption &option,
				      IssueInfoVect *issueInfoVect = NULL);
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
			     const ItemsQueryOption &option);
	void addMonitoringServerStatus(MonitoringServerStatus *serverStatus);

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
	size_t getNumberOfBadTriggers(const TriggersQueryOption &option,
				      TriggerSeverityType severity);
	size_t getNumberOfTriggers(const TriggersQueryOption &option);
	size_t getNumberOfHosts(const TriggersQueryOption &option);
	size_t getNumberOfGoodHosts(const TriggersQueryOption &option);
	size_t getNumberOfBadHosts(const TriggersQueryOption &option);
	HatoholError getNumberOfMonitoredItemsPerSecond(
	  const DataQueryOption &option,
	  MonitoringServerStatus &serverStatus);

	void pickupAbsentHostIds(std::vector<uint64_t> &absentHostIdVector,
	                         const std::vector<uint64_t> &hostIdVector);

	void addIssueInfo(IssueInfo *issueInfo);
	HatoholError getIssueInfoVect(IssueInfoVect &issueInfoVect,
				      const IssuesQueryOption &option);

protected:
	void addTriggerInfoWithoutTransaction(const TriggerInfo &triggerInfo);
	void addEventInfoWithoutTransaction(const EventInfo &eventInfo);
	void addItemInfoWithoutTransaction(const ItemInfo &itemInfo);
	void addHostgroupInfoWithoutTransaction(const HostgroupInfo &groupInfo);
	void addHostgroupElementWithoutTransaction(
	  const HostgroupElement &hostgroupElement);
	void addHostInfoWithoutTransaction(const HostInfo &hostInfo);
	void addMonitoringServerStatusWithoutTransaction(
	  const MonitoringServerStatus &serverStatus);
	void addIssueInfoWithoutTransaction(const IssueInfo &issueInfo);
	size_t getNumberOfTriggers(const TriggersQueryOption &option,
				   const std::string &additionalCondition);


private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

void operator>>(ItemGroupStream &itemGroupStream, TriggerStatusType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, TriggerSeverityType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, EventType &rhs);

#endif // DBClientHatohol_h
