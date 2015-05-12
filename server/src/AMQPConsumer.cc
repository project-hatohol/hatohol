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

#include "AMQPConsumer.h"
#include "AMQPConnection.h"
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include <unistd.h>
#include <Logger.h>
#include <Reaper.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

AMQPConsumer::AMQPConsumer(const AMQPConnectionPtr &connection,
			   AMQPMessageHandler *handler)
: m_connection(connection),
  m_handler(handler)
{
}

AMQPConsumer::~AMQPConsumer()
{
}

gpointer AMQPConsumer::mainThread(HatoholThreadArg *arg)
{
	while (!isExitRequested()) {
		if (!m_connection->isConnected()) {
			m_connection->connect();
		}

		if (!m_connection->isConnected()) {
			sleep(1); // TODO: Make retry interval customizable
			continue;
		}

		if (!m_connection->startConsuming()) {
			sleep(1); // TODO: Same with above
			continue;
		}

		amqp_envelope_t envelope;
		Reaper<amqp_envelope_t> envelopeReaper(&envelope,
						       amqp_destroy_envelope);
		const bool consumed = m_connection->consume(envelope);
		if (!consumed)
			continue;

		m_handler->handle(&envelope);
	}
	return NULL;
}
