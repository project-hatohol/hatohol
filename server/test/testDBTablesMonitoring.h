/*
 * Copyright (C) 2013-2015 Project Hatohol
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

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <gcutter.h>
#include "StringUtils.h"
#include "Helpers.h"

template<class TResourceType, class TQueryOption>
struct AssertGetHostResourceArg {
	std::list<TResourceType> actualRecordList;
	TQueryOption option;
	UserIdType userId;
	ServerIdType targetServerId;
	LocalHostIdType targetHostId;
	DataQueryOption::SortDirection sortDirection;
	size_t maxNumber;
	size_t offset;
	timespec beginTime;
	timespec endTime;
	HatoholErrorCode expectedErrorCode;
	std::vector<const TResourceType*> authorizedRecords;
	std::vector<const TResourceType*> expectedRecords;
	ServerHostGrpSetMap authMap;
	const TResourceType *fixtures;
	size_t numberOfFixtures;
	gconstpointer ddtParam;
	bool excludeDefunctServers;
	bool assertContent;

	AssertGetHostResourceArg(void)
	: userId(USER_ID_SYSTEM),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_LOCAL_HOSTS),
	  sortDirection(DataQueryOption::SORT_DONT_CARE),
	  maxNumber(0),
	  offset(0),
	  beginTime({0, 0}),
	  endTime({0, 0}),
	  expectedErrorCode(HTERR_OK),
	  fixtures(NULL),
	  numberOfFixtures(0),
	  ddtParam(NULL),
	  excludeDefunctServers(false),
	  assertContent(true)
	{
	}

	virtual void fixupOption(void)
	{
		option.setUserId(userId);
		option.setTargetServerId(targetServerId);
		option.setTargetHostId(targetHostId);
	}

	virtual void fixup(void)
	{
		fixupOption();
		fixupAuthorizedMap();
		fixupAuthorizedRecords();
		fixupExpectedRecords();
	}

	virtual void fixupAuthorizedMap(void)
	{
		makeServerHostGrpSetMap(authMap, userId);
	}

	virtual bool isAuthorized(const TResourceType &record)
	{
		return ::isAuthorized(authMap, userId,
		                      record.serverId, getHostId(record));
	}

	virtual void fixupAuthorizedRecords(void)
	{
		for (size_t i = 0; i < numberOfFixtures; i++) {
			if (isAuthorized(fixtures[i]))
				authorizedRecords.push_back(&fixtures[i]);
		}
	}

	virtual bool filterOutExpectedRecord(const TResourceType *info)
	{
		if (excludeDefunctServers) {
			if (!option.isValidServer(info->serverId))
				return true;
		}
		return false;
	}

	virtual void fixupExpectedRecords(void)
	{
		for (size_t i = 0; i < authorizedRecords.size(); i++) {
			const TResourceType *record = authorizedRecords[i];
			if (filterOutExpectedRecord(record))
				continue;
			if (targetServerId != ALL_SERVERS) {
				if (record->serverId != targetServerId)
					continue;
			}
			if (targetHostId != ALL_LOCAL_HOSTS) {
				if (getHostId(*record) != targetHostId)
					continue;
			}
			expectedRecords.push_back(record);
		}
	}

	virtual LocalHostIdType
	  getHostId(const TResourceType &record) const = 0;

	virtual const TResourceType &getExpectedRecord(size_t idx)
	{
		idx += offset;
		if (sortDirection == DataQueryOption::SORT_DESCENDING)
			idx = (expectedRecords.size() - 1) - idx;
		cut_assert_true(idx < expectedRecords.size());
		return *expectedRecords[idx];
	}

	virtual void assertNumberOfRecords(void)
	{
		size_t expectedNum = expectedRecords.size();
		if (expectedNum > offset)
			expectedNum -= offset;
		else
			expectedNum = 0;
		if (maxNumber && maxNumber < expectedNum)
			expectedNum = maxNumber;
		cppcut_assert_equal(expectedNum, actualRecordList.size());
		if (isVerboseMode())
			cut_notify("expectedNum: %zd\n", expectedNum);
	}

	virtual std::string makeOutputText(const TResourceType &record) = 0;

	virtual void assert(void)
	{
		assertNumberOfRecords();
		if (!assertContent)
			return;

		std::string expectedText;
		std::string actualText;
		typename std::list<TResourceType>::iterator it
		  = actualRecordList.begin();
		LinesComparator linesComparator;
		for (size_t i = 0; it != actualRecordList.end(); i++, ++it) {
			const TResourceType &expectedRecord =
			  getExpectedRecord(i);
			linesComparator.add(makeOutputText(expectedRecord),
			                    makeOutputText(*it));
		}
		const bool strictOrder = 
		 (sortDirection != DataQueryOption::SORT_DONT_CARE);
		linesComparator.assert(strictOrder);
	}

	void setDataDrivenTestParam(gconstpointer _ddtParam)
	{
		ddtParam = _ddtParam;
		excludeDefunctServers =
		  gcut_data_get_boolean(ddtParam,
		                        "excludeDefunctServers");
		option.setExcludeDefunctServers(excludeDefunctServers);
	}
};

struct AssertGetEventsArg
  : public AssertGetHostResourceArg<EventInfo, EventsQueryOption>
{
	uint64_t limitOfUnifiedId;
	EventsQueryOption::SortType sortType;
	EventType type;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;
	TriggerIdType triggerId;
	std::map<const EventInfo *, uint64_t> idMap;
	IncidentInfoVect actualIncidentInfoVect;
	bool withIncidentInfo;
	std::string hostname;
	std::map<std::string, const IncidentInfo *> eventIncidentMap;

 AssertGetEventsArg(gconstpointer ddtParam,
		    const EventInfo *eventInfo = testEventInfo,
		    size_t numEventInfo = NumTestEventInfo)
	: limitOfUnifiedId(0), sortType(EventsQueryOption::SORT_UNIFIED_ID),
	  type(EVENT_TYPE_ALL),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL),
	  triggerId(ALL_TRIGGERS),
	  withIncidentInfo(false)
	{

		fixtures = eventInfo;
		numberOfFixtures = numEventInfo;

		fixupIdMap();
		if (ddtParam)
			setDataDrivenTestParam(ddtParam);
	}

	virtual void fixupIdMap(void)
	{
		for (size_t i = 0; i < numberOfFixtures; i++)
			idMap[&fixtures[i]] = i + 1;
	}

	virtual void fixupOption(void) override
	{
		AssertGetHostResourceArg<EventInfo, EventsQueryOption>::
			fixupOption();
		if (maxNumber)
			option.setMaximumNumber(maxNumber);
		if (offset)
			option.setOffset(offset);
		if (limitOfUnifiedId)
			option.setLimitOfUnifiedId(limitOfUnifiedId);
		if (beginTime.tv_sec != 0 || beginTime.tv_nsec)
			option.setBeginTime(beginTime);
		if (endTime.tv_sec != 0 || endTime.tv_nsec)
			option.setEndTime(endTime);
		if (!hostname.empty())
			option.setHostnameList({hostname});
		option.setSortType(sortType, sortDirection);
		option.setType(type);
		option.setMinimumSeverity(minSeverity);
		option.setTriggerStatus(triggerStatus);
		option.setTriggerId(triggerId);
	}

	virtual bool filterOutExpectedRecord(const EventInfo *info) override
	{
  		if (AssertGetHostResourceArg <EventInfo, EventsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}
		if (limitOfUnifiedId && idMap[info] > limitOfUnifiedId)
			return true;

		if (type != EVENT_TYPE_ALL && info->type != type)
			return true;

		// TODO: Use TriggerInfo instead of EventInfo because actual
		//       EventInfo doesn't store correct severity.
		if (info->severity < minSeverity)
			return true;

		if (triggerStatus != TRIGGER_STATUS_ALL &&
		    info->status != triggerStatus) {
			return true;
		}

		if (triggerId != ALL_TRIGGERS && info->triggerId != triggerId)
			return true;

		if (beginTime.tv_sec != 0 || beginTime.tv_nsec != 0) {
			if (info->time.tv_sec < beginTime.tv_sec ||
			    (info->time.tv_sec == beginTime.tv_sec &&
			     info->time.tv_nsec < beginTime.tv_nsec)) {
				return true;
			}
		}

		if (endTime.tv_sec != 0 || endTime.tv_nsec != 0) {
			if (info->time.tv_sec > endTime.tv_sec ||
			    (info->time.tv_sec == endTime.tv_sec &&
			     info->time.tv_nsec > endTime.tv_nsec)) {
				return true;
			}
		}

		if (!hostname.empty()) {
			if (info->hostName != hostname)
				return true;
                }

		return false;
	}

	class lessTime
	{
	public:
		bool operator()(const EventInfo *lhs,
				const EventInfo *rhs)
		{
			if (lhs->time.tv_sec == rhs->time.tv_sec &&
			    lhs->time.tv_nsec == rhs->time.tv_nsec) {
				return lhs->unifiedId < rhs->unifiedId;
			} else if (lhs->time.tv_sec == rhs->time.tv_sec) {
				return lhs->time.tv_nsec < rhs->time.tv_nsec;
			} else {
				return lhs->time.tv_sec < rhs->time.tv_sec;
			}
		}
	};

	virtual void fixupExpectedRecords(void) override
	{
		AssertGetHostResourceArg<EventInfo, EventsQueryOption>::
			fixupExpectedRecords();
		
		if (sortType == EventsQueryOption::SORT_TIME)
			std::sort(expectedRecords.begin(),
				  expectedRecords.end(),
				  lessTime());

		makeEventIncidentMap(eventIncidentMap);
	}

	virtual LocalHostIdType getHostId(const EventInfo &info) const override
	{
		return info.hostIdInServer;
	}

	virtual std::string makeOutputText(const EventInfo &eventInfo)
	{
		return makeEventOutput(eventInfo);
	}

	IncidentInfo getExpectedIncidentInfo(const EventInfo &event) {
		std::string key = makeEventIncidentMapKey(event);
		if (eventIncidentMap.find(key) == eventIncidentMap.end()) {
			IncidentInfo incident;
			incident.trackerId = 0;
			incident.serverId  = event.serverId;
			incident.eventId   = event.id;
			incident.triggerId = event.triggerId;
			incident.doneRatio = 0;
			incident.createdAt.tv_sec  = 0;
			incident.createdAt.tv_nsec = 0;
			incident.updatedAt.tv_sec  = 0;
			incident.updatedAt.tv_nsec = 0;
			incident.unifiedEventId = 0;
			incident.commentCount = 0;
			return incident;
		} else {
			return *eventIncidentMap[key];
		}
	}

	virtual void assert(void) override
	{
		AssertGetHostResourceArg<EventInfo, EventsQueryOption>::assert();

		if (!withIncidentInfo)
			return;

		cppcut_assert_equal(actualRecordList.size(),
				    actualIncidentInfoVect.size());

		EventInfoListIterator eventIt = actualRecordList.begin();
		IncidentInfoVectIterator incidentIt
		  = actualIncidentInfoVect.begin();
		IncidentInfoVectIterator incidentEndIt
		  = actualIncidentInfoVect.end();
		std::string expected, actual;
		for (; incidentIt != incidentEndIt; incidentIt++, eventIt++) {
			IncidentInfo incident
			  = getExpectedIncidentInfo(*eventIt);
			// TODO: should get correct expected unifiedEventId
			incident.unifiedEventId = incidentIt->unifiedEventId;
			expected += makeIncidentOutput(incident);
			actual += makeIncidentOutput(*incidentIt);
		}
		cppcut_assert_equal(expected, actual);
	}

	UnifiedEventIdType findLastUnifiedId(void)
	{
		UnifiedEventIdType lastId = -1;
		for (size_t i = 0; i < numberOfFixtures; i++) {
			if (!isAuthorized(fixtures[i]))
				continue;
			if (excludeDefunctServers &&
			    !option.isValidServer(fixtures[i].serverId)) {
				continue;
			}
			lastId = i + 1;
		}
		return lastId;
	}
};
