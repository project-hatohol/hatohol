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
#include <string>
#include <cstring>
#include <AtomicValue.h>
#include <MutexLock.h>
#include <Reaper.h>
#include "UnifiedDataStore.h"
#include "HatoholArmPluginGate.h"
#include "CacheServiceDBClient.h"
#include "ChildProcessManager.h"
#include "StringUtils.h"
#include "DBClientZabbix.h" // deprecated

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const char *HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR = "HAPI_QUEUE_ADDR";
const int   HatoholArmPluginGate::NO_RETRY = -1;
static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

struct HatoholArmPluginGate::PrivateContext
{
	HatoholArmPluginGate *hapg;
	MonitoringServerInfo serverInfo;    // we have a copy.
	ArmPluginInfo        armPluginInfo;
	ArmStatus            armStatus;
	GPid                 pid;

	PrivateContext(const MonitoringServerInfo &_serverInfo,
	               HatoholArmPluginGate *_hapg)
	: hapg(_hapg),
	  serverInfo(_serverInfo),
	  pid(0)
	{
	}
};

const string HatoholArmPluginGate::PassivePluginQuasiPath = "#PASSIVE_PLUGIN#";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: HatoholArmPluginInterface("", true),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo, this);

	string address = generateBrokerAddress(m_ctx->serverInfo);
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

void HatoholArmPluginGate::exitSync(void)
{
	HatoholArmPluginInterface::exitSync();
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

void HatoholArmPluginGate::onConnected(qpid::messaging::Connection &conn)
{
	if (m_ctx->pid)
		return;

	CacheServiceDBClient cache;
	const MonitoringSystemType &type = m_ctx->serverInfo.type;
	DBClientConfig *dbConfig = cache.getConfig();
	if (!dbConfig->getArmPluginInfo(m_ctx->armPluginInfo, type)) {
		MLPL_ERR("Failed to get ArmPluginInfo: type: %d\n", type);
		return;
	}
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

void HatoholArmPluginGate::cmdHandlerGetMonitoringServerInfo(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	MonitoringServerInfo &svInfo = m_ctx->serverInfo;

	const size_t lenHostName  = svInfo.hostName.size();
	const size_t lenIpAddress = svInfo.ipAddress.size();
	const size_t lenNickname  = svInfo.nickname.size();
	// +1: Null Term.
	size_t addSize =
	  (lenHostName + 1) + (lenIpAddress + 1) + (lenNickname + 1);
	
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

void HatoholArmPluginGate::cmdHandlerSendUpdatedTriggers(
  const HapiCommandHeader *header)
{
	MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	return;

	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr tablePtr = createItemTable(*cmdBuf);

	// We don't save trigger data to DBClientZabbix. Instead
	// save the data to DBClientHatohol directly.
	// Different from ArmZabbixAPI, our new design deprecates
	// using DBClinetZabbix, because it hasn't worked usefully.

	TriggerInfoList triggerInfoList;
	// TODO: Build triggerInfoList with the host data.
	// We implemnet code to get host data first.

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addTriggerInfoList(triggerInfoList);
}

void HatoholArmPluginGate::cmdHandlerSendHosts(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostTablePtr = createItemTable(*cmdBuf);

	// We don't save host data to DBClientZabbix.
	// See also the comment in cmdHandlerSendUpdatedTriggers().
	// TODO: replace DBClientZabbix::transformHostsToHatoholFormat()
	// with a similar helper function.
	HostInfoList hostInfoList;
	DBClientZabbix::transformHostsToHatoholFormat(
	  hostInfoList, hostTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostInfoList(hostInfoList);
}

void HatoholArmPluginGate::cmdHandlerSendHostgroupElements(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupTablePtr = createItemTable(*cmdBuf);

	// We don't save host data to DBClientZabbix.
	// See also the comment in cmdHandlerSendUpdatedTriggers().
	// TODO: replace DBClientZabbix::transformHostsGroupsToHatoholFormat()
	// with a similar helper function.
	HostgroupElementList hostgroupElementList;
	HostInfoList hostInfoList;
	DBClientZabbix::transformHostsGroupsToHatoholFormat(
	  hostgroupElementList, hostgroupTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostgroupElementList(hostgroupElementList);
}
