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

struct HatoholArmPluginBase::AsyncCbData {
	HatoholArmPluginBase *obj;
	void                 *priv;

	AsyncCbData(HatoholArmPluginBase *_obj, void *_priv)
	: obj(_obj),
	  priv(_priv)
	{
	}
};

typedef void (*AsyncCallback)(HatoholArmPluginBase::AsyncCbData *data);

struct HatoholArmPluginBase::PrivateContext {
	SimpleSemaphore replyWaitSem;
	SmartBuffer     responseBuf;
	AsyncCallback   currAsyncCb;
	AsyncCbData    *currAsyncCbData;

	PrivateContext(void)
	: replyWaitSem(0),
	  currAsyncCb(NULL),
	  currAsyncCbData(NULL)
	{
	}
};

// ---------------------------------------------------------------------------
// GetMonitoringServerInfoAsyncArg
// ---------------------------------------------------------------------------
HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::GetMonitoringServerInfoAsyncArg(MonitoringServerInfo *serverInfo)
: m_serverInfo(serverInfo)
{
}

void HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::doneCb(
  const bool &succeeded)
{
}

MonitoringServerInfo &
  HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::getMonitoringServerInfo(void)
{
	return *m_serverInfo;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginBase::HatoholArmPluginBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	const char *env = getenv(ENV_NAME_QUEUE_ADDR);
	if (env)
		setQueueAddress(env);

	registerCommandHandler(
	  HAPI_CMD_REQ_TERMINATE,
	  (CommandHandler)
	    &HatoholArmPluginBase::cmdHandlerTerminate);
}

HatoholArmPluginBase::~HatoholArmPluginBase()
{
	if (m_ctx)
		delete m_ctx;
}

bool HatoholArmPluginBase::getMonitoringServerInfo(
  MonitoringServerInfo &serverInfo)
{
	sendCmdGetMonitoringServerInfo();
	waitResponseAndCheckHeader();
	return parseReplyGetMonitoringServerInfo(serverInfo);
}

void HatoholArmPluginBase::getMonitoringServerInfoAsync(
  GetMonitoringServerInfoAsyncArg *arg)
{
	HATOHOL_ASSERT(!m_ctx->currAsyncCb,
	               "Async. process is already running.");
	sendCmdGetMonitoringServerInfo();
	m_ctx->currAsyncCb = _getMonitoringServerInfoAsyncCb;
	m_ctx->currAsyncCbData = new AsyncCbData(this, arg);
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
	if (m_ctx->currAsyncCb) {
		(*m_ctx->currAsyncCb)(m_ctx->currAsyncCbData);
		m_ctx->currAsyncCb = NULL;
		m_ctx->currAsyncCbData = NULL;
		return;
	}
	m_ctx->replyWaitSem.post();
}

void HatoholArmPluginBase::onReceivedTerminate(void)
{
	MLPL_INFO("Got the teminate command.\n");
	exit(EXIT_SUCCESS);
}

void HatoholArmPluginBase::sendCmdGetMonitoringServerInfo(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_MONITORING_SERVER_INFO);
	send(cmdBuf);
}

bool HatoholArmPluginBase::parseReplyGetMonitoringServerInfo(
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

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->userNameOffset, svInfo->userNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: userName.\n");
		return false;
	}
	serverInfo.userName = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->passwordOffset, svInfo->passwordLength);
	if (!str) {
		MLPL_ERR("Broken packet: password.\n");
		return false;
	}
	serverInfo.password = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->dbNameOffset, svInfo->dbNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: dbName.\n");
		return false;
	}
	serverInfo.dbName = str;

	serverInfo.port               = LtoN(svInfo->port);
	serverInfo.pollingIntervalSec = LtoN(svInfo->pollingIntervalSec);
	serverInfo.retryIntervalSec   = LtoN(svInfo->retryIntervalSec);

	return true;
}

void HatoholArmPluginBase::_getMonitoringServerInfoAsyncCb(
  HatoholArmPluginBase::AsyncCbData *data)
{
	GetMonitoringServerInfoAsyncArg *arg =
	  static_cast<GetMonitoringServerInfoAsyncArg *>(data->priv);
	data->obj->getMonitoringServerInfoAsyncCb(arg);
	delete data;
}

void HatoholArmPluginBase::getMonitoringServerInfoAsyncCb(
  HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg *arg)
{
	bool succeeded = parseReplyGetMonitoringServerInfo(
	                   arg->getMonitoringServerInfo());
	arg->doneCb(succeeded);
}

void HatoholArmPluginBase::waitResponseAndCheckHeader(void)
{
	HATOHOL_ASSERT(!m_ctx->currAsyncCb,
	               "Async. process is already running.");
	// TODO: Fix a potential bug.
	// If the initiation happens while the following wait() is running,
	// it may not return forever. It will look that the thread running
	// this method stalls.
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

void HatoholArmPluginBase::sendArmInfo(const ArmInfo &armInfo)
{
	SmartBuffer cmdBuf;
	const size_t failureCommentLen = armInfo.failureComment.size();
	const size_t additionalSize = failureCommentLen + 1;
	HapiArmInfo *body =
	  setupCommandHeader<HapiArmInfo>(cmdBuf, HAPI_CMD_SEND_ARM_INFO,
	                                  additionalSize);
	body->running = NtoL(armInfo.running);
	body->stat    = NtoL(armInfo.stat);

	const timespec *ts = &armInfo.statUpdateTime.getAsTimespec();
	body->statUpdateTime = NtoL(ts->tv_sec);
	body->statUpdateTimeNanosec = NtoL(ts->tv_nsec);

	ts = &armInfo.lastSuccessTime.getAsTimespec();
	body->lastSuccessTime = NtoL(ts->tv_sec);
	body->lastSuccessTimeNanosec = NtoL(ts->tv_nsec);

	ts = &armInfo.lastFailureTime.getAsTimespec();
	body->lastFailureTime = NtoL(ts->tv_sec);
	body->lastFailureTimeNanosec = NtoL(ts->tv_nsec);

	body->numUpdate  = NtoL(armInfo.numUpdate);
	body->numFailure = NtoL(armInfo.numFailure);

	char *buf = reinterpret_cast<char *>(body) + sizeof(HapiArmInfo);
	buf = putString(buf, body, armInfo.failureComment,
	                &body->failureCommentOffset,
	                &body->failureCommentLength);
	send(cmdBuf);
}

void HatoholArmPluginBase::cmdHandlerTerminate(const HapiCommandHeader *header)
{
	onReceivedTerminate();
}
