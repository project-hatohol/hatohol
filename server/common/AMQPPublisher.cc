/*
 * Copyright (C) 2015 Project Hatohol
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

#include "AMQPPublisher.h"
#include "AMQPConnection.h"
#include "AMQPMessageHandler.h"

using namespace std;

struct AMQPPublisher::Impl {
	Impl()
	: m_connection(NULL)
	{
	}

	~Impl()
	{
	}

	shared_ptr<AMQPConnection> m_connection;
	AMQPMessage m_message;
};

AMQPPublisher::AMQPPublisher(const AMQPConnectionInfo &connectionInfo)
: m_impl(new Impl())
{
	m_impl->m_connection = AMQPConnection::create(connectionInfo);
}

AMQPPublisher::AMQPPublisher(shared_ptr<AMQPConnection> connection)
: m_impl(new Impl())
{
	m_impl->m_connection = connection;
}

AMQPPublisher::~AMQPPublisher()
{
}

shared_ptr<AMQPConnection> AMQPPublisher::getConnection(void)
{
	return m_impl->m_connection;
}

void AMQPPublisher::setMessage(const AMQPMessage &message)
{
	m_impl->m_message = message;
}

void AMQPPublisher::clear(void)
{
	AMQPMessage message;
	m_impl->m_message = message;
}

bool AMQPPublisher::publish(void)
{
	if (!m_impl->m_connection->isConnected())
		m_impl->m_connection->connect();
	return m_impl->m_connection->publish(m_impl->m_message);
}
