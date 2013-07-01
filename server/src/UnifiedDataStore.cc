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

using namespace mlpl;

struct UnifiedDataStore::PrivateContext
{
	static UnifiedDataStore *instance;
	static MutexLock         mutex;
};

UnifiedDataStore *UnifieddataStore::PrivateContext::instance = NULL;
MutexLock UnifieddataStore::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
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
void UnifiedDataStore::start(void)
{
}

void UnifiedDataStore::getTriggerList(TriggerInfoList &triggerList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList);
}

void UnifiedDataStore::getEventList(EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getEventInfoList(eventList);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList);
}


// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

UnifiedDataStore::~UnifiedDataStore()
{
	if (m_ctx)
		delete m_ctx;
}
