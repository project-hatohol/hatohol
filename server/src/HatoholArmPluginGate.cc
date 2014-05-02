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
#include <AtomicValue.h>
#include <MutexLock.h>
#include <Reaper.h>
#include "HatoholArmPluginGate.h"
#include "CacheServiceDBClient.h"
#include "ChildProcessManager.h"
#include "StringUtils.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const char *HatoholArmPluginGate::DEFAULT_BROKER_URL = "localhost:5672";
const char *HatoholArmPluginGate::ENV_NAME_QUEUE_ADDR = "HAPI_QUEUE_ADDR";
const int   HatoholArmPluginGate::NO_RETRY = -1;
static const int DEFAULT_RETRY_INTERVAL = 10 * 1000; // ms

class Locker {
public:
	Locker(MutexLock &lock, const bool &lockNow = true)
	: m_lock(lock),
	  m_inUse(lockNow)
	{
		if (lockNow)
			m_lock.lock();
	}

	virtual ~Locker()
	{
		if (m_inUse)
			m_lock.unlock();
	}

	void lock(void)
	{
		m_lock.lock();
		m_inUse = true;
	}

	void unlock(void)
	{
		m_lock.unlock();
		m_inUse = false;
	}

private:
	MutexLock &m_lock;
	bool m_inUse;

};

struct HatoholArmPluginGate::PrivateContext
{
	MonitoringServerInfo serverInfo;    // we have a copy.
	ArmPluginInfo        armPluginInfo;
	ArmStatus            armStatus;
	GPid                 pid;
	AtomicValue<bool>    exitRequest;
	Session             *sessionPtr;
	MutexLock            sessionLock;
	MutexLock            retrySleeper;

	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo),
	  pid(0),
	  exitRequest(false),
	  sessionPtr(NULL)
	{
		retrySleeper.lock();
	}

	void sessionPtrToNull(void)
	{
		sessionLock.lock();
		sessionPtr = NULL;
		sessionLock.unlock();
	}
};

const string HatoholArmPluginGate::PassivePluginQuasiPath = "#PASSIVE_PLUGIN#";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

bool HatoholArmPluginGate::start(const MonitoringSystemType &type)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	if (!dbConfig->getArmPluginInfo(m_ctx->armPluginInfo, type)) {
		MLPL_ERR("Failed to get ArmPluginInfo: type: %d\n", type);
		return false;
	}
	if (m_ctx->armPluginInfo.path == PassivePluginQuasiPath) {
		MLPL_INFO("Started: passive plugin (%d) %s\n",
		          m_ctx->armPluginInfo.type,
		          m_ctx-> armPluginInfo.path.c_str());
	} else {
		// launch a plugin process
		if (!launchPluginProcess(m_ctx->armPluginInfo))
			return false;
	}

	// start a thread
	m_ctx->armStatus.setRunningStatus(true);
	HatoholThreadBase::start();
	return true;
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

gpointer HatoholArmPluginGate::mainThread(HatoholThreadArg *arg)
{
begin:
	try {
		string brokerUrl;
		if (m_ctx->armPluginInfo.brokerUrl.empty())
			brokerUrl = DEFAULT_BROKER_URL;
		else
			brokerUrl = m_ctx->armPluginInfo.brokerUrl;
		string address = generateBrokerAddress(m_ctx->serverInfo);
		address += "; {create: always}";
		string connectionOptions;

		Locker sessionLocker(m_ctx->sessionLock);
		if (m_ctx->exitRequest) 
			return NULL;
		Connection connection(brokerUrl, connectionOptions);
		connection.open();

		Session session = connection.createSession();
		m_ctx->sessionPtr = &session;
		Receiver receiver = session.createReceiver(address);
		sessionLocker.unlock();

		while (true) {
			Message message;
			receiver.fetch(message);
			if (m_ctx->exitRequest)
				break;
			onReceived(message);

			sessionLocker.lock();
			if (m_ctx->exitRequest)
				break;
			session.acknowledge();
			sessionLocker.unlock();
		}

		m_ctx->sessionPtrToNull();
		connection.close();
	} catch (const exception &e) {
		m_ctx->sessionPtrToNull();
		int sleepTime = onCaughtException(e);
		if (sleepTime >= 0)
			m_ctx->retrySleeper.timedlock(sleepTime);
		if (m_ctx->exitRequest || sleepTime < 0)
			return NULL;
		goto begin;
	}

	return NULL;
}

void HatoholArmPluginGate::waitExit(void)
{
	m_ctx->retrySleeper.unlock();
	m_ctx->sessionLock.lock();
	// Closing the receiver doesn't unblock fetch() due to a QPid's bug:
	// http://qpid.2158936.n2.nabble.com/Forcibly-close-a-receiver-td7581255.html
	// So we close the session here.
	// Note: The bug was fixed at Qpid 0.26.
	m_ctx->exitRequest = true;
	if (m_ctx->sessionPtr)
		m_ctx->sessionPtr->close();
	m_ctx->sessionLock.unlock();

	HatoholThreadBase::waitExit();
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

void HatoholArmPluginGate::onReceived(Message &message)
{
}

void HatoholArmPluginGate::onTerminated(const siginfo_t *siginfo)
{
}

int HatoholArmPluginGate::onCaughtException(const exception &e)
{
	MLPL_INFO("Caught an exception: %s. Retry afeter %d ms.\n",
	          e.what(), DEFAULT_RETRY_INTERVAL);
	return DEFAULT_RETRY_INTERVAL;
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
		                        GError *gerror) // override
		{
			succeededInCreation = succeeded;
		}

		virtual void onCollected(const siginfo_t *siginfo) // override
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

	MLPL_INFO("Started: plugin (%d) %s\n",
	          armPluginInfo.type, armPluginInfo.path.c_str());
	return true;
}

string HatoholArmPluginGate::generateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return StringUtils::sprintf("hatohol-arm-plugin.%"FMT_SERVER_ID,
	                            serverInfo.id);
}
