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

#include <ReadWriteLock.h>
#include "HatoholArmPluginStandard.h"

using namespace hfl;

struct HatoholArmPluginStandard::Impl {
	ReadWriteLock        lock;
	MonitoringServerInfo serverInfo;

	void set(MonitoringServerInfo &_serverInfo)
	{
		lock.writeLock();
		serverInfo = _serverInfo;
		lock.unlock();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginStandard::HatoholArmPluginStandard(void)
: m_impl(new Impl())
{
}

HatoholArmPluginStandard::~HatoholArmPluginStandard()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholArmPluginStandard::parseReplyGetMonitoringServerInfoOnInitiated(
  MonitoringServerInfo &serverInfo, const SmartBuffer &replyBuf)
{
	if (!parseReplyGetMonitoringServerInfo(serverInfo, replyBuf)) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to parse the reply for monitoring server "
		  "information.\n");
	}
	m_impl->set(serverInfo);
}

void HatoholArmPluginStandard::onInitiated(void)
{
	// TODO: This code is almost the same as HapZabbicAPI::onInitiated().
	// HapZabbix should inherit this class.
	HatoholArmPluginBase::onInitiated();

	struct Callback : public CommandCallbacks {
		HatoholArmPluginStandard *obj;

		Callback(HatoholArmPluginStandard *_obj)
		: obj(_obj)
		{
		}

		virtual void onGotReply(
		  SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			MonitoringServerInfo serverInfo;
			obj->parseReplyGetMonitoringServerInfoOnInitiated(
			  serverInfo, replyBuf);
			obj->onReady(serverInfo);
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			THROW_HATOHOL_EXCEPTION(
			  "Failed to get monitoring server information. "
			  "This process will try it when an initiation "
			  "happens again.\n");
		}
	} *cb = new Callback(this);
	sendCmdGetMonitoringServerInfo(cb);
}

void HatoholArmPluginStandard::onReady(const MonitoringServerInfo &serverInfo)
{
}
