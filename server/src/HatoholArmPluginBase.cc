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
#include <MutexLock.h>
#include <Reaper.h>
#include "HatoholArmPluginBase.h"

using namespace mlpl;

const size_t HatoholArmPluginBase::WAIT_INFINITE = 0;

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
	SmartBuffer     responseBuf;
	AsyncCallback   currAsyncCb;
	AsyncCbData    *currAsyncCbData;


	PrivateContext(void)
	: currAsyncCb(NULL),
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
	struct Callback : public CommandCallbacks {
		HatoholArmPluginBase *obj;
		SimpleSemaphore sem;
		bool succeeded;
		bool parseResult;
		MonitoringServerInfo &serverInfo;

		Callback(HatoholArmPluginBase *_obj,
		         MonitoringServerInfo &_serverInfo)
		: CommandCallbacks(false), // autoDelete
		  obj(_obj),
		  sem(0),
		  succeeded(false),
		  parseResult(false),
		  serverInfo(_serverInfo)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			Reaper<SimpleSemaphore>
			   poster(&sem, SimpleSemaphore::post);
			parseResult = obj->parseReplyGetMonitoringServerInfo(
			                serverInfo, replyBuf);
			succeeded = true;
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			sem.post();
		}
	} cb(this, serverInfo);

	sendCmdGetMonitoringServerInfo(&cb);
	cb.sem.wait();
	if (!cb.succeeded) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER\n");
	}
	return cb.parseResult;
}

void HatoholArmPluginBase::getMonitoringServerInfoAsync(
  GetMonitoringServerInfoAsyncArg *arg)
{
	HATOHOL_ASSERT(!m_ctx->currAsyncCb,
	               "Async. process is already running.");
	sendCmdGetMonitoringServerInfo(new CommandCallbacks());
	m_ctx->currAsyncCb = _getMonitoringServerInfoAsyncCb;
	m_ctx->currAsyncCbData = new AsyncCbData(this, arg);
}

SmartTime HatoholArmPluginBase::getTimestampOfLastTrigger(void)
{
	struct Callback : public CommandCallbacks {
		HatoholArmPluginBase *obj;
		SimpleSemaphore sem;
		timespec ts;
		bool succeeded;

		Callback(HatoholArmPluginBase *_obj)
		: CommandCallbacks(false), // autoDelete
		  obj(_obj),
		  sem(0),
		  succeeded(false)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			Reaper<SimpleSemaphore>
			   poster(&sem, SimpleSemaphore::post);
			const HapiResTimestampOfLastTrigger *body =
			  obj->getResponseBody
			    <HapiResTimestampOfLastTrigger>(replyBuf);
			ts.tv_sec  = LtoN(body->timestamp);
			ts.tv_nsec = LtoN(body->nanosec);
			succeeded = true;
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			sem.post();
		}
	} cb(this);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER);
	send(cmdBuf, &cb);
	cb.sem.wait();
	if (!cb.succeeded) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER\n");
	}
	return SmartTime(cb.ts);
}

EventIdType HatoholArmPluginBase::getLastEventId(void)
{
	struct Callback : public CommandCallbacks {
		HatoholArmPluginBase *obj;
		SimpleSemaphore sem;
		EventIdType eventId;
		bool succeeded;

		Callback(HatoholArmPluginBase *_obj)
		: CommandCallbacks(false), // autoDelete
		  obj(_obj),
		  sem(0),
		  eventId(INVALID_EVENT_ID),
		  succeeded(false)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			Reaper<SimpleSemaphore>
			   poster(&sem, SimpleSemaphore::post);
			const HapiResLastEventId *body =
			  obj->getResponseBody<HapiResLastEventId>(replyBuf);
			eventId = body->lastEventId;
			succeeded = true;
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			sem.post();
		}
	} cb(this);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_GET_LAST_EVENT_ID);
	send(cmdBuf, &cb);
	cb.sem.wait();
	if (!cb.succeeded) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_LAST_EVENT_ID\n");
	}
	return cb.eventId;
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
}

void HatoholArmPluginBase::onInitiated(void)
{
}

void HatoholArmPluginBase::onReceivedTerminate(void)
{
	MLPL_INFO("Got the teminate command.\n");
	exit(EXIT_SUCCESS);
}

void HatoholArmPluginBase::sendCmdGetMonitoringServerInfo(
  CommandCallbacks *callbacks)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_MONITORING_SERVER_INFO);
	send(cmdBuf, callbacks);
}

bool HatoholArmPluginBase::parseReplyGetMonitoringServerInfo(
  MonitoringServerInfo &serverInfo, const SmartBuffer &responseBuf)
{
	const HapiResMonitoringServerInfo *svInfo =
	  getResponseBody<HapiResMonitoringServerInfo>(responseBuf);
	serverInfo.id   = LtoN(svInfo->serverId);
	serverInfo.type = static_cast<MonitoringSystemType>(LtoN(svInfo->type));

	const char *str;
	str = getString(responseBuf, svInfo,
	                svInfo->hostNameOffset, svInfo->hostNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: hostName.\n");
		return false;
	}
	serverInfo.hostName = str;

	str = getString(responseBuf, svInfo,
	                svInfo->ipAddressOffset, svInfo->ipAddressLength);
	if (!str) {
		MLPL_ERR("Broken packet: ipAddress.\n");
		return false;
	}
	serverInfo.ipAddress = str;

	str = getString(responseBuf, svInfo,
	                svInfo->nicknameOffset, svInfo->nicknameLength);
	if (!str) {
		MLPL_ERR("Broken packet: nickname.\n");
		return false;
	}
	serverInfo.nickname = str;

	str = getString(responseBuf, svInfo,
	                svInfo->userNameOffset, svInfo->userNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: userName.\n");
		return false;
	}
	serverInfo.userName = str;

	str = getString(responseBuf, svInfo,
	                svInfo->passwordOffset, svInfo->passwordLength);
	if (!str) {
		MLPL_ERR("Broken packet: password.\n");
		return false;
	}
	serverInfo.password = str;

	str = getString(responseBuf, svInfo,
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
	                   arg->getMonitoringServerInfo(), m_ctx->responseBuf);
	arg->doneCb(succeeded);
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
