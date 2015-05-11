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
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include <unistd.h>
#include <Logger.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

AMQPPublisher::AMQPPublisher(const AMQPConnectionInfo &connectionInfo,
			     string body)
: m_connectionInfo(connectionInfo),
  m_body(body)
{
}

AMQPPublisher::~AMQPPublisher()
{
}

bool AMQPPublisher::publish(void)
{
	AMQPConnection connection(m_connectionInfo);
	if (!connection.isConnected()) {
		connection.connect();
	}
	return connection.publish(m_body);
}
