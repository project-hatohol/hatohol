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

#ifndef DBTablesMonitoring_h
#define DBTablesMonitoring_h

#include <list>
#include "DBTables.h"
#include "DataQueryOption.h"
#include "DBTablesUser.h"
#include "ItemGroupStream.h"
#include "HostResourceQueryOption.h"
#include "SmartTime.h"
#include "Monitoring.h"
#include "DBTablesHost.h"

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

	void setType(const EventType &type);
	EventType getType(void) const;
	void setMinimumSeverity(const TriggerSeverityType &severity);
	TriggerSeverityType getMinimumSeverity(void) const;
	void setTriggerStatus(const TriggerStatusType &status);
	TriggerStatusType getTriggerStatus(void) const;

	void setTriggerId(const TriggerIdType &triggerId);
	TriggerIdType getTriggerId(void) const;

	void setBeginTime(const timespec &beginTime);
	const timespec &getBeginTime(void);
	void setEndTime(const timespec &endTime);
	const timespec &getEndTime(void);

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
	void setExcludeFlags(const ExcludeFlags &flg);

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
	void setAppName(const std::string &appName) const;
	void setExcludeFlags(const ExcludeFlags &flg);
	const std::string &getAppName(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
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
	static const SetupInfo &getConstSetupInfo(void);

	static const char *TABLE_NAME_TRIGGERS;
	static const char *TABLE_NAME_EVENTS;
	static const char *TABLE_NAME_ITEMS;
	static const char *TABLE_NAME_SERVER_STATUS;
	static const char *TABLE_NAME_INCIDENTS;

	DBTablesMonitoring(DBAgent &dbAgent);
	virtual ~DBTablesMonitoring();

	void addTriggerInfo(TriggerInfo *triggerInfo);
	void addTriggerInfoList(const TriggerInfoList &triggerInfoList);

	void updateTrigger(const TriggerInfoList &triggerInfoList,
			   const ServerIdType &serverId);
	HatoholError deleteTriggerInfo(const TriggerIdList &idList,
	                               const ServerIdType &serverId);
	HatoholError syncTriggers(const TriggerInfoList &triggerInfoList,
	                          const ServerIdType &serverId);

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
	void addEventInfoList(EventInfoList &eventInfoList);
	HatoholError getEventInfoList(EventInfoList &eventInfoList,
	                              const EventsQueryOption &option,
				      IncidentInfoVect *incidentInfoVect = NULL);

	/**
	 * get the maximum event ID that belongs to the specified server
	 *
	 * @param serverId A target server ID.
	 * @return
	 * The max event ID. If there is no event data, EVENT_NOT_FOUND
	 * is returned.
	 */
	EventIdType getMaxEventId(const ServerIdType &serverId);

	/**
	 * Get the time of the last event.
	 *
	 * @param serverId  A target server ID.
	 * @param triggerId A target trigger ID.
	 *
	 * @return
	 * The time of the last event. If there's no event, hasValidTime() of
	 * the returned object is false.
	 */
	mlpl::SmartTime getTimeOfLastEvent(
	  const ServerIdType &serverId,
	  const TriggerIdType &triggerId = ALL_TRIGGERS);

	void addItemInfo(ItemInfo *itemInfo);
	void addItemInfoList(const ItemInfoList &itemInfoList);
	void getItemInfoList(ItemInfoList &itemInfoList,
			     const ItemsQueryOption &option);
	void getApplicationInfoVect(ApplicationInfoVect &applicationInfoVect,
			     const ItemsQueryOption &option);
	void addMonitoringServerStatus(
	  const MonitoringServerStatus &serverStatus);

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

	void addIncidentInfo(IncidentInfo *incidentInfo);
	HatoholError getIncidentInfoVect(IncidentInfoVect &incidentInfoVect,
					 const IncidentsQueryOption &option);
	uint64_t getLastUpdateTimeOfIncidents(
	  const IncidentTrackerIdType &trackerId);

	/**
	 * Update an incident information which has specified trackerId &
	 * identifier. Note that it doesn't update monitoring system side
	 * information (serverId, eventId and triggerId) beacuse they are
	 * tied with the incident and shouldn't be changed.
	 *
	 * @param incidentInfo
	 * An IncidentInfo to update.
	 */
	void updateIncidentInfo(IncidentInfo &incidentInfo);

protected:
	static SetupInfo &getSetupInfo(void);

	static void addTriggerInfoWithoutTransaction(
	  DBAgent &dbAgent, const TriggerInfo &triggerInfo);
	static void addEventInfoWithoutTransaction(
	  DBAgent &dbAgent, EventInfo &eventInfo);
	static void addItemInfoWithoutTransaction(
	  DBAgent &dbAgent, const ItemInfo &itemInfo);
	static void addMonitoringServerStatusWithoutTransaction(
	  DBAgent &dbAgent, const MonitoringServerStatus &serverStatus);
	static void addIncidentInfoWithoutTransaction(
	  DBAgent &dbAgent, const IncidentInfo &incidentInfo);

	/**
	 * Fill the following members if they are not set, with the
	 * corresponding trigger information.
	 *   - severity
	 *   - globalHostId
	 *   - hostIdInServer
	 *   - hostName
	 *   - brief
	 *   - extenededInfo
	 *
	 * @param eventInfo An EventInfo instance to be set.
	 *
	 * @return
	 * true if the corresponding trigger is found and the values are set.
	 * Otherwise false is returned.
	 */
	static bool mergeTriggerInfo(DBAgent &dbAgent, EventInfo &eventInfo);

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
