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

#include "HatoholArmPluginGate.h"

struct HatoholArmPluginGate::PrivateContext
{
	MonitoringServerInfo serverInfo; // we have a copy.
	ArmStatus            armStatus;

	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

void HatoholArmPluginGate::start(void)
{
	HatoholThreadBase::start();
	m_ctx->armStatus.setRunningStatus(true);
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

gpointer HatoholArmPluginGate::mainThread(HatoholThreadArg *arg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

void HatoholArmPluginGate::waitExit(void)
{
	HatoholThreadBase::waitExit();
	m_ctx->armStatus.setRunningStatus(false);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	if (m_ctx)
		delete m_ctx;
}

