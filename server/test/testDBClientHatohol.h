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
	uint64_t targetHostId;
	DataQueryOption::SortDirection sortDirection;
	size_t maxNumber;
	size_t offset;
	HatoholErrorCode expectedErrorCode;
	std::vector<TResourceType*> authorizedRecords;
	std::vector<TResourceType*> expectedRecords;
	ServerHostGrpSetMap authMap;
	TResourceType *fixtures;
	size_t numberOfFixtures;
	gconstpointer ddtParam;
	bool filterForDataOfDefunctSv;

	AssertGetHostResourceArg(void)
	: userId(USER_ID_SYSTEM),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_HOSTS),
	  sortDirection(DataQueryOption::SORT_DONT_CARE),
	  maxNumber(0),
	  offset(0),
	  expectedErrorCode(HTERR_OK),
	  fixtures(NULL),
	  numberOfFixtures(0),
	  ddtParam(NULL),
	  filterForDataOfDefunctSv(false)
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
		return ::isAuthorized(authMap, userId, record.serverId);
	}

	virtual void fixupAuthorizedRecords(void)
	{
		for (size_t i = 0; i < numberOfFixtures; i++) {
			if (isAuthorized(fixtures[i]))
				authorizedRecords.push_back(&fixtures[i]);
		}
	}

	virtual bool filterOutExpectedRecord(TResourceType *info)
	{
		if (filterForDataOfDefunctSv) {
			if (!option.isValidServer(info->serverId))
				return true;
		}
		return false;
	}

	virtual void fixupExpectedRecords(void)
	{
		for (size_t i = 0; i < authorizedRecords.size(); i++) {
			TResourceType *record = authorizedRecords[i];
			if (filterOutExpectedRecord(record))
				continue;
			if (targetServerId != ALL_SERVERS) {
				if (record->serverId != targetServerId)
					continue;
			}
			if (targetHostId != ALL_HOSTS) {
				if (getHostId(*record) != targetHostId)
					continue;
			}
			expectedRecords.push_back(record);
		}
	}

	virtual uint64_t getHostId(TResourceType &record) = 0;

	virtual TResourceType &getExpectedRecord(size_t idx)
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
	}

	virtual std::string makeOutputText(const TResourceType &record) = 0;

	virtual void assert(void)
	{
		assertNumberOfRecords();

		std::string expectedText;
		std::string actualText;
		typename std::list<TResourceType>::iterator it
		  = actualRecordList.begin();
		LinesComparator linesComparator;
		for (size_t i = 0; it != actualRecordList.end(); i++, ++it) {
			TResourceType &expectedRecord = getExpectedRecord(i);
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
		filterForDataOfDefunctSv =
		  gcut_data_get_boolean(ddtParam,
		                        "filterDataOfDefunctServers");
		option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	}
};

struct AssertGetEventsArg
  : public AssertGetHostResourceArg<EventInfo, EventsQueryOption>
{
	uint64_t limitOfUnifiedId;
	EventsQueryOption::SortType sortType;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;
	std::map<const EventInfo *, uint64_t> idMap;

	AssertGetEventsArg(gconstpointer ddtParam)
	: limitOfUnifiedId(0), sortType(EventsQueryOption::SORT_UNIFIED_ID),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL)
	{
		fixtures = testEventInfo;
		numberOfFixtures = NumTestEventInfo;
		fixupIdMap();
		if (ddtParam)
			setDataDrivenTestParam(ddtParam);
	}

	virtual void fixupIdMap(void)
	{
		for (size_t i = 0; i < numberOfFixtures; i++)
			idMap[&fixtures[i]] = i + 1;
	}

	virtual void fixupOption(void) // override
	{
		AssertGetHostResourceArg<EventInfo, EventsQueryOption>::
			fixupOption();
		if (maxNumber)
			option.setMaximumNumber(maxNumber);
		if (offset)
			option.setOffset(offset);
		if (limitOfUnifiedId)
			option.setLimitOfUnifiedId(limitOfUnifiedId);
		option.setSortType(sortType, sortDirection);
		option.setMinimumSeverity(minSeverity);
		option.setTriggerStatus(triggerStatus);
	}

	virtual bool filterOutExpectedRecord(EventInfo *info) // override
	{
  		if (AssertGetHostResourceArg <EventInfo, EventsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}
		if (limitOfUnifiedId && idMap[info] > limitOfUnifiedId)
			return true;

		// TODO: Use TriggerInfo instead of EventInfo because actual
		//       EventInfo doesn't store correct severity.
		if (info->severity < minSeverity)
			return true;

		if (triggerStatus != TRIGGER_STATUS_ALL &&
		    info->status != triggerStatus) {
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

	virtual void fixupExpectedRecords(void) // override
	{
		AssertGetHostResourceArg<EventInfo, EventsQueryOption>::
			fixupExpectedRecords();
		
		if (sortType == EventsQueryOption::SORT_TIME)
			std::sort(expectedRecords.begin(),
				  expectedRecords.end(),
				  lessTime());
	}

	virtual uint64_t getHostId(EventInfo &info)
	{
		return info.hostId;
	}

	virtual std::string makeOutputText(const EventInfo &eventInfo)
	{
		return makeEventOutput(eventInfo);
	}
};
