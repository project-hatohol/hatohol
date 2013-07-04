/*
 * Copyright (C) 2013 Project Hatohol
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

#include <stdexcept>
#include <MutexLock.h>
#include "UnifiedDataStore.h"
#include "VirtualDataStoreZabbix.h"
#include "VirtualDataStoreNagios.h"

using namespace mlpl;

struct UnifiedDataStore::PrivateContext
{
	static UnifiedDataStore *instance;
	static MutexLock         mutex;
	VirtualDataStoreZabbix *vdsZabbix;
	VirtualDataStoreNagios *vdsNagios;
};

UnifiedDataStore *UnifiedDataStore::PrivateContext::instance = NULL;
MutexLock UnifiedDataStore::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	// start VirtualDataStoreZabbix
	m_ctx->vdsZabbix = VirtualDataStoreZabbix::getInstance();

	// start VirtualDataStoreNagios
	m_ctx->vdsNagios = VirtualDataStoreNagios::getInstance();
}

UnifiedDataStore::~UnifiedDataStore()
{
	if (m_ctx)
		delete m_ctx;
}

UnifiedDataStore *UnifiedDataStore::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::mutex.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new UnifiedDataStore();
	PrivateContext::mutex.unlock();

	return PrivateContext::instance;
}


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	// start VirtualDataStoreZabbix
	m_ctx->vdsZabbix = VirtualDataStoreZabbix::getInstance();

	// start VirtualDataStoreNagios
	m_ctx->vdsNagios = VirtualDataStoreNagios::getInstance();
}

UnifiedDataStore::~UnifiedDataStore()
{
	if (m_ctx)
		delete m_ctx;
}

void UnifiedDataStore::start(void)
{
	m_ctx->vdsZabbix->start();
	m_ctx->vdsNagios->start();
}

void UnifiedDataStore::stop(void)
{
	m_ctx->vdsZabbix->stop();
	m_ctx->vdsNagios->stop();
}

void UnifiedDataStore::getTriggerList(TriggerInfoList &triggerList,
                                      uint32_t targetServerId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList, targetServerId);
}

void UnifiedDataStore::getEventList(EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getEventInfoList(eventList);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList,
                                   uint32_t targetServerId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList, targetServerId);
}

void UnifiedDataStore::getHostList(HostInfoList &hostInfoList,
                                   uint32_t targetServerId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getHostInfoList(hostInfoList, targetServerId);
}

int UnifiedDataStore::getNumberOfHosts(uint32_t serverId,
                                       uint64_t hostGroupId,
                                       TriggerSeverityType severity)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfHosts(serverId, hostGroupId, severity);
}

int UnifiedDataStore::getNumberOfGoodHosts(uint32_t serverId,
                                           uint64_t hostGroupId)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfGoodHosts(serverId, hostGroupId);
}

int UnifiedDataStore::getNumberOfBadHosts(uint32_t serverId,
                                          uint64_t hostGroupId)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfBadHosts(serverId, hostGroupId);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
