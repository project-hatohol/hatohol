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

#include <MutexLock.h>
#include <Reaper.h>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include "HatoholArmPluginInterface.h"
#include "HatoholArmPluginGate.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

struct HatoholArmPluginInterface::PrivateContext {
	HatoholArmPluginInterface *hapi;
	string queueAddr;
	Connection connection;
	Session    session;
	Sender     sender;
	Receiver   receiver;

	PrivateContext(HatoholArmPluginInterface *_hapi,
	               const string &_queueAddr)
	: hapi(_hapi),
	  queueAddr(_queueAddr),
	  connected(false)
	{
	}

	virtual ~PrivateContext()
	{
		disconnect();
	}

	void connect(void)
	{
		const string brokerUrl =
		   HatoholArmPluginGate::DEFAULT_BROKER_URL;
		const string connectionOptions;
		connectionLock.lock();
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		connection = Connection(brokerUrl, connectionOptions);
		connection.open();
		session = connection.createSession();
		sender = session.createSender(queueAddr);
		receiver = session.createReceiver(queueAddr);
		connected = true;
		hapi->onConnected(connection);
		hapi->onSessionChanged(&session);
	}

	void disconnect(void)
	{
		connectionLock.lock();
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		if (!connected)
			return;
		try {
			session.sync();
			session.close();
			connection.close();
		} catch (...) {
		}
		connected = false;
		hapi->onSessionChanged(NULL);
	}

	void acknowledge(void)
	{
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		connectionLock.lock();
		if (!connected)
			return;
		session.acknowledge();
	}

private:
	bool       connected;
	MutexLock  connectionLock;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginInterface::HatoholArmPluginInterface(const string &queueAddr)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(this, queueAddr);
}

HatoholArmPluginInterface::~HatoholArmPluginInterface()
{
	exitSync();
	if (m_ctx)
		delete m_ctx;
}

void HatoholArmPluginInterface::setQueueAddress(const string &queueAddr)
{
	m_ctx->queueAddr = queueAddr;
}

void HatoholArmPluginInterface::send(const string &message)
{
	Message request;
	request.setContent(message);
	m_ctx->sender.send(request);
}

void HatoholArmPluginInterface::exitSync(void)
{
	requestExit();
	m_ctx->disconnect();
	HatoholThreadBase::exitSync();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HatoholArmPluginInterface::mainThread(HatoholThreadArg *arg)
{
	m_ctx->connect();
	while (!isExitRequested()) {
		Message message;
		m_ctx->receiver.fetch(message);
		if (isExitRequested()) 
			break;
		SmartBuffer sbuf;
		load(sbuf, message);
		sbuf.resetIndex();
		onReceived(sbuf);
		m_ctx->acknowledge();
	};
	m_ctx->disconnect();
	return NULL;
}

void HatoholArmPluginInterface::onConnected(Connection &conn)
{
}

void HatoholArmPluginInterface::onSessionChanged(Session *session)
{
}

void HatoholArmPluginInterface::onReceived(mlpl::SmartBuffer &smbuf)
{
}

void HatoholArmPluginInterface::onGotError(
  const HatoholArmPluginError &hapError)
{
	THROW_HATOHOL_EXCEPTION("Got error: %d\n", hapError.code);
}

void HatoholArmPluginInterface::load(SmartBuffer &sbuf, const Message &message)
{
	sbuf.addEx(message.getContentPtr(), message.getContentSize());
}
