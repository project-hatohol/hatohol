/*
 * Copyright (C) 2014 Project Hatohol
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
#include "VirtualDataStoreFake.h"
#include "DataStoreFake.h"

using namespace mlpl;

struct VirtualDataStoreFake::PrivateContext
{
	static VirtualDataStoreFake *instance;
	static MutexLock             mutex;
};

VirtualDataStoreFake *VirtualDataStoreFake::PrivateContext::instance = NULL;
MutexLock VirtualDataStoreFake::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
VirtualDataStoreFake *VirtualDataStoreFake::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::mutex.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new VirtualDataStoreFake();
	PrivateContext::mutex.unlock();

	return PrivateContext::instance;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void VirtualDataStoreFake::start(const bool &autoRun)
{
	VirtualDataStore::start<DataStoreFake>(MONITORING_SYSTEM_FAKE, autoRun);
}

HatoholError VirtualDataStoreFake::start(const MonitoringServerInfo &svInfo,
                                         const bool &autoRun)
{
	return VirtualDataStore::start<DataStoreFake>
	         (MONITORING_SYSTEM_FAKE, svInfo, autoRun);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
VirtualDataStoreFake::VirtualDataStoreFake(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

VirtualDataStoreFake::~VirtualDataStoreFake()
{
	if (m_ctx)
		delete m_ctx;
}
