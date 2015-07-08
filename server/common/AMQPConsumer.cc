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
#include <SimpleSemaphore.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

const vector<size_t> retryInterval = { 1, 2, 5, 10, 30 };

struct AMQPConsumer::Impl {
	Impl()
	: m_connection(NULL),
	  m_handler(NULL),
	  m_waitSem(0)
	{
	}

	~Impl()
	{
	}

	AMQPConnectionPtr m_connection;
	AMQPMessageHandler *m_handler;
	SimpleSemaphore m_waitSem;
};

AMQPConsumer::AMQPConsumer(const AMQPConnectionInfo &connectionInfo,
			   AMQPMessageHandler *handler)
: m_impl(new Impl())
{
	m_impl->m_connection = AMQPConnection::create(connectionInfo);
	m_impl->m_handler = handler;
}

AMQPConsumer::AMQPConsumer(AMQPConnectionPtr &connection,
			   AMQPMessageHandler *handler)
: m_impl(new Impl())
{
	m_impl->m_connection = connection;
	m_impl->m_handler = handler;
}

AMQPConsumer::~AMQPConsumer()
{
	m_impl->m_waitSem.post();
}

AMQPConnectionPtr AMQPConsumer::getConnection(void)
{
	return m_impl->m_connection;
}

gpointer AMQPConsumer::mainThread(HatoholThreadArg *arg)
{
	bool started = false;
	size_t i = 0, numRetry = retryInterval.size();
	while (!isExitRequested()) {
		if (!m_impl->m_connection->isConnected()) {
			started = false;
			m_impl->m_connection->connect();
		}

		if (!started && m_impl->m_connection->isConnected())
			started = m_impl->m_connection->startConsuming();

		if (!started) {
			if (i >= numRetry) {
				MLPL_ERR("Try to connect after every 30 sec.\n");
				m_impl->m_waitSem.timedWait(30 * 1000);
				continue;
			}
			size_t sleepTimeSec = retryInterval.at(i);
			m_impl->m_waitSem.timedWait(sleepTimeSec * 1000);
			MLPL_INFO("Try to connect after %zd sec. (%zd/%zd)\n",
			          sleepTimeSec, i+1, numRetry);
			i++;
			continue;
		}

		i = 0; // reset wait counter
		AMQPMessage message;
		const bool consumed = m_impl->m_connection->consume(message);
		if (!consumed)
			continue;

		m_impl->m_handler->handle(*m_impl->m_connection, message);
	}
	return NULL;
}
