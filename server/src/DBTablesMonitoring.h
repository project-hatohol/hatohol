/*
 * Copyright (C) 2013-2016 Project Hatohol
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
#include "StatisticsCounter.h"

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
	void setGroupByColumns(const std::vector<std::string> &columns);
	std::vector<std::string> getGroupByColumns(void) const;

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
	void setHostnameList(const std::list<std::string> &hostnameList);
	const std::list<std::string> getHostnameList(void);

	void setEventTypes(const std::set<EventType> &types);
	const std::set<EventType> &getEventTypes(void) const;
	void setEventIds(const std::list<EventIdType> &eventIds);
	void setTriggerSeverities(const std::set<TriggerSeverityType> &severities);
	const std::set<TriggerSeverityType> &getTriggerSeverities(void) const;
	void setTriggerStatuses(const std::set<TriggerStatusType> &statuses);
	const std::set<TriggerStatusType> &getTriggerStatuses(void) const;

	void setIncidentStatuses(const std::set<std::string> &statuses);
	const std::set<std::string> &getIncidentStatuses(void) const;

	std::string makeHostnameListCondition(
	  const std::list<std::string> &hostnameList) const;

	std::string makeEventIdListCondition(
	  const std::list<EventIdType> &eventIds) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class TriggersQueryOption : public HostResourceQueryOption {
public:
	enum SortType {
		SORT_ID,
		SORT_TIME,
		NUM_SORT_TYPES
	};

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

	void setBeginTime(const timespec &beginTime);
	const timespec &getBeginTime(void);
	void setEndTime(const timespec &endTime);
	const timespec &getEndTime(void);
	void setHostnameList(const std::list<std::string> &hostnameList);
	const std::list<std::string> getHostnameList(void);

	void setSortType(const SortType &type, const SortDirection &direction);
	SortType getSortType(void) const;
	SortDirection getSortDirection(void) const;
	void setTriggerBrief(const std::string &triggerBrief);
	std::string getTriggerBrief(void) const;

	std::string makeHostnameListCondition(
	  const std::list<std::string> &hostnameList) const;

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
	void setTargetItemCategoryName(const std::string &categoryName);
	const std::string &getTargetItemCategoryName(void);
	void setExcludeFlags(const ExcludeFlags &flg);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class IncidentHistoriesQueryOption : public DataQueryOption {
public:
	enum SortType {
		SORT_UNIFIED_EVENT_ID,
		SORT_TIME,
		NUM_SORT_TYPES
	};

	static const UnifiedEventIdType INVALID_ID;

public:
	IncidentHistoriesQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IncidentHistoriesQueryOption(DataQueryContext *dataQueryContext);
	IncidentHistoriesQueryOption(const IncidentHistoriesQueryOption &src);
	~IncidentHistoriesQueryOption();

	virtual std::string getCondition(void) const override;

	void setTargetId(const IncidentHistoryIdType &id);
	const IncidentHistoryIdType &getTargetId(void);
	void setTargetUnifiedEventId(const UnifiedEventIdType &id);
	const UnifiedEventIdType getTargetUnifiedEventId(void);
	void setTargetUserId(const UserIdType &userId);
	const UserIdType getTargetUserId(void);
	void setSortType(const SortType &type, const SortDirection &direction);
	SortType getSortType(void) const;
	SortDirection getSortDirection(void) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class IncidentsQueryOption : public DataQueryOption {
public:
	static const UnifiedEventIdType ALL_INCIDENTS;

public:
	IncidentsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	IncidentsQueryOption(DataQueryContext *dataQueryContext);
	IncidentsQueryOption(const IncidentsQueryOption &src);
	~IncidentsQueryOption();

	void setTargetUnifiedEventId(const UnifiedEventIdType &id);
	const UnifiedEventIdType getTargetUnifiedEventId(void);

	virtual std::string getCondition(void) const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class DBTablesMonitoring : public DBTables {
public:
	static const int         MONITORING_DB_VERSION;
	static constexpr size_t  NUM_EVENTS_COUNTERS = 3;
	static void reset(void);
	static const SetupInfo &getConstSetupInfo(void);

	static const char *TABLE_NAME_TRIGGERS;
	static const char *TABLE_NAME_EVENTS;
	static const char *TABLE_NAME_ITEMS;
	static const char *TABLE_NAME_ITEM_CATEGORIES;
	static const char *TABLE_NAME_SERVER_STATUS;
	static const char *TABLE_NAME_INCIDENTS;
	static const char *TABLE_NAME_INCIDENT_HISTORIES;

	DBTablesMonitoring(DBAgent &dbAgent);
	virtual ~DBTablesMonitoring();

	void addTriggerInfo(const TriggerInfo *triggerInfo);
	void addTriggerInfoList(const TriggerInfoList &triggerInfoList,
	                        DBAgent::TransactionHooks *hooks = NULL);

	void updateTrigger(const TriggerInfoList &triggerInfoList,
			   const ServerIdType &serverId);
	HatoholError deleteTriggerInfo(const TriggerIdList &idList,
	                               const ServerIdType &serverId);
	HatoholError syncTriggers(const TriggerInfoList &triggerInfoList,
	                          const ServerIdType &serverId,
	                          DBAgent::TransactionHooks *hooks = NULL);

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
	HatoholError getTriggerBriefList(std::list<std::string> &triggerBriefList,
	                                 const TriggersQueryOption &option);
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
	void addEventInfoList(EventInfoList &eventInfoList,
	                      DBAgent::TransactionHooks *hooks = NULL);
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

	void addItemInfo(const ItemInfo *itemInfo);
	void addItemInfoList(const ItemInfoList &itemInfoList);
	HatoholError deleteItemInfo(const TriggerIdList &idList, const ServerIdType &serverId);
	HatoholError syncItems(const ItemInfoList &itemInfoList, const ServerIdType &serverId);
	void getItemInfoList(ItemInfoList &itemInfoList,
			     const ItemsQueryOption &option);
	void getItemCategoryNames(std::vector<std::string> &itemCategoryNames,
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
	size_t getNumberOfEvents(const EventsQueryOption &option);
	size_t getNumberOfHostsWithSpecifiedEvents(const EventsQueryOption &option);
	struct EventSeverityStatistics {
		TriggerSeverityType severity;
		int64_t num;
	};
	HatoholError getEventSeverityStatistics(
	  std::vector<EventSeverityStatistics> &importantEventGroupVect,
	  const EventsQueryOption &option);

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
	 *
	 * @rerurn A HatoholError instance.
	 */
	HatoholError updateIncidentInfo(IncidentInfo &incidentInfo);

	struct SystemInfo {
		StatisticsCounter::Slot
		  eventsCounterPrevSlots[NUM_EVENTS_COUNTERS],
		  eventsCounterCurrSlots[NUM_EVENTS_COUNTERS];
	};
	static HatoholError getSystemInfo(SystemInfo &info,
	                                  const DataQueryOption &option);
	HatoholError addIncidentHistory(
	  IncidentHistory &incidentHistory);
	HatoholError updateIncidentHistory(
	  IncidentHistory &incidentHistory);
	HatoholError getIncidentHistory(
	  std::list<IncidentHistory> &IncidentHistoriesList,
	  const IncidentHistoriesQueryOption &option);
	size_t getIncidentCommentCount(UnifiedEventIdType unifiedEventId);
	HatoholError updateIncidentCommentCount(UnifiedEventIdType unifiedEventId);

protected:
	static SetupInfo &getSetupInfo(void);

	static void addTriggerInfoWithoutTransaction(
	  DBAgent &dbAgent, const TriggerInfo &triggerInfo);
	static void addEventInfoWithoutTransaction(
	  DBAgent &dbAgent, EventInfo &eventInfo);
	static void addItemInfoWithoutTransaction(
	  DBAgent &dbAgent, const ItemInfo &itemInfo);
	static void addItemCategoryWithoutTransaction(
	  DBAgent &dbAgent, const ItemCategory &category);
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
