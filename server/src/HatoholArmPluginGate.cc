/*
 * Copyright (C) 2014 Project Hatohol
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
#include "UnifiedDataStore.h"
#include "HatoholArmPluginGate.h"
#include "ThreadLocalDBCache.h"
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

static const int PLUGIN_REPLY_TIMEOUT_FACTOR = 3;

static const size_t TIMEOUT_PLUGIN_TERM_CMD_MS     =  30 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGTERM_MS =  60 * 1000;
static const size_t TIMEOUT_PLUGIN_TERM_SIGKILL_MS = 120 * 1000;

static const char *HAP_SELF_MONITORING_SUFFIX = "_HAP";

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

	virtual bool isFetchItemsSupported(void) const override
	{
		return false;
	}
};

struct HatoholArmPluginGate::Impl
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
	bool                 createdSelfTriggers;
	guint                timerTag;
	HAPIWtchPointInfo    hapiWtchPointInfo[NUM_COLLECT_NG_KIND];

	Impl(const MonitoringServerInfo &_serverInfo,
	               HatoholArmPluginGate *_hapg)
	: serverInfo(_serverInfo),
	  armBase(_serverInfo),
	  pid(0),
	  pluginTermSem(0),
	  exitSyncDone(false),
	  createdSelfTriggers(false)
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

	void setInitialTriggerTable(void)
	{
		for (int i = 0; i < NUM_COLLECT_NG_KIND; i++)
			hapiWtchPointInfo[i].statusType = TRIGGER_STATUS_ALL;
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

	registerCommandHandler(
	  HAPI_CMD_SEND_HAP_SELF_TRIGGERS,
	  (CommandHandler)
	  &HatoholArmPluginGate::cmdHandlerAvailableTrigger);
}

void HatoholArmPluginGate::start(void)
{
	m_impl->armStatus.setRunningStatus(true);

	HostInfo hostInfo;
	hostInfo.serverId = m_impl->serverInfo.id;
	hostInfo.id       = INAPPLICABLE_HOST_ID;
	hostInfo.hostName = "N/A";
	ThreadLocalDBCache cache;
	cache.getMonitoring().addHostInfo(&hostInfo);

	HatoholArmPluginInterface::start();
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_impl->armStatus;
}

// TODO: remove this method
ArmBase &HatoholArmPluginGate::getArmBase(void)
{
	return m_impl->armBase;
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

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	exitSync();
}

gboolean HatoholArmPluginGate::detectedArmPluginTimeout(void *data)
{
	MLPL_ERR("Detect the timeout of connection to ArmPlugin.");
	HatoholArmPluginGate *obj = static_cast<HatoholArmPluginGate *>(data);
	obj->m_impl->timerTag = INVALID_EVENT_ID;
	obj->setPluginConnectStatus(COLLECT_NG_PLGIN_CONNECT_ERROR,
				    HAPERR_UNAVAILABLE_HAP);
	return G_SOURCE_REMOVE;
}

void HatoholArmPluginGate::removeArmPluginTimeout(gpointer data)
{
	HatoholArmPluginGate *obj = static_cast<HatoholArmPluginGate *>(data);
	if (obj->m_impl->timerTag != INVALID_EVENT_ID) {
		g_source_remove(obj->m_impl->timerTag);
		obj->m_impl->timerTag = INVALID_EVENT_ID;
	}
}

void HatoholArmPluginGate::onPriorToFetchMessage(void)
{
	const MonitoringServerInfo &svInfo = m_impl->serverInfo;
	if (svInfo.pollingIntervalSec != 0) {
		m_impl->timerTag = g_timeout_add(
		  svInfo.pollingIntervalSec * PLUGIN_REPLY_TIMEOUT_FACTOR * 1000,
		  detectedArmPluginTimeout,
		  this);
	}
}

void HatoholArmPluginGate::onSuccessFetchMessage(void)
{
	Utils::executeOnGLibEventLoop(removeArmPluginTimeout, this);
}

void HatoholArmPluginGate::onFailureFetchMessage(void)
{
	setPluginConnectStatus(COLLECT_NG_AMQP_CONNECT_ERROR,
			       HAPERR_UNAVAILABLE_HAP);
	Utils::executeOnGLibEventLoop(removeArmPluginTimeout, this);
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

	const MonitoringServerInfo &svInfo = m_impl->serverInfo;
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();

	HostInfo hostInfo;
	hostInfo.serverId = svInfo.id;
	hostInfo.id = MONITORING_SERVER_SELF_ID;
	hostInfo.hostName = 
		StringUtils::sprintf("%s%s", svInfo.hostName.c_str(),
				     HAP_SELF_MONITORING_SUFFIX);
	dbMonitoring.addHostInfo(&hostInfo);

	m_impl->setInitialTriggerTable();

	setPluginAvailabelTrigger(COLLECT_NG_AMQP_CONNECT_ERROR,
				  FAILED_CONNECT_BROKER_TRIGGERID,
				  HTERR_FAILED_CONNECT_BROKER);
	setPluginAvailabelTrigger(COLLECT_NG_HATOHOL_INTERNAL_ERROR,
				  FAILED_INTERNAL_ERROR_TRIGGERID,
				  HTERR_INTERNAL_ERROR);
	setPluginAvailabelTrigger(COLLECT_NG_PLGIN_CONNECT_ERROR,
				  FAILED_CONNECT_HAP_TRIGGERID,
				  HTERR_FAILED_CONNECT_HAP);
	m_impl->createdSelfTriggers = true;
}

void HatoholArmPluginGate::createPluginTriggerInfo(const HAPIWtchPointInfo &resTrigger,
						   TriggerInfoList &triggerInfoList)
{
	const MonitoringServerInfo &svInfo = m_impl->serverInfo;
	TriggerInfo triggerInfo;

	triggerInfo.serverId = svInfo.id;
	triggerInfo.lastChangeTime = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	triggerInfo.hostId = MONITORING_SERVER_SELF_ID;
	triggerInfo.hostName = 
		StringUtils::sprintf("%s%s", svInfo.hostName.c_str(),
				     HAP_SELF_MONITORING_SUFFIX);
	triggerInfo.id = resTrigger.triggerId;
	triggerInfo.brief = resTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status = resTrigger.statusType;

	triggerInfoList.push_back(triggerInfo);
}

void HatoholArmPluginGate::createPluginEventInfo(const HAPIWtchPointInfo &resTrigger,
						 EventInfoList &eventInfoList)
{
	const MonitoringServerInfo &svInfo = m_impl->serverInfo;
	EventInfo eventInfo;

	eventInfo.serverId = svInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	eventInfo.hostId = MONITORING_SERVER_SELF_ID;
	eventInfo.triggerId = resTrigger.triggerId;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.status = resTrigger.statusType;
	if (resTrigger.statusType == TRIGGER_STATUS_OK) {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else {
		eventInfo.type = EVENT_TYPE_BAD;
	}
	eventInfoList.push_back(eventInfo);
}

void HatoholArmPluginGate::setPluginAvailabelTrigger(const HatoholArmPluginWatchType &type,
						     const TriggerIdType &trrigerId,
						     const HatoholError &hatoholError)
{
	TriggerInfoList triggerInfoList;
	m_impl->hapiWtchPointInfo[type].statusType = TRIGGER_STATUS_UNKNOWN;
	m_impl->hapiWtchPointInfo[type].triggerId = trrigerId;
	m_impl->hapiWtchPointInfo[type].msg = hatoholError.getMessage().c_str();

	HAPIWtchPointInfo &resTrigger = m_impl->hapiWtchPointInfo[type];
	createPluginTriggerInfo(resTrigger, triggerInfoList);

	ThreadLocalDBCache cache;
	cache.getMonitoring().addTriggerInfoList(triggerInfoList);
}

void HatoholArmPluginGate::setPluginConnectStatus(const HatoholArmPluginWatchType &type,
					     const HatoholArmPluginErrorCode &errorCode)
{
	TriggerInfoList triggerInfoList;
	EventInfoList eventInfoList;
	TriggerStatusType istatusType;

	if (errorCode == HAPERR_UNAVAILABLE_HAP) {
		istatusType = TRIGGER_STATUS_PROBLEM;
	} else if (errorCode == HAPERR_OK) {
		istatusType = TRIGGER_STATUS_OK;
	} else {
		istatusType = TRIGGER_STATUS_UNKNOWN;
	}

	if (type == COLLECT_OK) {
		for (int i = 0; i < NUM_COLLECT_NG_KIND; i++) {
			HAPIWtchPointInfo &trgInfo = m_impl->hapiWtchPointInfo[i];
			if (trgInfo.statusType == TRIGGER_STATUS_PROBLEM) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createPluginTriggerInfo(trgInfo, triggerInfoList);
				createPluginEventInfo(trgInfo, eventInfoList);
			} else if (trgInfo.statusType == TRIGGER_STATUS_UNKNOWN) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createPluginTriggerInfo(trgInfo, triggerInfoList);
			}				
		}
	}
	else {
		if (m_impl->hapiWtchPointInfo[type].statusType == istatusType)
			return;
		if (m_impl->hapiWtchPointInfo[type].statusType != TRIGGER_STATUS_UNKNOWN) {
			m_impl->hapiWtchPointInfo[type].statusType = istatusType;
			createPluginTriggerInfo(m_impl->hapiWtchPointInfo[type], triggerInfoList);
			createPluginEventInfo(m_impl->hapiWtchPointInfo[type], eventInfoList);
		} else {
			m_impl->hapiWtchPointInfo[type].statusType = istatusType;
			createPluginTriggerInfo(m_impl->hapiWtchPointInfo[type], triggerInfoList);
		}
		for (int i = static_cast<int>(type) + 1; i < NUM_COLLECT_NG_KIND; i++) {
			HAPIWtchPointInfo &trgInfo = m_impl->hapiWtchPointInfo[i];
			if (trgInfo.statusType == TRIGGER_STATUS_PROBLEM) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createPluginTriggerInfo(trgInfo, triggerInfoList);
				createPluginEventInfo(trgInfo, eventInfoList);
			} else if (trgInfo.statusType == TRIGGER_STATUS_UNKNOWN) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createPluginTriggerInfo(trgInfo, triggerInfoList);
			}
		}
		for (int i = 0; i < type; i++) {
			HAPIWtchPointInfo &trgInfo = m_impl->hapiWtchPointInfo[i];
			if (trgInfo.statusType == TRIGGER_STATUS_OK) {
				trgInfo.statusType = TRIGGER_STATUS_UNKNOWN;
				createPluginTriggerInfo(trgInfo, triggerInfoList);
			}
		}
	}

	if (!triggerInfoList.empty()) {
		ThreadLocalDBCache cache;
		cache.getMonitoring().addTriggerInfoList(triggerInfoList);
	}
	if (!eventInfoList.empty()) {
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		dataStore->addEventList(eventInfoList);
	}
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
	timeoutMSec = TIMEOUT_PLUGIN_TERM_SIGKILL_MS - elapsedTime;
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
	SmartBuffer resBuf;
	HapiResLastEventId *body =
	  setupResponseBuffer<HapiResLastEventId>(resBuf);
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	body->lastEventId = dbMonitoring.getLastEventId(m_impl->serverInfo.id);
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
	const TriggerIdType triggerId = LtoN(param->triggerId);

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

void HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr tablePtr = createItemTable(*cmdBuf);

	TriggerInfoList trigInfoList;
	HatoholDBUtils::transformTriggersToHatoholFormat(
	  trigInfoList, tablePtr, m_impl->serverInfo.id, m_impl->hostInfoCache);

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

	HostInfoList hostInfoList;
	HatoholDBUtils::transformHostsToHatoholFormat(
	  hostInfoList, hostTablePtr, m_impl->serverInfo.id);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addHostInfoList(hostInfoList);

	// TODO: consider if DBClientHatohol should have the cache
	HostInfoListConstIterator hostInfoItr = hostInfoList.begin();
	for (; hostInfoItr != hostInfoList.end(); ++hostInfoItr)
		m_impl->hostInfoCache.update(*hostInfoItr);

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
	  hostgroupElementList, hostgroupElementTablePtr, m_impl->serverInfo.id);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addHostgroupElementList(hostgroupElementList);

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
	  hostgroupInfoList, hostgroupTablePtr, m_impl->serverInfo.id);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);

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

void HatoholArmPluginGate::addInitialTrigger(HatoholArmPluginWatchType addtrigger)
{
	if (addtrigger == COLLECT_NG_DISCONNECT_ZABBIX) {
		setPluginAvailabelTrigger(COLLECT_NG_DISCONNECT_ZABBIX,
					  FAILED_CONNECT_ZABBIX_TRIGGERID,
					  HTERR_FAILED_CONNECT_ZABBIX);
	} else if (addtrigger == COLLECT_NG_PARSER_ERROR) {
		setPluginAvailabelTrigger(COLLECT_NG_PARSER_ERROR,
					  FAILED_PARSER_JSON_DATA_TRIGGERID,
					  HTERR_FAILED_TO_PARSE_JSON_DATA);
	} else if (addtrigger == COLLECT_NG_DISCONNECT_NAGIOS) {
		setPluginAvailabelTrigger(COLLECT_NG_DISCONNECT_NAGIOS,
					  FAILED_CONNECT_MYSQL_TRIGGERID,
					  HTERR_FAILED_CONNECT_MYSQL);
	} else if (addtrigger == COLLECT_NG_PLGIN_INTERNAL_ERROR) {
		setPluginAvailabelTrigger(COLLECT_NG_PLGIN_INTERNAL_ERROR,
					  FAILED_HAP_INTERNAL_ERROR_TRIGGERID,
					  HTERR_HAP_INTERNAL_ERROR);
	}
}

void HatoholArmPluginGate::cmdHandlerAvailableTrigger(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	HapiAvailableTrigger *body = getCommandBody<HapiAvailableTrigger>(*cmdBuf);

	int numTriggerList = LtoN(body->numTriggers);
	uint64_t *buf = reinterpret_cast<uint64_t *>(body + 1);
	for (int i = 0; i < numTriggerList; i++) {
		addInitialTrigger(static_cast<HatoholArmPluginWatchType>(LtoN(buf[i])));
	}
	replyOk();
}
