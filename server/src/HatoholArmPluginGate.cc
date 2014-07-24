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

#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <errno.h>
#include <string>
#include <cstring>
#include <AtomicValue.h>
#include <Mutex.h>
#include <Reaper.h>
#include <SimpleSemaphore.h>
#include "UnifiedDataStore.h"
#include "HatoholArmPluginGate.h"
#include "CacheServiceDBClient.h"
#include "ChildProcessManager.h"
#include "StringUtils.h"
#include "HostInfoCache.h"
#include "HatoholDBUtils.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const int   HatoholArmPluginGate::NO_RETRY = -1;
static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

static const size_t TIMEOUT_PLUGIN_TERM_CMD_MS     =  30 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGTERM_MS =  60 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGKILL_MS = 120 * 1000;

class ImpromptuArmBase : public ArmBase {
public:
	ImpromptuArmBase(const MonitoringServerInfo &serverInfo)
	: ArmBase("HatoholArmPluginGate", serverInfo)
	{
	}

	virtual bool mainThreadOneProc(void) override
	{
		// This method is never called because nobody calls start().
		// Just written to pass the build.
		return true;
	}

	virtual bool isFetchItemsSupported(void) const override
	{
		return false;
	}
};

struct HatoholArmPluginGate::PrivateContext
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo serverInfo;

	ImpromptuArmBase     armBase;
	ArmPluginInfo        armPluginInfo;
	ArmStatus            armStatus;
	AtomicValue<GPid>    pid;
	SimpleSemaphore      pluginTermSem;
	HostInfoCache        hostInfoCache;
	Mutex                exitSyncLock;
	bool                 exitSyncDone;

	PrivateContext(const MonitoringServerInfo &_serverInfo,
	               HatoholArmPluginGate *_hapg)
	: serverInfo(_serverInfo),
	  armBase(_serverInfo),
	  pid(0),
	  pluginTermSem(0),
	  exitSyncDone(false)
	{
	}

	/**
	 * Wait for the temination of the plugin process.
	 *
	 * @param timeoutInMSec A timeout value in millisecond.
	 *
	 * @return
	 * true if the plugin process is terminated within the given timeout
	 * value. Otherwise, false is returned.
	 */
	bool waitTermPlugin(const size_t &timeoutInMSec)
	{
		SimpleSemaphore::Status status =
		  pluginTermSem.timedWait(timeoutInMSec);
		return (status == SimpleSemaphore::STAT_OK);
	}
};

const string HatoholArmPluginGate::PassivePluginQuasiPath = "#PASSIVE_PLUGIN#";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: HatoholArmPluginInterface(true),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo, this);

	CacheServiceDBClient cache;
	const ServerIdType &serverId = m_ctx->serverInfo.id;
	DBClientConfig *dbConfig = cache.getConfig();
	if (!dbConfig->getArmPluginInfo(m_ctx->armPluginInfo, serverId)) {
		MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
		         serverId);
		return;
	}
	if (!m_ctx->armPluginInfo.brokerUrl.empty())
		setBrokerUrl(m_ctx->armPluginInfo.brokerUrl);

	string address;
	if (!m_ctx->armPluginInfo.staticQueueAddress.empty())
		address = m_ctx->armPluginInfo.staticQueueAddress;
	else
		address = generateBrokerAddress(m_ctx->serverInfo);
	setQueueAddress(address);

	registerCommandHandler(
	  HAPI_CMD_GET_MONITORING_SERVER_INFO,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetMonitoringServerInfo);

	registerCommandHandler(
	  HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetTimestampOfLastTrigger);

	registerCommandHandler(
	  HAPI_CMD_GET_LAST_EVENT_ID,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetLastEventId);

	registerCommandHandler(
	  HAPI_CMD_SEND_UPDATED_TRIGGERS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers);

	registerCommandHandler(
	  HAPI_CMD_SEND_HOSTS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendHosts);

	registerCommandHandler(
	  HAPI_CMD_SEND_HOST_GROUP_ELEMENTS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendHostgroupElements);

	registerCommandHandler(
	  HAPI_CMD_SEND_HOST_GROUPS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendHostgroups);

	registerCommandHandler(
	  HAPI_CMD_SEND_UPDATED_EVENTS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendUpdatedEvents);

	registerCommandHandler(
	  HAPI_CMD_SEND_ARM_INFO,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendArmInfo);
}

void HatoholArmPluginGate::start(void)
{
	m_ctx->armStatus.setRunningStatus(true);
	HatoholArmPluginInterface::start();
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

// TODO: remove this method
ArmBase &HatoholArmPluginGate::getArmBase(void)
{
	return m_ctx->armBase;
}

void HatoholArmPluginGate::exitSync(void)
{
	AutoMutex autoMutex(&m_ctx->exitSyncLock);
	if (m_ctx->exitSyncDone)
		return;
	MLPL_INFO("HatoholArmPluginGate: [%d:%s]: requested to exit.\n",
	          m_ctx->serverInfo.id, m_ctx->serverInfo.hostName.c_str());
	terminatePluginSync();
	HatoholArmPluginInterface::exitSync();
	m_ctx->armStatus.setRunningStatus(false);
	MLPL_INFO("  => [%d:%s]: done.\n",
	          m_ctx->serverInfo.id, m_ctx->serverInfo.hostName.c_str());
	m_ctx->exitSyncDone = true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	exitSync();
	if (m_ctx)
		delete m_ctx;
}

void HatoholArmPluginGate::onConnected(qpid::messaging::Connection &conn)
{
	if (m_ctx->pid)
		return;

	// TODO: Check the type.
	if (m_ctx->armPluginInfo.path == PassivePluginQuasiPath) {
		MLPL_INFO("Started: passive plugin (%d) %s\n",
		          m_ctx->armPluginInfo.type,
		          m_ctx->armPluginInfo.path.c_str());
		onLaunchedProcess(true, m_ctx->armPluginInfo);
		return;
	}

	// launch a plugin process
	bool succeeded = launchPluginProcess(m_ctx->armPluginInfo);
	onLaunchedProcess(succeeded, m_ctx->armPluginInfo);
}

int HatoholArmPluginGate::onCaughtException(const exception &e)
{
	MLPL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
	          e.what(), DEFAULT_RETRY_INTERVAL);
	return DEFAULT_RETRY_INTERVAL;
}

void HatoholArmPluginGate::onLaunchedProcess(
  const bool &succeeded, const ArmPluginInfo &armPluginInfo)
{
}

void HatoholArmPluginGate::onTerminated(const siginfo_t *siginfo)
{
	m_ctx->pid = 0;
	m_ctx->pluginTermSem.post();
}

void HatoholArmPluginGate::terminatePluginSync(void)
{
	// If the the plugin is a passive type, it will naturally return at
	// the following condition.
	if (!m_ctx->pid)
		return;

	// Send the teminate command.
	sendTerminateCommand();
	size_t timeoutMSec = TIMEOUT_PLUGIN_TERM_CMD_MS;
	if (m_ctx->waitTermPlugin(timeoutMSec))
		return;
	size_t elapsedTime = timeoutMSec;

	// Send SIGTERM
	timeoutMSec = TIMEOUT_PLUGIN_TERM_SIGKILL_MS - elapsedTime;
	MLPL_INFO("Send SIGTERM to the plugin and wait for %zd sec.\n",
	          timeoutMSec/1000);
	if (kill(m_ctx->pid, SIGTERM) == -1) {
		MLPL_ERR("Failed to send SIGTERM, pid: %d, errno: %d\n",
		         (int)m_ctx->pid, errno);
	}
	if (m_ctx->waitTermPlugin(timeoutMSec))
		return;
	elapsedTime += timeoutMSec;

	// Send SIGKILL
	timeoutMSec = TIMEOUT_PLUGIN_TERM_SIGKILL_MS - elapsedTime;
	MLPL_INFO("Send SIGKILL to the plugin and wait for %zd sec.\n",
	          timeoutMSec/1000);
	if (kill(m_ctx->pid, SIGKILL) == -1) {
		MLPL_ERR("Failed to send SIGKILL, pid: %d, errno: %d\n",
		         (int)m_ctx->pid, errno);
	}
	if (m_ctx->waitTermPlugin(timeoutMSec))
		return;
	elapsedTime += timeoutMSec;

	MLPL_WARN(
	  "The plugin (%d) hasn't terminated within a timeout. Ignore it.\n",
	  (int)m_ctx->pid);
}

bool HatoholArmPluginGate::launchPluginProcess(
  const ArmPluginInfo &armPluginInfo)
{
	struct EventCb : public ChildProcessManager::EventCallback {

		HatoholArmPluginGate *hapg;
		bool succeededInCreation;

		EventCb(HatoholArmPluginGate *_hapg)
		: hapg(_hapg),
		  succeededInCreation(false)
		{
		}

		virtual void onExecuted(const bool &succeeded,
		                        GError *gerror) override
		{
			succeededInCreation = succeeded;
		}

		virtual void onCollected(const siginfo_t *siginfo) override
		{
			hapg->onTerminated(siginfo);
		}
	} *eventCb = new EventCb(this);

	ChildProcessManager::CreateArg arg;
	arg.args.push_back(armPluginInfo.path);
	arg.addFlag(G_SPAWN_SEARCH_PATH);
	const char *ldlibpath = getenv("LD_LIBRARY_PATH");
	if (ldlibpath) {
		string env = "LD_LIBRARY_PATH=";
		env += ldlibpath;
		arg.envs.push_back(env);
	}
	arg.eventCb = eventCb;
	arg.envs.push_back(StringUtils::sprintf(
	  "%s=%s", ENV_NAME_QUEUE_ADDR,
	           generateBrokerAddress(m_ctx->serverInfo).c_str()));
	ChildProcessManager::getInstance()->create(arg);
	if (!eventCb->succeededInCreation) {
		MLPL_ERR("Failed to execute: (%d) %s\n",
		         armPluginInfo.type, armPluginInfo.path.c_str());
		return false;
	}

	m_ctx->pid = arg.pid;
	MLPL_INFO("Started: plugin (%d) %s\n",
	          armPluginInfo.type, armPluginInfo.path.c_str());
	return true;
}

string HatoholArmPluginGate::generateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return StringUtils::sprintf("hatohol-arm-plugin.%" FMT_SERVER_ID,
	                            serverInfo.id);
}

void HatoholArmPluginGate::sendTerminateCommand(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_REQ_TERMINATE);
	send(cmdBuf);
}

void HatoholArmPluginGate::cmdHandlerGetMonitoringServerInfo(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	const MonitoringServerInfo &svInfo = m_ctx->serverInfo;

	const size_t lenHostName  = svInfo.hostName.size();
	const size_t lenIpAddress = svInfo.ipAddress.size();
	const size_t lenNickname  = svInfo.nickname.size();
	const size_t lenUserName  = svInfo.userName.size();
	const size_t lenPassword  = svInfo.password.size();
	const size_t lenDbName    = svInfo.dbName.size();
	// +1: Null Term.
	const size_t addSize =
	  (lenHostName + 1) + (lenIpAddress + 1) + (lenNickname + 1) +
	  (lenUserName + 1) + (lenPassword + 1) + (lenDbName + 1);
	
	HapiResMonitoringServerInfo *body =
	  setupResponseBuffer<HapiResMonitoringServerInfo>(resBuf, addSize);
	body->serverId = NtoL(svInfo.id);
	body->type     = NtoL(svInfo.type);
	body->port     = NtoL(svInfo.port);
	body->pollingIntervalSec = NtoL(svInfo.pollingIntervalSec);
	body->retryIntervalSec   = NtoL(svInfo.retryIntervalSec);

	char *buf =
	   reinterpret_cast<char *>(body) + sizeof(HapiResMonitoringServerInfo);
	buf = putString(buf, body, svInfo.hostName,
	                &body->hostNameOffset, &body->hostNameLength);
	buf = putString(buf, body, svInfo.ipAddress,
	                &body->ipAddressOffset, &body->ipAddressLength);
	buf = putString(buf, body, svInfo.nickname,
	                &body->nicknameOffset, &body->nicknameLength);
	buf = putString(buf, body, svInfo.userName,
	                &body->userNameOffset, &body->userNameLength);
	buf = putString(buf, body, svInfo.password,
	                &body->passwordOffset, &body->passwordLength);
	buf = putString(buf, body, svInfo.dbName,
	                &body->dbNameOffset, &body->dbNameLength);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetTimestampOfLastTrigger(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	HapiResTimestampOfLastTrigger *body =
	  setupResponseBuffer<HapiResTimestampOfLastTrigger>(resBuf);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	SmartTime last = uds->getTimestampOfLastTrigger(m_ctx->serverInfo.id);
	const timespec &lastTimespec = last.getAsTimespec();
	body->timestamp = NtoL(lastTimespec.tv_sec);
	body->nanosec   = NtoL(lastTimespec.tv_nsec);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetLastEventId(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	HapiResLastEventId *body =
	  setupResponseBuffer<HapiResLastEventId>(resBuf);
	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	body->lastEventId = dbHatohol->getLastEventId(m_ctx->serverInfo.id);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr tablePtr = createItemTable(*cmdBuf);

	TriggerInfoList trigInfoList;
	HatoholDBUtils::transformTriggersToHatoholFormat(
	  trigInfoList, tablePtr, m_ctx->serverInfo.id, m_ctx->hostInfoCache);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addTriggerInfoList(trigInfoList);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHosts(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostTablePtr = createItemTable(*cmdBuf);

	HostInfoList hostInfoList;
	HatoholDBUtils::transformHostsToHatoholFormat(
	  hostInfoList, hostTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostInfoList(hostInfoList);

	// TODO: consider if DBClientHatohol should have the cache
	HostInfoListConstIterator hostInfoItr = hostInfoList.begin();
	for (; hostInfoItr != hostInfoList.end(); ++hostInfoItr)
		m_ctx->hostInfoCache.update(*hostInfoItr);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHostgroupElements(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupElementTablePtr = createItemTable(*cmdBuf);

	HostgroupElementList hostgroupElementList;
	HostInfoList hostInfoList;
	HatoholDBUtils::transformHostsGroupsToHatoholFormat(
	  hostgroupElementList, hostgroupElementTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostgroupElementList(hostgroupElementList);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHostgroups(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupTablePtr = createItemTable(*cmdBuf);

	HostgroupInfoList hostgroupInfoList;
	HatoholDBUtils::transformGroupsToHatoholFormat(
	  hostgroupInfoList, hostgroupTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostgroupInfoList(hostgroupInfoList);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendUpdatedEvents(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr eventTablePtr = createItemTable(*cmdBuf);

	EventInfoList eventInfoList;
	HatoholDBUtils::transformEventsToHatoholFormat(
	  eventInfoList, eventTablePtr, m_ctx->serverInfo.id);
	UnifiedDataStore::getInstance()->addEventList(eventInfoList);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendArmInfo(
  const HapiCommandHeader *header)
{
	ArmInfo armInfo;
	timespec ts;
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	HapiArmInfo *body = getCommandBody<HapiArmInfo>(*cmdBuf);
	// We use running status in this side, because this is used for
	// the decision of restart of dataStore in UnifiedDataStore when
	// the Parameter of monitoring server is changed.
	armInfo.running = m_ctx->armStatus.getArmInfo().running;
	//armInfo.running = LtoN(body->running);

	armInfo.stat    = static_cast<ArmWorkingStatus>(LtoN(body->stat));

	ts.tv_sec  = LtoN(body->statUpdateTime);
	ts.tv_nsec = LtoN(body->statUpdateTimeNanosec);
	armInfo.statUpdateTime = SmartTime(ts);

	ts.tv_sec  = LtoN(body->lastSuccessTime);
	ts.tv_nsec = LtoN(body->lastSuccessTimeNanosec);
	armInfo.lastSuccessTime = SmartTime(ts);

	ts.tv_sec  = LtoN(body->lastFailureTime);
	ts.tv_nsec = LtoN(body->lastFailureTimeNanosec);
	armInfo.lastFailureTime = SmartTime(ts);

	armInfo.numUpdate  = LtoN(body->numUpdate);
	armInfo.numFailure = LtoN(body->numFailure);

	armInfo.failureComment = getString(*cmdBuf, body,
	                                   body->failureCommentOffset,
	                                   body->failureCommentLength);
	m_ctx->armStatus.setArmInfo(armInfo);

	replyOk();
}
