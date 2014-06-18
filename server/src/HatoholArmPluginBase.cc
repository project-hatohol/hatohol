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

#include <cstdio>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginBase.h"

using namespace mlpl;

struct HatoholArmPluginBase::PrivateContext {
	SimpleSemaphore replyWaitSem;
	SmartBuffer     responseBuf;

	PrivateContext(void)
	: replyWaitSem(0)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginBase::HatoholArmPluginBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HatoholArmPluginBase::~HatoholArmPluginBase()
{
	if (m_ctx)
		delete m_ctx;
}

bool HatoholArmPluginBase::getMonitoringServerInfo(
  MonitoringServerInfo &serverInfo)
{
	getMonitoringServerInfoTopHalf();
	waitResponseAndCheckHeader();
	return getMonitoringServerInfoBottomHalf(serverInfo);
}

SmartTime HatoholArmPluginBase::getTimestampOfLastTrigger(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER);
	send(cmdBuf);
	waitResponseAndCheckHeader();

	const HapiResTimestampOfLastTrigger *body = 
	  getResponseBody<HapiResTimestampOfLastTrigger>(m_ctx->responseBuf);
	timespec ts;
	ts.tv_sec  = LtoN(body->timestamp);
	ts.tv_nsec = LtoN(body->nanosec);
	return SmartTime(ts);
}

EventIdType HatoholArmPluginBase::getLastEventId(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_GET_LAST_EVENT_ID);
	send(cmdBuf);
	waitResponseAndCheckHeader();

	const HapiResLastEventId *body =
	  getResponseBody<HapiResLastEventId>(m_ctx->responseBuf);
	return body->lastEventId;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholArmPluginBase::onGotResponse(
  const HapiResponseHeader *header, SmartBuffer &resBuf)
{
	resBuf.handOver(m_ctx->responseBuf);
	m_ctx->replyWaitSem.post();
}

void HatoholArmPluginBase::getMonitoringServerInfoTopHalf(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_MONITORING_SERVER_INFO);
	send(cmdBuf);
}

bool HatoholArmPluginBase::getMonitoringServerInfoBottomHalf(
  MonitoringServerInfo &serverInfo)
{
	const HapiResMonitoringServerInfo *svInfo =
	  getResponseBody<HapiResMonitoringServerInfo>(m_ctx->responseBuf);
	serverInfo.id   = LtoN(svInfo->serverId);
	serverInfo.type = static_cast<MonitoringSystemType>(LtoN(svInfo->type));

	const char *str;
	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->hostNameOffset, svInfo->hostNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: hostName.\n");
		return false;
	}
	serverInfo.hostName = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->ipAddressOffset, svInfo->ipAddressLength);
	if (!str) {
		MLPL_ERR("Broken packet: ipAddress.\n");
		return false;
	}
	serverInfo.ipAddress = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->nicknameOffset, svInfo->nicknameLength);
	if (!str) {
		MLPL_ERR("Broken packet: nickname.\n");
		return false;
	}
	serverInfo.nickname = str;

	serverInfo.port               = LtoN(svInfo->port);
	serverInfo.pollingIntervalSec = LtoN(svInfo->pollingIntervalSec);
	serverInfo.retryIntervalSec   = LtoN(svInfo->retryIntervalSec);

	return true;
}

void HatoholArmPluginBase::waitResponseAndCheckHeader(void)
{
	m_ctx->replyWaitSem.wait();

	// To check the sainity of the header
	getResponseHeader(m_ctx->responseBuf);
}

void HatoholArmPluginBase::sendTable(
  const HapiCommandCode &code, const ItemTablePtr &tablePtr)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, code);
	appendItemTable(cmdBuf, tablePtr);
	send(cmdBuf);
}
