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

#include <Logger.h>
#include "ArmCeilometer.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmCeilometer::PrivateContext
{
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmCeilometer::ArmCeilometer(const MonitoringServerInfo &serverInfo)
: ArmBase("ArmCeilometer", serverInfo),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ArmCeilometer::~ArmCeilometer()
{
	const MonitoringServerInfo &svInfo = getServerInfo();

	MLPL_INFO("ArmCeilometer [%d:%s]: exit process started.\n",
	          svInfo.id, svInfo.hostName.c_str());

	// wait for the finish of the thread
	requestExit();
	stop();

	if (m_ctx)
		delete m_ctx;
	MLPL_INFO("ArmCeilometer [%d:%s]: exit process completed.\n",
	          svInfo.id, svInfo.hostName.c_str());
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool ArmCeilometer::mainThreadOneProc(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}
