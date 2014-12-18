/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef Monitoring_h
#define Monitoring_h

#include <SmartTime.h>
#include <Params.h>
#include <vector>
#include <list>
#include <map>

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

enum HostValidity {

	// Hosts that are in HOST_VALID, HOST_VALID_INAPPLICABLE, and
	// VALID_SELF_MONITORING.
	HOST_ALL_VALID = -2,

	// All hosts
	HOST_ANY_VALIDITY = -1,

	// Hosts that are probably deleted in the monitoring server.
	HOST_INVALID = 0,

	// Normal active hosts
	HOST_VALID,

	// Hosts that cannot be associated with a specific machine/devices.
	HOST_VALID_INAPPLICABLE,

	// Fake hosts for self monitoring.
	HOST_VALID_SELF_MONITORING,
};

struct HostInfo {
	ServerIdType        serverId;
	HostIdType          id;
	std::string         hostName;
	HostValidity        validity;

	// The follwong members are currently not used.
	std::string         ipAddr;
	std::string         nickname;
};

typedef std::list<HostInfo>          HostInfoList;
typedef HostInfoList::iterator       HostInfoListIterator;
typedef HostInfoList::const_iterator HostInfoListConstIterator;

typedef std::map<HostIdType, const HostInfo *> HostIdHostInfoMap;
typedef HostIdHostInfoMap::iterator            HostIdHostInfoMapIterator;
typedef HostIdHostInfoMap::const_iterator      HostIdHostInfoMapConstIterator;

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
	TriggerIdType       triggerId;

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

enum ItemInfoValueType {
	ITEM_INFO_VALUE_TYPE_UNKNOWN,
	ITEM_INFO_VALUE_TYPE_FLOAT,
	ITEM_INFO_VALUE_TYPE_INTEGER,
	ITEM_INFO_VALUE_TYPE_STRING,
	NUM_ITEM_INFO_VALUE_TYPES,
};

struct ItemInfo {
	ServerIdType        serverId;
	ItemIdType          id;
	HostIdType          hostId;
	std::string         brief;
	timespec            lastValueTime;
	std::string         lastValue;
	std::string         prevValue;
	std::string         itemGroupName;
	int                 delay;
	ItemInfoValueType   valueType;
	std::string         unit;
};

typedef std::list<ItemInfo>          ItemInfoList;
typedef ItemInfoList::iterator       ItemInfoListIterator;
typedef ItemInfoList::const_iterator ItemInfoListConstIterator;

struct ApplicationInfo {
	std::string           applicationName;
};

typedef std::vector<ApplicationInfo>        ApplicationInfoVect;
typedef ApplicationInfoVect::iterator       ApplicationInfoVectIterator;
typedef ApplicationInfoVect::const_iterator ApplicationInfoVectConstIterator;

struct HistoryInfo {
	ServerIdType serverId;
	ItemIdType   itemId;
	std::string  value;
	timespec     clock;
};

typedef std::vector<HistoryInfo>        HistoryInfoVect;
typedef HistoryInfoVect::iterator       HistoryInfoVectIterator;
typedef HistoryInfoVect::const_iterator HistoryInfoVectConstIterator;

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
	typedef enum {
		STATUS_UNKNOWN,
		STATUS_OPENED,
		STATUS_ASSIGNED,
		STATUS_RESOLVED,
		STATUS_CLOSED,
		STATUS_REOPENED,
	} Status;

	IncidentTrackerIdType trackerId;

	/* fetched from a monitoring system */
	ServerIdType       serverId;
	EventIdType        eventId;
	TriggerIdType      triggerId;

	/* fetched from an incident tracking system */
	std::string        identifier;
	std::string        location;
	std::string        status;
	std::string        priority;
	std::string        assignee;
	int                doneRatio;
	mlpl::Time         createdAt;
	mlpl::Time         updatedAt;

	Status             statusCode;

	/* fetched from a monitoring system (since 14.12) */
	uint64_t           unifiedEventId;
};

typedef std::vector<IncidentInfo>        IncidentInfoVect;
typedef IncidentInfoVect::iterator       IncidentInfoVectIterator;
typedef IncidentInfoVect::const_iterator IncidentInfoVectConstIterator;

#endif // Monitoring_h
