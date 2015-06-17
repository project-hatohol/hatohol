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

#include "DataStoreNagios.h"

#include <cstdio>

struct DataStoreNagios::Impl
{
	ArmNagiosNDOUtils  armNDO;

	Impl(const MonitoringServerInfo &serverInfo)
	: armNDO(serverInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreNagios::DataStoreNagios(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: m_impl(new Impl(serverInfo))
{
	if (autoStart)
		m_impl->armNDO.start();
}

DataStoreNagios::~DataStoreNagios()
{
}

const MonitoringServerInfo &DataStoreNagios::getMonitoringServerInfo(void)
  const
{
	return m_impl->armNDO.getServerInfo();
}

const ArmStatus &DataStoreNagios::getArmStatus(void) const
{
	return m_impl->armNDO.getArmStatus();
}

void DataStoreNagios::setCopyOnDemandEnable(bool enable)
{
	m_impl->armNDO.setCopyOnDemandEnabled(enable);
}

bool DataStoreNagios::isFetchItemsSupported(void)
{
	return m_impl->armNDO.isFetchItemsSupported();
}

bool DataStoreNagios::startOnDemandFetchItems(
  const LocalHostIdVector &hostIds, Closure0 *closure)
{
	m_impl->armNDO.fetchItems(closure);
	return true;
}

bool DataStoreNagios::startOnDemandFetchTriggers(
  const LocalHostIdVector &hostIds, Closure0 *closure)
{
	m_impl->armNDO.fetchTriggers(closure);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
