/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef DataStore_h
#define DataStore_h

#include <map>
#include <vector>
#include <Monitoring.h>
#include <MonitoringServerInfo.h>
#include <ArmStatus.h>
#include "Closure.h"
#include "Params.h"

class DataStore {
public:
	DataStore(void);

	virtual const MonitoringServerInfo
	  &getMonitoringServerInfo(void) const = 0;
	virtual const ArmStatus &getArmStatus(void) const = 0;
	virtual bool isFetchItemsSupported(void);
	virtual bool startOnDemandFetchItems(
	  const LocalHostIdVector &hostIds,
	  Closure0 *closure);
	virtual bool startOnDemandFetchTriggers(
	  const LocalHostIdVector &hostIds,
	  Closure0 *closure);
	virtual void startOnDemandFetchHistory(
	  const ItemInfo &itemInfo,
	  const time_t &beginTime,
	  const time_t &endTime,
	  Closure1<HistoryInfoVect> *closure);
	virtual bool startOnDemandFetchEvents(
	  const std::string &lastInfo,
	  const size_t count,
	  const bool ascending,
	  Closure0 *closure);
protected:
	virtual ~DataStore();
};

typedef std::vector<std::shared_ptr<DataStore>> DataStoreVector;
typedef DataStoreVector::iterator               DataStoreVectorIterator;
typedef DataStoreVector::const_iterator         DataStoreVectorConstIterator;

#endif // DataStore_h
