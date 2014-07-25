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
#include <Mutex.h>
#include <Reaper.h>
#include "HatoholArmPluginBase.h"

using namespace mlpl;

const size_t HatoholArmPluginBase::WAIT_INFINITE = 0;

class HatoholArmPluginBase::SyncCommand : public CommandCallbacks {
public:
	SyncCommand(HatoholArmPluginBase *obj)
	: m_obj(obj),
	  m_sem(0),
	  m_succeeded(false)
	{
	}

	virtual void onGotReply(
	  const mlpl::SmartBuffer &replyBuf,
	  const HapiCommandHeader &cmdHeader) override
	{
	}

	virtual void onError(
	  const HapiResponseCode &code,
	  const HapiCommandHeader &cmdHeader) override
	{
		post();
	}

	void wait(void)
	{
		m_sem.wait();
	}

	bool getSucceeded(void) const
	{
		return m_succeeded;
	}

protected:
	struct SemaphorePoster {
		SyncCommand *syncCmd;

		SemaphorePoster(SyncCommand *_syncCmd)
		: syncCmd(_syncCmd)
		{
		}

		virtual ~SemaphorePoster()
		{
			syncCmd->post();
		}
	};

	void setSucceeded(const bool &succeeded = true)
	{
		m_succeeded = succeeded;
	}

	void post(void)
	{
		m_sem.post();
	}

	HatoholArmPluginBase *getObject(void) const
	{
		return m_obj;
	}

private:
	HatoholArmPluginBase *m_obj;
	SimpleSemaphore       m_sem;
	bool                  m_succeeded;

};

struct HatoholArmPluginBase::PrivateContext {
};

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
	delete m_ctx;
}

bool HatoholArmPluginBase::getMonitoringServerInfo(
  MonitoringServerInfo &serverInfo)
{
	struct Callback : public SyncCommand {
		bool parseResult;
		MonitoringServerInfo &serverInfo;

		Callback(HatoholArmPluginBase *obj,
		         MonitoringServerInfo &_serverInfo)
		: SyncCommand(obj),
		  parseResult(false),
		  serverInfo(_serverInfo)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			SemaphorePoster poster(this);
			parseResult =
			  getObject()->parseReplyGetMonitoringServerInfo(
			    serverInfo, replyBuf);
			setSucceeded();
		}
	} *cb = new Callback(this, serverInfo);
	Reaper<UsedCountable> reaper(cb, UsedCountable::unref);

	sendCmdGetMonitoringServerInfo(cb);
	cb->wait();
	if (!cb->getSucceeded()) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER\n");
	}
	return cb->parseResult;
}

SmartTime HatoholArmPluginBase::getTimestampOfLastTrigger(void)
{
	struct Callback : public SyncCommand {
		timespec ts;

		Callback(HatoholArmPluginBase *obj)
		: SyncCommand(obj)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			SemaphorePoster poster(this);
			const HapiResTimestampOfLastTrigger *body =
			  getObject()->getResponseBody
			    <HapiResTimestampOfLastTrigger>(replyBuf);
			ts.tv_sec  = LtoN(body->timestamp);
			ts.tv_nsec = LtoN(body->nanosec);
			setSucceeded();
		}

	} *cb = new Callback(this);
	Reaper<UsedCountable> reaper(cb, UsedCountable::unref);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER);
	send(cmdBuf, cb);
	cb->wait();
	if (!cb->getSucceeded()) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER\n");
	}
	return SmartTime(cb->ts);
}

EventIdType HatoholArmPluginBase::getLastEventId(void)
{
	struct Callback : public SyncCommand {
		EventIdType eventId;

		Callback(HatoholArmPluginBase *obj)
		: SyncCommand(obj),
		  eventId(INVALID_EVENT_ID)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			SemaphorePoster poster(this);
			const HapiResLastEventId *body =
			  getObject()->getResponseBody
			    <HapiResLastEventId>(replyBuf);
			eventId = body->lastEventId;
			setSucceeded();
		}
	} *cb = new Callback(this);
	Reaper<UsedCountable> reaper(cb, UsedCountable::unref);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_GET_LAST_EVENT_ID);
	send(cmdBuf, cb);
	cb->wait();
	if (!cb->getSucceeded()) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_LAST_EVENT_ID\n");
	}
	return cb->eventId;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
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
