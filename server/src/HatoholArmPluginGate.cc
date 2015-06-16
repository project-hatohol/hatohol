/*
 * Copyright (C) 2014-2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
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
#include "ArmUtils.h"
#include "NamedPipe.h"
#include "UnifiedDataStore.h"
#include "HatoholArmPluginGate.h"
#include "ThreadLocalDBCache.h"
#include "ChildProcessManager.h"
#include "StringUtils.h"
#include "HostInfoCache.h"
#include "HatoholDBUtils.h"
#include "UnifiedDataStore.h"
#include "ConfigManager.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const int   HatoholArmPluginGate::NO_RETRY = -1;
static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

static const int PLUGIN_REPLY_TIMEOUT_FACTOR = 3;

static const size_t TIMEOUT_PLUGIN_TERM_CMD_MS     =  30 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGTERM_MS =  60 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGKILL_MS = 120 * 1000;

class ImpromptuArmBase : public ArmBase {
public:
	ImpromptuArmBase(const MonitoringServerInfo &serverInfo)
	: ArmBase("HatoholArmPluginGate", serverInfo)
	{
	}

	virtual ArmPollingResult mainThreadOneProc(void) override
	{
		// This method is never called because nobody calls start().
		// Just written to pass the build.
		return COLLECT_OK;
	}
};

struct HatoholArmPluginGate::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo serverInfo;

	ArmUtils             utils;
	ImpromptuArmBase     armBase;
	ArmPluginInfo        armPluginInfo;
	ArmStatus            armStatus;
	AtomicValue<GPid>    pid;
	SimpleSemaphore      pluginTermSem;
	HostInfoCache        hostInfoCache;
	Mutex                exitSyncLock;
	bool                 exitSyncDone;
	bool                 createdSelfTriggers;
	guint                timerTag;
	ArmUtils::ArmTrigger armTrigger[NUM_COLLECT_NG_KIND];
	NamedPipe            pipeRd, pipeWr;
	bool                 allowSetGLibMainContext;

	Impl(const MonitoringServerInfo &_serverInfo,
	               HatoholArmPluginGate *_hapg)
	: serverInfo(_serverInfo),
	  utils(serverInfo, armTrigger, NUM_COLLECT_NG_KIND),
	  armBase(_serverInfo),
	  pid(0),
	  pluginTermSem(0),
	  hostInfoCache(&_serverInfo.id),
	  exitSyncDone(false),
	  createdSelfTriggers(false),
	  pipeRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  allowSetGLibMainContext(true)
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
  m_impl(new Impl(serverInfo, this))
{
	ThreadLocalDBCache cache;
	const ServerIdType &serverId = m_impl->serverInfo.id;
	DBTablesConfig &dbConfig = cache.getConfig();
	if (!dbConfig.getArmPluginInfo(m_impl->armPluginInfo, serverId)) {
		MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
		         serverId);
		return;
	}
	if (!m_impl->armPluginInfo.brokerUrl.empty())
		setBrokerUrl(m_impl->armPluginInfo.brokerUrl);

	string address;
	if (!m_impl->armPluginInfo.staticQueueAddress.empty())
		address = m_impl->armPluginInfo.staticQueueAddress;
	else
		address = generateBrokerAddress(m_impl->serverInfo);
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
	  HAPI_CMD_GET_TIME_OF_LAST_EVENT,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetTimeOfLastEvent);

	registerCommandHandler(
	  HAPI_CMD_GET_SHOULD_LOAD_OLD_EVENT,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetShouldLoadOldEvent);

	registerCommandHandler(
	  HAPI_CMD_GET_IF_HOSTS_CHANGED,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerGetHostsChanged);

	registerCommandHandler(
	  HAPI_CMD_SEND_UPDATED_TRIGGERS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers);

	registerCommandHandler(
	  HAPI_CMD_SEND_ALL_TRIGGERS,
	  (CommandHandler)
	    &HatoholArmPluginGate::cmdHandlerSendAllTriggers);

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

	registerCommandHandler(
	  HAPI_CMD_SEND_HAP_SELF_TRIGGERS,
	  (CommandHandler)
	  &HatoholArmPluginGate::cmdHandlerSendHapSelfTriggers);
}

void HatoholArmPluginGate::start(void)
{
	m_impl->armStatus.setRunningStatus(true);

	ServerHostDef svHostDef;
	svHostDef.id       = AUTO_INCREMENT_VALUE;
	svHostDef.hostId   = AUTO_ASSIGNED_ID;
	svHostDef.serverId = m_impl->serverInfo.id;
	svHostDef.hostIdInServer = INAPPLICABLE_LOCAL_HOST_ID;
	svHostDef.name     = "N/A";
	svHostDef.status   = HOST_STAT_INAPPLICABLE;
	UnifiedDataStore::getInstance()->upsertHost(svHostDef);
	m_impl->hostInfoCache.update(svHostDef);

	HatoholArmPluginInterface::start();
}

const MonitoringServerInfo &HatoholArmPluginGate::getMonitoringServerInfo(void)
  const
{
	return m_impl->armBase.getServerInfo();
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_impl->armStatus;
}

void HatoholArmPluginGate::exitSync(void)
{

	AutoMutex autoMutex(&m_impl->exitSyncLock);
	if (m_impl->exitSyncDone)
		return;
	MLPL_INFO("HatoholArmPluginGate: [%d:%s]: requested to exit.\n",
	          m_impl->serverInfo.id, m_impl->serverInfo.hostName.c_str());
	terminatePluginSync();
	HatoholArmPluginInterface::exitSync();
	m_impl->armStatus.setRunningStatus(false);
	MLPL_INFO("  => [%d:%s]: done.\n",
	          m_impl->serverInfo.id, m_impl->serverInfo.hostName.c_str());
	m_impl->exitSyncDone = true;
}

pid_t HatoholArmPluginGate::getPid()
{
	return m_impl->pid;
}

bool HatoholArmPluginGate::isFetchItemsSupported(void)
{
	return true;
}

bool HatoholArmPluginGate::startOnDemandFetchItems(
  HostIdVector hostIds, Closure0 *closure)
{
	if (!isConnetced())
		return false;

	struct Callback : public CommandCallbacks {
		const ServerIdType  &serverId;
		const HostInfoCache &hostInfoCache;
		Signal0 itemUpdatedSignal;

		Callback(const ServerIdType &_serverId,
		         const HostInfoCache &_hostInfoCache)
		: serverId(_serverId),
		  hostInfoCache(_hostInfoCache)
		{
		}

		virtual void onGotReply(mlpl::SmartBuffer &replyBuf,
		                        const HapiCommandHeader &cmdHeader)
		                          override
		{
			// TODO: fill proper value
			MonitoringServerStatus serverStatus;
			serverStatus.serverId = serverId;

			replyBuf.setIndex(sizeof(HapiResponseHeader));
			ItemTablePtr itemTablePtr = createItemTable(replyBuf);
			ItemTablePtr appTablePtr  = createItemTable(replyBuf);
			ItemInfoList itemList;
			HatoholDBUtils::transformItemsToHatoholFormat(
			  itemList, serverStatus, itemTablePtr, appTablePtr,
			  serverId, hostInfoCache);
			UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
			dataStore->addItemList(itemList);
			dataStore->addMonitoringServerStatus(serverStatus);
			cleanup();
		}

		virtual void onError(const HapiResponseCode &code,
		                     const HapiCommandHeader &cmdHeader)
		                          override
		{
			cleanup();
		}

		void cleanup(void)
		{
			itemUpdatedSignal();
			itemUpdatedSignal.clear();
			this->unref();
		}
	};
	Callback *callback =
	  new Callback(m_impl->serverInfo.id, m_impl->hostInfoCache);
	callback->itemUpdatedSignal.connect(closure);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_REQ_FETCH_ITEMS);
	try {
		send(cmdBuf, callback);
		return true;
	} catch (const exception &e) {
		return false;
	}
}

void HatoholArmPluginGate::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	struct Callback : public CommandCallbacks {
		Signal1<HistoryInfoVect> historyUpdatedSignal;
		ServerIdType serverId;
		HistoryInfoVect historyInfoVect;
		virtual void onGotReply(mlpl::SmartBuffer &replyBuf,
					const HapiCommandHeader &cmdHeader)
					  override
		{
			replyBuf.setIndex(sizeof(HapiResponseHeader));
			ItemTablePtr itemTablePtr = createItemTable(replyBuf);
			HatoholDBUtils::transformHistoryToHatoholFormat(
			  historyInfoVect, itemTablePtr, serverId);
			// We don't store history data to Hatohol's DB.
			// Pass it directly to Hatohol client.
			cleanup();
		}

		virtual void onError(const HapiResponseCode &code,
				     const HapiCommandHeader &cmdHeader)
				       override
		{
			cleanup();
		}

		void cleanup(void)
		{
			historyUpdatedSignal(historyInfoVect);
			historyUpdatedSignal.clear();
			this->unref();
		}
	};
	Callback *callback = new Callback();
	callback->historyUpdatedSignal.connect(closure);
	callback->serverId = m_impl->serverInfo.id;

	const size_t additionalSize =
	  itemInfo.hostIdInServer.size() + 1 +
	  itemInfo.id.size()             + 1; // +1: NULL term.
	SmartBuffer cmdBuf;
	HapiParamReqFetchHistory *body =
	  setupCommandHeader<HapiParamReqFetchHistory>(
	    cmdBuf, HAPI_CMD_REQ_FETCH_HISTORY, additionalSize);
	body->valueType = NtoL(static_cast<uint16_t>(itemInfo.valueType));
	body->beginTime = NtoL(beginTime);
	body->endTime   = NtoL(endTime);

	char *buf = reinterpret_cast<char *>(body + 1);
	buf = putString(buf, body, itemInfo.hostIdInServer,
	                &body->hostIdOffset, &body->hostIdLength);
	buf = putString(buf, body, itemInfo.id,
	                &body->itemIdOffset, &body->itemIdLength);
	send(cmdBuf, callback);
}

bool HatoholArmPluginGate::startOnDemandFetchTriggers(
  HostIdVector hostIds, Closure0 *closure)
{
	if (!isConnetced())
		return false;

	struct Callback : public CommandCallbacks {
		Signal0 triggerUpdatedSignal;
		ServerIdType serverId;
		HostInfoCache *hostInfoCache;
		virtual void onGotReply(mlpl::SmartBuffer &replyBuf,
		                        const HapiCommandHeader &cmdHeader)
		                          override
		{
			replyBuf.setIndex(sizeof(HapiResponseHeader));
			ItemTablePtr tablePtr = createItemTable(replyBuf);
			TriggerInfoList trigInfoList;
			HatoholDBUtils::transformTriggersToHatoholFormat(
			  trigInfoList, tablePtr, serverId, *hostInfoCache);

			ThreadLocalDBCache cache;
			cache.getMonitoring().updateTrigger(trigInfoList, serverId);

			cleanup();
		}

		virtual void onError(const HapiResponseCode &code,
		                     const HapiCommandHeader &cmdHeader)
		                          override
		{
			cleanup();
		}

		void cleanup(void)
		{
			triggerUpdatedSignal();
			triggerUpdatedSignal.clear();
			this->unref();
		}
	};
	Callback *callback = new Callback();
	callback->triggerUpdatedSignal.connect(closure);
	callback->serverId = m_impl->serverInfo.id;
	callback->hostInfoCache = &m_impl->hostInfoCache;

	HostsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(m_impl->serverInfo.id);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect svHostDefs;
	dataStore->getServerHostDefs(svHostDefs, option);
	callback->hostInfoCache->update(svHostDefs);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_REQ_FETCH_TRIGGERS);
	try {
		send(cmdBuf, callback);
		return true;
	} catch (const exception &e) {
		return false;
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	exitSync();
}

gboolean HatoholArmPluginGate::detectedArmPluginTimeout(void *data)
{
	MLPL_ERR("Detect the timeout of connection to ArmPlugin.\n");
	HatoholArmPluginGate *obj = static_cast<HatoholArmPluginGate *>(data);
	obj->m_impl->timerTag = INVALID_EVENT_ID;
	obj->setPluginConnectStatus(COLLECT_NG_PLGIN_CONNECT_ERROR,
				    HAPERR_UNAVAILABLE_HAP);
	return G_SOURCE_REMOVE;
}

void HatoholArmPluginGate::onPriorToFetchMessage(void)
{
	const MonitoringServerInfo &svInfo = m_impl->serverInfo;
	if (svInfo.pollingIntervalSec != 0) {
		guint timeout = svInfo.pollingIntervalSec *
		                  PLUGIN_REPLY_TIMEOUT_FACTOR * 1000;
		m_impl->timerTag = Utils::setGLibTimer(
		                     timeout, detectedArmPluginTimeout, this,
		                     getGLibMainContext());
	}
}

void HatoholArmPluginGate::onSuccessFetchMessage(void)
{
	Utils::removeEventSourceIfNeeded(
	  m_impl->timerTag, SYNC, getGLibMainContext());
	m_impl->timerTag = INVALID_EVENT_ID;
}

void HatoholArmPluginGate::onFailureFetchMessage(void)
{
	setPluginConnectStatus(COLLECT_NG_AMQP_CONNECT_ERROR,
			       HAPERR_UNAVAILABLE_HAP);
	Utils::removeEventSourceIfNeeded(
	  m_impl->timerTag, SYNC, getGLibMainContext());
	m_impl->timerTag = INVALID_EVENT_ID;
}

void HatoholArmPluginGate::onFailureReceivedMessage(void)
{
	setPluginConnectStatus(COLLECT_NG_HATOHOL_INTERNAL_ERROR,
			       HAPERR_UNAVAILABLE_HAP);
	setPluginConnectStatus(COLLECT_NG_PLGIN_CONNECT_ERROR,
			       HAPERR_UNKNOWN);
	HatoholArmPluginInterface::sendInitiationPacket();
}

void HatoholArmPluginGate::onSetPluginInitialInfo(void)
{
	if (m_impl->createdSelfTriggers)
		return;

	m_impl->utils.registerSelfMonitoringHost();
	m_impl->utils.initializeArmTriggers();

	setPluginAvailabelTrigger(COLLECT_NG_AMQP_CONNECT_ERROR,
				  FAILED_CONNECT_BROKER_TRIGGER_ID,
				  HTERR_FAILED_CONNECT_BROKER);
	setPluginAvailabelTrigger(COLLECT_NG_HATOHOL_INTERNAL_ERROR,
				  FAILED_INTERNAL_ERROR_TRIGGER_ID,
				  HTERR_INTERNAL_ERROR);
	setPluginAvailabelTrigger(COLLECT_NG_PLGIN_CONNECT_ERROR,
				  FAILED_CONNECT_HAP_TRIGGER_ID,
				  HTERR_FAILED_CONNECT_HAP);
	m_impl->createdSelfTriggers = true;
}

void HatoholArmPluginGate::setPluginAvailabelTrigger(const HatoholArmPluginWatchType &type,
						     const TriggerIdType &trrigerId,
						     const HatoholError &hatoholError)
{
	TriggerInfoList triggerInfoList;
	m_impl->armTrigger[type].status = TRIGGER_STATUS_UNKNOWN;
	m_impl->armTrigger[type].triggerId = trrigerId;
	m_impl->armTrigger[type].msg = hatoholError.getMessage().c_str();

	ArmUtils::ArmTrigger &armTrigger = m_impl->armTrigger[type];
	m_impl->utils.createTrigger(armTrigger, triggerInfoList);

	ThreadLocalDBCache cache;
	cache.getMonitoring().addTriggerInfoList(triggerInfoList);
}

void HatoholArmPluginGate::setPluginConnectStatus(const HatoholArmPluginWatchType &type,
					     const HatoholArmPluginErrorCode &errorCode)
{
	TriggerStatusType status;
	if (errorCode == HAPERR_UNAVAILABLE_HAP) {
		status = TRIGGER_STATUS_PROBLEM;
	} else if (errorCode == HAPERR_OK) {
		status = TRIGGER_STATUS_OK;
	} else {
		status = TRIGGER_STATUS_UNKNOWN;
	}
	m_impl->utils.updateTriggerStatus(type, status);
}

void HatoholArmPluginGate::onConnected(qpid::messaging::Connection &conn)
{
	setPluginConnectStatus(COLLECT_NG_AMQP_CONNECT_ERROR,
			       HAPERR_OK);
	if (m_impl->pid)
		return;

	// TODO: Check the type.
	if (m_impl->armPluginInfo.path == PassivePluginQuasiPath) {
		MLPL_INFO("Started: passive plugin (%d) %s\n",
		          m_impl->armPluginInfo.type,
		          m_impl->armPluginInfo.path.c_str());
		onLaunchedProcess(true, m_impl->armPluginInfo);
		return;
	}

	// launch a plugin process
	bool succeeded = launchPluginProcess(m_impl->armPluginInfo);
	onLaunchedProcess(succeeded, m_impl->armPluginInfo);
}

void HatoholArmPluginGate::onFailureConnected(void)
{
	setPluginConnectStatus(COLLECT_NG_AMQP_CONNECT_ERROR,
			       HAPERR_UNAVAILABLE_HAP);
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
	m_impl->pid = 0;
	m_impl->pluginTermSem.post();
}

void HatoholArmPluginGate::onInitiated(void)
{
	HatoholArmPluginInterface::onInitiated();
	setPluginConnectStatus(COLLECT_NG_PLGIN_CONNECT_ERROR,
			      HAPERR_OK);
}

void HatoholArmPluginGate::terminatePluginSync(void)
{
	// If the the plugin is a passive type, it will naturally return at
	// the following condition.
	if (!m_impl->pid)
		return;

	// Send the teminate command.
	sendTerminateCommand();
	size_t timeoutMSec = TIMEOUT_PLUGIN_TERM_CMD_MS;
	if (m_impl->waitTermPlugin(timeoutMSec))
		return;
	size_t elapsedTime = timeoutMSec;

	// Send SIGTERM
	timeoutMSec = TIMEOUT_PLUGIN_TERM_SIGTERM_MS - elapsedTime;
	MLPL_INFO("Send SIGTERM to the plugin and wait for %zd sec.\n",
	          timeoutMSec/1000);
	if (kill(m_impl->pid, SIGTERM) == -1) {
		MLPL_ERR("Failed to send SIGTERM, pid: %d, errno: %d\n",
		         (int)m_impl->pid, errno);
	}
	if (m_impl->waitTermPlugin(timeoutMSec))
		return;
	elapsedTime += timeoutMSec;

	// Send SIGKILL
	timeoutMSec = TIMEOUT_PLUGIN_TERM_SIGKILL_MS - elapsedTime;
	MLPL_INFO("Send SIGKILL to the plugin and wait for %zd sec.\n",
	          timeoutMSec/1000);
	if (kill(m_impl->pid, SIGKILL) == -1) {
		MLPL_ERR("Failed to send SIGKILL, pid: %d, errno: %d\n",
		         (int)m_impl->pid, errno);
	}
	if (m_impl->waitTermPlugin(timeoutMSec))
		return;
	elapsedTime += timeoutMSec;

	MLPL_WARN(
	  "The plugin (%d) hasn't terminated within a timeout. Ignore it.\n",
	  (int)m_impl->pid);
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

	// Envrionment variables passsed to the plugin process
	const char *ldlibpath = getenv("LD_LIBRARY_PATH");
	if (ldlibpath) {
		string env = "LD_LIBRARY_PATH=";
		env += ldlibpath;
		arg.envs.push_back(env);
	}

	const char *mlplLoggerLevel = getenv(Logger::LEVEL_ENV_VAR_NAME);
	if (mlplLoggerLevel) {
		string env = StringUtils::sprintf("%s=%s",
		  Logger::LEVEL_ENV_VAR_NAME, mlplLoggerLevel);
		arg.envs.push_back(env);
	}

	// Open PIPEs
	// NOTE: Now PIPEs are used only for the detection of death of the
	// Hatohol server by (non-passive) Arm Plugins. However, we may use
	// the pipes to carry payloads instead of AMQP daemon to
	// improve performance.
	const string pipeName = StringUtils::sprintf(HAP_PIPE_NAME_FMT,
	                                             m_impl->serverInfo.id);
	if (!initPipesIfNeeded(pipeName))
		return false;
	arg.args.push_back("--" HAP_PIPE_OPT);
	arg.args.push_back(pipeName);

	arg.eventCb = eventCb;
	arg.envs.push_back(StringUtils::sprintf(
	  "%s=%s", ENV_NAME_QUEUE_ADDR,
	           generateBrokerAddress(m_impl->serverInfo).c_str()));
	ChildProcessManager::getInstance()->create(arg);
	if (!eventCb->succeededInCreation) {
		MLPL_ERR("Failed to execute: (%d) %s\n",
		         armPluginInfo.type, armPluginInfo.path.c_str());
		return false;
	}

	m_impl->pid = arg.pid;
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
	const MonitoringServerInfo &svInfo = m_impl->serverInfo;

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
	SmartTime last = uds->getTimestampOfLastTrigger(m_impl->serverInfo.id);
	const timespec &lastTimespec = last.getAsTimespec();
	body->timestamp = NtoL(lastTimespec.tv_sec);
	body->nanosec   = NtoL(lastTimespec.tv_nsec);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetLastEventId(
  const HapiCommandHeader *header)
{
	ThreadLocalDBCache cache;
	const EventIdType lastEventId =
	  cache.getMonitoring().getMaxEventId(m_impl->serverInfo.id);
	const size_t additionalSize = lastEventId.size() + 1;
	SmartBuffer resBuf;
	HapiResLastEventId *body =
	  setupResponseBuffer<HapiResLastEventId>(resBuf, additionalSize);
	char *buf = reinterpret_cast<char *>(body + 1);
	buf = putString(buf, body, lastEventId,
	                &body->lastEventIdOffset, &body->lastEventIdLength);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetTimeOfLastEvent(
  const HapiCommandHeader *header)
{
	// Parse a command paramter.
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");
	HapiParamTimeOfLastEvent *param =
	  getCommandBody<HapiParamTimeOfLastEvent>(*cmdBuf);

	const TriggerIdType triggerId =
	   getString(*cmdBuf, param,
	             param->triggerIdOffset, param->triggerIdLength);

	// Make a response buffer.
	SmartBuffer resBuf;
	HapiResTimeOfLastEvent *body =
	  setupResponseBuffer<HapiResTimeOfLastEvent>(resBuf);
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	SmartTime time = dbMonitoring.getTimeOfLastEvent(m_impl->serverInfo.id,
	                                                 triggerId);
	const timespec &ts = time.getAsTimespec();
	body->sec = ts.tv_sec;
	body->nsec = ts.tv_nsec;
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetShouldLoadOldEvent(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	HapiResShouldLoadOldEvent *body =
	  setupResponseBuffer<HapiResShouldLoadOldEvent>(resBuf);
	bool type;
	type = ConfigManager::getInstance()->getLoadOldEvents();
	body->type = NtoL(type);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerGetHostsChanged(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	HapiTriggerCollect *body =
	  setupResponseBuffer<HapiTriggerCollect>(resBuf);
	bool type;
	type = UnifiedDataStore::getInstance()->wasStoredHostsChanged();
	body->type = NtoL(type);
	reply(resBuf);
}

void HatoholArmPluginGate::cmdHandlerSendAllTriggers(
  const HapiCommandHeader *header)
{
	TriggerInfoList trigInfoList;
	parseCmdHandlerTriggerList(trigInfoList);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.updateTrigger(trigInfoList, m_impl->serverInfo.id);

	replyOk();
}
void HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers(
  const HapiCommandHeader *header)
{
	TriggerInfoList trigInfoList;
	parseCmdHandlerTriggerList(trigInfoList);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addTriggerInfoList(trigInfoList);

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHosts(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostTablePtr = createItemTable(*cmdBuf);

	ServerHostDefVect svHostDefs;
	HatoholDBUtils::transformHostsToHatoholFormat(
	  svHostDefs, hostTablePtr, m_impl->serverInfo.id);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  uds->syncHosts(svHostDefs, m_impl->serverInfo.id,
	                 m_impl->hostInfoCache));
	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHostgroupElements(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupElementTablePtr = createItemTable(*cmdBuf);

	HostgroupMemberVect hostgroupMembers;
	HatoholDBUtils::transformHostsGroupsToHatoholFormat(
	  hostgroupMembers, hostgroupElementTablePtr, m_impl->serverInfo.id,
	  m_impl->hostInfoCache);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  uds->upsertHostgroupMembers(hostgroupMembers));

	replyOk();
}

void HatoholArmPluginGate::cmdHandlerSendHostgroups(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupTablePtr = createItemTable(*cmdBuf);

	HostgroupVect hostgroups;
	HatoholDBUtils::transformGroupsToHatoholFormat(
	  hostgroups, hostgroupTablePtr, m_impl->serverInfo.id);

	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  UnifiedDataStore::getInstance()->upsertHostgroups(hostgroups));

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
	  eventInfoList, eventTablePtr, m_impl->serverInfo.id);
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
	armInfo.running = m_impl->armStatus.getArmInfo().running;
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

	HatoholArmPluginWatchType type =
		(HatoholArmPluginWatchType)LtoN(body->failureReason);
	if (armInfo.stat == ARM_WORK_STAT_OK ) {
		setPluginConnectStatus(COLLECT_OK, HAPERR_OK);
	} else {
		if (type != COLLECT_OK)
			setPluginConnectStatus(type, HAPERR_UNAVAILABLE_HAP);
	}

	m_impl->armStatus.setArmInfo(armInfo);

	replyOk();

}

void HatoholArmPluginGate::parseCmdHandlerTriggerList(TriggerInfoList &trigInfoList)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr tablePtr = createItemTable(*cmdBuf);

	HatoholDBUtils::transformTriggersToHatoholFormat(
	  trigInfoList, tablePtr, m_impl->serverInfo.id, m_impl->hostInfoCache);

	return;
}

void HatoholArmPluginGate::addInitialTrigger(HatoholArmPluginWatchType addtrigger)
{
	if (addtrigger == COLLECT_NG_DISCONNECT_ZABBIX) {
		setPluginAvailabelTrigger(COLLECT_NG_DISCONNECT_ZABBIX,
					  FAILED_CONNECT_ZABBIX_TRIGGER_ID,
					  HTERR_FAILED_CONNECT_ZABBIX);
	} else if (addtrigger == COLLECT_NG_PARSER_ERROR) {
		setPluginAvailabelTrigger(COLLECT_NG_PARSER_ERROR,
					  FAILED_PARSER_JSON_DATA_TRIGGER_ID,
					  HTERR_FAILED_TO_PARSE_JSON_DATA);
	} else if (addtrigger == COLLECT_NG_DISCONNECT_NAGIOS) {
		setPluginAvailabelTrigger(COLLECT_NG_DISCONNECT_NAGIOS,
					  FAILED_CONNECT_MYSQL_TRIGGER_ID,
					  HTERR_FAILED_CONNECT_MYSQL);
	} else if (addtrigger == COLLECT_NG_PLGIN_INTERNAL_ERROR) {
		setPluginAvailabelTrigger(COLLECT_NG_PLGIN_INTERNAL_ERROR,
					  FAILED_HAP_INTERNAL_ERROR_TRIGGER_ID,
					  HTERR_HAP_INTERNAL_ERROR);
	}
}

void HatoholArmPluginGate::cmdHandlerSendHapSelfTriggers(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	HapiHapSelfTriggers *body = getCommandBody<HapiHapSelfTriggers>(*cmdBuf);

	int numTriggerList = LtoN(body->numTriggers);
	uint64_t *buf = reinterpret_cast<uint64_t *>(body + 1);
	for (int i = 0; i < numTriggerList; i++) {
		addInitialTrigger(static_cast<HatoholArmPluginWatchType>(LtoN(buf[i])));
	}
	replyOk();
}

bool HatoholArmPluginGate::initPipesIfNeeded(const string &pipeName)
{
	GMainContext *ctx = getGLibMainContext();
	if (m_impl->pipeRd.getFd() < 0) {
		if (!m_impl->pipeRd.init(pipeName, pipeRdErrCb, this, ctx))
			return false;
	}
	if (m_impl->pipeWr.getFd() < 0) {
		if (!m_impl->pipeWr.init(pipeName, pipeWrErrCb, this, ctx))
			return false;
	}
	m_impl->allowSetGLibMainContext = false;
	return true;
}

NamedPipe &HatoholArmPluginGate::getHapPipeForRead(void)
{
	return m_impl->pipeRd;
}

NamedPipe &HatoholArmPluginGate::getHapPipeForWrite(void)
{
	return m_impl->pipeWr;
}

void HatoholArmPluginGate::setGLibMainContext(GMainContext *context)
{
	HATOHOL_ASSERT(m_impl->allowSetGLibMainContext,
	               "Calling setGLibMainContext() is too late.");
	HatoholArmPluginInterface::setGLibMainContext(context);
}

HostInfoCache &HatoholArmPluginGate::getHostInfoCache(void)
{
	return m_impl->hostInfoCache;
}

gboolean HatoholArmPluginGate::pipeRdErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	MLPL_INFO("Got callback (PIPE): %08x\n", condition);
	return G_SOURCE_REMOVE;
}

gboolean HatoholArmPluginGate::pipeWrErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	MLPL_INFO("Got callback (PIPE): %08x\n", condition);
	return G_SOURCE_REMOVE;
}
