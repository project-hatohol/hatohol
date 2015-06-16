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

#include <cstdio>
#include "DataStoreZabbix.h"
using namespace std;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreZabbix::DataStoreZabbix(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: m_armApi(serverInfo)
{
	if (autoStart)
		m_armApi.start();
}

DataStoreZabbix::~DataStoreZabbix(void)
{
}

const MonitoringServerInfo &DataStoreZabbix::getMonitoringServerInfo(void)
  const
{
	return m_armApi.getServerInfo();
}

const ArmStatus &DataStoreZabbix::getArmStatus(void) const
{
	return m_armApi.getArmStatus();
}

void DataStoreZabbix::setCopyOnDemandEnable(bool enable)
{
	m_armApi.setCopyOnDemandEnabled(enable);
}

bool DataStoreZabbix::isFetchItemsSupported(void)
{
	return m_armApi.isFetchItemsSupported();
}

bool DataStoreZabbix::startOnDemandFetchItems(const HostIdVector &hostIds, Closure0 *closure)
{
	m_armApi.fetchItems(closure);
	return true;
}

void DataStoreZabbix::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	m_armApi.fetchHistory(itemInfo, beginTime, endTime, closure);
}

bool DataStoreZabbix::startOnDemandFetchTriggers(
  const HostIdVector &hostIds, Closure0 *closure)
{
	m_armApi.fetchTriggers(closure);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
