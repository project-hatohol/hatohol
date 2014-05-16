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
#include "HatoholArmPluginInterface.h"
#include "HatoholArmPluginGate.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

struct HatoholArmPluginInterface::PrivateContext {
	string queueAddr;
	Connection connection;
	Session    session;
	Sender     sender;
	Receiver   receiver;

	PrivateContext(const string &_queueAddr)
	: queueAddr(_queueAddr)
	{
	}

	virtual ~PrivateContext()
	{
		//session.sync();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginInterface::HatoholArmPluginInterface(const string &queueAddr)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(queueAddr);
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

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HatoholArmPluginInterface::mainThread(HatoholThreadArg *arg)
{
	setupConnection();
	while (!isExitRequested()) {
		Message message;
		m_ctx->receiver.fetch(message);
		if (isExitRequested()) 
			break;
		SmartBuffer sbuf;
		load(sbuf, message);
		onReceived(sbuf);
	};
	m_ctx->connection.close();
	return NULL;
}

void HatoholArmPluginInterface::onConnected(void)
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

void HatoholArmPluginInterface::setupConnection(void)
{
	const string brokerUrl = HatoholArmPluginGate::DEFAULT_BROKER_URL;
	const string connectionOptions;
	m_ctx->connection = Connection(brokerUrl, connectionOptions);
	m_ctx->connection.open();
	m_ctx->session = m_ctx->connection.createSession();
	m_ctx->sender = m_ctx->session.createSender(m_ctx->queueAddr);
	m_ctx->receiver = m_ctx->session.createReceiver(m_ctx->queueAddr);
	onConnected();
}

void HatoholArmPluginInterface::load(SmartBuffer &sbuf, const Message &message)
{
	sbuf.add(message.getContentPtr(), message.getContentSize());
}
