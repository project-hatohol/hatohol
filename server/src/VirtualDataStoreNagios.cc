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
#include "VirtualDataStoreNagios.h"
#include "DataStoreNagios.h"

using namespace mlpl;

struct VirtualDataStoreNagios::PrivateContext
{
	static VirtualDataStoreNagios *instance;
	static MutexLock               mutex;
};

VirtualDataStoreNagios *VirtualDataStoreNagios::PrivateContext::instance = NULL;
MutexLock VirtualDataStoreNagios::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
VirtualDataStoreNagios *VirtualDataStoreNagios::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::mutex.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new VirtualDataStoreNagios();
	PrivateContext::mutex.unlock();

	return PrivateContext::instance;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void VirtualDataStoreNagios::start(void)
{
	VirtualDataStore::start<DataStoreNagios>(MONITORING_SYSTEM_NAGIOS);
}

void VirtualDataStoreNagios::update(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
VirtualDataStoreNagios::VirtualDataStoreNagios(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

VirtualDataStoreNagios::~VirtualDataStoreNagios()
{
	if (m_ctx)
		delete m_ctx;
}
