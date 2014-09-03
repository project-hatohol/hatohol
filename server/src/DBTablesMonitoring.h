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

#ifndef DBTablesMonitoring_h
#define DBTablesMonitoring_h

#include <list>
#include "DBTables.h"
#include "DataQueryOption.h"
#include "DBTablesUser.h"
#include "ItemGroupStream.h"
#include "HostResourceQueryOption.h"
#include "SmartTime.h"

enum TriggerStatusType {
	TRIGGER_STATUS_ALL = -1,
	TRIGGER_STATUS_OK,
	TRIGGER_STATUS_PROBLEM,
	TRIGGER_STATUS_UNKNOWN,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_ALL = -1,
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

static const TriggerIdType FAILED_CONNECT_ZABBIX_TRIGGERID = LONG_MAX - 1;
static const TriggerIdType FAILED_CONNECT_MYSQL_TRIGGERID = LONG_MAX - 2;
static const TriggerIdType FAILED_INTERNAL_ERROR_TRIGGERID = LONG_MAX - 3;
static const TriggerIdType FAILED_PARSER_ERROR_TRIGGERID = LONG_MAX - 4;

// Define a special ID By using the LONG_MAX.
// Because, it does not overlap with the hostID set by the Zabbix.
// There is a possibility that depends on Zabbix version of this.
static const HostIdType MONITORING_SERVER_SELF_ID = LONG_MAX;

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

static const EventIdType DISCONNECT_SERVER_EVENT_ID = 0;

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

struct IncidentInfo {
	IncidentTrackerIdType trackerId;

	/* fetched from a monitoring system */
	ServerIdType       serverId;
	EventIdType        eventId;
	TriggerIdType      triggerId;

	/* fetched from an incident tracking system */
	std::string        identifier;
	std::string        location;
	std::string        status;
	std::string        assignee;
	mlpl::Time         createdAt;
	mlpl::Time         updatedAt;
};

typedef std::vector<IncidentInfo>        IncidentInfoVect;
typedef IncidentInfoVect::iterator       IncidentInfoVectIterator;
typedef IncidentInfoVect::const_iterator IncidentInfoVectConstIterator;

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
	struct Impl;
	std::unique_ptr<Impl> m_impl;
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
	struct Impl;
	std::unique_ptr<Impl> m_impl;
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
	struct Impl;
	std::unique_ptr<Impl> m_impl;
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

class IncidentsQueryOption : public DataQueryOption {
public:
	IncidentsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IncidentsQueryOption(DataQueryContext *dataQueryContext);
};

class DBTablesMonitoring : public DBTables {
public:
	static const int         MONITORING_DB_VERSION;
	static void reset(void);

	static const char *TABLE_NAME_TRIGGERS;
	static const char *TABLE_NAME_EVENTS;
	static const char *TABLE_NAME_ITEMS;
	static const char *TABLE_NAME_HOSTS;
	static const char *TABLE_NAME_HOSTGROUPS;
	static const char *TABLE_NAME_MAP_HOSTS_HOSTGROUPS;
	static const char *TABLE_NAME_SERVER_STATUS;
	static const char *TABLE_NAME_INCIDENTS;

	DBTablesMonitoring(DBAgent &dbAgent);
	virtual ~DBTablesMonitoring();

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
				      IncidentInfoVect *incidentInfoVect = NULL);
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
	size_t getNumberOfItems(const ItemsQueryOption &option);
	HatoholError getNumberOfMonitoredItemsPerSecond(
	  const DataQueryOption &option,
	  MonitoringServerStatus &serverStatus);

	void pickupAbsentHostIds(std::vector<uint64_t> &absentHostIdVector,
	                         const std::vector<uint64_t> &hostIdVector);

	void addIncidentInfo(IncidentInfo *incidentInfo);
	HatoholError getIncidentInfoVect(IncidentInfoVect &incidentInfoVect,
					 const IncidentsQueryOption &option);
	uint64_t getLastUpdateTimeOfIncidents(
	  const IncidentTrackerIdType &trackerId);

protected:
	static SetupInfo &getSetupInfo(void);

	static void addTriggerInfoWithoutTransaction(
	  DBAgent &dbAgent, const TriggerInfo &triggerInfo);
	static void addEventInfoWithoutTransaction(
	  DBAgent &dbAgent, const EventInfo &eventInfo);
	static void addItemInfoWithoutTransaction(
	  DBAgent &dbAgent, const ItemInfo &itemInfo);
	static void addHostgroupInfoWithoutTransaction(
	  DBAgent &dbAgent, const HostgroupInfo &groupInfo);
	static void addHostgroupElementWithoutTransaction(
	  DBAgent &dbAgent, const HostgroupElement &hostgroupElement);
	static void addHostInfoWithoutTransaction(
	  DBAgent &dbAgent, const HostInfo &hostInfo);
	static void addMonitoringServerStatusWithoutTransaction(
	  DBAgent &dbAgent, const MonitoringServerStatus &serverStatus);
	static void addIncidentInfoWithoutTransaction(
	  DBAgent &dbAgent, const IncidentInfo &incidentInfo);
	size_t getNumberOfTriggers(const TriggersQueryOption &option,
				   const std::string &additionalCondition);


private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

void operator>>(ItemGroupStream &itemGroupStream, TriggerStatusType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, TriggerSeverityType &rhs);
void operator>>(ItemGroupStream &itemGroupStream, EventType &rhs);

#endif // DBTablesMonitoring_h
