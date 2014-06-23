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
#include "HostInfoCache.h"
#include "DBClientZabbix.h" // deprecated
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const int   HatoholArmPluginGate::NO_RETRY = -1;
static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

struct HatoholArmPluginGate::PrivateContext
{
	HatoholArmPluginGate *hapg;
	MonitoringServerInfo serverInfo;    // we have a copy.
	ArmPluginInfo        armPluginInfo;
	ArmStatus            armStatus;
	GPid                 pid;
	HostInfoCache        hostInfoCache;

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
	class Impromptu : public ArmBase {
	public:
		Impromptu(const MonitoringServerInfo &serverInfo)
		: ArmBase("HatoholArmPluginGate", serverInfo)
		{
		}

		virtual bool mainThreadOneProc(void) override
		{
			return true;
		}
	};
	static Impromptu armBase(m_ctx->serverInfo);
	return armBase;
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

	// TODO: check in the constructor.
	CacheServiceDBClient cache;
	const ServerIdType &serverId = m_ctx->serverInfo.id;
	DBClientConfig *dbConfig = cache.getConfig();
	if (!dbConfig->getArmPluginInfo(m_ctx->armPluginInfo, serverId)) {
		MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
		         serverId);
		return;
	}
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

void HatoholArmPluginGate::cmdHandlerGetMonitoringServerInfo(
  const HapiCommandHeader *header)
{
	SmartBuffer resBuf;
	MonitoringServerInfo &svInfo = m_ctx->serverInfo;

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

	// We don't save trigger data to DBClientZabbix. Instead
	// save the data to DBClientHatohol directly.
	// Different from ArmZabbixAPI, our new design deprecates
	// using DBClinetZabbix, because it hasn't worked usefully.

	TriggerInfoList trigInfoList;
	const ItemGroupList &trigGrpList = tablePtr->getItemGroupList();
	ItemGroupListConstIterator trigGrpItr = trigGrpList.begin();
	for (; trigGrpItr != trigGrpList.end(); ++trigGrpItr) {
		ItemGroupStream trigGroupStream(*trigGrpItr);
		TriggerInfo trigInfo;

		trigInfo.serverId = m_ctx->serverInfo.id;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_TRIGGERID);
		trigGroupStream >> trigInfo.id;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_VALUE);
		trigGroupStream >> trigInfo.status;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_PRIORITY);
		trigGroupStream >> trigInfo.severity;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_LASTCHANGE);
		trigGroupStream >> trigInfo.lastChangeTime.tv_sec;
		trigInfo.lastChangeTime.tv_nsec = 0;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_DESCRIPTION);
		trigGroupStream >> trigInfo.brief;

		trigGroupStream.seek(ITEM_ID_ZBX_TRIGGERS_HOSTID);
		trigGroupStream >> trigInfo.hostId;

		if (!m_ctx->hostInfoCache.getName(trigInfo.id,
		                                  trigInfo.hostName)) {
			MLPL_WARN(
			  "Ignored a trigger whose host name was not found: "
			  "server: %" FMT_SERVER_ID ", host: %" FMT_HOST_ID
			  "\n",
			  m_ctx->serverInfo.id, trigInfo.id);
			continue;
		}

		trigInfoList.push_back(trigInfo);
	}

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addTriggerInfoList(trigInfoList);
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

	// TODO: consider if DBClientHatohol should have the cache
	HostInfoListConstIterator hostInfoItr = hostInfoList.begin();
	for (; hostInfoItr != hostInfoList.end(); ++hostInfoItr)
		m_ctx->hostInfoCache.update(*hostInfoItr);
}

void HatoholArmPluginGate::cmdHandlerSendHostgroupElements(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupElementTablePtr = createItemTable(*cmdBuf);

	// We don't save host data to DBClientZabbix.
	// See also the comment in cmdHandlerSendUpdatedTriggers().
	// TODO: replace DBClientZabbix::transformHostsGroupsToHatoholFormat()
	// with a similar helper function.
	HostgroupElementList hostgroupElementList;
	HostInfoList hostInfoList;
	DBClientZabbix::transformHostsGroupsToHatoholFormat(
	  hostgroupElementList, hostgroupElementTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostgroupElementList(hostgroupElementList);
}

void HatoholArmPluginGate::cmdHandlerSendHostgroups(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr hostgroupTablePtr = createItemTable(*cmdBuf);

	// We don't save host data to DBClientZabbix.
	// See also the comment in cmdHandlerSendUpdatedTriggers().
	// TODO: replace DBClientZabbix::transformGroupsToHatoholFormat()
	// with a similar helper function.
	HostgroupInfoList hostgroupInfoList;
	DBClientZabbix::transformGroupsToHatoholFormat(
	  hostgroupInfoList, hostgroupTablePtr, m_ctx->serverInfo.id);

	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	dbHatohol->addHostgroupInfoList(hostgroupInfoList);
}

void HatoholArmPluginGate::cmdHandlerSendUpdatedEvents(
  const HapiCommandHeader *header)
{
	SmartBuffer *cmdBuf = getCurrBuffer();
	HATOHOL_ASSERT(cmdBuf, "Current buffer: NULL");

	cmdBuf->setIndex(sizeof(HapiCommandHeader));
	ItemTablePtr eventTablePtr = createItemTable(*cmdBuf);

	// We don't save host data to DBClientZabbix.
	// See also the comment in cmdHandlerSendUpdatedTriggers().
	// TODO: replace DBClientZabbix::transformEventsToHatoholFormat()
	// with a similar helper function.
	EventInfoList eventInfoList;
	DBClientZabbix::transformEventsToHatoholFormat(
	  eventInfoList, eventTablePtr, m_ctx->serverInfo.id);
	UnifiedDataStore::getInstance()->addEventList(eventInfoList);
}
